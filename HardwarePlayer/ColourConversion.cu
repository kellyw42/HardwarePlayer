// nvcc --cubin -O3 -gencode=arch=compute_75,code=sm_75  ColourConversion.cu
// Note: path to ColourConversion.cubin is hardcoded in VideoConvert.h

extern "C" __global__ void NV12ToGrayScale(unsigned char *srcImage, size_t nSourcePitch, unsigned int *dstImage, size_t nDestPitch, int width, int height)
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

extern "C" __global__ void Copy(unsigned int* srcImage, unsigned int* dstImage, int centreX, int centreY, int dstX, int dstY)
{
	int x = blockIdx.x * blockDim.x + threadIdx.x;
	int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= 100 || y >= 50) return;

	int dstPitch = 1920;
	int srcPitch = 1440;

	dstImage[(dstY * 64 + y) * dstPitch + (dstX * 128 + x)] = 
		srcImage[(centreY - 32 + y) * srcPitch + (centreX - 64 + x)];
}

__device__ static void YUV2RGB(const unsigned int* yuvi, float* red, float* green, float* blue)
{
	float constHueColorSpaceMat[9] = { 1.1644f, 0.0f, 1.596f, 1.1644f, -0.3918f, -0.813f, 1.1644f, 2.0172f, 0.0f };

	float luma, chromaCb, chromaCr;

	// Prepare for hue adjustment
	luma = (float)yuvi[0];
	chromaCb = (float)((int)yuvi[1] - 512.0f);
	chromaCr = (float)((int)yuvi[2] - 512.0f);

	// Convert YUV To RGB with hue adjustment
	*red = (luma     * constHueColorSpaceMat[0]) +
		(chromaCb * constHueColorSpaceMat[1]) +
		(chromaCr * constHueColorSpaceMat[2]);

	*green = (luma     * constHueColorSpaceMat[3]) +
		(chromaCb * constHueColorSpaceMat[4]) +
		(chromaCr * constHueColorSpaceMat[5]);

	*blue = (luma     * constHueColorSpaceMat[6]) +
		(chromaCb * constHueColorSpaceMat[7]) +
		(chromaCr * constHueColorSpaceMat[8]);
}

__device__ static unsigned int RGBA_pack_10bit(float red, float green, float blue, unsigned int alpha)
{
	unsigned int ARGBpixel = 0;

	// Clamp final 10 bit results
	red = ::fmin(::fmax(red, 0.0f), 1023.f);
	green = ::fmin(::fmax(green, 0.0f), 1023.f);
	blue = ::fmin(::fmax(blue, 0.0f), 1023.f);

	// Convert to 8 bit unsigned integers per color component
	ARGBpixel = (((unsigned int)blue >> 2) |
		(((unsigned int)green >> 2) << 8) |
		(((unsigned int)red >> 2) << 16) |
		(unsigned int)alpha);

	return ARGBpixel;
}

// CUDA kernel for outputting the final ARGB output from NV12

#define COLOR_COMPONENT_BIT_SIZE 10
#define COLOR_COMPONENT_MASK     0x3FF

extern "C" __global__ void NV12ToARGB(const unsigned char* srcImage, size_t nSourcePitch,
	unsigned int* dstImageTop, unsigned int *dstImageBottom, size_t nDestPitch,
	unsigned int width, unsigned int height)
{
	// Pad borders with duplicate pixels, and we multiply by 2 because we process 2 pixels per thread
	const int x = blockIdx.x * (blockDim.x << 1) + (threadIdx.x << 1);
	const int y = blockIdx.y *  blockDim.y + threadIdx.y;

	if (x >= width || y >= height)
		return;

	// Read 2 Luma components at a time, so we don't waste processing since CbCr are decimated this way.
	// if we move to texture we could read 4 luminance values

	unsigned int yuv101010Pel[2];
	yuv101010Pel[0] = (srcImage[y * nSourcePitch + x]) << 2;
	yuv101010Pel[1] = (srcImage[y * nSourcePitch + x + 1]) << 2;

	const size_t chromaOffset = nSourcePitch * height;

	int y_chroma = ((y >> 2) << 1) | (y & 1);

	unsigned int chromaCb = srcImage[chromaOffset + y_chroma * nSourcePitch + x];
	unsigned int chromaCr = srcImage[chromaOffset + y_chroma * nSourcePitch + x + 1];

	yuv101010Pel[0] |= (chromaCb << (COLOR_COMPONENT_BIT_SIZE + 2));
	yuv101010Pel[0] |= (chromaCr << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));
	yuv101010Pel[1] |= (chromaCb << (COLOR_COMPONENT_BIT_SIZE + 2));
	yuv101010Pel[1] |= (chromaCr << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));

	unsigned int yuvi[6];
	float red[2], green[2], blue[2];

	yuvi[0] = (yuv101010Pel[0] & COLOR_COMPONENT_MASK);
	yuvi[1] = ((yuv101010Pel[0] >> COLOR_COMPONENT_BIT_SIZE)        & COLOR_COMPONENT_MASK);
	yuvi[2] = ((yuv101010Pel[0] >> (COLOR_COMPONENT_BIT_SIZE << 1)) & COLOR_COMPONENT_MASK);
	yuvi[3] = (yuv101010Pel[1] & COLOR_COMPONENT_MASK);
	yuvi[4] = ((yuv101010Pel[1] >> COLOR_COMPONENT_BIT_SIZE)        & COLOR_COMPONENT_MASK);
	yuvi[5] = ((yuv101010Pel[1] >> (COLOR_COMPONENT_BIT_SIZE << 1)) & COLOR_COMPONENT_MASK);

	YUV2RGB(&yuvi[0], &red[0], &green[0], &blue[0]);
	YUV2RGB(&yuvi[3], &red[1], &green[1], &blue[1]);

	unsigned int* dstImage = (y & 1) ? dstImageBottom : dstImageTop;
	int offset = (y >> 1) * width + x;
	dstImage[offset] = RGBA_pack_10bit(red[0], green[0], blue[0], ((unsigned int)0xff << 24));
	dstImage[offset + 1] = RGBA_pack_10bit(red[1], green[1], blue[1], ((unsigned int)0xff << 24));
}

extern "C" __global__ void FindFlash(unsigned int* image, int width, int height)
{
	const int x = blockIdx.x * blockDim.x + threadIdx.x;
	const int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width || y >= height)
		return;

	unsigned int* mem = &image[y * width + x];

	if (*mem == 0xFFFFFFFF) 
		*mem = 0xFF000000;

	//FF|red|green|blue
}

extern "C" __global__ void DrawComponents(unsigned int* image, int width, int height, unsigned int* componentLabels, unsigned int* sizes, int roiLeft, int roiTop, int roiWidth, int roiHeight, int step)
{
	const int x = blockIdx.x * blockDim.x + threadIdx.x;
	const int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width || y >= height)
		return;

	if (x < roiLeft || x >= roiLeft + roiWidth || y < roiTop || y >= roiTop + roiHeight)
		return;

	int component = componentLabels[(y - roiTop) * step + (x - roiLeft) ];

	int left = componentLabels[(y - roiTop) * step + (x-1 - roiLeft)];
	int right = componentLabels[(y - roiTop) * step + (x+1 - roiLeft)];
	int top = componentLabels[(y-1 - roiTop) * step + (x - roiLeft)];
	int bottom = componentLabels[(y + 1 - roiTop) * step + (x - roiLeft)];

	int edge = component != left || component != right || component != top || component != bottom;
	
	if (component == 2048)
		image[y * width + x] = 0xFFFFFF00;
	else if (component == 4096)
		image[y * width + x] = 0xFFFF00FF;
	else if (component > 0 && sizes[component] > 1000 && edge)
        image[y * width + x] = 0xFF0000FF;

	//FF|red|green|blue
}

extern "C" __global__ void FinishLine3(const unsigned char* srcImage, size_t nSourcePitch,
	unsigned int* dstImageTop, unsigned int *dstImageBottom, size_t nDestPitch,
	unsigned int width, unsigned int height, float angle)
{
	// Pad borders with duplicate pixels, and we multiply by 2 because we process 2 pixels per thread
	const int y = blockIdx.y *  blockDim.y + threadIdx.y;

	if (y >= height)
		return;

	for (int dx=0; dx<width; dx+=2)
	{
	    int x = y * angle + dx;

		unsigned int yuv101010Pel[2];
		yuv101010Pel[0] = (srcImage[y * nSourcePitch + x]) << 2;
		yuv101010Pel[1] = (srcImage[y * nSourcePitch + x + 1]) << 2;

		const size_t chromaOffset = nSourcePitch * height;

		int y_chroma = ((y >> 2) << 1) | (y & 1);

		unsigned int chromaCb = srcImage[chromaOffset + y_chroma * nSourcePitch + x];
		unsigned int chromaCr = srcImage[chromaOffset + y_chroma * nSourcePitch + x + 1];

		yuv101010Pel[0] |= (chromaCb << (COLOR_COMPONENT_BIT_SIZE + 2));
		yuv101010Pel[0] |= (chromaCr << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));
		yuv101010Pel[1] |= (chromaCb << (COLOR_COMPONENT_BIT_SIZE + 2));
		yuv101010Pel[1] |= (chromaCr << ((COLOR_COMPONENT_BIT_SIZE << 1) + 2));

		unsigned int yuvi[6];
		float red[2], green[2], blue[2];

		yuvi[0] = (yuv101010Pel[0] & COLOR_COMPONENT_MASK);
		yuvi[1] = ((yuv101010Pel[0] >> COLOR_COMPONENT_BIT_SIZE)        & COLOR_COMPONENT_MASK);
		yuvi[2] = ((yuv101010Pel[0] >> (COLOR_COMPONENT_BIT_SIZE << 1)) & COLOR_COMPONENT_MASK);
		yuvi[3] = (yuv101010Pel[1] & COLOR_COMPONENT_MASK);
		yuvi[4] = ((yuv101010Pel[1] >> COLOR_COMPONENT_BIT_SIZE)        & COLOR_COMPONENT_MASK);
		yuvi[5] = ((yuv101010Pel[1] >> (COLOR_COMPONENT_BIT_SIZE << 1)) & COLOR_COMPONENT_MASK);

		YUV2RGB(&yuvi[0], &red[0], &green[0], &blue[0]);
		YUV2RGB(&yuvi[3], &red[1], &green[1], &blue[1]);

		unsigned int* dstImage = (y & 1) ? dstImageBottom : dstImageTop;
		int offset = (y >> 1) * width + dx;
		dstImage[offset] = RGBA_pack_10bit(red[0], green[0], blue[0], ((unsigned int)0xff << 24));
		dstImage[offset + 1] = RGBA_pack_10bit(red[1], green[1], blue[1], ((unsigned int)0xff << 24));
	}
}

extern "C" __global__ void FinishLine2(unsigned int* dstImage, unsigned int width, unsigned int height, float top, float bottom, int y1, int y2, unsigned char* mask, int step)
{
	const int x = blockIdx.x * blockDim.x + threadIdx.x;
	const int y = blockIdx.y * blockDim.y + threadIdx.y;

	if (x >= width || y >= height)
		return;

	if (mask[y * step + x] > 0)
		return;

	double m = ((double)height) / (bottom - top);

	double dist = y - (m * (x - top));

	double LineWidth = 100;
	double LineBlur = 0;

	if (-LineWidth <= dist && dist <= LineWidth)
		dstImage[y * width + x] = 0xFFFF0000;
	else if (-LineWidth - LineBlur < dist && dist < -LineWidth)
	{
		double mix = (-LineWidth - dist) / LineBlur;
		int old = dstImage[y * width + x];
		int red = (old >> 16) & 0xFF;
		int green = (old >> 8) & 0xFF;
		int blue = old & 0xFF;
		red = red * mix + 0xFF * (1 - mix);
		dstImage[y * width + x] = (0xFF << 24) | (red << 16) | (green << 8) | blue;
	}
	else if (LineWidth < dist && dist < LineWidth + LineBlur)
	{
		double mix = (dist - LineWidth) / LineBlur;
		int old = dstImage[y * width + x];
		int red = (old >> 16) & 0xFF;
		int green = (old >> 8) & 0xFF;
		int blue = old & 0xFF;
		red = red * mix + 0xFF * (1 - mix);
		dstImage[y * width + x] = (0xFF << 24) | (red << 16) | (green << 8) | blue;
	}

	return;

	if (x >= width || x < bottom || y < y1 || y > y2)
		return;

	float dy = ((float)y) / height;
	int copyx = dy * bottom + (1 - dy) * top;
	int src = dstImage[y * width + copyx];
	int R = ((src >> 16) & 0xFF) - 180;
	int G = ((src >> 8) & 0xFF) - 73;
	int B = ((src >> 0) & 0xFF) - 100;
	float diff = (R * R + G * G + B * B);
	if (diff < 2000)
		dstImage[y * width + x] = 0xFF000000;
	else
		dstImage[y * width + x] = 0xFFFFFFFF;
}

extern "C" __global__ void FinishLine(unsigned int* dstImage, unsigned int width, unsigned int height, float top, float bottom, int y1, int y2, int*hits)
{
	int total = 0;
	for (int y = y1; y < y2; y++)
	{
		float dy = ((float)y) / height;
		int copyx = dy * bottom + (1 - dy) * top;
		int src = dstImage[y * width + copyx];
		int R = ((src >> 16) & 0xFF) - 180;
		int G = ((src >> 8) & 0xFF) - 73;
		int B = ((src >> 0) & 0xFF) - 100;
		float diff = (R * R + G * G + B * B);
		if (diff < 2000)
			total++;
	}
	*hits = total;
}


extern "C" __global__ void Luminance(unsigned char *srcImage, size_t nSourcePitch, int left, int right, int top, int bottom, long long *lum)
{
	long long topLum = 0, bottomLum = 0;
	for (int x = left; x <= right; x++)
		for (int y = top; y <= bottom; y++)
			if (y & 1)
				bottomLum += srcImage[y * nSourcePitch + x];
			else
				topLum += srcImage[y * nSourcePitch + x];
	lum[0] = topLum;
	lum[1] = bottomLum;
}

