//nvcc --cubin -O3 -gencode=arch=compute_30,code=sm_30  ColourConversion.cu -odir ..\x64\Debug

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

extern "C" __global__ void Luminance(unsigned char *srcImage, size_t nSourcePitch, int left, int right, int top, int bottom, long long *result)
{
	long long total = 0;
	for (int x=left; x<=right; x++)
		for (int y=top; y<=bottom; y++)
			total += srcImage[y * nSourcePitch + x];
	*result = total;
}

