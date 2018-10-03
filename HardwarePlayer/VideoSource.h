#pragma once

#define TS_SYNC			  0x47



using namespace std::chrono;

class VideoSource
{
private:
	uint8_t PES[1000000];
	uint8_t buffer[TS_PACKET_SIZE * BATCH];
	FILE *file;
	int packet_length;
	CUVIDSOURCEPARAMS params;
	bool started, waiting;
	HANDLE startEvent;
	char* filename;
	VideoIndex* index;

public:
	static DWORD WINAPI Parse(LPVOID src)
	{
		((VideoSource*)src)->Parse();
		return 0;
	}

	VideoSource(char* filename, CUVIDSOURCEPARAMS params)
	{
		this->filename = filename;
		this->params = params;

		index = new VideoIndex(filename);

		fopen_s(&file, filename, "rb");

		_fseeki64(file, 4, SEEK_SET);

		packet_length = 0;

		started = false;

		startEvent = CreateEvent(NULL, true, false, NULL);

		CreateThread(NULL, 0, Parse, this, 0, NULL);
	}

	void SendEndOfStream()
	{
		Trace("VideoSource::SendEndOfStream();");
		CUVIDSOURCEDATAPACKET packet;
		packet.payload_size = 0;
		packet.payload = PES;
		packet.flags = CUVID_PKT_ENDOFSTREAM;
		packet.timestamp = 0;
		params.pfnVideoDataHandler(params.pUserData, &packet);
	}

	CUvideotimestamp PTS;

	void SendPacket()
	{
		//Trace("VideoSource::SendPacket();");
		CUVIDSOURCEDATAPACKET packet;
		packet.payload_size = packet_length;
		packet.payload = PES;
		packet.flags = CUVID_PKT_TIMESTAMP;
		packet.timestamp = PTS;
		if (started)
			params.pfnVideoDataHandler(params.pUserData, &packet);
	}

	void Parse()
	{
		while (1)
		{
			while (!started)
			{
				waiting = true;
				WaitForSingleObject(startEvent, INFINITE);
			}
			waiting = false;

			size_t bytes_read = fread(buffer, 1, TS_PACKET_SIZE, file);
			if (bytes_read == 0)
				break;

			int pos = 0;

			if (buffer[pos++] != TS_SYNC)
			{
				fprintf(stderr, "missing sync!\n");
				exit(1);
			}

			uint8_t item1 = buffer[pos++];
			uint8_t item2 = buffer[pos++];
			uint8_t start = (0x40 & item1) >> 6;
			uint16_t pid = (0x1F & item1) << 8 | item2;

			uint8_t item3 = buffer[pos++];
			uint8_t adaptation = (0x20 & item3) >> 5;
			uint8_t payload = (0x10 & item3) >> 4;
			uint8_t counter = (0xF & item3);

			if (adaptation)
			{
				uint8_t adaptationFieldLength = buffer[pos++];
				pos += adaptationFieldLength;
			}

			if (payload && pid == 0x1011)
			{
				if (start)
				{
					if (packet_length > 0)
					{
						Trace("VideoSource send packet length = %d", packet_length);
						SendPacket();
						packet_length = 0;
					}

					pos += 8;
					uint8_t header_length = buffer[pos++];
					pos += header_length;

					int64_t item13 = (buffer[13] & 0x0F) >> 1;
					int64_t item14 = (buffer[14] << 8 | buffer[15]) >> 1;
					int64_t item16 = (buffer[16] << 8 | buffer[17]) >> 1;
					PTS = (item13 << 30) | (item14 << 15) | item16;
				}

				memcpy(PES + packet_length, buffer + pos, 188 - pos);

				packet_length += 188 - pos;
			}
		}

		if (packet_length > 0)
			SendPacket();

		SendEndOfStream();
	}

	void Start()
	{
		Trace("VideoSource::Start();");
		started = true;
		SetEvent(startEvent);
	}

	void Stop()
	{
		Trace("VideoSource::Stop();");
		started = false;
		while (!waiting)
			Sleep(1);

		SendEndOfStream();
	}

	void Goto(CUvideotimestamp pts)
	{
		Trace("VideoSource::Goto(%ld);", pts);
		packet_length = 0;
 
		__int64 pos = index->Lookup(pts);
		Trace("VideoSource seek to pos = %lld", pos);
		_fseeki64(file, pos, SEEK_SET);
	}
};