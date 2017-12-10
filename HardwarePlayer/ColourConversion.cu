//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <cuda.h>

//nvcc --cubin -G -gencode=arch=compute_30,code=sm_30  ColourConversion.cu

extern "C" __global__ void NV12ToARGB(unsigned char *srcImage, size_t nSourcePitch, unsigned int *dstImage, size_t nDestPitch, int width, int height)
{
	int x, y;

	x = blockIdx.x * blockDim.x + threadIdx.x;
	y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width || y >= height)
	{
		//printf("%d,%d", x, y);
		return;
	}

	unsigned char lum = srcImage[y * nSourcePitch + x];

	dstImage[y * nDestPitch + x] = (0xFF << 24) | (lum << 16) | (lum << 8) | lum;
}

