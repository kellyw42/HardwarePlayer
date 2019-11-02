#pragma once


#include <vector>

class VideoFrame
{
public:
	CUvideotimestamp pts = 0;
	GLuint gl_pbo;
	long long luminance = 0;
	CUgraphicsResource resource = 0;
	int field;

	VideoFrame(int width, int height)
	{
		glGenBuffers(1, &gl_pbo);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gl_pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, 0, GL_STREAM_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		
		//cuGLRegisterBufferObject(gl_pbo); // deprecated?
		CHECK(cuGraphicsGLRegisterBuffer(&resource, gl_pbo, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
	}
};
