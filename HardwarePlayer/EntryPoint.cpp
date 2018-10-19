#define CUDA_ENABLE_DEPRECATED

#include <stdio.h>
#include <io.h>
#include <inttypes.h>
#include <locale.h>
#include <Windows.h>
#include <Windowsx.h>
#include <chrono>
#include <assert.h>
#include <memory.h>
#include <vector_types.h>
#include "GL/glew.h"
#include <GL/GL.h>
#include <cuda.h>
#include <cudaGL.h>
//#include <cuda_gl_interop.h>
#include "nvcuvid.h"
#include <opencv/cv.hpp>
#include "opencv2/core.hpp"
#include <opencv2/core/cuda.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include "opencv2/cudaimgproc.hpp"
#include "opencv2/cudabgsegm.hpp"
#include "opencv2/video.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/cudaobjdetect.hpp"
#include <opencv2/cudawarping.hpp>
#include "opencv2/cudafilters.hpp"

//using namespace cv;
//using namespace cv::cuda;

extern "C"
{
	typedef void(__stdcall *eventhandler)(long long);
	typedef void(__stdcall *timehandler)(CUvideotimestamp, CUvideotimestamp);
}

LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

void Trace(const char* format, ...)
{
	/*
	char msg[1024];
	va_list argptr;
	va_start(argptr, format);
	vsprintf(msg, format, argptr);
	va_end(argptr);
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");
	*/
}

#define BATCH 1000
#define TS_PACKET_SIZE	  192
#define TIME_PER_FIELD 1800
#define TIME_PER_FRAME (TIME_PER_FIELD*2)


enum Messages { OPENVIDEO = WM_USER + 1, GOTO, PLAYPAUSE, STEPNEXTFRAME, STEPPREVFRAME, VISUALSEARCH };


#include "FrameQueue.h"
#include "VideoIndex.h"
#include "VideoSource.h"
#include "VideoDecoder.h"
#include "VideoFrame.h"
#include "VideoBuffer.h"
#include "VideoWindow.h"
#include "VideoPlayer.h"
#include "SingletonWindow.h"
#include "Audio.h"
#include "Lenovo.h"

FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }


extern "C" __declspec(dllexport) VideoBuffer* OpenVideo(char* filename, eventhandler event_handler, timehandler time_handler)
{
	return new VideoBuffer(filename, event_handler, time_handler);
}

extern "C" __declspec(dllexport) void GotoTime(VideoBuffer *newBuffer, CUvideotimestamp pts)
{
	videoBuffer = newBuffer;
	PostThreadMessage(eventLoopThread, Messages::GOTO, 0, (LPARAM)pts);
}

extern "C" __declspec(dllexport) void NextFrame()
{
	//player->PostStepNextFrame();
}

extern "C" __declspec(dllexport) void PrevFrame()
{
	//player->PostStepPrevFrame();
}

extern "C" __declspec(dllexport) void VisualSearch()
{
	//player->PostVisualSearch();
}

extern "C" __declspec(dllexport) void FastForw()
{
	//player->FastForw();
}

extern "C" __declspec(dllexport) void Rewind()
{
	//player->Rewind();
}

extern "C" __declspec(dllexport) void FastForward()
{
	//player->FastForw();
}

extern "C" __declspec(dllexport) void PlayPause()
{
	//player->PostPlayPause();
}

extern "C" __declspec(dllexport) void Play()
{
	//player->Play();
}

extern "C" __declspec(dllexport) void Pause()
{
	//player->Pause();
}

extern "C" __declspec(dllexport) void Init()
{
	CHECK(cuInit(0));
	CreateSingleWindow(1);
}