#pragma once



class Athlete
{
public:
	int crossing = 0;
	double feetX, feetY;
	double virtualX, virtualY;
	Athlete* previous;
	double centreX, centreY;
	int left, right, top, bottom, width, height;
	cv::Rect roi, finishLine;
	double virtualHeight;
	double hitRatio;

private:
	int componentId;
	Lanes* lanes;

	bool Hit(int y)
	{
		float topX = (float)finishLine.x;
		float topY = (float)finishLine.y;
		float bottomX = (float)finishLine.width;
		float bottomY = (float)finishLine.height;

		if (y < topY || y > bottomY)
			return false;

		float dy = (y - topY) / (bottomY - topY);

		int x = (int)(dy * bottomX + (1 - dy) * topX);

		bool hit = componentLabels.at<int>(y, x) == componentId;

		//componentLabels.at<int>(y, x) = 4096;

		return hit;
	}

public:
	cv::Mat componentLabels;

	Athlete(int componentId, cv::Mat stats, cv::Mat centroid, cv::Mat componentLabels, cv::Rect roi, Lanes *lanes, cv::Rect finishLine)
	{
		this->componentId = componentId;
		this->roi = roi;
		this->lanes = lanes;
		this->finishLine = finishLine;

		left = stats.at<int>(0, cv::CC_STAT_LEFT);
		top = stats.at<int>(0, cv::CC_STAT_TOP);
		height = stats.at<int>(0, cv::CC_STAT_HEIGHT);
		width = stats.at<int>(0, cv::CC_STAT_WIDTH);
		bottom = top + height;
		right = left + width;

		this->centreX = roi.x + centroid.at<double>(0, 0);
		this->centreY = (roi.y + centroid.at<double>(0, 1)) * 2;

		this->componentLabels = componentLabels;

		this->virtualHeight = lanes->Height(roi, stats);

		this->crossing = crossingFinish();

		FindFeet();
		lanes->Unproject(feetX, feetY, &virtualX, &virtualY);
	}

	cv::Point2i FindFeet()
	{
		return cv::Point2i(feetX = centreX, feetY = (roi.y + bottom) * 2);

		for (int y = bottom-1; y >= top; y--)
			for (int x = left; x < right; x++)
				if (componentLabels.at<int>(y, x) == componentId)
					return cv::Point2i(feetX = roi.x + x, feetY = (roi.y + y) * 2);

		assert(false);
	}

	int crossingFinish()
	{
		int topTorso = (int)(top + height * 0.2);
		int bottomTorso = (int)(top + height * 0.5);

		int longest = 0;
		int currentSequence = 0;
		int currentStart = topTorso;
		int longestStart = 0;

		for (int y = topTorso; y <= bottomTorso; y++)
			if (Hit(y))
			{
				currentSequence++;
				if (currentSequence > longest)
				{
					longest = currentSequence;
					longestStart = currentStart;
				}
			}
			else
			{
				currentStart = y + 1;
				currentSequence = 0;
			}

		//for (int i = longestStart; i < longestStart + longest; i++)
		//for (int i = top; i < top + height; i++)
		//	for (int x = left; x < right; x++)
		//		if (componentLabels.at<int>(i, x) == componentId)
		//			componentLabels.at<int>(i, x) = 4096;

		hitRatio = (float)longest / height;

		if (longest > height * 0.1)
			return crossing = 2;
		else if (longest > 0)
			return crossing = 1;
		else
			return crossing = 0;
	}

	bool firstCrossing()
	{
		if (crossing > 1)
		{
			bool alreadyCrossed = false;
			for (Athlete* previous = this->previous; previous != NULL; previous = previous->previous)
				if (previous->crossing > 1)
				{
					alreadyCrossed = true;
					break;
				}
			if (!alreadyCrossed)
				return true;
		}
		return false;
	}

	int getLaneNumber(FILE *file)
	{
		int max_lane = (int)lanes->getLane(this->feetX, this->feetY);

		for (Athlete* previous = this->previous; previous != NULL; previous = previous->previous)
		{
			int lane = (int)lanes->getLane(previous->feetX, previous->feetY);
			if (lane > max_lane)
			{
				max_lane = lane;
				if (file) fprintf(file, ",*%d", max_lane);
			}
			else
				if (file) fprintf(file, ",%d", max_lane);
		}
		return max_lane;
	}

	RECT boundingBox()
	{
		RECT rect;
		rect.left = roi.x + left;
		rect.right = roi.x + right;
		rect.top = (roi.y + top) * 2;
		rect.bottom = (roi.y + bottom) * 2;
		return rect;
	}

	RECT torsoBox()
	{
		RECT rect;
		float height = (bottom - top);
		rect.left = roi.x + left;
		rect.right = roi.x + right;
		rect.top = (roi.y + top + height * 0.2) * 2;
		rect.bottom = (roi.y + top + height * 0.5) * 2;
		return rect;
	}

	void FindPrevious(std::vector<Athlete*> athletes)
	{
		double best_dist = 1000;
		Athlete *closest = NULL;
		for (Athlete* athlete : athletes)
		{
			double dx = this->virtualX - athlete->virtualX;
			double dy = this->virtualY - athlete->virtualY;
			double dist = dx * dx + dy * dy;
			if (dist < best_dist)
			{
				best_dist = dist;
				closest = athlete;
			}
		}

		if (closest != NULL)
		{
			double dx = this->virtualX - closest->virtualX;
			double dy = this->virtualY - closest->virtualY;
			double dist = sqrt(dx * dx + dy * dy);

			if (virtualX < -3 && dx < -1)
				return;
			if (virtualY > 10 || dy*dy > 9)
				return;
			if (dist > 2)
				return;

			//Trace2("(%f,%f) - (%f,%f) = (%f,%f)\n", virtualX, virtualY, closest->virtualX, closest->virtualY, dx, dy);
			previous = closest;
		}
	}
};