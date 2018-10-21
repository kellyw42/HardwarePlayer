#pragma once

#define NumFrames	40

void RenderFrame(VideoFrame *frame);

class VideoBuffer
{
private:
	VideoConverter * videoConverter;
	VideoDecoder* decoder;
	int num_fields;
	CUvideotimestamp displayed;
	VideoFrame* frames[NumFrames];
	CUvideotimestamp firstPts;
	timehandler time_handler;
	eventhandler event_handler;

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
		CUVIDPARSERDISPINFO frameInfo = decoder->FetchFrame();

		for (int active_field = 0; active_field < num_fields; active_field++)
		{
			CUvideotimestamp pts = frameInfo.timestamp + active_field * TIME_PER_FIELD;
			VideoFrame *frame = CreateFrameFor(pts);
			frame->luminance = videoConverter->ConvertField(decoder->decoder, width, height, frameInfo, active_field, frame->gl_pbo, search ? &searchRect : NULL);
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
	float top, bottom;
	RECT searchRect;
	int width, height;

	VideoBuffer(eventhandler event_handler, timehandler time_handler)
	{
		this->event_handler = event_handler;
		this->time_handler = time_handler;
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
