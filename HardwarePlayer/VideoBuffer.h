#pragma once

#define NumFrames	10


class VideoBuffer
{
private:
	CUfunction NV12ToARGB, Luminance;
	CUvideotimestamp current, last;
	VideoFrame* frames[NumFrames];
	VideoDecoder* decoder;
	CUVIDEOFORMAT format;
	int num_fields;
	CUdeviceptr totalLuminance;

/*
	char* Info()
	{
		sprintf_s(title, "%lld <= %lld (%lld) <= %lld", First(), current, (current - First())/TIME_PER_FIELD, last);
		return title;
	}
*/

	VideoFrame* CreateFrameFor(CUvideotimestamp pts)
	{
		Trace("VideoBuffer::CreateFrameFor(%ld);", pts);
		CUvideotimestamp frame = pts / TIME_PER_FIELD;
		CUvideotimestamp index = frame % NumFrames;
		frames[index]->pts = pts;
		return frames[index];
	}

	VideoFrame* GetFrame(CUvideotimestamp pts)
	{
		Trace("VideoBuffer::GetFrame(%ld);", pts);
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

	long long  cudaLaunchLuminance(CUdeviceptr d_srcNV12, size_t nSourcePitch, int width, int height, int left, int right, int top, int bottom)
	{
		void *args[7] = { &d_srcNV12, &nSourcePitch, &left, &right, &top, &bottom, &totalLuminance };

		CHECK(cuLaunchKernel(Luminance, 1, 1, 1, 1, 1, 1, 0, 0, args, 0));

		long long hostResult = 0;
		cuMemcpyDtoH(&hostResult, totalLuminance, sizeof(long long));
		return hostResult;
	}

	long long ConvertFrame(CUVIDPARSERDISPINFO frameInfo, int active_field, GLuint pbo, RECT *SearchRectangle)
	{
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
		CHECK(cuvidMapVideoFrame(decoder->decoder, frameInfo.picture_index, &pDecodedFrame, &decodePitch, &params));

		size_t texturePitch = 0;
		CUdeviceptr  pInteropFrame = 0;
		CHECK(cuGLMapBufferObject(&pInteropFrame, &texturePitch, pbo));

		int display_width = format.display_area.right - format.display_area.left;
		int display_height = format.display_area.bottom - format.display_area.top;

		texturePitch /= display_height * 4;

		cudaLaunchNV12toARGBDrv(pDecodedFrame, decodePitch, pInteropFrame, texturePitch, display_width, display_height);

		//cuMemcpyDtoH(&luminance, pDecodedFrame, sizeof(luminance));

		if (SearchRectangle)
			luminance = cudaLaunchLuminance(pDecodedFrame, decodePitch, display_width, display_height, SearchRectangle->left, SearchRectangle->right, SearchRectangle->top, SearchRectangle->bottom);

		CHECK(cuGLUnmapBufferObject(pbo));
		CHECK(cuvidUnmapVideoFrame(decoder->decoder, pDecodedFrame));

		return luminance;
	}

	void AppendNextFrame(RECT *searchRectangle)
	{
		//RECT whole;
		//whole.left = format.display_area.left;
		//whole.top = format.display_area.top;
		//whole.right = format.display_area.right;
		//whole.bottom = format.display_area.bottom;

		//if (searchRectangle == NULL)
		//	searchRectangle = &whole;

		Trace("VideoBuffer::AppendNextFrame();");
		CUVIDPARSERDISPINFO frameInfo = decoder->FetchFrame();

		for (int active_field = 0; active_field < num_fields; active_field++)
		{
			CUvideotimestamp pts = frameInfo.timestamp + active_field * TIME_PER_FIELD;
			VideoFrame *frame = CreateFrameFor(pts);
			frame->luminance = ConvertFrame(frameInfo, active_field, frame->gl_pbo, searchRectangle);
			Trace("store field %d, pts=%ld in frame %p with lum %lld\n", active_field, pts, frame, frame->luminance);
			last = frame->pts;
		}

		decoder->releaseFrame(&frameInfo);
	}

	CUvideotimestamp First()
	{
		return last - (NumFrames-1) * TIME_PER_FIELD;
	}

public:

	CUVIDEOFORMAT OpenVideo(char* filename)
	{
		decoder = new VideoDecoder();
		format = decoder->OpenVideo(filename);

		num_fields = format.progressive_sequence ? 1 : 2;

		return format;
	}

	void Init()
	{
		for (int i = 0; i < NumFrames; i++)
			frames[i] = new VideoFrame(format.display_area.right - format.display_area.left, format.display_area.bottom - format.display_area.top);

		CUmodule colourConversionModule;
		CHECK(cuModuleLoad(&colourConversionModule, "ColourConversion.cubin"));

		CHECK(cuModuleGetFunction(&NV12ToARGB, colourConversionModule, "NV12ToARGB"));
		CHECK(cuModuleGetFunction(&Luminance, colourConversionModule, "Luminance"));

		CHECK(cuMemAlloc(&totalLuminance, sizeof(long long)));

		decoder->Init();
	}

	void StartDecode()
	{
		decoder->Start();
	}

	VideoFrame* FirstFrame()
	{
		AppendNextFrame(NULL);
		current = last - TIME_PER_FIELD;
		return GetFrame(current);
	}

	VideoFrame* NextFrame(RECT *searchRectangle)
	{
		Trace("VideoBuffer::NextFrame();");
		current += TIME_PER_FIELD;

		while (current > last)
			AppendNextFrame(searchRectangle);

		return GetFrame(current);
	}

	VideoFrame* PrevFrame()
	{
		Trace("VideoBuffer::PrevFrame();");
		if (current > 0)
		{
			current -= TIME_PER_FIELD;
			if (current < First())
			{
				//printf("--------------------------------------------------------------------------\n");
				decoder->Goto(current - (NumFrames * TIME_PER_FIELD));
				do
				{
					AppendNextFrame(NULL);
					//printf("last = %lld < current = %lld\n", last, current);
				} while (last < current);
			}

			return GetFrame(current);
		}
		else
			return GetFrame(0);
	}

	VideoFrame* FastForwardImage(int speed)
	{
		Trace("VideoBuffer::FastForwardImage(%d);", speed);
		decoder->Goto(current + speed * TIME_PER_FIELD);
		do
		{
			AppendNextFrame(NULL);
		} while (last <= current);

		current = last;
		return GetFrame(current);
	}

	VideoFrame* FastRewindImage(int speed)
	{
		Trace("VideoBuffer::FastRewindImage(%d);", speed);
		decoder->Goto(current - speed * TIME_PER_FIELD);
		AppendNextFrame(NULL);
		current = last - TIME_PER_FIELD;
		return GetFrame(current);
	}

	VideoFrame* GotoTime(CUvideotimestamp pts)
	{
		Trace("VideoBuffer::GotoTime(%ld);", pts);

		// Fixme: check if already in buffer!!!

		decoder->Goto(pts);

		do
		{
			AppendNextFrame(NULL);
		} while (last < pts);

		current = pts;

		return GetFrame(current);
	}
};
