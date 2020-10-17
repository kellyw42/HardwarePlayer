enum Mode { PLAYING, PAUSED, REWINDING, SEARCHING };

using namespace std::chrono;

int drawing = 0;

int speed;
Mode mode = PAUSED;

void busyWait(int milliseconds)
{
	auto start = high_resolution_clock::now();
	while (true)
	{
		auto now = high_resolution_clock::now();
		duration<double> seconds = now - start;
		if (seconds.count()*1000 >= milliseconds)
			return;
	}
}

int delay = 10;

bool crossing = false;

void Render()
{
	switch (mode)
	{
		case Mode::PLAYING:
		{
			VideoFrame *frame;
			if (speed == 1)
			{
				frame = videoBuffer->NextFrame(0);
				Sleep(delay);
				if (crossing)
				{
					if (frame->hits == 0)
						crossing = false;
				}
				else
				{
					if (frame->hits > 0)
					{
						mode = Mode::PAUSED;
						crossing = true;
					}
				}
			}
			else
				frame = videoBuffer->FastForwardImage(speed);

			RenderFrame(frame);

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

void DoPlay()
{
	speed = 1;
	mode = PLAYING;
}

void DoPause()
{
	mode = PAUSED;
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

int dx = 0, dy = 0;

extern float angle, aspect;

extern float eyeX, eyeY, eyeZ;
extern float lookX, lookY, lookZ;
extern float upX, upY, upZ;

void UpCommand()
{
	if (dx == 0 && dy == 0) eyeX += 0.01f;
	if (dx == 1 && dy == 0) eyeY += 0.01f;
	if (dx == 2 && dy == 0) eyeZ += 0.01f;
	if (dx == 0 && dy == 1) lookX += 0.01f;
	if (dx == 1 && dy == 1) lookY += 0.01f;
	if (dx == 2 && dy == 1) lookZ += 0.01f;
	if (dx == 0 && dy == 2) upX += 0.01f;
	if (dx == 1 && dy == 2) upY += 0.01f;
	if (dx == 2 && dy == 2) upZ += 0.01f;

	if (dx == 0 && dy == 3) angle += 0.01f;
	if (dx == 1 && dy == 3) aspect += 0.01f;

	RenderFrame(latest);
}



void DownCommand()
{
	if (dx == 0 && dy == 0) eyeX -= 0.01f;
	if (dx == 1 && dy == 0) eyeY -= 0.01f;
	if (dx == 2 && dy == 0) eyeZ -= 0.01f;
	if (dx == 0 && dy == 1) lookX -= 0.01f;
	if (dx == 1 && dy == 1) lookY -= 0.01f;
	if (dx == 2 && dy == 1) lookZ -= 0.01f;
	if (dx == 0 && dy == 2) upX -= 0.01f;
	if (dx == 1 && dy == 2) upY -= 0.01f;
	if (dx == 2 && dy == 2) upZ -= 0.01f;

	if (dx == 0 && dy == 3) angle -= 0.01f;
	if (dx == 1 && dy == 3) aspect -= 0.01f;
	RenderFrame(latest);
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

void Search(VideoBuffer *startBuffer, CUvideotimestamp pts)
{
	mode = Mode::SEARCHING;
	if (videoBuffer != startBuffer)
	{
		prevLuminance = MAXLONGLONG;
		videoBuffer = startBuffer;
		RenderFrame(startBuffer->GotoTime(pts));
	}
}

void EventLoop()
{
	MSG msg;

	while (1)
	{
		//StartTime(0);
		while (true)
		{	
			//StartTime(1);
			BOOL messageWaiting = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
			//EndTime(1);

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
					case  Messages::OPENVIDEOSTART:
					case  Messages::OPENVIDEOFINISH:
					{
						videoBuffer = (VideoBuffer*)msg.wParam;
						VideoSource1* source = (VideoSource1*)msg.lParam;
						videoBuffer->Open(source, msg.message == Messages::OPENVIDEOFINISH);
						break;
					}
					case  Messages::CLOSE:
					{
						videoBuffer = (VideoBuffer*)msg.wParam;
						delete videoBuffer;
						break;
					}
					case  Messages::GOTO:
					{
						SetActiveWindow(hwnd);
						SetFocus(hwnd);
						videoBuffer = (VideoBuffer*)msg.wParam;
						assert(videoBuffer != NULL);
						CUvideotimestamp pts = (CUvideotimestamp)msg.lParam;
						RenderFrame(videoBuffer->GotoTime(pts));
						break;
					}
					case  Messages::PAUSE:
					{
						DoPause();
						break;
					}
					case  Messages::PLAY:
					{
						DoPlay();
						break;
					}
					case  Messages::PLAYPAUSE:
					{
						if (mode == PAUSED)
							DoPlay();
						else
							DoPause();
						break;
					}
					case  Messages::UP:
					{
						UpCommand();
						break;
					}
					case  Messages::DOWN:
					{
						DownCommand();
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
						VideoBuffer* start = (VideoBuffer*)msg.wParam;
						CUvideotimestamp pts = (CUvideotimestamp)msg.lParam;
						Search(start, pts);
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

		//StartTime(2);
		Render();
		//EndTime(2);

		//EndTime(0);
	}
}




void StartRectangle(LPARAM lParam)
{
	int x = GET_X_LPARAM(lParam);
	int y = GET_Y_LPARAM(lParam);

	if (x < 300 && y < 200)
	{
		dx = x / 100;
		dy = y / 50;
	}

	if (videoBuffer == NULL)
		return;

	videoBuffer->searchRect.left = videoBuffer->searchRect.right = x;
	videoBuffer->searchRect.top = videoBuffer->searchRect.bottom = y;

	drawing = 1;
	RenderFrame(latest);
}

void StartCrop(LPARAM lParam)
{
	if (videoBuffer == NULL)
		return;

	videoBuffer->cropRect.left = videoBuffer->cropRect.right = GET_X_LPARAM(lParam);
	videoBuffer->cropRect.top = videoBuffer->cropRect.bottom = GET_Y_LPARAM(lParam);

	drawing = 2;
	RenderFrame(latest);
}

void StretchRectangle(LPARAM lParam)
{
	if (drawing == 1)
	{
		videoBuffer->searchRect.right = GET_X_LPARAM(lParam);
		videoBuffer->searchRect.bottom = GET_Y_LPARAM(lParam);
		RenderFrame(latest);
	}
	else if (drawing == 2)
	{
		videoBuffer->cropRect.right = GET_X_LPARAM(lParam);
		videoBuffer->cropRect.bottom = GET_Y_LPARAM(lParam);
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);

		RenderFrame(latest);
	}
}

void DoneRectangle()
{
	drawing = 0;
}

void MoveLine(char direction)
{
	if (videoBuffer == NULL)
		return;

	if (videoBuffer->top == 0)
		videoBuffer->top = videoBuffer->bottom = 1920 / 2;

	if (direction == 'Q' || direction == 'A')
		videoBuffer->top--;
	if (direction == 'A' || direction == 'Z')
		videoBuffer->bottom--;
	if (direction == 'W' || direction == 'D')
		videoBuffer->top++;
	if (direction == 'D' || direction == 'X')
		videoBuffer->bottom++;

	videoBuffer->SaveFinishLine();

	RenderFrame(latest);
}

int MouseEvent(UINT Msg, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_LBUTTONDOWN:
		StartRectangle(lParam);
		return 0;
	case WM_RBUTTONDOWN:
		StartCrop(lParam);
		return 0;
	case WM_MOUSEMOVE:
		StretchRectangle(lParam);
		return 0;
	case WM_RBUTTONUP:
		drawing = 0;
		RenderFrame(videoBuffer->Crop(latest->pts));
		return 0;
	case WM_LBUTTONUP:
		DoneRectangle();
		return 0;
	default:
		return 1;
	}
}

int Keyup(WPARAM wParam, LPARAM lParam)
{
	if (videoBuffer) videoBuffer->InputEvent((int)wParam, 0);
	return 0;
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
		if (videoBuffer) videoBuffer->InputEvent((int)wParam, 1);
		return 0;
	}
}

LRESULT CALLBACK MyWindowProc2(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_KEYUP:
		return Keyup(wParam, lParam);
	case WM_KEYDOWN:
		return Keydown(wParam, lParam);
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
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