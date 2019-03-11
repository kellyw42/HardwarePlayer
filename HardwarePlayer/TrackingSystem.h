#pragma once



class TrackingSystem
{
private:
	cv::Ptr<cv::BackgroundSubtractor> mog2;
	cv::Ptr<cv::cuda::Filter>morphDilate;
	cv::Ptr<cv::Tracker> tracker;
	cv::Mat kernelDilate;
	cv::Rect2d roi;
	int width, height;
	int history = 128;
	int threshold = 20;
	int blur = 4;
	bool detectShadows = true;
	bool reset = false;

public:
	TrackingSystem(int width, int height)
	{
		this->width = width;
		this->height = height;
		mog2 = cv::cuda::createBackgroundSubtractorMOG2(history, threshold, detectShadows);

		kernelDilate = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2));
		morphDilate = cv::cuda::createMorphologyFilter(cv::MORPH_DILATE, CV_8UC1, kernelDilate);
		tracker = cv::TrackerKCF::create();
	}

	void Reset()
	{
		Trace2("threshold = %d", threshold);
		mog2->clear();
		mog2 = cv::cuda::createBackgroundSubtractorMOG2(history, threshold, detectShadows);
		reset = true;
	}

	void Up()
	{
		threshold++;
		Reset();
	}

	void Down()
	{
		threshold--;
		Reset();
	}

	std::vector<RECT> AnalyseFrame(CUdeviceptr pDecodedFrame, unsigned int pitch)
	{
		std::vector<RECT> athleteData;

		cv::cuda::GpuMat d_original(cv::Size(width, height), CV_8UC4, (void*)(pDecodedFrame), pitch);

		cv::Mat original;
		d_original.download(original);

		int c = original.channels();

		if (reset)
		{
			roi = cv::selectROI("original", original);
			tracker->init(original, roi);
			reset = false;
		}
		else
			tracker->update(original, roi);

		cv::rectangle(original, roi, cv::Scalar(255, 0, 0), 2, 1);

		cv::imshow("Original", original);

		//cv::cuda::GpuMat d_fgmask;
		//mog2->apply(d_original, d_fgmask);

		//cv::Mat fgmask;
		//d_fgmask.download(fgmask);

		//cv::blur(fgmask, fgmask, cv::Size(blur, blur));

		//cv::threshold(fgmask, fgmask, 128, 255, cv::THRESH_BINARY);

		//cv::imshow("foreground", fgmask);

		//cv::cuda::GpuMat d_background;
		//mog2->getBackgroundImage(d_background);
		//cv::Mat background;
		//d_background.download(background);

		//cv::imshow("background", background);

		/*
		morphDilate->apply(d_fgmask, d_fgmask);

		d_fgmask.download(fgmask);

		cv::Mat labelImage, stats, centroids;
		int nLabels = cv::connectedComponentsWithStats(fgmask, labelImage, stats, centroids, 8, CV_32S);

		for (int i = 0; i < nLabels; i++)
		{
			int left = stats.at<int>(i, cv::CC_STAT_LEFT);
			int top = stats.at<int>(i, cv::CC_STAT_TOP);
			int width = stats.at<int>(i, cv::CC_STAT_WIDTH);
			int height = stats.at<int>(i, cv::CC_STAT_HEIGHT);
			int area = stats.at<int>(i, cv::CC_STAT_AREA);

			if (area > 3000)
			{
				RECT pos;
				pos.left = left;
				pos.top = top;
				pos.bottom = top + height;
				pos.right = left + width;
				athleteData.push_back(pos);
			}
		}
		*/

		return athleteData;
	}
};