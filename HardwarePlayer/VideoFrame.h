#pragma once

class VideoFrame
{
public:
	CUvideotimestamp pts = 0;
	GLuint gl_pbo;
	long long luminance = 0;

	VideoFrame(int width, int height)
	{
		glGenBuffersARB(1, &gl_pbo);

		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, gl_pbo);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, width * height * 4, 0, GL_STREAM_DRAW_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		cuGLRegisterBufferObject(gl_pbo);
	}
};
