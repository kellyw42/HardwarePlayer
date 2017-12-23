#pragma once

enum Mode { PLAY, PAUSE, REWIND };
enum Messages { OPENVIDEO = WM_USER + 1 };


class VideoPlayer
{
private:
	VideoWindow* window;
	VideoBuffer* videoBuffer;
	int speed;
	Mode mode = PAUSE;
	eventhandler event_handler;
	DWORD eventLoopThread = 0;

public:
	VideoPlayer(eventhandler event_handler, timehandler time_handler, int monitor)
	{
		this->event_handler = event_handler;
		window = new VideoWindow(monitor, time_handler);

		videoBuffer = new VideoBuffer();

		eventLoopThread = 0;
		CreateThread(0, 0, EventLoop, this, 0, 0);

		while (!eventLoopThread)
			Sleep(1);
	}

	void PostOpenVideo(char* filename)
	{
		PostThreadMessage(eventLoopThread, OPENVIDEO, (WPARAM)this, (LPARAM)_strdup(filename));
	}

	void Open(char* filename)
	{
		CUVIDEOFORMAT format = videoBuffer->OpenVideo(filename);

		int display_width = (format.display_area.right - format.display_area.left);
		int display_height = (format.display_area.bottom - format.display_area.top);

		window->Open(filename, display_width, display_height);

		window->Bind((__int64)this);

		videoBuffer->Init();
		videoBuffer->StartDecode();

		window->FirstFrame(videoBuffer->FirstFrame());

		speed = 1;
		mode = PAUSE;
	}

	int Keydown(WPARAM wParam, LPARAM lParam)
	{
		//lParam includes repeat count (bits 0-15) and extended key (bit 24)
		switch (wParam)
		{
			case 'P':
				PlayPause();
				return 0;
			case 'R':
				Rewind();
				return 0;
			case 'F':
				FastForw();
				return 0;
			case VK_LEFT:
				StepPrevFrame();
				return 0;
			case VK_RIGHT:
				StepNextFrame();
				return 0;
			default:
				return 1;
		}
	}

	void FastForw()
	{
		if (mode == PLAY)
			speed *= 2;
		else
			speed = 2;
		mode = PLAY;
	}

	void Play()
	{
		speed = 1;
		mode = PLAY;
	}

	void Pause()
	{
		mode = PAUSE;
	}

	void PlayPause()
	{
		if (mode == PAUSE)
			Play();
		else
			Pause();
	}

	void Rewind()
	{
		if (mode == REWIND)
			speed *= 2;
		else
			speed = 1;
		mode = REWIND;
	}

	void StepNextFrame()
	{
		if (mode == PAUSE)
			window->RenderFrame(videoBuffer->NextFrame());
		else
			mode = PAUSE;
	}

	void StepPrevFrame()
	{
		if (mode == PAUSE)
			window->RenderFrame(videoBuffer->PrevFrame());
		else
			mode = PAUSE;
	}

	void Render()
	{
		if (!eventLoopThread)
			eventLoopThread = GetThreadId(GetCurrentThread());

		
		if (mode == PLAY)
		{
			if (speed == 1)
				window->RenderFrame(videoBuffer->NextFrame());
			else
				window->RenderFrame(videoBuffer->FastForwardImage(speed));
		}
		else if (mode == REWIND)
		{
			if (speed == 1)
				window->RenderFrame(videoBuffer->PrevFrame());
			else
				window->RenderFrame(videoBuffer->FastRewindImage(speed));
		}
	}

	void GotoTime(CUvideotimestamp pts)
	{
		window->RenderFrame(videoBuffer->GotoTime(pts));
	}

	static DWORD WINAPI EventLoop(LPVOID lpThreadParameter)
	{
		VideoPlayer *player = (VideoPlayer*)lpThreadParameter;

		MSG msg;

		while (1)
		{
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.hwnd)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				else
				{
					switch (msg.message)
					{
					case OPENVIDEO:
						char *filename = (char*)msg.lParam;
						player->Open(filename);
					}
				}
			}

			player->Render();
		}

		return 0;
	}
};

LRESULT CALLBACK MyWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_KEYDOWN:
	{
		VideoPlayer *player = (VideoPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		return player->Keydown(wParam, lParam);
	}
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
}