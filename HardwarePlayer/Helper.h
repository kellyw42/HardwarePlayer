#pragma once



#define BATCH 1000
#define TS_PACKET_SIZE	  192
#define TIME_PER_FIELD 1800
#define TIME_PER_FRAME (TIME_PER_FIELD*2)

extern "C"
{
	typedef void(__stdcall *ReportSync)(int, long, double, double);
	typedef void(__stdcall *progresshandler)(int, long long, long long, char*);
	typedef void(__stdcall *eventhandler)(long long, int);
	typedef void(__stdcall *timehandler)(CUvideotimestamp);
}

enum Messages { OPENVIDEO = WM_USER + 1, GOTO, PLAYPAUSE, PAUSE, PLAY, STEPNEXTFRAME, STEPPREVFRAME, VISUALSEARCH, REWIND, FASTFORWARD, CLOSE, UP, DOWN };

void inline Trace(const char* format, ...)
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

void Trace2(const char* format, ...)
{
	char msg[1024];
	va_list argptr;
	va_start(argptr, format);
	vsprintf(msg, format, argptr);
	va_end(argptr);
	OutputDebugStringA(msg);
	OutputDebugStringA("\n");
}

void CHECK(CUresult result)
{
	if (result != CUDA_SUCCESS)
	{
		char msg[256];
		const char* errorName = NULL;
		cuGetErrorName(result, &errorName);
		sprintf(msg, "WAK Error %d %s\n", result, errorName);
 		MessageBoxA(NULL, msg, "CUDA error", MB_OK);
		exit(1);
	}
}

using namespace std::chrono;

high_resolution_clock::time_point starts[10000];
high_resolution_clock::time_point ends[10000];

int sample[10] = { 0,0,0,0,0,0,0,0,0,0};

//void StartTime(int label)
//{
//	if (recording && sample[label] < 1000)
//		starts[label * 1000 + (sample[label])] = high_resolution_clock::now();
//}
//
//void EndTime(int label)
//{
//	if (recording && sample[label] < 1000)
//	{
//		ends[label * 1000 + (sample[label])] = high_resolution_clock::now();
//		sample[label]++;
//	}
//}

void DisplayAverages()
{
	for (int label = 0; label < 9; label++)
	{
		double sum = 0;
		for (int i = 0; i < sample[label]; i++)
		{
			high_resolution_clock::time_point start = starts[label * 1000 + i];
			high_resolution_clock::time_point end = ends[label * 1000 + i];

			duration<double> time_span = duration_cast<duration<double>>(end - start);
			sum += time_span.count();
		}
		double average = sum / sample[label];

		//Trace2("average %d = %f (%d)", label, 1000000 * average, sample[label]);
	}
}
