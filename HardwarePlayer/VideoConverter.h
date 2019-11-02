#pragma once

class VideoConverter
{
private:
	CUfunction NV12ToARGB, NV12ToGrayScale, Luminance;
	CUdeviceptr totalLuminance;

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
		void *args[7] = { &d_srcNV12, &nSourcePitch, &d_dstTop, &d_dstBottom, &nDestPitch, &width, &height };

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
	VideoConverter()
	{
		CUmodule colourConversionModule;
		CHECK(cuModuleLoad(&colourConversionModule, "C:\\Users\\kellyw\\Dropbox\\000000 AranaLA\\AAAAA\\HardwarePlayer\\HardwarePlayer\\ColourConversion.cubin"));

		CHECK(cuModuleGetFunction(&NV12ToGrayScale, colourConversionModule, "NV12ToGrayScale"));
		CHECK(cuModuleGetFunction(&NV12ToARGB, colourConversionModule, "NV12ToARGB"));
		CHECK(cuModuleGetFunction(&Luminance, colourConversionModule, "Luminance"));

		CHECK(cuMemAlloc(&totalLuminance, sizeof(long long)));
	}

	void DownloadFrame(CUvideotimestamp pts, CUdeviceptr device, unsigned int pitch, int width, int height)
	{
		uint32_t* host = (uint32_t*)malloc(height*pitch * sizeof(uint32_t));
		cuMemcpyDtoH(host, device, height*pitch * sizeof(uint32_t));

		char name[256];
		sprintf(name, "image%d.ppm", pts);

		FILE *file = fopen(name, "wb");
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
	}

	long long ConvertFields(CUvideodecoder _decoder, int width, int height, CUVIDPARSERDISPINFO frameInfo, 
		VideoFrame* topFrame, VideoFrame* bottomFrame, RECT *SearchRectangle)
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

		//if (SearchRectangle)
		//	luminance = cudaLaunchLuminance(pDecodedFrame, decodePitch, width, height, SearchRectangle->left, SearchRectangle->right, SearchRectangle->top, SearchRectangle->bottom);

		//DownloadFrame(topFrame->pts, RGBTopPtr, width, width, height);
		//DownloadFrame(bottomFrame->pts, RGBBottomPtr, width, width, height);

		CHECK(cuGraphicsUnmapResources(2, resources, 0));

		CHECK(cuvidUnmapVideoFrame(_decoder, YUVFramePtr));

		return luminance;
	}
};