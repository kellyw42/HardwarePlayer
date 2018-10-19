#pragma once

enum Mode { PLAYING, PAUSED, REWINDING, SEARCHING };
enum Messages { OPENVIDEO = WM_USER + 1, GOTO, PLAYPAUSE, STEPNEXTFRAME, STEPPREVFRAME, VISUALSEARCH};


class VideoPlayer
{
private:
	VideoWindow* window;
	VideoBuffer* videoBuffer;
	int speed;
	Mode mode = PAUSED;
	eventhandler event_handler;
	DWORD eventLoopThread = 0;

public:
	VideoPlayer(eventhandler event_handler, timehandler time_handler, int monitor)
	{
		Trace("VideoPlayer::VideoPlayer(%p, %p, %d);", event_handler, time_handler, monitor);
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
		PostThreadMessage(eventLoopThread, Messages::OPENVIDEO, (WPARAM)this, (LPARAM)_strdup(filename));
	}

	void PostGoto(CUvideotimestamp pts)
	{
		PostThreadMessage(eventLoopThread, Messages::GOTO, (WPARAM)this, (LPARAM)pts);
	}

	void PostStepNextFrame()
	{
		PostThreadMessage(eventLoopThread, Messages::STEPNEXTFRAME, (WPARAM)this, NULL);
	}

	void PostStepPrevFrame()
	{
		PostThreadMessage(eventLoopThread, Messages::STEPPREVFRAME, (WPARAM)this, NULL);
	}

	void PostPlayPause()
	{
		PostThreadMessage(eventLoopThread, Messages::PLAYPAUSE, (WPARAM)this, NULL);
	}

	void PostVisualSearch()
	{
		PostThreadMessage(eventLoopThread, Messages::VISUALSEARCH, (WPARAM)this, NULL);
	}

	void Open(char* filename)
	{
		Trace("VideoPlayer::Open(%s);", filename);
		CUVIDEOFORMAT format = videoBuffer->OpenVideo(filename);

		int display_width = (format.display_area.right - format.display_area.left);
		int display_height = (format.display_area.bottom - format.display_area.top);

		window->Open(filename, display_width, display_height);

		window->Bind((__int64)this);

		videoBuffer->Init();
		videoBuffer->StartDecode();

		window->FirstFrame(videoBuffer->FirstFrame());

		speed = 1;
		mode = PAUSED;
	}

	int MouseEvent(UINT Msg, LPARAM lParam)
	{
		//Trace("VideoPlayer::MouseEvent(%d, %d);", Msg, lParam);
		switch (Msg)
		{
		case WM_LBUTTONDOWN:
			window->StartRectangle(lParam);
			return 0;
		case WM_MOUSEMOVE:
			window->StretchRectangle(lParam);
			return 0;
		case WM_LBUTTONUP:
			window->DoneRectangle();
			return 0;
		default:
			return 1;
		}
	}

	int Keydown(WPARAM wParam, LPARAM lParam)
	{
		Trace("VideoPlayer::Keydown(%d, %d);", wParam, lParam);
		//lParam includes repeat count (bits 0-15) and extended key (bit 24)
		switch (wParam)
		{
			//case ' ':
			//	event_handler(' ');
			//	return 0;
			//case 'P':
			//	PlayPause();
			//	return 0;
			//case 'R':
			//	Rewind();
			//	return 0;
			//case 'F':
			//	FastForw();
			//	return 0;
			//case 'S':
			//	Search();
			//	return 0;
			case 'A':
			case 'D':
			case 'Q':
			case 'W':
			case 'Z':
			case 'X':
				window->MoveLine((char)wParam);
				return 0;
			default: 
				if (event_handler)
					event_handler((int)wParam);
				return 0;
			//case VK_LEFT:
			//	StepPrevFrame();
			//	return 0;
			//case VK_RIGHT:
			//	StepNextFrame();
			//	return 0;
			//default:
			//	return 1;
		}
	}

	void FastForw()
	{
		Trace("VideoPlayer::FastForw();");
		if (mode == PLAYING)
			if (speed == 1)
				speed = 13;
			else
				speed *= 2;
		else
			speed = 13;
		mode = PLAYING;
	}

	void Play()
	{
		Trace("VideoPlayer::Play();");
		speed = 1;
		mode = PLAYING;
	}

	void Pause()
	{
		Trace("VideoPlayer::Pause();");
		mode = PAUSED;
	}

	void PlayPause()
	{
		Trace("VideoPlayer::PlayPause();");
		if (mode == PAUSED)
			Play();
		else
			Pause();
	}

	void Rewind()
	{
		Trace("VideoPlayer::Rewind();");
		if (mode == REWINDING)
			if (speed == 1)
				speed = 4;
			else
				speed *= 2;
		else
			speed = 1;
		mode = REWINDING;
	}

	void StepNextFrame()
	{
		Trace("VideoPlayer::StepNextFrame();");
		if (mode == PAUSED)
			window->RenderFrame(videoBuffer->NextFrame(NULL), mode, speed);
		else
			mode = PAUSED;
	}

	void StepPrevFrame()
	{
		Trace("VideoPlayer::StepPrevFrame();");
		if (mode == PAUSED)
			window->RenderFrame(videoBuffer->PrevFrame(), mode, speed);
		else
			mode = PAUSED;
	}

	void VisualSearch()
	{
		Trace("VideoPlayer::VisualSearch();");
		mode = Mode::SEARCHING;
	}

	long long prevLuminance = MAXLONGLONG;

	void Render()
	{
		//Trace("VideoPlayer::Render();");
		if (!eventLoopThread)
			eventLoopThread = GetThreadId(GetCurrentThread());

		if (mode == Mode::SEARCHING)
		{
			VideoFrame * frame = videoBuffer->NextFrame(&window->rect);
			window->RenderFrame(frame, mode, speed);
			if (frame->luminance * 1.0 / prevLuminance > 1.10)
				mode = PAUSED;
			prevLuminance = frame->luminance;
		}
		
		else if (mode == Mode::PLAYING)
		{
			if (speed == 1)
				window->RenderFrame(videoBuffer->NextFrame(NULL), mode, speed);
			else
				window->RenderFrame(videoBuffer->FastForwardImage(speed), mode, speed);
		}
		else if (mode == Mode::REWINDING)
		{
			if (speed == 1)
				window->RenderFrame(videoBuffer->PrevFrame(), mode, speed);
			else
				window->RenderFrame(videoBuffer->FastRewindImage(speed), mode, speed);
		}
	}

	void GotoTime(CUvideotimestamp pts)
	{
		Trace("VideoPlayer::GotoTime(%lld);", pts);
		// round to nearest field frame ...
		CUvideotimestamp frames = (CUvideotimestamp)(((double)(pts - window->first) / TIME_PER_FIELD) + 0.5);
		CUvideotimestamp rounded = window->first + frames * TIME_PER_FIELD;
		Trace("rounded = %lld", pts);
		window->RenderFrame(videoBuffer->GotoTime(rounded), mode, speed);
	}

	static DWORD WINAPI EventLoop(LPVOID lpThreadParameter)
	{
		VideoPlayer *player = (VideoPlayer*)lpThreadParameter;

		MSG msg;

		while (1)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
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
						case  Messages::OPENVIDEO:
						{
							char *filename = (char*)msg.lParam;
							player->Open(filename);
							break;
						}
						case  Messages::GOTO:
						{
							CUvideotimestamp pts = (CUvideotimestamp)msg.lParam;
							player->GotoTime(pts);
							break;
						}
						case  Messages::PLAYPAUSE:
						{
							player->PlayPause();
							break;
						}
						case  Messages::STEPNEXTFRAME:
						{
							player->StepNextFrame();
							break;
						}
						case  Messages::STEPPREVFRAME:
						{
							player->StepPrevFrame();
							break;
						}
						case Messages::VISUALSEARCH:
						{
							player->VisualSearch();
						}
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
	VideoPlayer *player = (VideoPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (Msg)
	{
		case WM_KEYDOWN:
			return player->Keydown(wParam, lParam);
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			return player->MouseEvent(Msg, lParam);
		default:
			return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
}