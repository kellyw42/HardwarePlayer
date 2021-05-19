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

void RenderFinishLine2()
{
	videoBuffer->lanes->SetVirtualMatrices();

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glColor3f(1.0f, 0.0f, 0.0f);
	glLineWidth(2); // was 10

	glBegin(GL_LINES);
	glVertex3d(0, 8 * 1.22 + 34.69, 0);
	glVertex3d(0, 0, 0);
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

void RenderDot(float x, float y)
{
	glLineWidth(1);
	glBegin(GL_QUADS);

	x = x / display_width;
	y = 1 - (y / display_height);

	float w = 5.0f / display_width;
	float h = 5.0f / display_height;

	glVertex2f(x-w, y-h);
	glVertex2f(x-w, y+h);
	glVertex2f(x+w, y+h);
	glVertex2f(x+w, y-h);
	glEnd();
}

void RenderBox(RECT box)
{
	glLineWidth(1);
	glBegin(GL_LINE_STRIP);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.top / display_height);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.bottom / display_height);
	glVertex2f((float)box.right / display_width, 1.0f - (float)box.bottom / display_height);
	glVertex2f((float)box.right / display_width, 1.0f - (float)box.top / display_height);
	glVertex2f((float)box.left / display_width, 1.0f - (float)box.top / display_height);
	glEnd();
}


void RenderLaneNumber(int lane, bool first) 
{
	if (lane < 1 || lane > 8)
		return;

	lane = 9 - lane;

	float c, x, y;
	float m = -videoBuffer->lanes->skew / 3;

	if (first)
		glColor4f(0, 0.5, 0, 0.5);
	else
		glColor4f(0, 1, 0, 0.5);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUADS);

	c = lane * 1.22f;
	x = -10;
	y = m * x + c;
	glVertex2f(x, y);

	x = 0;
	y = m * x + c;
	glVertex2f(x, y);

	c = (lane-1) * 1.22f;
	x = 0;
	y = m * x + c;
	glVertex2f(x, y);
	
	x = -10;
	y = m * x + c;
	glVertex2f(x, y);

	glEnd();
}

void LineSegment(int x0, int y0, int x1, int y1)
{
	Line(x0 / 1920.0f, 1 - (y0 / 1080.0f), 0, x1 / 1920.0f, 1 - (y1 / 1080.0f), 0);
}

extern int drawing;

void SetColour(int i)
{
	switch (i) {
	case 0:
		glColor3f(1.0f, 0.0f, 0.0f);
		break;
	case 1:
		glColor3f(0.0f, 1.0f, 0.0f);
		break;
	case 2:
		glColor3f(0.0f, 0.0f, 1.0f);
		break;
	case 3:
		glColor3f(1.0f, 1.0f, 0.0f);
		break;
	case 4:
		glColor3f(1.0f, 0.0f, 1.0f);
		break;
	case 5:
		glColor3f(0.0f, 1.0f, 1.0f);
		break;
	case 6:
		glColor3f(1.0f, 0.0f, 0.0f);
		break;
	case 7:
		glColor3f(0.0f, 1.0f, 0.0f);
		break;
	case 8:
		glColor3f(0.0f, 0.0f, 1.0f);
		break;
	case 9:
		glColor3f(1.0f, 1.0f, 0.0f);
		break;
	case 10:
		glColor3f(1.0f, 0.0f, 1.0f);
		break;
	case 11:
		glColor3f(0.0f, 1.0f, 1.0f);
		break;
	default:
		glColor3f(0.0f, 0.0f, 0.0f);
	}
}

void RenderOverlay(VideoFrame *frame)
{
	//if (videoBuffer->top)
	//	RenderFinishLine(videoBuffer->top, videoBuffer->bottom);

	if (drawing == 2)
	{
		glColor3f(0, 0, 1); // blue
		RenderBox(videoBuffer->cropRect);
	}

	if (videoBuffer->searchRect.right != videoBuffer->searchRect.left)
	{
		glColor3f(1, 0, 0); // red
		RenderBox(videoBuffer->searchRect);
	}

	int i = 0;

	for (Athlete* athlete : frame->athletes)
	{
		SetColour(i);
		RenderBox(athlete->boundingBox());

		//LineSegment(athlete->centreX, (athlete->roi.y + athlete->top)    * 2, 
		//	        athlete->centreX, (athlete->roi.y + athlete->bottom) * 2);
		//LineSegment(athlete->roi.x + athlete->left,  athlete->centreY, 
		//	        athlete->roi.x + athlete->right, athlete->centreY);
/*
		Athlete* pp = athlete;
		int lane = (int)videoBuffer->lanes->getLane(athlete->feetX, athlete->feetY);
		SetColour(lane);
		RenderDot(athlete->feetX, athlete->feetY);
		for (Athlete* previous = athlete->previous; previous != NULL; previous = previous->previous)
		{
			int lane = (int)videoBuffer->lanes->getLane(previous->feetX, previous->feetY);
			SetColour(lane);
			RenderDot(previous->feetX, previous->feetY);
			SetColour(i);
			LineSegment(pp->feetX, pp->feetY, previous->feetX, previous->feetY);
			pp = previous;
		}
*/
		i++;
	}

	if (videoBuffer->isFinishCamera)
		RenderFinishLine2();

	for (Athlete* athlete : frame->athletes)
		if (athlete->crossing > 1)
			RenderLaneNumber(athlete->getLaneNumber(NULL), athlete->firstCrossing());
}

void UpdateFrameRate()
{
	frameCount++;
	high_resolution_clock::time_point now = high_resolution_clock::now();
	duration<double> time_span = duration_cast<duration<double>>(now - start);

	if (time_span.count() > 10)
	{
		int result = frameCount;
		frameCount = 0;
		start = now;

		char msg[128];
		sprintf(msg, "%d fps", result/10);
		SetWindowTextA(hwnd, msg);
	}
}

GLuint prev_pbo = 0;

void RenderVirtualLanes()
{
	videoBuffer->lanes->SetVirtualMatrices();

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glColor3f(1.0f, 0.0f, 0.0f);
	glLineWidth(2); // was 10

	glBegin(GL_LINES);
	glVertex3d(0, 8 * 1.22 + 34.69, 0);
    glVertex3d(0, 0, 0);
	glEnd();

	for (int lane = 0; lane <= 8; lane++)
	{
		glBegin(GL_LINES);

		float m = -videoBuffer->lanes->skew / 3;
		float c = lane * 1.22f;

		float x = -10;
		float y = m * x + c;
		glVertex3d(x, y, 0);

		c = lane * 1.22f;
		x = 0.5f;
		y = m * x + c;
		glVertex3d(x, y, 0);
		glEnd();
	}
}

extern bool setup_lanes;

struct Character42 {
	unsigned int TextureID;
	unsigned int width, height;
	unsigned int left, top;
	int advance;
};

Character42 alphabet[128];

// with respect to virtual coordinates ...
void RenderChar(char c, float x, float y)
{
	Character42 ch = alphabet[c];

	float aspect = (float)ch.width / ch.height;

	float left = (float)ch.left / ch.height;

	glBindTexture(GL_TEXTURE_2D, ch.TextureID);

	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex3f(x+left, y+0.25, 0);

	glTexCoord2f(0, 1);
	glVertex3f(x+left, y-0.25, 0);

	glTexCoord2f(1, 1);
	glVertex3f(x+left+0.5*aspect, y-0.25, 0);

	glTexCoord2f(1, 0);
	glVertex3f(x+left+0.5*aspect, y+0.25, 0);

	glEnd();
}

// with respect to screen coordinates ...
void RenderChar2(const char* str, float x, float y)
{
	y = display_height - y;

	glColor4f(0, 0, 1, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	float scale = 0.3;

	for (const char* c = str; *c != NULL; c++)
	{
		Character42 ch = alphabet[*c];
		glBindTexture(GL_TEXTURE_2D, ch.TextureID);

		float xpos = x + ch.left * scale;
		float ypos = y - (ch.height - ch.top) * scale;

		float w = ch.width * scale;
		float h = ch.height * scale;

		glBegin(GL_QUADS);

		glTexCoord2f(0, 0);
		glVertex2f(xpos / display_width, (ypos + h) / display_height);

		glTexCoord2f(0, 1);
		glVertex2f(xpos / display_width, ypos / display_height);

		glTexCoord2f(1, 1);
		glVertex2f((xpos + w) / display_width, ypos / display_height);

		glTexCoord2f(1, 0);
		glVertex2f((xpos + w) / display_width, (ypos + h) / display_height);

		glEnd();

		x += (ch.advance >> 6) * scale;
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}


void RenderFrame(VideoFrame *frame)
{
	//UpdateFrameRate();

	int width = frame->width;
	int height = frame->height;

	int dx = videoBuffer->decoder->decoderParams.display_area.left;
	int dy = videoBuffer->decoder->decoderParams.display_area.top;

	float fx = ((float)dx) / display_width;
	float fy = ((float)dy) / display_height;

	float w = ((float)width) / display_width;
	float h = ((float)height) / display_height;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 10.0);

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

/*
	for (Athlete* athlete : frame->athletes)
	{
		char data[128];
		sprintf(data, "height: %4.2f, longest: %4.2f", athlete->virtualHeight, athlete->hitRatio);
		RenderChar2(data, athlete->roi.x + athlete->right, (athlete->roi.y + athlete->top) * 2);
	}
*/
	RenderOverlay(frame);

	if (videoBuffer->isFinishCamera && setup_lanes)
		RenderVirtualLanes();

/*
	if (videoBuffer->isFinishCamera)
	{
		videoBuffer->lanes->SetVirtualMatrices();
		glColor4f(0, 0, 1, 1);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_TEXTURE_2D);

		for (int lane = 1; lane <= 8; lane++)
			RenderChar('0' + (9 - lane), -1, lane * 1.22 - 0.61); // was 0.-55

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}
*/

	SwapBuffers(hdc);

	glFinish();

	latest = frame;
	videoBuffer->TimeEvent(frame->pts);

	//char msg[128];
	//sprintf(msg, "pts=%lld hits=%d", frame->pts, frame->hits);
	//SetWindowTextA(hwnd, msg);
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

	// WS_POPUP or WS_OVERLAPPEDWINDOW
	hwnd = CreateWindow(TEXT("Wayne"), TEXT("Video"), WS_POPUP, x[monitor], y[monitor], display_width, display_height, NULL, NULL, hInstance, NULL);

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

	FT_Library ft;
	FT_Init_FreeType(&ft);

	FT_Face face;
	FT_New_Face(ft, "fonts/arial.ttf", 0, &face);

	FT_Set_Pixel_Sizes(face, 0, 48);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

	for (unsigned char c = 0; c < 128; c++)
	{
		FT_Load_Char(face, c, FT_LOAD_RENDER);

		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_ALPHA,
			face->glyph->bitmap.width,
			face->glyph->bitmap.rows,
			0,
			GL_ALPHA,
			GL_UNSIGNED_BYTE,
			face->glyph->bitmap.buffer
		);
		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		Character42 character = {
			texture,
			face->glyph->bitmap.width, 
			face->glyph->bitmap.rows, 
			face->glyph->bitmap_left, 
			face->glyph->bitmap_top, 
			face->glyph->advance.x };

		alphabet[c] = character;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}