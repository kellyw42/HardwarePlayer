using namespace std::chrono;

VideoBuffer* videoBuffer;

#define display_width 1920
#define  display_height 1080

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

void Line(float x0, float y0, float z0, float x1, float y1, float z1)
{
	glBegin(GL_LINES);
	glVertex3f(x0, y0, z0);
	glVertex3f(x1, y1, z1);
	glEnd();
}

void RenderFinishLine(float top, float bottom)
{
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glLineWidth(2); // was 10
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(bottom / display_width, 0);
	glVertex2f(top / display_width, 1);
	glEnd();
}

void RenderBox(RECT box, float red, float green, float blue)
{
	glLineWidth(1);
	glColor3f(red, green, blue);
	glBegin(GL_LINE_STRIP);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.top / display_height);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.bottom / display_height);
	glVertex2f((float)box.right / display_width, 1.0f - (float)box.bottom / display_height);
	glVertex2f((float)box.right / display_width, 1.0f - (float)box.top / display_height);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.top / display_height);
	glEnd();
}

extern int drawing;

void RenderOverlay(VideoFrame *frame)
{
	if (videoBuffer->top > 0)
		RenderFinishLine(videoBuffer->top, videoBuffer->bottom);

	if (drawing == 2)
		RenderBox(videoBuffer->cropRect, 0, 0, 1);

	if (videoBuffer->searchRect.right != videoBuffer->searchRect.left)
		RenderBox(videoBuffer->searchRect, 1, 0, 0);

/*
	for (int box = 0; box < frame->boundingBoxes.rows; box++)
	{
		int subArea = frame->boundingBoxes.at<int>(box, cv::CC_STAT_AREA);
		if (subArea < 1000)
			continue;

		int height = frame->boundingBoxes.at<int>(box, cv::CC_STAT_HEIGHT);
		if (height < 50)
			continue;

		RECT bounding;

		bounding.left = frame->boundingBoxes.at<int>(box, cv::CC_STAT_LEFT);
		bounding.top = 2 * frame->boundingBoxes.at<int>(box, cv::CC_STAT_TOP);
		bounding.right = bounding.left + frame->boundingBoxes.at<int>(box, cv::CC_STAT_WIDTH);
		bounding.bottom = bounding.top + 2 * height;
		RenderBox(bounding, 0, 0, 0);
	}
*/
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

GLuint prev_pbo = 0;


void LineSegment(int x0, int y0, int x1, int y1)
{
	Line(x0 / 1920.0, y0 / 1080.0, 0, x1 / 1920.0, y1 / 1080.0, 0);
}

float W = 20;

void Draw(float x, float y, int which)
{
	if (which & 1) LineSegment(x, y - W, x + W, y - W);
	if (which & 2) LineSegment(x, y - 2*W, x + W, y - 2*W);
	if (which & 4) LineSegment(x, y, x, y - W);
	if (which & 8) LineSegment(x, y - W, x, y - 2*W);
	if (which & 16) LineSegment(x+W, y, x + W, y - W);
	if (which & 32) LineSegment(x+W, y - W, x + W, y - 2*W);
	if (which & 64) LineSegment(x, y, x + W, y);
	if (which & 128) LineSegment(x+W/2, y, x + W/2, y-2*W);
	if (which & 256) LineSegment(x + W/2, y-1.5*W, x + W/2, y - 2*W);
}

int bits[10] = { 126, 128, 91, 115, 53, 103, 111, 112, 127, 119 };

void DrawNumber(int x, int y, float num)
{
	x = -1920 + x;
	y = 1080 - y;

	char buffer[10];
	sprintf(buffer, "%+4.2f", num);

	for (int i = 0; i < strlen(buffer); i++)
	{
		char ch = buffer[i];
		if (ch == '-')
			Draw(x, y, 1);
		else if (ch == '+')
			Draw(x, y, 129);
		else if (ch == '.')
			Draw(x, y, 256);
		else
			Draw(x, y, bits[(int)(ch - '0')]);

		x += W * 1.5;
	}
}

float angle = 34.0;
float aspect = 1920.0 / 1080.0;

float eyeX = 0, eyeY = -5.6, eyeZ = 2.7;
float lookX = 0.2, lookY = 2.4*1.22, lookZ = 0;
float upX = 0, upY = 0, upZ = 1;

void RenderFrame(VideoFrame *frame)
{
	//Trace("RenderFrame(%ld)", frame->pts);
	int rate = UpdateFrameRate();
	if (rate > 0)
	{
		char msg[128];
		sprintf(msg, "%d fps, frame %p, pts=%lld, hits=%d", rate, frame, frame->pts, frame->hits);
		SetWindowTextA(hwnd, msg);
	}

	int width = videoBuffer->width;
	int height = videoBuffer->height/2;

	int dx = videoBuffer->decoder->decoderParams.display_area.left;
	int dy = videoBuffer->decoder->decoderParams.display_area.top;

	float fx = ((float)dx) / display_width;
	float fy = ((float)dy) / display_height;

	float w = ((float)width) / display_width;
	float h = ((float)height) / display_height;

    //glViewport(0, 0, (int)width, (int)height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 10.0);

	// FixMe: only as required for small videos?
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(1, 0);
		glVertex2f(1, 1);
		glVertex2f(0, 1);
	glEnd();

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, frame->gl_pbo);
	glBindTexture(GL_TEXTURE_RECTANGLE, gl_texid);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, gl_shader);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); // image top left
		glVertex2f(fx + 0, 1 - fy); // screen

		glTexCoord2f(0, (float)height); // image left bottom
		glVertex2f(fx + 0, 1 - fy - 2 * h); // screen

		glTexCoord2f((float)width, (float)height); // image bottom right
		glVertex2f(fx + w, 1 - fy - 2 * h); // screen

		glTexCoord2f((float)width, 0); // image top right
		glVertex2f(fx + w, 1 - fy); // screen
	glEnd();

	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	
	RenderOverlay(frame);
	//RenderFinishLine(videoBuffer->top, videoBuffer->bottom);

/*
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	LineSegment(-10, 0, 10, -0);
	LineSegment(0, -10, 0, 10);

	DrawNumber(0, 0, eyeX);
	DrawNumber(200, 0, eyeY);
	DrawNumber(400, 0, eyeZ);
	DrawNumber(0, 100, lookX);
	DrawNumber(200, 100, lookY);
	DrawNumber(400, 100, lookZ);
	DrawNumber(0, 200, upX);
	DrawNumber(200, 200, upY);
	DrawNumber(400, 200, upZ);

	DrawNumber(0, 300, angle);
	DrawNumber(200, 300, aspect);

	gluPerspective(angle, aspect, 1.0f, 20.0f);
	
	gluLookAt(eyeX, eyeY, eyeZ, lookX, lookY, lookZ, upX, upY, upZ); 

	glBegin(GL_LINES);
	glVertex3d(0, 8 * 1.22, 0);
	glVertex3d(0, 0, 0);
	glEnd();


	for (int lane = 0; lane <= 8; lane++)
	{
		glBegin(GL_LINES);
		glVertex3d(-3000, lane * 1.22, 0);
		glVertex3d(0, lane * 1.22, 0);
		glEnd();
	}
*/

	SwapBuffers(hdc);

	glFinish();

	latest = frame;
	videoBuffer->TimeEvent(frame->pts);

	{
		//char msg[128];
		//sprintf(msg, "PTS %lld %d", frame->pts, frame->field);
		//SetWindowTextA(hwnd, msg);
	}
	
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

	//RECT rect;
	//SetRect(&rect, x[monitor], y[monitor] + 1, x[monitor] + display_width, y[monitor] + 1 + display_height);
	//AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	hwnd = CreateWindow(TEXT("Wayne"), TEXT("Video"), WS_POPUP | WS_VISIBLE, x[monitor], y[monitor], display_width, display_height, NULL, NULL, hInstance, NULL);
	//hwnd = CreateWindow(TEXT("Wayne"), TEXT("Video"), WS_OVERLAPPEDWINDOW, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
	//SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

	ShowWindow(hwnd, SW_SHOW);

	hdc = GetDC(hwnd);

	RECT client_display;
	GetClientRect(hwnd, &client_display);
	//assert(display_width == client_display.right - client_display.left);
	//assert(display_height == client_display.bottom - client_display.top);

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