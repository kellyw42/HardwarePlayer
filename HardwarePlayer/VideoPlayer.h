enum Mode { PLAYING, PAUSED, REWINDING, SEARCHING, TESTING };

using namespace std::chrono;

int drawing = 0;

int speed;
Mode mode = PAUSED;



int delay = 10;

bool crossing = false;

CUvideotimestamp lastRunner;

bool athleteCrossing(VideoFrame *frame)
{
	for (Athlete* athlete : frame->athletes)
		if (athlete->firstCrossing())
			return true;

	return false;
}

FILE* dumpFile;

VideoFrame* previouslySearched;

void Render()
{
	switch (mode)
	{
		case Mode::TESTING:
		{
			//while (true)
			{
				VideoFrame* frame = videoBuffer->NextFrame(0);
				RenderFrame(frame);
				//videoBuffer->displayed = frame->pts;

				for (Athlete* athlete : frame->athletes)
					if (athlete->firstCrossing())
					{
						if (athlete->getLaneNumber(NULL) != (int)videoBuffer->lanes->getLane(athlete->feetX, athlete->feetY))
							mode = Mode::PAUSED;
						//fprintf(dumpFile, "%lld", frame->pts);
						//fprintf(dumpFile, ",%d\n", athlete->getLaneNumber(dumpFile));
					}

				if (frame->pts > lastRunner)
				{
					fflush(dumpFile);
					mode = Mode::PAUSED;
					videoBuffer->InputEvent('T', 1);
					break;
				}
			}

			break;
		}

		case Mode::PLAYING:
		{
			VideoFrame *frame;
			if (speed == 1)
			{
				frame = videoBuffer->NextFrame(0);
				//Sleep(delay);

				bool cross = athleteCrossing(frame);
				if (cross)
					mode = Mode::PAUSED;
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
			videoBuffer->TimeEvent(frame->pts);

			if (frame->luminance * 1.0 / prevLuminance > 1.10)
			{
				//VideoFrame* summary = videoBuffer->ShowSummary(previouslySearched, frame);
				//if (summary != NULL)
				//	RenderFrame(summary);
				//else
					mode = PAUSED;
			}

			previouslySearched = frame;
			prevLuminance = previouslySearched->luminance;
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

int raceNr = 1;

void DoTest(CUvideotimestamp pts)
{
	fprintf(dumpFile, "race %d\n", raceNr++);
	mode = Mode::TESTING;
	lastRunner = pts;
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

//extern float angle, aspect, theta, skew;

//extern float eyeX, eyeY, eyeZ;
//extern float lookX, lookY, lookZ;
//extern float upX, upY, upZ;

void UpCommand()
{
}

void DownCommand()
{
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

void ConfirmLane()
{
	for (Athlete* athlete : latest->athletes)
		if (athlete->firstCrossing())
		{
			int lane = athlete->getLaneNumber(NULL);
			videoBuffer->InputEvent('0' + lane, 1);
		}
	mode = PLAYING;
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

void DoFindStarts(CUvideotimestamp* times)
{
	for (int i = 0; times[i] != 0; i++)
	{
		CUvideotimestamp pts = times[i];
		RenderFrame(videoBuffer->GotoTime(pts - TIME_PER_FIELD));
		Sleep(500);
		RenderFrame(videoBuffer->NextUntil(pts, false));
		Sleep(500);
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
					case Messages::FINDSTARTS:
					{
						videoBuffer = (VideoBuffer*)msg.wParam;
						CUvideotimestamp* times = (CUvideotimestamp*)msg.lParam;
						DoFindStarts(times);
						break;
					}
					case Messages::SETUPLANES:
					{
						setup_lanes = true;
					    RenderFrame(latest);		
						break;
					}
					case Messages::TEST:
					{
						CUvideotimestamp pts = (CUvideotimestamp)msg.wParam;
						DoTest(pts);
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
					case Messages::CONFIRM:
					{
						ConfirmLane();
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

extern HWND hwnd;

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
	else
	{
		if (videoBuffer == NULL || videoBuffer->lanes == NULL)
			return;

		HCURSOR crosshair = LoadCursor(NULL, IDC_CROSS);
		SetCursor(crosshair);

		POINT mouse;
		GetCursorPos(&mouse);
		ScreenToClient(hwnd, &mouse);

		double lane = videoBuffer->lanes->getLane(mouse.x, mouse.y);

		char str[256];
		sprintf(str, "%d,%d -> (%f,%f) = lane %d", mouse.x , mouse.y, x, y, (int)lane);
		SetWindowTextA(hwnd, str);
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
	//char msg[128];
	//sprintf(msg, "%d, %d", GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	//SetWindowTextA(hwnd, msg);


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

bool setup_lanes = false;
//extern float lane_width;

int Keydown(WPARAM wParam, LPARAM lParam)
{
	if (setup_lanes)
	{
		char ch = (char)wParam;
		if (ch == 109) // numpad '-'
		{
			setup_lanes = false;
			videoBuffer->UpdateLanes();
		}
		else if (ch == 100) // numeric left
			videoBuffer->lanes->theta += 0.01f;
		else if (ch == 102) // numeric right
			videoBuffer->lanes->theta -= 0.01f;
		else if (ch == 37) // left arrow
			videoBuffer->lanes->lookX += 0.01f;
		else if (ch == 39)// right arrow
			videoBuffer->lanes->lookX -= 0.01f;
		else if (ch == 40) // down arror
			videoBuffer->lanes->lookY += 0.01f;
		else if (ch == 38) // up arrow
			videoBuffer->lanes->lookY -= 0.01f;
		else if (ch == 104) // numeric up
			videoBuffer->lanes->skew += 0.01f;
		else if (ch == 98) // numeric down
			videoBuffer->lanes->skew -= 0.01f;

		RenderFrame(latest);

		return 0;
	}

	switch (wParam)
	{
	case 109:
		setup_lanes = true; // numpad '-'
		RenderFrame(latest);
		return 0;
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
	fopen_s(&dumpFile, "dump.csv", "w");

	CreateSingleWindow();

	EventLoop();

	return 0;
}