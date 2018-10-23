using namespace std::chrono;

VideoBuffer* videoBuffer;

int frameCount = 0;
int num, x[3], y[3];
HWND hwnd;
HDC hdc;
GLuint gl_texid;
GLuint gl_shader;
long long prevLuminance = MAXLONGLONG;
VideoFrame *latest;
high_resolution_clock::time_point start = high_resolution_clock::now();

BOOL ListMonitors(HMONITOR m, HDC h, LPRECT p, LPARAM u)
{
	x[num] = p->left;
	y[num] = p->top;
	num++;
	return TRUE;
}

void InitCUDA()
{
	Trace("InitCUDA() ...");
	GLenum OK = glewInit();

	CUdevice device = 0;
	CHECK(cuDeviceGet(&device, 0));

	CUcontext context = 0;
	CHECK(cuCtxCreate(&context, CU_CTX_BLOCKING_SYNC, device));
}

void InitOpenGL()
{
	glGenTextures(1, &gl_texid);

	glBindTexture(GL_TEXTURE_RECTANGLE, gl_texid);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, display_width, display_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	const char *gl_shader_code = "!!ARBfp1.0\nTEX result.color, fragment.texcoord, texture[0], RECT; \nEND";
	glGenProgramsARB(1, &gl_shader);

	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader);

	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(gl_shader_code), (GLubyte *)gl_shader_code);

	typedef bool (APIENTRY *PFNWGLSWAPINTERVALEXTPROC)(int interval);
	PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglSwapIntervalEXT(0);
}

void RenderFinishLine(float top, float bottom)
{
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glLineWidth(10);
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(bottom / display_width, 0);
	glVertex2f(top / display_width, 1);
	glEnd();
}

void RenderSearchBox(RECT box)
{
	glLineWidth(1);
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINE_STRIP);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.top / display_height);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.bottom / display_height);
	glVertex2f((float)box.right / display_width, 1.0f - (float)box.bottom / display_height);
	glVertex2f((float)box.right / display_width, 1.0f - (float)box.top / display_height);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.top / display_height);
	glEnd();
}

void RenderOverlay()
{
	if (videoBuffer->top > 0)
		RenderFinishLine(videoBuffer->top, videoBuffer->bottom);

	if (videoBuffer->searchRect.right != videoBuffer->searchRect.left)
		RenderSearchBox(videoBuffer->searchRect);
}

int UpdateFrameRate()
{
	frameCount++;
	high_resolution_clock::time_point now = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(now - start);

	if (time_span.count() > 10)
	{
		int result = frameCount;
		frameCount = 0;
		start = now;
		return result/10;
	}
	else
		return -1;
}

void RenderFrame(VideoFrame *frame)
{
	Trace("RenderFrame(%ld);", frame->pts);
	int rate = UpdateFrameRate();
	if (rate > 0)
	{
		char msg[128];
		sprintf(msg, "%d", rate);
		SetWindowTextA(hwnd, msg);
	}

	latest = frame;
	videoBuffer->TimeEvent(frame->pts);

	int width = videoBuffer->width;
	int height = videoBuffer->height;

	float w = ((float)width) / display_width;
	float h = ((float)height) / display_height;

    //glViewport(0, 0, (int)width, (int)height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);

	// FixMe: only as required for small videos?
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(1, 0);
		glVertex2f(1, 1);
		glVertex2f(0, 1);
	glEnd();

	StartTime(6);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, frame->gl_pbo);
	glBindTexture(GL_TEXTURE_RECTANGLE, gl_texid);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_QUADS);

		glTexCoord2f(0, (float)height);
		glVertex2f(0, 1-h);

		glTexCoord2f((float)width, (float)height);
		glVertex2f(w, 1-h);

		glTexCoord2f((float)width, 0);
		glVertex2f(w, 1);

		glTexCoord2f(0, 0);
		glVertex2f(0, 1);

	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);

	EndTime(6);

	RenderOverlay();

	glFlush();

	Trace("SwapBuffers(hdc)");

	StartTime(7);
	SwapBuffers(hdc);
	EndTime(7);
}

LRESULT CALLBACK MyWindowProc2(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

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
	wc.lpfnWndProc = MyWindowProc2;
	wc.lpszClassName = TEXT("Wayne");
	wc.lpszMenuName = 0;
	wc.style = CS_OWNDC;
	RegisterClass(&wc);

	RECT rect;
	SetRect(&rect, x[monitor], y[monitor] + 1, x[monitor] + display_width, y[monitor] + 1 + display_height);
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	hwnd = CreateWindow(TEXT("Wayne"), TEXT("Video"), WS_OVERLAPPEDWINDOW, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);

	SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

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

void CreateSingleWindow()
{
	CHECK(cuInit(0));

	num = 0;
	EnumDisplayMonitors(NULL, NULL, ListMonitors, 0);

	CreateWin32Window(1);

	InitCUDA();

	InitOpenGL();
}