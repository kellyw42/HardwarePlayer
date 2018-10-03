#include <windows.h>
#include <math.h>
#include <gl/gl.h>

#pragma comment(lib, "opengl32.lib")

struct MyWindow
{
	HDC   hdc;
	HGLRC hglrc;
	int width, height;
};


void display(MyWindow* myWindow)
{
	wglMakeCurrent(myWindow->hdc, myWindow->hglrc);

	glViewport(0, 0, myWindow->width, myWindow->height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(45.0, (float)myWindow->width / (float)myWindow->height, 1, 1000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//gluLookAt(0, 0, 10, 0, 0, 0, 0, 1, 0);

	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static float i = 0.01f;

	i += 0.001f;

	float c = (float)cos(i);
	float s = (float)sin(i);

	glBegin(GL_TRIANGLES);
	glColor3f(c, 0, 0);
	glVertex3f(1 + c, 0 + s, 0);

	glColor3f(c, s, 0);
	glVertex3f(0 + c, 1 + s, 0);

	glColor3f(s, 0.1f, s);
	glVertex3f(-1 + c, 0 + s, 0);
	glEnd();

	SwapBuffers(myWindow->hdc);

	wglMakeCurrent(0, 0);
}

void MyCreateWindow(HINSTANCE hInstance, MyWindow* myWindow)
{
	WNDCLASS wc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = DefWindowProc;
	wc.lpszClassName = TEXT("Wayne");
	wc.lpszMenuName = 0;
	wc.style = CS_OWNDC;
	RegisterClass(&wc);

	RECT rect;
	SetRect(&rect, 50, 50, 850, 650);

	myWindow->width = rect.right - rect.left;
	myWindow->height = rect.bottom - rect.top;

	AdjustWindowRect(&rect, 0, false);

	HWND hwnd = CreateWindow(TEXT("Wayne"), 0, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);

	SetWindowLong(hwnd, GWL_STYLE, 0);

	ShowWindow(hwnd, SW_SHOW);

	myWindow->hdc = GetDC(hwnd);

	PIXELFORMATDESCRIPTOR pfd = { 0 };
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 32;

	int chosenPixelFormat = ChoosePixelFormat(myWindow->hdc, &pfd);
	SetPixelFormat(myWindow->hdc, chosenPixelFormat, &pfd);

	myWindow->hglrc = wglCreateContext(myWindow->hdc);
}

void MyEventLoop(MyWindow* myWindow1, MyWindow* myWindow2)
{
	MSG msg;

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		display(myWindow1);
		display(myWindow2);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
{
	MyWindow myWindow1, myWindow2;

	MyCreateWindow(hInstance, &myWindow1);
	MyCreateWindow(hInstance, &myWindow2);

	MyEventLoop(&myWindow1, &myWindow2);

	return 0;
}