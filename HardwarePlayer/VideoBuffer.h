#pragma once

#define NumFrames	10


char title[256];

class VideoBuffer
{
private:
	CUfunction NV12ToARGB;
	CUvideotimestamp current, last;
	VideoFrame* frames[NumFrames];
	VideoDecoder* decoder;
	CUVIDEOFORMAT format;
	int num_fields;

public:
	CUVIDEOFORMAT OpenVideo(char* filename)
	{
		decoder = new VideoDecoder();
		format = decoder->OpenVideo(filename);

		num_fields = format.progressive_sequence ? 1 : 2;

		return format;
	}

	char* Info()
	{
		sprintf_s(title, "%lld <= %lld (%lld) <= %lld", First(), current, (current - First())/TIME_PER_FIELD, last);
		return title;
	}

	VideoFrame* CreateFrameFor(CUvideotimestamp pts)
	{
		CUvideotimestamp frame = pts / TIME_PER_FIELD;
		CUvideotimestamp index = frame % NumFrames;
		frames[index]->pts = pts;
		return frames[index];
	}

	VideoFrame* GetFrame(CUvideotimestamp pts)
	{
		CUvideotimestamp frame = pts / TIME_PER_FIELD;
		CUvideotimestamp index = frame % NumFrames;
		assert(frames[index]->pts == pts);
		return frames[index];
	}

	void  cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstARGB, size_t nDestPitch, int width, int height)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[6] = { &d_srcNV12, &nSourcePitch, &d_dstARGB, &nDestPitch, &width, &height };

		CHECK(cuLaunchKernel(NV12ToARGB, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	void ConvertFrame(CUVIDPARSERDISPINFO frameInfo, int active_field, GLuint pbo)
	{
		CUVIDPROCPARAMS params;
		memset(&params, 0, sizeof(CUVIDPROCPARAMS));
		params.output_stream = 0;
		params.progressive_frame = frameInfo.progressive_frame;
		params.top_field_first = frameInfo.top_field_first;
		params.unpaired_field = frameInfo.repeat_first_field < 0;
		params.second_field = active_field;

		unsigned int decodePitch = 0;
		CUdeviceptr pDecodedFrame;
		CHECK(cuvidMapVideoFrame(decoder->decoder, frameInfo.picture_index, &pDecodedFrame, &decodePitch, &params));

		size_t texturePitch = 0;
		CUdeviceptr  pInteropFrame = 0;
		CHECK(cuGLMapBufferObject(&pInteropFrame, &texturePitch, pbo));

		int display_width = format.display_area.right - format.display_area.left;
		int display_height = format.display_area.bottom - format.display_area.top;

		texturePitch /= display_height * 4;

		cudaLaunchNV12toARGBDrv(pDecodedFrame, decodePitch, pInteropFrame, texturePitch, display_width, display_height);
		CHECK(cuGLUnmapBufferObject(pbo));
		CHECK(cuvidUnmapVideoFrame(decoder->decoder, pDecodedFrame));
	}

	void AppendNextFrame(int N)
	{
		CUVIDPARSERDISPINFO frameInfo = decoder->FetchFrame();

		for (int active_field = 0; active_field < N; active_field++)
		{
			CUvideotimestamp pts = frameInfo.timestamp + active_field * TIME_PER_FIELD;
			VideoFrame *frame = CreateFrameFor(pts);
			ConvertFrame(frameInfo, active_field, frame->gl_pbo);
			last = frame->pts;
		}

		decoder->releaseFrame(&frameInfo);
	}

	CUvideotimestamp First()
	{
		return last - (NumFrames-1) * TIME_PER_FIELD;
	}

	VideoFrame* FirstFrame()
	{
		AppendNextFrame(2);
		current = last - TIME_PER_FIELD;
		return GetFrame(current);
	}

	VideoFrame* NextFrame()
	{
		current += TIME_PER_FIELD;

		while (current > last)
			AppendNextFrame(num_fields);

		return GetFrame(current);
	}

	VideoFrame* PrevFrame()
	{
		if (current > 0)
		{
			current -= TIME_PER_FIELD;
			if (current < First())
			{
				//printf("--------------------------------------------------------------------------\n");
				decoder->Goto(current - (NumFrames * TIME_PER_FIELD));
				do
				{
					AppendNextFrame(num_fields);
					//printf("last = %lld < current = %lld\n", last, current);
				} 
				while (last < current);
			}

			return GetFrame(current);
		}
		else
			return GetFrame(0);
	}

	VideoFrame* FastForwardImage(int speed)
	{
		decoder->Goto(current + (speed * 3) * TIME_PER_FIELD);
		do
		{
			AppendNextFrame(num_fields);
		} 
		while (last <= current);

		current = last;
		return GetFrame(current);
	}

	VideoFrame* FastRewindImage(int speed)
	{
		decoder->Goto(current - speed * 10 * TIME_PER_FIELD);
		AppendNextFrame(num_fields);
		current = last - TIME_PER_FIELD;
		return GetFrame(current);
	}

	VideoFrame* GotoTime(CUvideotimestamp pts)
	{
		decoder->Goto(pts);
		AppendNextFrame(num_fields);
		current = pts;
		return GetFrame(current);
	}

	void Init()
	{
		for (int i = 0; i < NumFrames; i++)
			frames[i] = new VideoFrame(format.display_area.right - format.display_area.left, format.display_area.bottom - format.display_area.top);

		CUmodule colourConversionModule;
		CHECK(cuModuleLoad(&colourConversionModule, "ColourConversion.cubin"));

		CHECK(cuModuleGetFunction(&NV12ToARGB, colourConversionModule, "NV12ToARGB"));

		decoder->Init();
	}

	void StartDecode()
	{
		decoder->Start();
	}
};
