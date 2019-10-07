//#define CUDA_ENABLE_DEPRECATED

extern "C"
{
#include "avcodec.h"
#include "avformat.h"
}

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
#include <cuda.h>
#include <cudaGL.h>
#include <cuda_gl_interop.h>
#include "nvcuvid.h"
#include "interpolation.h"

#include "Helper.h"
#include "FrameQueue.h"
#include "VideoIndex.h"
#include "SDCardReader.h"
#include "H264Parser.h"
#include "VideoFileWriter.h"
#include "Sync.h"
#include "AudioDecoder.h"
#include "VideoSource0.h"
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

extern "C" __declspec(dllexport) VideoSource0* OpenCardVideo(char* destFilename, char* directory, int which, char** filenames, int size, progresshandler progress_handler)
{
	return new VideoSource0(destFilename, directory, which, filenames, size, progress_handler);
}

extern "C" __declspec(dllexport) void SyncAudio(ReportSync sync_handler)
{
	StartSync(sync_handler);
}

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

extern "C" __declspec(dllexport) void VisualSearch(VideoBuffer * startPlayer, VideoBuffer * finishPlayer)
{
	PostThreadMessage(eventLoopThread, Messages::VISUALSEARCH, (WPARAM)startPlayer, (LPARAM)finishPlayer);
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