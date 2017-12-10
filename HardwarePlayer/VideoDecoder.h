#include <memory.h>
#include <stdio.h>
#include "GL/glew.h"
#include "GL/freeglut.h"
#include <vector_types.h>
#include <cuda.h>
#include <cudaGL.h>
#include "nvcuvid.h"
#include "cuviddec.h"
#include "FrameQueue.h"

void CHECK(CUresult result)
{
	if (result != CUDA_SUCCESS)
	{
		fprintf(stderr, "Error %d\n", result);
		exit(1);
	}
}

#define NumFrames	100

int CUDAAPI HandleVideoData(void *userData, CUVIDSOURCEDATAPACKET *pPacket);
int CUDAAPI HandlePictureDecode(void *decoder, CUVIDPICPARAMS * pPicParams);
int CUDAAPI HandlePictureDisplay(void *a, CUVIDPARSERDISPINFO *p);

class VideoDecoder
{
public:
	int id;
	GLuint gl_pbo[NumFrames];
	CUfunction NV12ToARGB;
	int display_width, display_height;

	FrameQueue* frameQueue;
	CUvideodecoder decoder;
	CUvideosource source;
	CUvideoparser parser;
	CUVIDEOFORMAT format;

	VideoDecoder()
	{
	}

	CUVIDEOFORMAT OpenVideo(char* filename)
	{
		frameQueue = new FrameQueue();

		CUVIDSOURCEPARAMS params;
		memset(&params, 0, sizeof(CUVIDSOURCEPARAMS));
		params.pUserData = this;
		params.pfnVideoDataHandler = HandleVideoData;

		CHECK(cuvidCreateVideoSource(&source, filename, &params));
		CHECK(cuvidGetSourceVideoFormat(source, &format, 0));

		return format;
	}

	void StartDecode()
	{
		CUVIDEOFORMATEX formatEx;
		memset(&formatEx, 0, sizeof(CUVIDEOFORMATEX));
		formatEx.format = format;

		CUVIDPARSERPARAMS params2;
		memset(&params2, 0, sizeof(CUVIDPARSERPARAMS));
		params2.pUserData = this;
		params2.CodecType = format.codec;
		params2.pExtVideoInfo = &formatEx;
		params2.pfnDecodePicture = HandlePictureDecode; 
		params2.pfnDisplayPicture = HandlePictureDisplay; 
		params2.pfnSequenceCallback = 0; 
		params2.ulClockRate = 0;
		params2.ulErrorThreshold = 0;
		params2.ulMaxDisplayDelay = 2;  // this flag is needed so the parser will push frames out to the decoder as quickly as it can
		params2.ulMaxNumDecodeSurfaces = 20;

		CHECK(cuvidCreateVideoParser(&parser, &params2));

		CUVIDDECODECREATEINFO   decoderParams;
		memset(&decoderParams, 0, sizeof(CUVIDDECODECREATEINFO));
		decoderParams.bitDepthMinus8 = format.bit_depth_luma_minus8;
		decoderParams.ChromaFormat = format.chroma_format;
		decoderParams.CodecType = format.codec;
		decoderParams.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive; // ???
		decoderParams.OutputFormat = cudaVideoSurfaceFormat_NV12;
		decoderParams.ulCreationFlags = cudaVideoCreate_PreferCUVID;
		decoderParams.ulIntraDecodeOnly = 0;
		decoderParams.ulNumDecodeSurfaces = 20;
		decoderParams.ulNumOutputSurfaces = 2;
		decoderParams.vidLock = 0;
		decoderParams.ulWidth = format.coded_width;
		decoderParams.ulHeight = format.coded_height;
		decoderParams.ulTargetHeight = format.display_area.bottom - format.display_area.top;
		decoderParams.ulTargetWidth = format.display_area.right - format.display_area.left;
		decoderParams.display_area.left = format.display_area.left;
		decoderParams.display_area.right = format.display_area.right;
		decoderParams.display_area.top = format.display_area.top;
		decoderParams.display_area.bottom = format.display_area.bottom;
		decoderParams.target_rect.left = format.display_area.left;
		decoderParams.target_rect.right = format.display_area.right;
		decoderParams.target_rect.top = format.display_area.top;
		decoderParams.target_rect.bottom = format.display_area.bottom;

		display_width = decoderParams.ulTargetWidth;
		display_height = decoderParams.ulTargetHeight;

		CHECK(cuvidCreateDecoder(&decoder, &decoderParams));

		CUmodule colourConversionModule;
		CHECK(cuModuleLoad(&colourConversionModule, "ColourConversion.cubin"));

		CHECK(cuModuleGetFunction(&NV12ToARGB, colourConversionModule, "NV12ToARGB"));

		glGenBuffersARB(NumFrames, gl_pbo);

		for (int i = 0; i<NumFrames; i++)
		{
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, gl_pbo[i]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, display_width * display_height * 4, NULL, GL_STREAM_DRAW_ARB);
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
			CHECK(cuGLRegisterBufferObject(gl_pbo[i]));
		}

		ReStart();
	}

	void ReStart()
	{
		CHECK(cuvidSetVideoSourceState(source, cudaVideoState_Started));
	}

	bool Flush()
	{
	}

	bool ReadNextVideoFrame()
	{
	}

	void  cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstARGB, size_t nDestPitch, int width, int height)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[6] = { &d_srcNV12, &nSourcePitch, &d_dstARGB, &nDestPitch, &width, &height };
		CHECK(cuLaunchKernel(NV12ToARGB, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
		//CHECK(cuCtxSynchronize());
	}

	void Goto(int target)
	{
	}

	int framePos = 0;

	GLuint PrevFrame()
	{
		framePos = (framePos-1) % NumFrames;
		if (framePos < 0) framePos = NumFrames - 1;

		return gl_pbo[framePos];
	}

	GLuint NextFrame()
	{
		framePos = (framePos + 1) % NumFrames;
		return gl_pbo[framePos];
	}

	GLuint ConvertFrame(CUVIDPARSERDISPINFO frameInfo, int active_field)
	{
		framePos = framePos % NumFrames;

		CUVIDPROCPARAMS params;
		memset(&params, 0, sizeof(CUVIDPROCPARAMS));
		params.output_stream = 0;
		params.progressive_frame = frameInfo.progressive_frame;
		params.top_field_first = frameInfo.top_field_first;
		params.unpaired_field = frameInfo.repeat_first_field < 0;
		params.second_field = active_field;

		unsigned int decodePitch = 0;

		CUdeviceptr pDecodedFrame;
		CHECK(cuvidMapVideoFrame(decoder, frameInfo.picture_index, &pDecodedFrame, &decodePitch, &params));

		size_t texturePitch = 0;
		CUdeviceptr  pInteropFrame;
		CHECK(cuGLMapBufferObject(&pInteropFrame, &texturePitch, gl_pbo[framePos]));

		texturePitch /= display_height * 4;

		cudaLaunchNV12toARGBDrv(pDecodedFrame, decodePitch, pInteropFrame, texturePitch, display_width, display_height);
		CHECK(cuGLUnmapBufferObject(gl_pbo[framePos]));
		CHECK(cuvidUnmapVideoFrame(decoder, pDecodedFrame));

		return gl_pbo[framePos++];
	}
};

int CUDAAPI HandleVideoData(void *userData, CUVIDSOURCEDATAPACKET *pPacket)
{
	VideoDecoder *container = (VideoDecoder*)userData;
	CHECK(cuvidParseVideoData(container->parser, pPacket));
	return true;
}

int CUDAAPI HandlePictureDecode(void *userData, CUVIDPICPARAMS * pPicParams)
{
	VideoDecoder *container = (VideoDecoder*)userData;
	container->frameQueue->waitUntilFrameAvailable(pPicParams->CurrPicIdx);
	CHECK(cuvidDecodePicture(container->decoder, pPicParams));
	return true;
}

int CUDAAPI HandlePictureDisplay(void *userData, CUVIDPARSERDISPINFO *p)
{
	VideoDecoder *container = (VideoDecoder*)userData;
	container->frameQueue->enqueue(p);
	return true;
}