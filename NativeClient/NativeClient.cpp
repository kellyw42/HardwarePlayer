typedef void(__stdcall *eventhandler)(int);

typedef void(__stdcall *timehandler)(int, int);

class VideoPlayer;

extern "C" __declspec(dllimport) void Test();

extern "C" __declspec(dllimport) void Init();

extern "C" __declspec(dllimport) void EventLoop();

extern "C" __declspec(dllimport) VideoPlayer* CreateVideoPlayer(eventhandler event_handler, timehandler time_handler, int monitor);

extern "C" __declspec(dllimport) void OpenVideo(VideoPlayer *player, char* filename);

void __stdcall event_handler(int a)
{

}

void __stdcall time_handler(int b, int c)
{

}

int main()
{
	Init();

	VideoPlayer *player = CreateVideoPlayer(event_handler, time_handler, 2);
	OpenVideo(player, "C:\\PhotoFinish\\Meets2\\2017-12-15\\Track1-Finish-17-52-13.MTS");
	
	while (1);

    return 0;
}

