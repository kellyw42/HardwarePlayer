//#define CUDA_ENABLE_DEPRECATED

extern "C"
{
#include "avcodec.h"
#include "avformat.h"


}

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <io.h>
#include <inttypes.h>
#include <locale.h>
#include <vector>
#include <Windows.h>
#include <Windowsx.h>
#include <chrono>
#include <assert.h>
#include <memory.h>
#include <vector_types.h>
#include "GL/glew.h"
#include <GL/GL.h>
#include <GL/glut.h>



#include <cuda.h>
#include <cudaGL.h>
#include <cuda_gl_interop.h>
#include "nvcuvid.h"
//#include "interpolation.h"

#include <opencv2/opencv.hpp>

#include "Helper.h"
#include "FrameQueue.h"
#include "SDCardReader.h"
#include "H264Parser.h"
#include "Sync.h"
#include "AudioDecoder.h"
#include "VideoSource0.h"
#include "VideoSource1.h"
#include "VideoDecoder.h"
#include "VideoFrame.h"
#include "Lanes.h"
#include "Athlete.h"
#include "AthleteDetector.h"
#include "VideoConverter.h"
#include "VideoBuffer.h"
#include "VideoWindow.h"
#include "VideoPlayer.h"
#include "Audio.h"
#include "Lenovo.h"

DWORD eventLoopThread;

extern "C" __declspec(dllexport) VideoSource0* OpenCardVideo(char* destFilename, int which, char** filenames, int count, size_t totalSize, progresshandler progress_handler)
{
	return new VideoSource0(destFilename, which, filenames, count, totalSize, progress_handler);
}

extern "C" __declspec(dllexport) void SyncAudio(ReportSync sync_handler, BangHandler bang_handler)
{
	StartSync(sync_handler, bang_handler);
}

extern "C" __declspec(dllexport) void SetupLanes()
{
	PostThreadMessage(eventLoopThread, Messages::SETUPLANES, 0, 0);
}

extern "C" __declspec(dllexport) VideoSource1* LoadVideo(char* filename, progresshandler progress_handler)
{
	return new VideoSource1(filename, progress_handler);
}

extern "C" __declspec(dllexport) VideoBuffer* OpenVideo(VideoSource1* source, eventhandler event_handler, timehandler time_handler, int track)
{
	VideoBuffer *buffer = new VideoBuffer(event_handler, time_handler);
	if (track)
		PostThreadMessage(eventLoopThread, Messages::OPENVIDEOFINISH, (WPARAM)buffer, (LPARAM)source);
	else
		PostThreadMessage(eventLoopThread, Messages::OPENVIDEOSTART, (WPARAM)buffer, (LPARAM)source);
	return buffer;
}

extern "C" __declspec(dllexport) void Close(VideoBuffer* buffer)
{
	PostThreadMessage(eventLoopThread, Messages::CLOSE, (WPARAM)buffer, 0);
}

extern "C" __declspec(dllexport) void Test(CUvideotimestamp pts)
{
	PostThreadMessage(eventLoopThread, Messages::TEST, (WPARAM)pts, 0);
}

extern "C" __declspec(dllexport) void GotoTime(VideoBuffer *newBuffer, CUvideotimestamp pts)
{
	assert(newBuffer != NULL);
	PostThreadMessage(eventLoopThread, Messages::GOTO, (WPARAM)newBuffer, (LPARAM)pts);
}

extern "C" __declspec(dllexport) void Up()
{
	PostThreadMessage(eventLoopThread, Messages::UP, 0, 0);
}

extern "C" __declspec(dllexport) void Down()
{
	PostThreadMessage(eventLoopThread, Messages::DOWN, 0, 0);
}

extern "C" __declspec(dllexport) void NextFrame()
{
	PostThreadMessage(eventLoopThread, Messages::STEPNEXTFRAME, 0, 0);
}

extern "C" __declspec(dllexport) void PrevFrame()
{
	PostThreadMessage(eventLoopThread, Messages::STEPPREVFRAME, 0, 0);
}

extern "C" __declspec(dllexport) void VisualSearch(VideoBuffer * startPlayer, CUvideotimestamp pts)
{
	PostThreadMessage(eventLoopThread, Messages::VISUALSEARCH, (WPARAM)startPlayer, (LPARAM)pts);
}

extern "C" __declspec(dllexport) void Confirm()
{
	PostThreadMessage(eventLoopThread, Messages::CONFIRM, 0, 0);
}

extern "C" __declspec(dllexport) void FindStarts(CUvideotimestamp *times, VideoBuffer* startPlayer)
{
	PostThreadMessage(eventLoopThread, Messages::FINDSTARTS, (WPARAM)startPlayer, (LPARAM)times);
}

extern "C" __declspec(dllexport) void Rewind()
{
	PostThreadMessage(eventLoopThread, Messages::REWIND, 0, 0);
}

extern "C" __declspec(dllexport) void FastForward()
{
	PostThreadMessage(eventLoopThread, Messages::FASTFORWARD, 0, 0);
}

extern "C" __declspec(dllexport) void Pause()
{
	PostThreadMessage(eventLoopThread, Messages::PAUSE, 0, 0);
}

extern "C" __declspec(dllexport) void Play()
{
	PostThreadMessage(eventLoopThread, Messages::PLAY, 0, 0);
}

extern "C" __declspec(dllexport) void PlayPause()
{
	PostThreadMessage(eventLoopThread, Messages::PLAYPAUSE, 0, 0);
}

extern "C" __declspec(dllexport) void Init()
{
	CreateThread(0, 0, CreateVideoPlayer, 0, 0, &eventLoopThread);
}