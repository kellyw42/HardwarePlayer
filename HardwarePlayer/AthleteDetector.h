#pragma once

class AthleteDetector
{
private:
	cv::Ptr<cv::BackgroundSubtractorMOG2> pBackSub;
	Lanes *lanes;
	cv::Rect roi, finishLine;

public:
	AthleteDetector(Lanes *lanes)
	{
		this->lanes = lanes;
		roi = lanes->getRegionToTrack();
		finishLine = lanes->getFinishLine(roi);

		// TODO: parameter sweep testing ...
		pBackSub = cv::cuda::createBackgroundSubtractorMOG2(50, 16, false); // was 50,  11
		//pBackSub->setShadowThreshold(0.5);
		//pBackSub->setShadowValue(0);
	}

	void UpdateLanes()
	{
		roi = lanes->getRegionToTrack();
		finishLine = lanes->getFinishLine(roi);
	}

	void GetDetections(VideoFrame *frame, CUdeviceptr inputImagePtr, int width, int height, cv::Mat *returnLabels, cv::Mat *sizes)
	{
		// get image into OpenCV matrix
		cv::cuda::GpuMat inputImage(cv::Size(width, height), CV_8UC4, (void*)(inputImagePtr));

		cv::cuda::GpuMat cropped(inputImage, roi);

		// Background subtraction
		cv::cuda::GpuMat foregroundGPU;
		pBackSub->apply(cropped, foregroundGPU);

		// Download to CPU
		cv::Mat foregroundCPU;
		foregroundGPU.download(foregroundCPU);

		// Connected component analysis on CPU
		cv::Mat labels, stats, centroids;
		int numComponents = cv::connectedComponentsWithStats(foregroundCPU, labels, stats, centroids, 4);

		// Treat large components as athletes
		frame->athletes.clear();
		for (int i = 1; i < numComponents; i++)
			if (stats.at<int>(i, cv::CC_STAT_AREA) > 1000 && lanes->Height(roi, stats.row(i)) > 1.3) // minimum height!!!
				frame->athletes.push_back(new Athlete(i, stats.row(i), centroids.row(i), labels, roi, lanes, finishLine));

		//frame->hits = 0;
		//for (Athlete* athlete : frame->athletes)
		//	if (athlete->crossingFinish(finishLine) > 1)
		//		frame->hits++;

		*returnLabels = labels;
		*sizes = stats.col(cv::CC_STAT_AREA);
	}
};