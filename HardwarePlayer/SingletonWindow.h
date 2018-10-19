#define display_width 1920
#define display_height 1080

enum Mode { PLAYING, PAUSED, REWINDING, SEARCHING };

HWND hwnd;
HDC hdc;
VideoBuffer* videoBuffer;
int speed;
Mode mode = PAUSED;
GLuint gl_texid;
GLuint gl_shader;
float top, bottom;
RECT searchRect;
long long prevLuminance = MAXLONGLONG;
bool drawing = false;
VideoFrame *latest;
DWORD eventLoopThread = 0;

BOOL ListMonitors(HMONITOR m, HDC h, LPRECT p, LPARAM u)
{
	x[num] = p->left;
	y[num] = p->top;
	num++;
	return TRUE;
}

void InitCUDA()
{
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

void RenderFrame(VideoFrame *frame)
{
	if (frame == NULL) return;

	latest = frame;

	videoBuffer->TimeEvent(frame->pts);

	glViewport(0, 0, (int)(display_width), (int)(display_height));
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, frame->gl_pbo);
	glBindTexture(GL_TEXTURE_RECTANGLE, gl_texid);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, display_width, display_height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_QUADS);
	glTexCoord2f(0, (float)display_height);
	glVertex2f(0, 0);
	glTexCoord2f((float)display_width, (float)display_height);
	glVertex2f(1, 0);
	glTexCoord2f((float)display_width, 0);
	glVertex2f(1, 1);
	glTexCoord2f(0, 0);
	glVertex2f(0, 1);
	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);

	if (top > 0)
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

	if (searchRect.right != searchRect.left)
	{
		glLineWidth(1);
		glColor3f(1.0f, 0.0f, 0.0f);
		glBegin(GL_LINE_STRIP);
		glVertex2f((float)searchRect.left / display_width, 1.0f - (float)searchRect.top / display_height);
		glVertex2f((float)searchRect.left / display_width, 1.0f - (float)searchRect.bottom / display_height);
		glVertex2f((float)searchRect.right / display_width, 1.0f - (float)searchRect.bottom / display_height);
		glVertex2f((float)searchRect.right / display_width, 1.0f - (float)searchRect.top / display_height);
		glVertex2f((float)searchRect.left / display_width, 1.0f - (float)searchRect.top / display_height);
		glEnd();
	}

	glFlush();
	SwapBuffers(hdc);
}

void StartRectangle(LPARAM lParam)
{
	searchRect.left = searchRect.right = GET_X_LPARAM(lParam);
	searchRect.top = searchRect.bottom = GET_Y_LPARAM(lParam);

	drawing = true;
	RenderFrame(latest);
}

void StretchRectangle(LPARAM lParam)
{
	if (drawing)
	{
		searchRect.right = GET_X_LPARAM(lParam);
		searchRect.bottom = GET_Y_LPARAM(lParam);
		RenderFrame(latest);
	}
}

void DoneRectangle()
{
	drawing = false;
}

void MoveLine(char direction)
{
	if (top == 0)
		top = bottom = (float)display_width / 2;

	if (direction == 'Q' || direction == 'A')
		top--;
	if (direction == 'A' || direction == 'Z')
		bottom--;
	if (direction == 'W' || direction == 'D')
		top++;
	if (direction == 'D' || direction == 'X')
		bottom++;

	RenderFrame(latest);
}

void Render()
{
	if (mode == Mode::SEARCHING)
	{
		VideoFrame * frame = videoBuffer->NextFrame(&searchRect);
		RenderFrame(frame);
		if (frame->luminance * 1.0 / prevLuminance > 1.10)
			mode = PAUSED;
		prevLuminance = frame->luminance;
	}

	else if (mode == Mode::PLAYING)
	{
		if (speed == 1)
			RenderFrame(videoBuffer->NextFrame(NULL));
		else
			RenderFrame(videoBuffer->FastForwardImage(speed));
	}
	else if (mode == Mode::REWINDING)
	{
		if (speed == 1)
			RenderFrame(videoBuffer->PrevFrame());
		else
			RenderFrame(videoBuffer->FastRewindImage(speed));
	}
}

DWORD WINAPI EventLoop(LPVOID lpThreadParameter)
{
	MSG msg;

	while (1)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.hwnd)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				switch (msg.message)
				{
				case  Messages::OPENVIDEO:
				{
					char *filename = (char*)msg.lParam;
					//player->Open(filename);
					break;
				}
				case  Messages::GOTO:
				{
					CUvideotimestamp pts = (CUvideotimestamp)msg.lParam;
					videoBuffer->DisplayTime(pts);
					break;
				}
				case  Messages::PLAYPAUSE:
				{
					//player->PlayPause();
					break;
				}
				case  Messages::STEPNEXTFRAME:
				{
					//player->StepNextFrame();
					break;
				}
				case  Messages::STEPPREVFRAME:
				{
					//player->StepPrevFrame();
					break;
				}
				case Messages::VISUALSEARCH:
				{
					//player->VisualSearch();
				}
				}
			}
		}

		Render();
	}

	return 0;
}

int MouseEvent(UINT Msg, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_LBUTTONDOWN:
			StartRectangle(lParam);
			return 0;
		case WM_MOUSEMOVE:
			StretchRectangle(lParam);
			return 0;
		case WM_LBUTTONUP:
			DoneRectangle();
			return 0;
		default:
			return 1;
	}
}

int Keydown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case 'A':
		case 'D':
		case 'Q':
		case 'W':
		case 'Z':
		case 'X':
			MoveLine((char)wParam);
			return 0;
		default:
			videoBuffer->InputEvent((int)wParam);
			return 0;
	}
}

LRESULT CALLBACK MyWindowProc2(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_KEYDOWN:
			return Keydown(wParam, lParam);
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			return MouseEvent(Msg, lParam);
		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
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

void CreateSingleWindow(int monitor)
{
	num = 0;
	EnumDisplayMonitors(NULL, NULL, ListMonitors, 0);

	CreateWin32Window(monitor);

	InitCUDA();

	InitOpenGL();

	CreateThread(0, 0, EventLoop, 0, 0, &eventLoopThread);
}
