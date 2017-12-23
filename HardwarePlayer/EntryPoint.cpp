#include <stdio.h>
#include <io.h>
#include <locale.h>
#include <Windows.h>
#include <chrono>
#include <assert.h>
#include <memory.h>
#include <vector_types.h>
#include "GL/glew.h"
#include <GL/GL.h>
#include <cuda.h>
#include <cudaGL.h>
#include "nvcuvid.h"

LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

#define BATCH 1000
#define TS_PACKET_SIZE	  192
#define TIME_PER_FIELD 1800
#define TIME_PER_FRAME (TIME_PER_FIELD*2)

#include "FrameQueue.h"
#include "VideoIndex.h"
#include "VideoSource.h"
#include "VideoDecoder.h"
#include "VideoFrame.h"
#include "VideoBuffer.h"
#include "VideoWindow.h"
#include "VideoPlayer.h"

FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }




extern "C" __declspec(dllexport) VideoPlayer* CreateVideoPlayer(eventhandler event_handler, timehandler time_handler, int monitor)
{
	return new VideoPlayer(event_handler, time_handler, monitor);
}

extern "C" __declspec(dllexport) void OpenVideo(VideoPlayer *player, char* filename)
{
	player->PostOpenVideo(filename);
}

extern "C" __declspec(dllexport) void NextFrame(VideoPlayer *player)
{
	player->StepNextFrame();
}

extern "C" __declspec(dllexport) void PrevFrame(VideoPlayer *player)
{
	player->StepPrevFrame();
}

extern "C" __declspec(dllexport) void FastForw(VideoPlayer *player)
{
	player->FastForw();
}

extern "C" __declspec(dllexport) void Rewind(VideoPlayer *player)
{
	player->Rewind();
}

extern "C" __declspec(dllexport) void GotoTime(VideoPlayer *player, CUvideotimestamp pts)
{
	player->GotoTime(pts);
}

extern "C" __declspec(dllexport) void Play(VideoPlayer *player)
{
	player->Play();
}

extern "C" __declspec(dllexport) void Pause(VideoPlayer *player)
{
	player->Pause();
}






extern "C" __declspec(dllexport) void Init()
{
	CHECK(cuInit(0));
}