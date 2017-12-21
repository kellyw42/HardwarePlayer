#pragma once

#define NumFrames	10


char title[256];

class VideoBuffer
{
private:
	CUfunction NV12ToARGB;
	CUvideotimestamp current, last;
	GLuint gl_pbo[NumFrames];
	VideoDecoder* decoder;
	CUVIDEOFORMAT format;
	int num_fields;
	CUcontext context;

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

	GLuint GetFrame(CUvideotimestamp time)
	{
		//assert(First() <= time && time <= last);
		CUvideotimestamp frame = time / TIME_PER_FIELD;
		CUvideotimestamp index = frame % NumFrames;
		return gl_pbo[index];
	}

	void  cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstARGB, size_t nDestPitch, int width, int height)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[6] = { &d_srcNV12, &nSourcePitch, &d_dstARGB, &nDestPitch, &width, &height };

		CHECK(cuLaunchKernel(NV12ToARGB, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	void ConvertFrame(CUVIDPARSERDISPINFO frameInfo, int active_field, GLuint gl_pbo)
	{
		//printf("push context %p\n", context);
		CHECK(cuCtxPushCurrent(context));

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

		CUcontext curr;
		CHECK(cuCtxGetCurrent(&curr));

		HGLRC ctx = wglGetCurrentContext();

		HANDLE h = GetCurrentThread();
	
		printf("cuda context = %p, openGL context = %p, thread = %p\n", curr, ctx, h);

		printf("cuGLMapBufferObject(%p, %p, %d)\n", &pInteropFrame, &texturePitch, gl_pbo);

		CHECK(cuGLMapBufferObject(&pInteropFrame, &texturePitch, gl_pbo));

		int display_width = format.display_area.right - format.display_area.left;
		int display_height = format.display_area.bottom - format.display_area.top;

		texturePitch /= display_height * 4;

		cudaLaunchNV12toARGBDrv(pDecodedFrame, decodePitch, pInteropFrame, texturePitch, display_width, display_height);
		CHECK(cuGLUnmapBufferObject(gl_pbo));
		CHECK(cuvidUnmapVideoFrame(decoder->decoder, pDecodedFrame));

		CHECK(cuCtxPopCurrent(NULL));
	}

	void AppendNextFrame(int N)
	{
		CUVIDPARSERDISPINFO frameInfo = decoder->FetchFrame();

		for (int active_field = 0; active_field < N; active_field++)
		{
			last = frameInfo.timestamp + active_field * TIME_PER_FIELD;

			ConvertFrame(frameInfo, active_field, GetFrame(last));
		}

		decoder->releaseFrame(&frameInfo);
	}

	CUvideotimestamp First()
	{
		return last - (NumFrames-1) * TIME_PER_FIELD;
	}

	GLuint FirstFrame()
	{
		AppendNextFrame(1);
		current = last;
		return GetFrame(current);
	}

	GLuint NextFrame()
	{
		current += TIME_PER_FIELD;

		while (current > last)
			AppendNextFrame(num_fields);

		return GetFrame(current);
	}

	GLuint PrevFrame()
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

	GLuint FastForwardImage(int speed)
	{
		decoder->Goto(current + (speed * 3) * TIME_PER_FIELD);
		do
		{
			AppendNextFrame(1);
		} 
		while (last <= current);

		current = last;
		return GetFrame(current);
	}

	GLuint FastRewindImage(int speed)
	{
		decoder->Goto(current - speed * 10 * TIME_PER_FIELD);
		AppendNextFrame(1);
		current = last;
		return GetFrame(current);
	}

	GLuint GotoTime(CUvideotimestamp pts)
	{
		decoder->Goto(pts);
		AppendNextFrame(1);
		current = last;
		return GetFrame(current);
	}

	void Init(CUcontext context)
	{
		this->context = context;

		glGenBuffersARB(NumFrames, gl_pbo);

		CUcontext curr;
		CHECK(cuCtxGetCurrent(&curr));

		for (int i = 0; i < NumFrames; i++)
		{
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, gl_pbo[i]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, (format.display_area.right - format.display_area.left) * (format.display_area.bottom - format.display_area.top) * 4, 0, GL_STREAM_DRAW_ARB);
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
			cuGLRegisterBufferObject(gl_pbo[i]);
		}

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
