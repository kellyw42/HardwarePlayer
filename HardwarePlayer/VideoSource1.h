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
				handleVideoData(decoder, &videoPackets[currentFrameNr++]);
		}
	}

#define MegaBytes 1024000000
#define BLOCK 0x3F

public:
	char *videoFilename;
	CUVIDEOFORMAT format;

	VideoSource1(char* videoFilename, progresshandler progress_handler)
	{
		this->videoFilename = _strdup(videoFilename);
		fopen_s(&file, videoFilename, "rb");
		if (file == NULL) MessageBoxA(NULL, videoFilename, "Error: cannot open file", MB_OK);

		_fseeki64(file, 0, SEEK_END);
		long long length = _ftelli64(file);
		_fseeki64(file, 0, SEEK_SET);

		progress_handler(-1, 0, length/MegaBytes, "MB");
		uint8_t *buffer = start_buffer = new uint8_t[length];
		long long total_read = 0;

		while (total_read < length)
		{
			progress_handler(0, total_read/MegaBytes, length/MegaBytes, "MB");
			size_t read_bytes = fread(buffer + total_read, 1, min(102400000LL,length-total_read), file);
			total_read += read_bytes;
		}
		progress_handler(0, total_read / MegaBytes, length / MegaBytes, "MB");

		format = *(CUVIDEOFORMAT*)buffer;
		buffer += sizeof(format);

		videoLastPacket = *(long*)buffer;
		buffer += sizeof(long);
		videoPackets = (CUVIDSOURCEDATAPACKET*)buffer;
		buffer += videoLastPacket * sizeof(CUVIDSOURCEDATAPACKET);

		for (long i = 0; i < videoLastPacket; i++)
		{
			if (((~i) & BLOCK) == BLOCK)
				progress_handler(1, i, videoLastPacket, "packets");
			videoPackets[i].payload = buffer;
			buffer += videoPackets[i].payload_size;
		}
		progress_handler(1, videoLastPacket, videoLastPacket, "packets");
		progress_handler(2, 0, 0, "Done!");
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
		currentFrameNr = 13 * ((targetPts - 100800) / 46800);

		if (currentFrameNr < 0)
			currentFrameNr = 0;
	}
};