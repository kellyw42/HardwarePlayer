#pragma once

class VideoConverter
{
private:
	TrackingSystem* trackingSystem;
	CUfunction NV12ToARGB, NV12ToGrayScale, Luminance;
	CUdeviceptr totalLuminance;

	void  cudaLaunchNV12toGrayScaleDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstARGB, size_t nDestPitch, int width, int height)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[6] = { &d_srcNV12, &nSourcePitch, &d_dstARGB, &nDestPitch, &width, &height };

		CHECK(cuLaunchKernel(NV12ToGrayScale, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	void  cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstARGB, size_t nDestPitch, int width, int height)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[6] = { &d_srcNV12, &nSourcePitch, &d_dstARGB, &nDestPitch, &width, &height };

		CHECK(cuLaunchKernel(NV12ToARGB, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	long long  cudaLaunchLuminance(CUdeviceptr d_srcNV12, size_t nSourcePitch, int width, int height, int left, int right, int top, int bottom)
	{
		void *args[7] = { &d_srcNV12, &nSourcePitch, &left, &right, &top, &bottom, &totalLuminance };

		CHECK(cuLaunchKernel(Luminance, 1, 1, 1, 1, 1, 1, 0, 0, args, 0));

		long long hostResult = 0;
		cuMemcpyDtoH(&hostResult, totalLuminance, sizeof(long long));
		return hostResult;
	}

public:
	VideoConverter(TrackingSystem *trackingSystem)
	{
		this->trackingSystem = trackingSystem;

		CUmodule colourConversionModule;
		CHECK(cuModuleLoad(&colourConversionModule, "ColourConversion.cubin"));

		CHECK(cuModuleGetFunction(&NV12ToGrayScale, colourConversionModule, "NV12ToGrayScale"));
		CHECK(cuModuleGetFunction(&NV12ToARGB, colourConversionModule, "NV12ToARGB"));
		CHECK(cuModuleGetFunction(&Luminance, colourConversionModule, "Luminance"));

		CHECK(cuMemAlloc(&totalLuminance, sizeof(long long)));
	}

	long long ConvertField(CUvideodecoder _decoder, int width, int height, CUVIDPARSERDISPINFO frameInfo, int active_field, VideoFrame* frame, RECT *SearchRectangle)
	{
		//Trace2("ConvertField(decoder = %p, resource = %p)", _decoder, resource);

		long long luminance = 0;

		CUVIDPROCPARAMS params;
		memset(&params, 0, sizeof(CUVIDPROCPARAMS));
		params.output_stream = 0;
		params.progressive_frame = frameInfo.progressive_frame;
		params.top_field_first = frameInfo.top_field_first;
		params.unpaired_field = frameInfo.repeat_first_field < 0;
		params.second_field = active_field;

		unsigned int decodePitch = 0;
		CUdeviceptr pDecodedFrame;

		CHECK(cuvidMapVideoFrame(_decoder, frameInfo.picture_index, &pDecodedFrame, &decodePitch, &params));



		CHECK(cuGraphicsMapResources(1, &frame->resource, 0));

		size_t size = 0;
		CUdeviceptr pInteropFrame = 0;
		//CHECK(cuGLMapBufferObject(&pInteropFrame, &size, pbo)); // deprecated?

		cuGraphicsResourceGetMappedPointer(&pInteropFrame, &size, frame->resource);

		//cudaLaunchNV12toGrayScaleDrv(pDecodedFrame, decodePitch, pInteropFrame, width, width, height);
		cudaLaunchNV12toARGBDrv(pDecodedFrame, decodePitch, pInteropFrame, width*4, width, height);

		if (trackingSystem != NULL)
			frame->athletePositions = trackingSystem->AnalyseFrame(pInteropFrame, width * 4);
		else
			frame->athletePositions.clear();

		if (SearchRectangle)
			luminance = cudaLaunchLuminance(pDecodedFrame, decodePitch, width, height, SearchRectangle->left, SearchRectangle->right, SearchRectangle->top, SearchRectangle->bottom);

		//CHECK(cuGLUnmapBufferObject(pbo)); // deprecated?
		CHECK(cuGraphicsUnmapResources(1, &frame->resource, 0));

		CHECK(cuvidUnmapVideoFrame(_decoder, pDecodedFrame));

		return luminance;
	}
};