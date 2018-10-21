#pragma once

#define display_width 1920
#define display_height 1080

#define BATCH 1000
#define TS_PACKET_SIZE	  192
#define TIME_PER_FIELD 1800
#define TIME_PER_FRAME (TIME_PER_FIELD*2)

extern "C"
{
	typedef void(__stdcall *eventhandler)(long long);
	typedef void(__stdcall *timehandler)(CUvideotimestamp, CUvideotimestamp);
}

enum Messages { OPENVIDEO = WM_USER + 1, GOTO, PLAYPAUSE, STEPNEXTFRAME, STEPPREVFRAME, VISUALSEARCH, REWIND, FASTFORWARD };

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

void CHECK(CUresult result)
{
	if (result != CUDA_SUCCESS)
	{
		char msg[256];
		const char* e = msg;
		cuGetErrorName(result, &e);
		printf("WAK Error %d %s\n", result, e);
		exit(1);
	}
}

