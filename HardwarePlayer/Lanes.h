#pragma once

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

class Lanes
{
public:
	float lookX, lookY, skew, theta;
	char lanes_filename[256];
	RECT cropRect;

	Lanes(RECT cropRect)
	{
		this->cropRect = cropRect;
	}

	void saveLanes()
	{
		GetVirtualMatrices();

		FILE* file = fopen(lanes_filename, "w");
		fprintf(file, "%f,%f,%f,%f\n", lookX, lookY, theta, skew);
		fclose(file);
	}

	void SetVirtualMatrices()
	{
		const float angle = 31.72f;
		const float aspect = 1920.0f / 1080.0f;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(angle, aspect, 1.0f, 100.0f);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		const float eyeX = 0.05f, eyeY = -5.79f, eyeZ = 3.17f;
		const float lookZ = 0.0f;

		float fX = lookX - eyeX;
		float fY = lookY - eyeY;
		float fZ = lookZ - eyeZ;

		float upX = sin(theta);
		float upY = cos(theta);
		float upZ = -(fX * upX + fY * upY) / fZ;

		gluLookAt(eyeX, eyeY, eyeZ,
			lookX, lookY, 0.0,
			upX, upY, upZ);
	}

	GLdouble modelMatrix[16], projMatrix[16];
	GLint viewPort[4];

	void GetVirtualMatrices()
	{
		SetVirtualMatrices();

		glGetIntegerv(GL_VIEWPORT, viewPort);
		glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
		glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	}

	void Project(GLdouble x, GLdouble y, GLdouble z, GLdouble* winX, GLdouble* winY)
	{
		GLdouble winZ;
		gluProject(x, y, z, modelMatrix, projMatrix, viewPort, winX, winY, &winZ);

		*winY = (float)viewPort[3] - *winY;
	}

	double UnprojectHeight(GLfloat winX, GLfloat winY, double y)
	{
		winY = (float)viewPort[3] - winY;

		GLdouble x0, y0, z0;
		GLdouble x1, y1, z1;

		gluUnProject(winX, winY, 0.0, modelMatrix, projMatrix, viewPort, &x0, &y0, &z0);
		gluUnProject(winX, winY, 1.0, modelMatrix, projMatrix, viewPort, &x1, &y1, &z1);

		float t = (y - y0) / (y1 - y0);

		return (z1 - z0) * t + z0;
	}

	void Unproject(GLfloat winX, GLfloat winY, double* x, double* y)
	{
		winY = (float)viewPort[3] - winY;

		GLdouble x0, y0, z0;
		GLdouble x1, y1, z1;

		gluUnProject(winX, winY, 0.0, modelMatrix, projMatrix, viewPort, &x0, &y0, &z0);
		gluUnProject(winX, winY, 1.0, modelMatrix, projMatrix, viewPort, &x1, &y1, &z1);

		double t = -z0 / (z1 - z0); // solving for z2 = 0

		*x = (x1 - x0) * t + x0;
		*y = (y1 - y0) * t + y0;
	}

	void loadLaneMarkings(char* videoFilename)
	{
		sprintf(lanes_filename, "%.56slanes.csv", videoFilename);

		FILE* file = fopen(lanes_filename, "r");
		if (file)
		{
			fscanf(file, "%f,%f,%f,%f", &lookX, &lookY, &theta, &skew);
			fclose(file);
		}
		else
		{
			lookX = 0;
			lookY = 4;
			theta = 0;
			skew = 0.2f;
		}

		GetVirtualMatrices();
	}

	double getLane(float screenX, float screenY)
	{
		double x, y;
		Unproject(screenX, screenY, &x, &y);

		float m = -skew / 3;
		double lane = 9 - (y - m * x) / 1.22;
		return lane;
	}

	cv::Rect getRegionToTrack()
	{
		GLdouble topLeftX, topY, bottomLeftX, bottomY;

		GLdouble topRightX, bottomRightX;
		GLdouble topMiddleX, bottomMiddleX;

		Project(-2, 8 * 1.22, 1.8, &topLeftX, &topY);
		Project(+2, 8 * 1.22, 1.8, &topRightX, &topY);
		Project(0, 8 * 1.22, 1.8, &topMiddleX, &topY);

		Project(-2, 0, 0, &bottomLeftX, &bottomY);
		Project(+2, 0, 0, &bottomRightX, &bottomY);
		Project(0, 0, 0, &bottomMiddleX, &bottomY);

		int top = (int)max(topY, cropRect.top);
		int bottom = (int)min(bottomY, cropRect.bottom-1);
		int left = (int)min(topLeftX, bottomLeftX);
		int right = (int)max(topRightX, bottomRightX);

		cv::Rect roi(left, top / 2, right - left, (bottom - top) / 2);

		assert(0 <= roi.x && roi.x <= cropRect.right);
		assert(0 <= roi.y && roi.y <= cropRect.bottom / 2);
		assert(0 <= roi.x + roi.width && roi.x + roi.width <= cropRect.right);
		assert(0 <= roi.y + roi.height && roi.y + roi.height <= cropRect.bottom / 2);

		return roi;
	}

	cv::Rect getFinishLine(cv::Rect roi)
	{
		GLdouble topX, topY, bottomX, bottomY;

		Project(0, 8 * 1.22 + 34.5, 0, &topX, &topY);
		Project(0, 0, 0, &bottomX, &bottomY);

		cv::Rect finishLine((int)topX - roi.x, (int)(topY/2.0 - roi.y), (int)bottomX - roi.x, (int)(bottomY/2.0 - roi.y));

		return finishLine;
	}

	double Height(cv::Rect roi, cv::Mat stats)
	{
		int left = roi.x + stats.at<int>(0, cv::CC_STAT_LEFT);
		int top = roi.y + stats.at<int>(0, cv::CC_STAT_TOP);
		int height = 2 * stats.at<int>(0, cv::CC_STAT_HEIGHT);
		int width = stats.at<int>(0, cv::CC_STAT_WIDTH);
		float bottom = (float)(top + height);
		float right = (float)(left + width);
		float middle = (float)(left + width) / 2;

		double x, y;
		Unproject(middle, bottom, &x, &y);
		double h = UnprojectHeight(middle, top, y);
		return h;
	}
};
