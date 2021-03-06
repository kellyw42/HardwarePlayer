#include <Windows.h>
#include <stdio.h>

typedef void(__stdcall *eventhandler)(int);

typedef void(__stdcall *timehandler)(int, int);

void __stdcall event_handler(int a)
{

}

void __stdcall time_handler(int b, int c)
{

}

class VideoBuffer;
class VideoSource0;

extern "C" typedef void(__stdcall *progresshandler)(int, long long, long long, char*);

extern "C" __declspec(dllimport) void Init();

extern "C" __declspec(dllimport) VideoBuffer* OpenVideo(char* filename, eventhandler event_handler, timehandler time_handler);

extern "C" __declspec(dllexport) void PlayPause();

extern "C" __declspec(dllexport) VideoSource0* OpenCardVideo(char* destFilename, int which, char** filenames, int count, size_t maxSize, progresshandler progress_handler);

void progress_handler(int a, long long b, long long c, char* d)
{
	printf("thread %d, completed %lld of %lld %s\n", a, b, c, d);
}

int main()
{
	char* in[1] ={"C:\\PhotoFinish\\Meets2\\2019-10-25\\Track1-Start-17-49-47.MTS" };
	OpenCardVideo("C:\\PhotoFinish\\Meets2\\2019-10-25\\Track1-Start-17-49-47.video", 0, in, 1, 10*(1ULL << 30), progress_handler);

	while (1);

	Init();

	Sleep(10);

	VideoBuffer *buffer = OpenVideo("C:\\PhotoFinish\\Meets2\\2019-10-18\\Track1-Start-17-48-23.MTS", event_handler, time_handler);

	//Sleep(10);

	PlayPause();

	while (true);

    return 0;
}

