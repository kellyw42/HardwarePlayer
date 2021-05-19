#pragma once


#include <vector>

class Athlete;

class VideoFrame
{
public:
	int width, height;
	CUvideotimestamp pts = 0;
	GLuint gl_pbo;
	long long luminance = MAXLONGLONG;
	int hits;
	//uint32_t* host;
	CUgraphicsResource resource = 0;
	int field;
	std::vector<Athlete*> athletes;
	//cv::Mat boundingBoxes;

	VideoFrame(int width, int height)
	{
		this->width = width;
		this->height = height;
		//host = new uint32_t[height*width];
		//printf("use OpenGL on thread %x\n", GetCurrentThreadId());
		glGenBuffers(1, &gl_pbo);
		
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, 0, GL_STREAM_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		
		CHECK(cuGraphicsGLRegisterBuffer(&resource, gl_pbo, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
	}

	~VideoFrame()
	{
		CHECK(cuGraphicsUnregisterResource(resource));
		glDeleteBuffers(1, &gl_pbo);
	}
};
