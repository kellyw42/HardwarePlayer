#include "VideoDecoder.h"
#include <windows.h>
#include "GL/freeglut.h"
#include <chrono>

using namespace std::chrono;



int num, x[3], y[3];

BOOL ListMonitors(HMONITOR m, HDC h, LPRECT p, LPARAM u)
{
	x[num] = p->left;
	y[num] = p->top;
	num++;
	return TRUE;
}

class VideoWindow;

VideoWindow* windowList[10];

class VideoWindow
{
public:
	int display_width, display_height;
	int windowId;
	GLuint gl_texid;
	GLuint gl_shader;
	VideoDecoder* decoder = new VideoDecoder();
	CUcontext context;
	CUvideoctxlock lock;

	bool playing;

	//static void idle()
	//{
	//	for (int i = 0; i<10; i++)
	//		if (windowList[i])
	//		{
	//			glutSetWindow(i);
	//			glutPostRedisplay();
	//		}
	//}

	static void display()
	{
		//for (int i = 0; i < 10; i++)
		//	if (windowList[i])
		//	{
		//		glutSetWindow(i);
		//		windowList[i]->Render();
		//	}
		windowList[glutGetWindow()]->Render();
		glutPostRedisplay();
	}

	static void keyboard(unsigned char key, int x, int y)
	{
		if (key == 'p')
			windowList[glutGetWindow()]->PlayPause();
	}

	static void special(int key, int x, int y)
	{
		if (key == GLUT_KEY_RIGHT)
			windowList[glutGetWindow()]->NextFrame();
		else if (key == GLUT_KEY_LEFT)
			windowList[glutGetWindow()]->PrevFrame();
	}

	static void reshape(int window_x, int window_y)
	{
		glViewport(0, 0, window_x, window_y);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
	}

	VideoWindow(char* filename, int monitor)
	{
		CUVIDEOFORMAT format = decoder->OpenVideo(filename);
		display_width = format.display_area.right - format.display_area.left;
		display_height = format.display_area.bottom - format.display_area.top;
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

		glutInitWindowSize(display_width, display_height);

		num = 0;
		EnumDisplayMonitors(NULL, NULL, ListMonitors, 0);

		//int monitor = 0;
		//if (display_width == 1920 && num > 2)
		//	monitor = 2;
		//else if (num > 1)
		//	monitor = 1;
		//else
		//	monitor = 0;

		windowId = glutCreateWindow("video");

		reshape(display_width, display_height);

		glutPositionWindow(x[monitor] - 8, y[monitor]/* - 31 */);

		windowList[windowId] = this;
		glutDisplayFunc(display);
		glutKeyboardFunc(keyboard);
		glutReshapeFunc(reshape);
		glutSpecialFunc(special);
		//glutIdleFunc(idle);

		GLenum OK = glewInit();

		CUdevice device = 0;
		CHECK(cuDeviceGet(&device, 0));

		context = 0;
		CHECK(cuCtxCreate(&context, CU_CTX_BLOCKING_SYNC, device));

		lock = 0;
		CHECK(cuvidCtxLockCreate(&lock, context));

		glGenTextures(1, &gl_texid);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gl_texid);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, display_width, display_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

		const char *gl_shader_code = "!!ARBfp1.0\nTEX result.color, fragment.texcoord, texture[0], RECT; \nEND";
		glGenProgramsARB(1, &gl_shader);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader);
		glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(gl_shader_code), (GLubyte *)gl_shader_code);

		typedef bool (APIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int interval);
		PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		wglSwapIntervalEXT(0);

		playing = true;
		decoder->StartDecode();

		CUcontext cuCurrent = NULL;
		CHECK(cuCtxPopCurrent(&cuCurrent));
	}

	void RenderFrame(GLuint pbo)
	{
		// load texture from pbo
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, gl_texid);
		glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, display_width, display_height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader);
		glEnable(GL_FRAGMENT_PROGRAM_ARB);
		glDisable(GL_DEPTH_TEST);

		glBegin(GL_QUADS);
		{
			glTexCoord2f(0, (float)display_height);
			glVertex2f(0, 0);
			glTexCoord2f((float)display_width, (float)display_height);
			glVertex2f(1, 0);
			glTexCoord2f((float)display_width, 0);
			glVertex2f(1, 1);
			glTexCoord2f(0, 0);
			glVertex2f(0, 1);
		}
		glEnd();

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glutSwapBuffers();
		//glutPostRedisplay();
	}

	void PlayPause()
	{
		playing = !playing;
		glutPostRedisplay();
	}

	void NextFrame()
	{
		playing = false;
		//RenderNext();
		RenderFrame(decoder->NextFrame());
	}

	void PrevFrame()
	{
		playing = false;
		RenderFrame(decoder->PrevFrame());
	}

	void Render()
	{
		if (playing)
			RenderNext();
	}

	int active_field = 0;
	CUVIDPARSERDISPINFO frameInfo;

	high_resolution_clock::time_point start = high_resolution_clock::now();

	int frameCount = 0;

	void RenderNext()
	{
		if (active_field == 0 && decoder->frameQueue->dequeue(&frameInfo) == 0)
			return;

		//CCtxAutoLock lck(vw->lock);
		//CHECK(cuvidCtxLock(lock, 0));

		CHECK(cuCtxPushCurrent(context));

		GLuint pbo = decoder->ConvertFrame(frameInfo, active_field);
		RenderFrame(pbo);

		CHECK(cuCtxPopCurrent(NULL));

		//CHECK(cuvidCtxUnlock(lock, 0));

		if (active_field == 1)
			decoder->frameQueue->releaseFrame(&frameInfo);

		active_field = !active_field;

		frameCount++;

		high_resolution_clock::time_point now = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(now - start);

		if (time_span.count() > 1)
		{
			char title[128];
			sprintf_s(title, "%d", frameCount);
			glutSetWindowTitle(title);
			frameCount = 0;
			start = now;
		}
	}
};