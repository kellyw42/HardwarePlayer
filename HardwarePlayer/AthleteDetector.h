#pragma once

class AthleteDetector
{
private:
	cv::Ptr<cv::BackgroundSubtractor> pBackSub;
	cv::cuda::GpuMat fgMask;

public:
	AthleteDetector()
	{
		pBackSub = cv::cuda::createBackgroundSubtractorMOG2(50, 11, false);
	}

	cv::cuda::GpuMat GetDetections(CUdeviceptr inputImagePtr, VideoFrame* frame, int width, int height)
	{
		cv::cuda::GpuMat inputImage(cv::Size(width, height), CV_8UC4, (void*)(inputImagePtr));
		cv::Mat deviceFrame;

		pBackSub->apply(inputImage, fgMask);

		return fgMask;
		// not needed unless detecting and excluding shadows
		//cv::cuda::threshold(fgMask, fgMask, 128, 255, cv::THRESH_BINARY);

		//cv::Mat tempFrame;
		//fgMask.download(tempFrame);

		//cv::Mat labels, stats, centroids;
		// careful: do boundingBoxes get deallocated?
		//int nLabels = cv::connectedComponentsWithStats(tempFrame, labels, frame->boundingBoxes, centroids, 4);

		//return 0;
	}
};