#pragma once

#define NumFrames	10


void RenderFrame(VideoFrame *frame);

class VideoBuffer
{
private:
	VideoConverter * videoConverter;
	VideoDecoder* decoder;
	VideoFrame* frames[NumFrames];
	timehandler time_handler;
	eventhandler event_handler;
	char finish_filename[256];

	CUvideotimestamp Index(CUvideotimestamp pts)
	{
		CUvideotimestamp frame = pts / TIME_PER_FIELD;
		return frame % NumFrames;
	}

	VideoFrame* CreateFrameFor(CUvideotimestamp pts)
	{
		CUvideotimestamp index = Index(pts);

		frames[index]->pts = pts;
		return frames[index];
	}

	VideoFrame* GetFrame(CUvideotimestamp pts)
	{
		CUvideotimestamp index = Index(pts);

		if (frames[index]->pts == pts)
			return frames[index];
		else
			return NULL;
	}

	VideoFrame* ConvertNextFrame(bool search)
	{
		CUVIDPARSERDISPINFO frameInfo = decoder->FetchFrame();

		VideoFrame *topField = CreateFrameFor(frameInfo.timestamp);
		VideoFrame *bottomField = CreateFrameFor(frameInfo.timestamp + TIME_PER_FIELD);

		videoConverter->ConvertFields(decoder->decoder, width, height, frameInfo, topField, bottomField, search ? &searchRect : NULL);

		decoder->releaseFrame(&frameInfo);
		return topField;
	}

	VideoFrame* NextUntil(CUvideotimestamp targetPts, bool search)
	{
		while (true)
		{
			VideoFrame *cached = GetFrame(targetPts);
			if (cached != NULL)
				return cached;
			CUvideotimestamp latestPts = ConvertNextFrame(search)->pts;
			assert(latestPts <= targetPts);
		}
	}

public:
	CUvideotimestamp firstPts, displayed;
	float top, bottom;
	RECT searchRect;
	int width, height;

	VideoBuffer(eventhandler event_handler, timehandler time_handler)
	{
		this->event_handler = event_handler;
		this->time_handler = time_handler;
	}

	void SaveFinishLine()
	{
		FILE *file = fopen(finish_filename, "w");
		fprintf(file, "%f,%f\n", top, bottom);
		fclose(file);
	}

	void Open(char* filename, progresshandler progress_handler)
	{
		decoder = new VideoDecoder();

		CUVIDEOFORMAT format = decoder->OpenVideo(filename, progress_handler);

		width = format.display_area.right - format.display_area.left;
		height = (format.display_area.bottom - format.display_area.top);

		for (int i = 0; i < NumFrames; i++)
			frames[i] = new VideoFrame(width, height/2);

		decoder->Init();
		decoder->Start();

		sprintf(finish_filename, "%s.finishline", filename);

		FILE *file = fopen(finish_filename, "r");
		if (file != 0)
		{
			fscanf(file, "%f,%f", &top, &bottom);
			fclose(file);
		}
		else
			top = bottom = -10;

		videoConverter = new VideoConverter();

		//return FirstFrame();
	}

	VideoFrame* FirstFrame()
	{
		VideoFrame* firstFrame = ConvertNextFrame(NULL);
		firstPts = firstFrame->pts;
		return firstFrame;
	}

	VideoFrame * GotoTime(CUvideotimestamp targetPts)
	{
		if (targetPts < firstPts)
			targetPts = firstPts;

		VideoFrame *cached = GetFrame(targetPts);
		if (cached != NULL)
			return cached;

		CUvideotimestamp fetchedPts = decoder->Goto(targetPts);
		assert(fetchedPts <= targetPts);
		return NextUntil(targetPts, false);
	}

	VideoFrame * SkipFrames(int count)
	{
		return NextUntil(displayed + TIME_PER_FIELD * count, false);
	}

	void Up()
	{
	}

	void Down()
	{
	}

	VideoFrame * NextFrame(bool search)
	{
		return NextUntil(displayed + TIME_PER_FIELD, search);
	}

	VideoFrame* PrevFrame()
	{
		return GotoTime(displayed - TIME_PER_FIELD);
	}

	VideoFrame* FastForwardImage(int speed)
	{
		return GotoTime(displayed + speed * TIME_PER_FIELD);
	}

	VideoFrame* FastRewindImage(int speed)
	{
		return GotoTime(displayed - speed * TIME_PER_FIELD);
	}

	void TimeEvent(CUvideotimestamp pts)
	{
		displayed = pts;
		if (time_handler)
			time_handler(firstPts, pts);
	}

	void InputEvent(int keyDown)
	{
		if (event_handler)
			event_handler(keyDown);
	}
};
