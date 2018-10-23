enum Mode { PLAYING, PAUSED, REWINDING, SEARCHING };

bool drawing = false;
int speed;
Mode mode = PAUSED;

void Render()
{
	switch (mode)
	{
		case Mode::PLAYING:
		{
			VideoFrame *frame;
			if (speed == 1)
			{
				StartTime(4);
				frame = videoBuffer->NextFrame(false);
				EndTime(4);
			}
			else
				frame = videoBuffer->FastForwardImage(speed);

			StartTime(5);
			RenderFrame(frame);
			EndTime(5);

			break;
		}
		case Mode::SEARCHING:
		{
			VideoFrame * frame = videoBuffer->NextFrame(true);
			RenderFrame(frame);
			if (frame->luminance * 1.0 / prevLuminance > 1.10)
				mode = PAUSED;
			prevLuminance = frame->luminance;
			break;
		}
		case Mode::REWINDING:
		{
			VideoFrame* previous;
			if (speed == 1)
				previous = videoBuffer->PrevFrame();
			else
				previous = videoBuffer->FastRewindImage(speed);

			if (previous == latest)
			{
				mode = PAUSED;
				speed = 1;
			}
			else
				RenderFrame(previous);
			break;
		}
	}
}

void Play()
{
	speed = 1;
	mode = PLAYING;
}

void Pause()
{
	mode = PAUSED;
	recording = false;
	DisplayAverages();
}

void FastForw()
{
	if (mode == PLAYING)
		if (speed == 1)
			speed = 13;
		else
			speed *= 2;
	else
		speed = 13;
	mode = PLAYING;
}

void FastRewind()
{
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
	if (mode == PAUSED)
		RenderFrame(videoBuffer->NextFrame(NULL));
	else
		mode = PAUSED;
}

void StepPrevFrame()
{
	if (mode == PAUSED)
		RenderFrame(videoBuffer->PrevFrame());
	else
		mode = PAUSED;
}

void Search()
{
	mode = Mode::SEARCHING;
}

void EventLoop()
{
	MSG msg;

	while (1)
	{
		if (mode == Mode::PLAYING)
			recording = true;

		StartTime(0);
		while (true)
		{	
			StartTime(1);
			BOOL messageWaiting = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
			EndTime(1);

			if (!messageWaiting)
				break;

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
					videoBuffer = (VideoBuffer*)msg.wParam;
					char *filename = (char*)msg.lParam;
					RenderFrame(videoBuffer->Open(filename));
					break;
				}
				case  Messages::GOTO:
				{
					videoBuffer = (VideoBuffer*)msg.wParam;
					CUvideotimestamp pts = (CUvideotimestamp)msg.lParam;
					RenderFrame(videoBuffer->GotoTime(pts));
					break;
				}
				case  Messages::PLAYPAUSE:
				{
					if (mode == PAUSED)
						Play();
					else
						Pause();
					break;
				}
				case  Messages::STEPNEXTFRAME:
				{
					StepNextFrame();
					break;
				}
				case  Messages::STEPPREVFRAME:
				{
					StepPrevFrame();
					break;
				}
				case Messages::VISUALSEARCH:
				{
					Search();
					break;
				}
				case Messages::FASTFORWARD:
				{
					FastForw();
					break;
				}
				case Messages::REWIND:
				{
					FastRewind();
					break;
				}
				}
			}
		}

		StartTime(2);
		Render();
		EndTime(2);

		EndTime(0);
	}
}

void StartRectangle(LPARAM lParam)
{
	if (videoBuffer == NULL)
		return;

	videoBuffer->searchRect.left = videoBuffer->searchRect.right = GET_X_LPARAM(lParam);
	videoBuffer->searchRect.top = videoBuffer->searchRect.bottom = GET_Y_LPARAM(lParam);

	drawing = true;
	RenderFrame(latest);
}

void StretchRectangle(LPARAM lParam)
{
	if (drawing)
	{
		videoBuffer->searchRect.right = GET_X_LPARAM(lParam);
		videoBuffer->searchRect.bottom = GET_Y_LPARAM(lParam);
		RenderFrame(latest);
	}
}

void DoneRectangle()
{
	drawing = false;
}

void MoveLine(char direction)
{
	if (videoBuffer->top == 0)
		videoBuffer->top = videoBuffer->bottom = (float)display_width / 2;

	if (direction == 'Q' || direction == 'A')
		videoBuffer->top--;
	if (direction == 'A' || direction == 'Z')
		videoBuffer->bottom--;
	if (direction == 'W' || direction == 'D')
		videoBuffer->top++;
	if (direction == 'D' || direction == 'X')
		videoBuffer->bottom++;

	RenderFrame(latest);
}

int MouseEvent(UINT Msg, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_LBUTTONDOWN:
		StartRectangle(lParam);
		return 0;
	case WM_MOUSEMOVE:
		StretchRectangle(lParam);
		return 0;
	case WM_LBUTTONUP:
		DoneRectangle();
		return 0;
	default:
		return 1;
	}
}

int Keydown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case 'A':
	case 'D':
	case 'Q':
	case 'W':
	case 'Z':
	case 'X':
		MoveLine((char)wParam);
		return 0;
	default:
		videoBuffer->InputEvent((int)wParam);
		return 0;
	}
}

LRESULT CALLBACK MyWindowProc2(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_KEYDOWN:
		return Keydown(wParam, lParam);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MOUSEMOVE:
		return MouseEvent(Msg, lParam);
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
	}
}

DWORD WINAPI CreateVideoPlayer(LPVOID lpThreadParameter)
{
	CreateSingleWindow();

	EventLoop();

	return 0;
}