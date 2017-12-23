using namespace std::chrono;

static int num, x[3], y[3];

extern "C"
{
	typedef void(__stdcall *eventhandler)(int);
	typedef void(__stdcall *timehandler)(int, int);
}

class VideoWindow
{
public:
	high_resolution_clock::time_point start = high_resolution_clock::now();
	int frameCount = 0;
	int display_width, display_height;
	HWND hwnd;
	HDC hdc;
	GLuint gl_texid;
	GLuint gl_shader;
	CUcontext context;
	int monitor;
	timehandler time_handler;
	CUvideotimestamp first;

	static BOOL ListMonitors(HMONITOR m, HDC h, LPRECT p, LPARAM u)
	{
		x[num] = p->left;
		y[num] = p->top;
		num++;
		return TRUE;
	}

	VideoWindow(int monitor, timehandler time_handler)
	{
		this->monitor = monitor;
		this->time_handler = time_handler;
		num = 0;
		EnumDisplayMonitors(NULL, NULL, ListMonitors, 0);
	}

	void Open(char *filename, int display_width, int display_height)
	{
		this->display_width = display_width;
		this->display_height = display_height;

		CreateWin32Window(monitor);

		InitCUDA();

		InitOpenGL();
	}

	void CreateWin32Window(int monitor)
	{
		HINSTANCE hInstance = GetModuleHandle(NULL);

		WNDCLASS wc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hIcon = NULL;
		wc.hInstance = hInstance;
		wc.lpfnWndProc = MyWindowProc;
		wc.lpszClassName = TEXT("Wayne");
		wc.lpszMenuName = 0;
		wc.style = CS_OWNDC;
		RegisterClass(&wc);

		RECT rect;
		SetRect(&rect, x[monitor], y[monitor], x[monitor]+display_width, y[monitor]+display_height);

		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

		hwnd = CreateWindow(TEXT("Wayne"), TEXT("Title"), WS_OVERLAPPEDWINDOW, rect.left, rect.top+1, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
		//SetWindowLong(hwnd, GWL_STYLE, 0);

		ShowWindow(hwnd, SW_SHOW);

		hdc = GetDC(hwnd);

		PIXELFORMATDESCRIPTOR pfd = { 0 };
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 32;

		int chosenPixelFormat = ChoosePixelFormat(hdc, &pfd);
		SetPixelFormat(hdc, chosenPixelFormat, &pfd);

		HGLRC hglrc = wglCreateContext(hdc);

		wglMakeCurrent(hdc, hglrc);
	}

	void Bind(__int64 value)
	{
		SetWindowLongPtr(hwnd, GWLP_USERDATA, value);
		SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	}

	void InitOpenGL()
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

	void InitCUDA()
	{
		GLenum OK = glewInit();

		CUdevice device = 0;
		CHECK(cuDeviceGet(&device, 0));

		context = 0;
		CHECK(cuCtxCreate(&context, CU_CTX_BLOCKING_SYNC, device));
	}

	void FirstFrame(VideoFrame *frame)
	{
		first = frame->pts;
		RenderFrame(frame);
	}

	void RenderFrame(VideoFrame *frame)
	{
		time_handler(first, frame->pts);
		UpdateFrameRate();

		glViewport(0, 0, display_width, display_height);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);

		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, frame->gl_pbo);
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
		SwapBuffers(hdc);
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
			SetWindowTextA(hwnd, title);
			frameCount = 0;
			start = now;
		}
	}
};