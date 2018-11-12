#pragma once

#define NumFrames	10

void RenderFrame(VideoFrame *frame);

class VideoBuffer
{
private:
	VideoConverter * videoConverter;
	VideoDecoder* decoder;
	int num_fields;
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
		VideoFrame *first = NULL;
		StartTime(8);
		CUVIDPARSERDISPINFO frameInfo = decoder->FetchFrame();
		EndTime(8);

		for (int active_field = 0; active_field < num_fields; active_field++)
		{
			CUvideotimestamp pts = frameInfo.timestamp + active_field * TIME_PER_FIELD;
			VideoFrame *frame = CreateFrameFor(pts);

			StartTime(3);
			frame->luminance = videoConverter->ConvertField(decoder->decoder, width, height, frameInfo, active_field, frame->resource, search ? &searchRect : NULL);
			EndTime(3);

			if (active_field == 0)
				first = frame;
		}

		decoder->releaseFrame(&frameInfo);
		return first;
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

	VideoFrame* Open(char* filename)
	{
		decoder = new VideoDecoder();

		CUVIDEOFORMAT format = decoder->OpenVideo(filename);

		width = format.display_area.right - format.display_area.left;
		height = format.display_area.bottom - format.display_area.top;

		num_fields = format.progressive_sequence ? 1 : 2;

		for (int i = 0; i < NumFrames; i++)
			frames[i] = new VideoFrame(format.display_area.right - format.display_area.left, format.display_area.bottom - format.display_area.top);

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

		return FirstFrame();
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
