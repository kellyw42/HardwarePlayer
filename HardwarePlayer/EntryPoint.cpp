#include <stdio.h>
#include <io.h>
#include <locale.h>
#include <Windows.h>
#include <chrono>
#include <assert.h>
#include <memory.h>

#include <vector_types.h>

#define FREEGLUT_PRINT_ERRORS

#include "GL/glew.h"
#include "GL/freeglut.h"

#include <cuda.h>
#include <cudaGL.h>

#include "nvcuvid.h"

#define BATCH 1000
#define TS_PACKET_SIZE	  192
#define TIME_PER_FIELD 1800
#define TIME_PER_FRAME (TIME_PER_FIELD*2)

#include "FrameQueue.h"
#include "VideoIndex.h"
#include "VideoSource.h"
#include "VideoDecoder.h"
#include "VideoBuffer.h"
#include "VideoWindow.h"

FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }


extern "C" __declspec(dllexport) VideoWindow* CreateVideoWindow(eventhandler event_handler, timehandler time_handler, char *filename, int monitor)
{
	VideoWindow *player = new VideoWindow(event_handler, time_handler, filename, monitor);
	return player;
}

extern "C" __declspec(dllexport) void NextFrame(VideoWindow *player)
{
	player->StepNextFrame();
}

extern "C" __declspec(dllexport) void PrevFrame(VideoWindow *player)
{
	player->StepPrevFrame();
}

extern "C" __declspec(dllexport) void FastForw(VideoWindow *player)
{
	player->FastForw();
}

extern "C" __declspec(dllexport) void Rewind(VideoWindow *player)
{
	player->Rewind();
}

extern "C" __declspec(dllexport) void GotoTime(VideoWindow *player, CUvideotimestamp pts)
{
	player->GotoTime(pts);
}

extern "C" __declspec(dllexport) void Play(VideoWindow *player)
{
	player->Play();
}

extern "C" __declspec(dllexport) void Pause(VideoWindow *player)
{
	player->Pause();
}

void GlutError(const char *fmt, va_list ap)
{
	vprintf(fmt, ap);
	printf("\n");
}

void event_handler(int)
{
}

void time_handler(int a, int b)
{
}

extern "C" __declspec(dllexport) void Init()
{
	CHECK(cuInit(0));

	int argcc = 0;
	glutInit(&argcc, NULL);
	glutInitErrorFunc(GlutError);
}

HDC hdc;
HGLRC hglrc;

DWORD WINAPI loop(LPVOID lpThreadParameter)
{
	wglMakeCurrent(hdc, hglrc);

	CUcontext curr;
	CHECK(cuCtxGetCurrent(&curr));

	HGLRC ctx = wglGetCurrentContext();

	HANDLE h = GetCurrentThread();

	printf("cuda context = %p, openGL context = %p, thread = %p\n", curr, ctx, h);

	while (1)
		VideoWindow::display();
	return 0;
}

extern "C" __declspec(dllexport) void EventLoop()
{
	//glutMainLoop();
	hdc = wglGetCurrentDC();
	hglrc = wglGetCurrentContext();
	wglMakeCurrent(0, 0);
	CreateThread(0, 0, loop, 0, 0, 0);
}
