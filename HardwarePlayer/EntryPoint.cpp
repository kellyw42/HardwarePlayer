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
#include "nvcuvid.h"
#include <opencv/cv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/cudaimgproc.hpp>
#include <opencv2/cudabgsegm.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/cudaobjdetect.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudafilters.hpp>

#include "Helper.h"

#include "FrameQueue.h"
#include "VideoIndex.h"
#include "VideoSource.h"
#include "VideoDecoder.h"
#include "VideoFrame.h"
#include "VideoConverter.h"
#include "VideoBuffer.h"
#include "VideoWindow.h"
#include "VideoPlayer.h"
#include "Audio.h"
#include "Lenovo.h"

DWORD eventLoopThread;

extern "C" __declspec(dllexport) VideoBuffer* OpenVideo(char* filename, eventhandler event_handler, timehandler time_handler)
{
	VideoBuffer *buffer = new VideoBuffer(event_handler, time_handler);
	PostThreadMessage(eventLoopThread, Messages::OPENVIDEO, (WPARAM)buffer, (LPARAM)_strdup(filename));
	return buffer;
}

extern "C" __declspec(dllexport) void GotoTime(VideoBuffer *newBuffer, CUvideotimestamp pts)
{
	PostThreadMessage(eventLoopThread, Messages::GOTO, (WPARAM)newBuffer, (LPARAM)pts);
}

extern "C" __declspec(dllexport) void NextFrame()
{
	PostThreadMessage(eventLoopThread, Messages::STEPNEXTFRAME, 0, 0);
}

extern "C" __declspec(dllexport) void PrevFrame()
{
	PostThreadMessage(eventLoopThread, Messages::STEPPREVFRAME, 0, 0);
}

extern "C" __declspec(dllexport) void VisualSearch()
{
	PostThreadMessage(eventLoopThread, Messages::VISUALSEARCH, 0, 0);
}

extern "C" __declspec(dllexport) void Rewind()
{
	PostThreadMessage(eventLoopThread, Messages::REWIND, 0, 0);
}

extern "C" __declspec(dllexport) void FastForward()
{
	PostThreadMessage(eventLoopThread, Messages::FASTFORWARD, 0, 0);
}

extern "C" __declspec(dllexport) void PlayPause()
{
	PostThreadMessage(eventLoopThread, Messages::PLAYPAUSE, 0, 0);
}

extern "C" __declspec(dllexport) void Init()
{
	CreateThread(0, 0, CreateVideoPlayer, 0, 0, &eventLoopThread);
}