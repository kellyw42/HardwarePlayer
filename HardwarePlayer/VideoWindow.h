using namespace std::chrono;

extern "C"
{
	typedef void(__stdcall *eventhandler)(int);
	typedef void(__stdcall *timehandler)(int, int);
	typedef void(__stdcall *mousehandler)(int, int, int);
}

high_resolution_clock::time_point start = high_resolution_clock::now();

int frameCount = 0;

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

double total = 0;
int number = 0;

enum Mode { PLAY, PAUSE, REWIND };

class VideoWindow
{
public:
	int speed;
	Mode mode;
	int display_width, display_height;
	int windowId;
	GLuint gl_texid;
	GLuint gl_shader;
	VideoBuffer* videoBuffer;
	CUcontext context;
	CUvideoctxlock lock;

	VideoWindow(eventhandler event_handler, timehandler time_handler, char* filename, int monitor)
	{
		videoBuffer = new VideoBuffer();

		CUVIDEOFORMAT format = videoBuffer->OpenVideo(filename);

		display_width = format.display_area.right - format.display_area.left;
		display_height = format.display_area.bottom - format.display_area.top;

		CreateMyWindow(monitor);

		InitContext();

		InitRender();

		videoBuffer->Init(context);

		speed = 1;
		mode = PLAY;
		videoBuffer->StartDecode();

		CUcontext curr;
		CHECK(cuCtxPopCurrent(&curr));

		RenderFrame(videoBuffer->FirstFrame());
	}

	static void display()
	{		
		int i = glutGetWindow();
		windowList[i]->Render();
	}

	static void keyboard(unsigned char key, int x, int y)
	{
		if (key == 'p')
			windowList[glutGetWindow()]->PlayPause();
		if (key == 'r')
			windowList[glutGetWindow()]->Rewind();
		if (key == 'f')
			windowList[glutGetWindow()]->FastForw();
	}

	static void special(int key, int x, int y)
	{
		if (key == GLUT_KEY_RIGHT)
			windowList[glutGetWindow()]->StepNextFrame();
		else if (key == GLUT_KEY_LEFT)
			windowList[glutGetWindow()]->StepPrevFrame();
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

	void CreateMyWindow(int monitor)
	{
		glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);

		glutInitWindowSize(display_width, display_height);

		//num = 0;
		//EnumDisplayMonitors(NULL, NULL, ListMonitors, 0);

		windowId = glutCreateWindow("video");
		windowList[windowId] = this;

		reshape(display_width, display_height);

		//glutPositionWindow(x[monitor] - 8, y[monitor]/* - 31 */);

		//glutDisplayFunc(display);
		//glutKeyboardFunc(keyboard);
		//glutReshapeFunc(reshape);
		//glutSpecialFunc(special);
	}

	void InitRender()
	{
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
	}

	void InitContext()
	{
		GLenum OK = glewInit();

		CUdevice device = 0;
		CHECK(cuDeviceGet(&device, 0));

		context = 0;
		CHECK(cuGLCtxCreate(&context, CU_CTX_BLOCKING_SYNC, device));

		lock = 0;
		CHECK(cuvidCtxLockCreate(&lock, context));
	}

	void RenderFrame(GLuint pbo)
	{
		//glutSetWindowTitle(videoBuffer->Info());

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
		glFlush();
		glutSwapBuffers();
		glutPostRedisplay();
	}

	void FastForw()
	{
		if (mode == PLAY)
			speed *= 2;
		else
			speed = 2;
		mode = PLAY;
		glutPostRedisplay();
	}

	void Play()
	{
		speed = 1;
		mode = PLAY;
		glutPostRedisplay();
	}
	
	void Pause()
	{
		mode = PAUSE;
	}

	void PlayPause()
	{
		if (mode == PAUSE)
			Play();
		else
			Pause();
	}

	void Rewind()
	{
		if (mode == REWIND)
			speed *= 2;
		else
			speed = 1;
		mode = REWIND;
		glutPostRedisplay();
	}

	void StepNextFrame()
	{
		if (mode == PAUSE)
			RenderFrame(videoBuffer->NextFrame());
		else
			mode = PAUSE;
	}

	void StepPrevFrame()
	{
		if (mode == PAUSE)
			RenderFrame(videoBuffer->PrevFrame());
		else
			mode = PAUSE;
	}

	void Render()
	{
		if (mode == PLAY)
		{
			if (speed == 1)
			{
				RenderFrame(videoBuffer->NextFrame());
				UpdateFrameRate();
			}
			else
				RenderFrame(videoBuffer->FastForwardImage(speed));

			//glutPostRedisplay();
		}
		else if (mode == REWIND)
		{
			if (speed == 1)
				RenderFrame(videoBuffer->PrevFrame());
			else
				RenderFrame(videoBuffer->FastRewindImage(speed));

			//glutPostRedisplay();
		}
	}

	void GotoTime(CUvideotimestamp pts)
	{
		RenderFrame(videoBuffer->GotoTime(pts));
	}

	void UpdateFrameRate()
	{
		frameCount++;
		high_resolution_clock::time_point now = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(now - start);

		if (time_span.count() > 1)
		{
			char title[128];
			sprintf_s(title, "%d", frameCount);
			//glutSetWindowTitle(title);
			frameCount = 0;
			start = now;
		}
	}
};