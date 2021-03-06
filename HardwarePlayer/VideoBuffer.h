#pragma once

#define NumFrames	10

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define min(a,b)            (((a) < (b)) ? (a) : (b))

void RenderFrame(VideoFrame *frame);

class VideoBuffer
{
public:
	VideoConverter * videoConverter;

	VideoFrame* frames[NumFrames];
	VideoFrame* summary;
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

	void TrackAthletes(std::vector<Athlete*> athletes, VideoFrame* previous)
	{
		if (previous != NULL)
			for (Athlete* athlete : athletes)
				athlete->FindPrevious(previous->athletes);
	}

	VideoFrame* ConvertNextFrame(bool search)
	{
		CUVIDPARSERDISPINFO frameInfo = decoder->FetchFrame();

		VideoFrame *topField = CreateFrameFor(frameInfo.timestamp);
		VideoFrame *bottomField = CreateFrameFor(frameInfo.timestamp + TIME_PER_FIELD);

		videoConverter->ConvertFields(decoder->decoder, width, height, frameInfo, 
			topField, bottomField, search ? &searchRect : NULL, top, bottom, searchRect, summary);

		TrackAthletes(topField->athletes, GetFrame(frameInfo.timestamp - TIME_PER_FIELD));
		TrackAthletes(bottomField->athletes, topField);

		decoder->releaseFrame(&frameInfo);
		return topField;
	}

	int xxx = 0;
	int yyy = 0;

	VideoFrame* ShowSummary(VideoFrame* previouslySearched, VideoFrame* nextFrame)
	{
		videoConverter->CopyFlash(previouslySearched, summary, xxx, yyy, searchRect);
		videoConverter->CopyFlash(nextFrame, summary, xxx+1, yyy, searchRect);

		if (yyy < 7)
			yyy++;
		else if (xxx < 14)
		{
			yyy = 0;
			xxx += 2;
		}
		else
			return NULL;

		return summary;
	}

	VideoFrame* NextUntil(CUvideotimestamp targetPts, bool search)
	{
		if (targetPts > source->lastPts)
			targetPts = source->lastPts;

		while (true)
		{
			VideoFrame *cached = GetFrame(targetPts);
			if (cached != NULL)
				return cached;
			CUvideotimestamp latestPts = ConvertNextFrame(search)->pts;
			if (latestPts > targetPts)
				return GetFrame(latestPts);
		}
	}

public:
	VideoSource1* source;
	CUvideotimestamp displayed;
	float top, bottom;
	VideoDecoder* decoder;
	RECT searchRect, cropRect;
	int width, height;
	bool isFinishCamera;
	Lanes* lanes;

	VideoBuffer(eventhandler event_handler, timehandler time_handler)
	{
		this->event_handler = event_handler;
		this->time_handler = time_handler;
	}

	void SaveFinishLine()
	{
		FILE *file = fopen(finish_filename, "w");
		if (file == NULL) MessageBoxA(NULL, finish_filename, "Error: cannot open file", MB_OK);
		fprintf(file, "%f,%f\n", top, bottom);
		fclose(file);
	}

	void Open(VideoSource1* source, bool isFinishCamera)
	{
		this->source = source;
		this->isFinishCamera = isFinishCamera;
		decoder = new VideoDecoder();

		decoder->OpenVideo(source);

		sprintf(finish_filename, "%.55s.MTS.finishline", source->videoFilename);

		FILE* file = fopen(finish_filename, "r");
		if (file != 0)
		{
			fscanf(file, "%f,%f", &top, &bottom);
			fclose(file);
		}
		else
			top = bottom = -10;

		cropRect.left = source->format.display_area.left;
		cropRect.right = source->format.display_area.right;
		cropRect.top = source->format.display_area.top;
		cropRect.bottom = source->format.display_area.bottom;

		if (isFinishCamera)
		{
			lanes = new Lanes(cropRect);
			lanes->loadLaneMarkings(source->videoFilename);
			videoConverter = new VideoConverter(isFinishCamera, lanes);
		}
		else
			videoConverter = new VideoConverter(isFinishCamera, NULL);

		decoder->Init(cropRect);

		width = decoder->decoderParams.ulTargetWidth;
		height = decoder->decoderParams.ulTargetHeight;

		for (int i = 0; i < NumFrames; i++)
			frames[i] = new VideoFrame(width, height/2);

		summary = new VideoFrame(1920, 1080 / 2);

		decoder->Start();
	}

	~VideoBuffer()
	{
		delete decoder;
		delete videoConverter;
		for (int i = 0; i < NumFrames; i++)
			delete frames[i];
	}

	void UpdateLanes()
	{
		lanes->saveLanes();
		videoConverter->UpdateLanes();
	}

	VideoFrame * Crop(CUvideotimestamp currentPts)
	{
		decoder->Pause();

		decoder->Init(cropRect);
		
		width = decoder->decoderParams.ulTargetWidth; 
		height = decoder->decoderParams.ulTargetHeight; 

		for (int i = 0; i < NumFrames; i++)
		{
			delete frames[i];
			frames[i] = new VideoFrame(width, height / 2);
		}

		return GotoTime(currentPts);
	}

	VideoFrame * GotoTime(CUvideotimestamp targetPts)
	{
		if (targetPts < source->firstPts)
			targetPts = source->firstPts;

		if (targetPts > source->lastPts)
			targetPts = source->lastPts;

		VideoFrame *cached = GetFrame(targetPts);
		if (cached != NULL)
			return cached;

		CUvideotimestamp fetchedPts = decoder->Goto(targetPts);

		CUvideotimestamp go = targetPts;
		while (fetchedPts > targetPts)
		{
			go -= 1800;
			fetchedPts = decoder->Goto(go);
		}
		assert(fetchedPts <= targetPts);
		return NextUntil(targetPts, false);
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
		if (pts > 0)
		{
			displayed = pts;
			if (time_handler)
				time_handler(pts);
		}
	}

	void InputEvent(int keyDown, int wm_keydown)
	{
		if (event_handler)
			event_handler(keyDown, wm_keydown);
	}
};
