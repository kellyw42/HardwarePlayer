#pragma once

class VideoConverter
{
private:
	CUfunction NV12ToARGB, Luminance;
	CUdeviceptr totalLuminance;

	cv::Ptr<cv::BackgroundSubtractor> mog2;
	cv::Ptr<cv::cuda::Filter>morphDilate;

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

	void OpenCVStuff(CUdeviceptr pDecodedFrame, unsigned int decodePitch)
	{
		//std::vector<int> y_pos;
		//std::vector<int> x_pos;

		//cv::cuda::GpuMat dimgO(cv::Size(decoder->decoderParams.ulWidth, decoder->decoderParams.ulHeight), CV_8UC1, (void*)(pDecodedFrame), decodePitch);

		//cv::cuda::GpuMat dimg;
		//cv::cuda::resize(dimgO, dimg, cv::Size(480, 270), 0, 0, 1);
		//cv::cuda::GpuMat roi(dimg, cv::Rect(80, 80, 220, 190));

		//cv::cuda::GpuMat d_fgmask;
		//mog2->apply(roi, d_fgmask);
		//mog2->apply(dimgO, d_fgmask);

		/*
		morphDilate->apply(d_fgmask, d_fgmask);

		cv::Mat fgmask;
		d_fgmask.download(fgmask);

		cv::Mat stats, centroids, labelImage;
		int nLabels = cv::connectedComponentsWithStats(fgmask, labelImage, stats, centroids, 8, CV_32S);


		cv::Mat mask(labelImage.size(), CV_8UC1, cv::Scalar(0));

		for (int i = 1; i < nLabels; i++)
		{
		if (stats.at<int>(i, 4) > 200) {
		mask = mask | (labelImage == i);
		std::vector<int> prev_x = x_pos;
		std::vector<int> prev_y = y_pos;

		x_pos.push_back(stats.at<int>(i, cv::CC_STAT_LEFT) + stats.at<int>(i, cv::CC_STAT_WIDTH));
		y_pos.push_back(stats.at<int>(i, cv::CC_STAT_TOP) + stats.at<int>(i, cv::CC_STAT_HEIGHT));

		cv::Rect r(cv::Rect(cv::Point(stats.at<int>(i, cv::CC_STAT_LEFT), stats.at<int>(i, cv::CC_STAT_TOP)), cv::Size(stats.at<int>(i, cv::CC_STAT_WIDTH), stats.at<int>(i, cv::CC_STAT_HEIGHT))));
		cv::Rect rO(cv::Rect(cv::Point(stats.at<int>(i, cv::CC_STAT_LEFT) + 80, stats.at<int>(i, cv::CC_STAT_TOP) + 80) * 4, cv::Size(stats.at<int>(i, cv::CC_STAT_WIDTH) * 4, stats.at<int>(i, cv::CC_STAT_HEIGHT) * 4)));
		cv::cuda::GpuMat i_dimgO(dimgO, rO);
		cv::Mat i_imgO;
		i_dimgO.download(i_imgO);

		if (x_pos.back() > 145 && x_pos.back() < 170) {
		//finish line
		}
		cv::imshow("Show", i_imgO);
		cv::waitKey(1000);
		cv::destroyAllWindows();
		}
		}
		*/
	}

public:
	VideoConverter()
	{
		CUmodule colourConversionModule;
		CHECK(cuModuleLoad(&colourConversionModule, "ColourConversion.cubin"));

		CHECK(cuModuleGetFunction(&NV12ToARGB, colourConversionModule, "NV12ToARGB"));
		CHECK(cuModuleGetFunction(&Luminance, colourConversionModule, "Luminance"));

		CHECK(cuMemAlloc(&totalLuminance, sizeof(long long)));

		mog2 = cv::cuda::createBackgroundSubtractorMOG2(500, 14, false);
		cv::Mat kernelDilate = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2));
		morphDilate = cv::cuda::createMorphologyFilter(cv::MORPH_DILATE, CV_8UC1, kernelDilate);
	}

	long long ConvertField(CUvideodecoder _decoder, int width, int height, CUVIDPARSERDISPINFO frameInfo, int active_field, GLuint pbo, RECT *SearchRectangle)
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

		CHECK(cuvidMapVideoFrame(_decoder, frameInfo.picture_index, &pDecodedFrame, &decodePitch, &params));

		OpenCVStuff(pDecodedFrame, decodePitch);

		size_t size = 0;
		CUdeviceptr  pInteropFrame = 0;
		CHECK(cuGLMapBufferObject(&pInteropFrame, &size, pbo)); // deprecated?

		cudaLaunchNV12toARGBDrv(pDecodedFrame, decodePitch, pInteropFrame, width, width, height);

		if (SearchRectangle)
			luminance = cudaLaunchLuminance(pDecodedFrame, decodePitch, width, height, SearchRectangle->left, SearchRectangle->right, SearchRectangle->top, SearchRectangle->bottom);

		CHECK(cuGLUnmapBufferObject(pbo)); // deprecated?
		CHECK(cuvidUnmapVideoFrame(_decoder, pDecodedFrame));

		return luminance;
	}
};