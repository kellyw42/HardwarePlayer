#pragma once

class VideoConverter
{
private:
	CUmodule colourConversionModule;
	CUfunction NV12ToARGB, NV12ToGrayScale, Luminance, FinishLine, FinishLine2, FinishLine3;
	CUdeviceptr totalLuminance, totalHits;

	void  cudaLaunchNV12toGrayScaleDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstARGB, size_t nDestPitch, int width, int height)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[6] = { &d_srcNV12, &nSourcePitch, &d_dstARGB, &nDestPitch, &width, &height };

		CHECK(cuLaunchKernel(NV12ToGrayScale, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	void  cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstTop, CUdeviceptr d_dstBottom, size_t nDestPitch, int width, int height)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[7] = { &d_srcNV12, &nSourcePitch, &d_dstTop, &d_dstBottom, &nDestPitch, &width, &height};

		CHECK(cuLaunchKernel(NV12ToARGB, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	void  cudaLaunchFinishLine3(CUdeviceptr d_srcNV12, size_t nSourcePitch, CUdeviceptr d_dstTop, CUdeviceptr d_dstBottom, size_t nDestPitch, int width, int height, float angle)
	{
		dim3 block(1, 32, 1);
		dim3 grid(1, (height + (block.y - 1)) / block.y, 1);
		void* args[8] = { &d_srcNV12, &nSourcePitch, &d_dstTop, &d_dstBottom, &nDestPitch, &width, &height, &angle };

		CHECK(cuLaunchKernel(FinishLine3, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	int cudaLaunchFinishLine(CUdeviceptr d_dst, int width, int height, float top, float bottom, int y1, int y2)
	{
		void *args[8] = { &d_dst, &width, &height, &top, &bottom, &y1, &y2, &totalHits };
		CHECK(cuLaunchKernel(FinishLine, 1, 1, 1, 1, 1, 1, 0, 0, args, 0));
		int hostHits;
		cuMemcpyDtoH(&hostHits, totalHits, sizeof(int));
		return hostHits;
	}

	void cudaLaunchFinishLine2(CUdeviceptr d_dst, int width, int height, float top, float bottom, int y1, int y2)
	{
		dim3 block(32, 32, 1);
		dim3 grid((width + (block.x - 1)) / block.x, (height + (block.y - 1)) / block.y, 1);
		void *args[7] = { &d_dst, &width, &height, &top, &bottom, &y1, &y2};

		CHECK(cuLaunchKernel(FinishLine2, grid.x, grid.y, grid.z, block.x, block.y, block.z, 0, 0, args, 0));
	}

	void  cudaLaunchLuminance(long long *topLum, long long *bottomLum, CUdeviceptr d_srcNV12, size_t nSourcePitch, int width, int height, int left, int right, int top, int bottom)
	{
		void *args[7] = { &d_srcNV12, &nSourcePitch, &left, &right, &top, &bottom, &totalLuminance };

		CHECK(cuLaunchKernel(Luminance, 1, 1, 1, 1, 1, 1, 0, 0, args, 0));

		long long hostResult[2];
		cuMemcpyDtoH(hostResult, totalLuminance, 2 * sizeof(long long));
		*topLum = hostResult[0];
		*bottomLum = hostResult[1];
	}

public:
	VideoConverter()
	{
		CHECK(cuModuleLoad(&colourConversionModule, "C:\\Users\\kellyw\\Dropbox\\000000 AranaLA\\AAAAA\\HardwarePlayer\\HardwarePlayer\\ColourConversion.cubin"));

		CHECK(cuModuleGetFunction(&NV12ToGrayScale, colourConversionModule, "NV12ToGrayScale"));
		CHECK(cuModuleGetFunction(&NV12ToARGB, colourConversionModule, "NV12ToARGB"));
		CHECK(cuModuleGetFunction(&FinishLine, colourConversionModule, "FinishLine"));
		CHECK(cuModuleGetFunction(&FinishLine2, colourConversionModule, "FinishLine2"));
		CHECK(cuModuleGetFunction(&FinishLine3, colourConversionModule, "FinishLine3"));
		CHECK(cuModuleGetFunction(&Luminance, colourConversionModule, "Luminance"));

		CHECK(cuMemAlloc(&totalLuminance, 2 * sizeof(long long)));
		CHECK(cuMemAlloc(&totalHits, sizeof(int)));
	}

	~VideoConverter()
	{
		cuModuleUnload(colourConversionModule);
		cuMemFree(totalLuminance);
		cuMemFree(totalHits);
	}

	void DownloadFrame(CUvideotimestamp pts, CUdeviceptr device, unsigned int pitch, int width, int height)
	{
		uint32_t* host = new uint32_t[height*pitch];
		cuMemcpyDtoH(host, device, height*pitch * sizeof(uint32_t));

		char name[256];
		sprintf(name, "image%d.ppm", pts);

		FILE *file = fopen(name, "wb");
		if (file == NULL) MessageBoxA(NULL, name, "Error: cannot open file", MB_OK);
		fprintf(file, "P6\n%d %d\n255\n", width, height);
		for (int row = 0; row < height; row++)
		{
			for (int col = 0; col < width; col++)
			{
				int entry = host[row*pitch + col];
				static unsigned char color[3];
				color[0] = (entry >> 16) & 0xFF;
				color[1] = (entry >> 8) & 0xFF;
				color[2] = (entry >> 0) & 0xFF;
				(void)fwrite(color, 1, 3, file);
			}
		}
		fclose(file);
		delete[] host;
	}

	void ConvertFields(CUvideodecoder _decoder, int width, int height, CUVIDPARSERDISPINFO frameInfo, 
		VideoFrame* topFrame, VideoFrame* bottomFrame, RECT *SearchRectangle, float top, float bottom, RECT range)
	{
		//Trace2("ConvertField(decoder = %p)", _decoder);

		long long luminance = 0;

		CUVIDPROCPARAMS params;
		memset(&params, 0, sizeof(CUVIDPROCPARAMS));
		params.output_stream = 0;
		params.progressive_frame = 1;

		unsigned int YUVPitch = 0;
		CUdeviceptr YUVFramePtr;

		CHECK(cuvidMapVideoFrame(_decoder, frameInfo.picture_index, &YUVFramePtr, &YUVPitch, &params));

		CUgraphicsResource resources[2] = { topFrame->resource, bottomFrame->resource };
		CHECK(cuGraphicsMapResources(2, resources, 0));

		size_t size = 0;
		CUdeviceptr RGBTopPtr = 0, RGBBottomPtr = 0;
		CHECK(cuGraphicsResourceGetMappedPointer(&RGBTopPtr, &size, topFrame->resource));
		CHECK(cuGraphicsResourceGetMappedPointer(&RGBBottomPtr, &size, bottomFrame->resource));

		cudaLaunchNV12toARGBDrv(YUVFramePtr, YUVPitch, RGBTopPtr, RGBBottomPtr, width*4, width, height);

		if (width > 1500)
		{
			topFrame->hits = cudaLaunchFinishLine(RGBTopPtr, width, height / 2, top, bottom, range.top/2, range.bottom/2);
			bottomFrame->hits = cudaLaunchFinishLine(RGBBottomPtr, width, height / 2, top, bottom, range.top/2, range.bottom/2);
		}

		if (SearchRectangle)
			cudaLaunchLuminance(&topFrame->luminance, &bottomFrame->luminance, YUVFramePtr, YUVPitch, width, height, SearchRectangle->left, SearchRectangle->right, SearchRectangle->top, SearchRectangle->bottom);

		//cuMemcpyDtoH(topFrame->host,    RGBTopPtr,    height / 2 * width * sizeof(uint32_t));
		//cuMemcpyDtoH(bottomFrame->host, RGBBottomPtr, height / 2 * width * sizeof(uint32_t));

		//DownloadFrame(topFrame->pts, RGBTopPtr, width, width, height/2);
		//DownloadFrame(bottomFrame->pts, RGBBottomPtr, width, width, height/2);

		CHECK(cuGraphicsUnmapResources(2, resources, 0));

		CHECK(cuvidUnmapVideoFrame(_decoder, YUVFramePtr));
	}
};