#include <Windows.h>

typedef void(__stdcall *eventhandler)(int);

typedef void(__stdcall *timehandler)(int, int);

void __stdcall event_handler(int a)
{

}

void __stdcall time_handler(int b, int c)
{

}

class VideoBuffer;

extern "C" __declspec(dllimport) void Init();

extern "C" __declspec(dllimport) VideoBuffer* OpenVideo(char* filename, eventhandler event_handler, timehandler time_handler);

extern "C" __declspec(dllexport) void PlayPause();

int main()
{
	Init();

	Sleep(10);

	VideoBuffer *buffer = OpenVideo("C:\\PhotoFinish\\Meets2\\2019-09-06\\Track1-Finish-17-47-04.MTS", event_handler, time_handler);

	//Sleep(10);

	PlayPause();

	while (true);

    return 0;
}

