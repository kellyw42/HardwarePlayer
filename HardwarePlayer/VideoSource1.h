#pragma once

using namespace std::chrono;

class VideoSource1
{
private:
	FILE *file;
	bool paused, waiting, running;
	HANDLE startEvent;
	PFNVIDSOURCECALLBACK handleVideoData;
	void *decoder;
	long videoLastPacket;
	uint8_t *start_buffer;
	CUVIDSOURCEDATAPACKET *videoPackets;
	long currentFrameNr;

	static DWORD WINAPI Parse(LPVOID src)
	{
		((VideoSource1*)src)->Parse();
		return 0;
	}

	void Parse()
	{
		running = true;
		waiting = false;
		currentFrameNr = 0;

		while (running)
		{
			if (paused)
			{
				waiting = true;
				WaitForSingleObject(startEvent, INFINITE);
				waiting = false;
			}
			else
			{
				if (currentFrameNr == videoLastPacket-1)
					// At end of file ...
					Sleep(100);
				else
				{ 
					if (startFrame > currentFrameNr || currentFrameNr >= endFrame)
						LoadMoreFrames(currentFrameNr);
					assert(startFrame <= currentFrameNr && currentFrameNr < endFrame);
					handleVideoData(decoder, &videoPackets[currentFrameNr++]);
				}
			}
		}
	}
public:
	char *videoFilename;
	CUVIDEOFORMAT format;

#define FrameBufferSize 100000000 // 100 Mbytes

	long startFrame = 0, endFrame = 0;
	long long *offsets;
	uint8_t* bufferStart;
	CUvideotimestamp firstPts, lastPts;

	VideoSource1(char* videoFilename, progresshandler progress_handler)
	{
		this->videoFilename = _strdup(videoFilename);
		bufferStart = new uint8_t[FrameBufferSize];

		fopen_s(&file, videoFilename, "rb");
		if (file == NULL) 
			MessageBoxA(NULL, videoFilename, "Error: cannot open file", MB_OK);

		fread(&format, sizeof(format), 1, file);
		fread(&videoLastPacket, sizeof(long), 1, file);

		videoPackets = new CUVIDSOURCEDATAPACKET[videoLastPacket];
		fread(videoPackets, sizeof(CUVIDSOURCEDATAPACKET), videoLastPacket, file);

		offsets = new long long[videoLastPacket+1];
		offsets[0] = _ftelli64(file);
		for (long i = 1; i <= videoLastPacket; i++)
			offsets[i] = offsets[i - 1] + videoPackets[i - 1].payload_size;

		firstPts = LLONG_MAX;
		for (int i = 0; i < 5; i++)
			if (videoPackets[i].timestamp < firstPts)
				firstPts = videoPackets[i].timestamp;

		lastPts = LLONG_MIN;
		for (int i = 1; i < 5; i++)
			if (videoPackets[videoLastPacket-i].timestamp > lastPts)
				lastPts = videoPackets[videoLastPacket-i].timestamp;

		lastPts -= 3600 * 4; // avoid last 4 frames (diffucult to complete flush pipeline)

		progress_handler(2, 0, 0, "Done!");
	}

	void LoadMoreFrames(long start)
	{
		startFrame = start;

		_fseeki64(file, offsets[start], SEEK_SET);
		size_t bytesRead = fread(bufferStart, 1, FrameBufferSize, file);

		uint8_t* buffer = bufferStart;
		uint8_t* bufferEnd = buffer + bytesRead;
		endFrame = startFrame;
		while (buffer < bufferEnd)
		{
			videoPackets[endFrame].payload = buffer; 
			buffer += videoPackets[endFrame].payload_size;
			endFrame++;
		}
		if (buffer > bufferEnd)
		    endFrame--;

		assert(startFrame <= start && start < endFrame);
	}

	~VideoSource1()
	{
		running = false;
		if (waiting)
		{
			SetEvent(startEvent);
			while (waiting)
				Sleep(1);
		}
		CloseHandle(startEvent);
		delete[] start_buffer;

	}

	void Attach(void *decoder, PFNVIDSOURCECALLBACK handleVideoData)
	{
		this->handleVideoData = handleVideoData;
		this->decoder = decoder;
		paused = true;
		startEvent = CreateEvent(NULL, true, false, NULL);
		CreateThread(NULL, 0, Parse, this, 0, NULL);
	}

	void Start()
	{
		paused = false;
		SetEvent(startEvent);
	}

	void Pause()
	{
		paused = true;
	}

	void Wait()
	{
		while (!waiting)
			Sleep(1);
	}

	void Goto(CUvideotimestamp targetPts)
	{
		currentFrameNr = (long)(13 * ((targetPts - 100800) / 46800));

		if (currentFrameNr < 0)
			currentFrameNr = 0;
	}
};