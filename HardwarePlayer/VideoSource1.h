#pragma once

using namespace std::chrono;

class VideoSource1
{
private:
	FILE *file;
	bool started, waiting;
	HANDLE startEvent;
	PFNVIDSOURCECALLBACK handleVideoData;
	void *decoder;
	char videoFilename[256];
	long videoLastPacket;
	CUVIDSOURCEDATAPACKET *videoPackets;
	long currentFrameNr;

	static DWORD WINAPI Parse(LPVOID src)
	{
		((VideoSource1*)src)->Parse();
		return 0;
	}

	void Parse()
	{
		waiting = false;

		currentFrameNr = 0;

		while (true)
		{
			if (started)
				handleVideoData(decoder, &videoPackets[currentFrameNr++]);
			else
			{
				waiting = true;
				WaitForSingleObject(startEvent, INFINITE);
				waiting = false;
			}
		}
	}

#define MegaBytes 1024000000
#define BLOCK 0x3F

public:
	CUVIDEOFORMAT format;

	VideoSource1(char* MTSFilename, progresshandler progress_handler)
	{
		MTSFilename[strlen(MTSFilename) - 4] = 0;
		sprintf(videoFilename, "%s.video", MTSFilename);

		fopen_s(&file, videoFilename, "rb");
		_fseeki64(file, 0, SEEK_END);
		long long length = _ftelli64(file);
		_fseeki64(file, 0, SEEK_SET);
		uint8_t *buffer = new uint8_t[length];
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

	void Attach(void *decoder, PFNVIDSOURCECALLBACK handleVideoData)
	{
		this->handleVideoData = handleVideoData;
		this->decoder = decoder;
		started = false;
		startEvent = CreateEvent(NULL, true, false, NULL);
		CreateThread(NULL, 0, Parse, this, 0, NULL);
	}

	void Start()
	{
		started = true;
		SetEvent(startEvent);
	}

	void Stop()
	{
		started = false;
		while (!waiting)
			Sleep(1);
	}

	void Goto(CUvideotimestamp targetPts)
	{
		Stop();

		currentFrameNr = 13 * ((targetPts - 100800) / 46800);

		if (currentFrameNr < 0)
			currentFrameNr = 0;

		CUvideotimestamp pts = videoPackets[currentFrameNr].timestamp;
	}
};