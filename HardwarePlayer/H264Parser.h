#pragma once

class H264Parser
{
private:
	uint8_t *video_buffer_start, *video_packet_start;
	uint8_t *audio_buffer_start, *audio_packet_start;
	long videoPacketNr = 0, audioPacketNr = 0;

	long videoClientPos = 0, audioClientPos = 0;

	AVPacket audioPackets[1000000];
	bool videoWaiting = false, audioWaiting = false;
	HANDLE more_videopackets_ready, more_audiopackets_ready;
	char* destVideoFilename;

	SDCardReader* reader;
	progresshandler progress_handler;
	size_t totalSize;

public:
	CUVIDEOFORMAT format;
	CUVIDSOURCEDATAPACKET videoPackets[1000000];
	long videoLastPacket = -1, audioLastPacket = -1;

	H264Parser(SDCardReader* reader, progresshandler progress_handler, char* destVideoFilename, CUVIDEOFORMAT format, size_t totalSize)
	{
		this->reader = reader;
		this->progress_handler = progress_handler;
		this->destVideoFilename = destVideoFilename;
		this->format = format;
		this->totalSize = totalSize;
	}

	~H264Parser()
	{
		CloseHandle(more_audiopackets_ready);
		CloseHandle(more_videopackets_ready);

		delete[] video_buffer_start;
		delete[] audio_buffer_start;
	}

private:
	static DWORD WINAPI ParseThreadProc(LPVOID lpThreadParameter)
	{
		H264Parser *parser = (H264Parser*)lpThreadParameter;
		parser->ParseThreadMethod();
		return 0;
	}

public:
	HANDLE StartThread()
	{
		more_audiopackets_ready = CreateEvent(0, FALSE, FALSE, 0);
		more_videopackets_ready = CreateEvent(0, FALSE, FALSE, 0);
		return CreateThread(0, 0, ParseThreadProc, this, 0, 0);
	}

	AVPacket *getNextAudioPacket()
	{
		while (true)
		{
			if (audioClientPos < audioPacketNr)
			{
				if (audioClientPos % 10000 == 0)
					progress_handler(2, audioClientPos, audioPacketNr, "audio packets");
				return &audioPackets[audioClientPos++];
			}
			if (audioLastPacket > 0 && audioClientPos >= audioLastPacket)
			{
				progress_handler(2, audioClientPos, audioLastPacket, "audio packets");
				return NULL;
			}

			audioWaiting = true;
			WaitForSingleObject(more_audiopackets_ready, INFINITE);
			audioWaiting = false;
		}
	}

private:

	void GenerateVideoPacket(unsigned long size, CUvideotimestamp PTS)
	{
		CUVIDSOURCEDATAPACKET *packet = &videoPackets[videoPacketNr];
		packet->payload = video_packet_start;
		packet->payload_size = size;
		packet->flags = CUVID_PKT_TIMESTAMP;
		packet->timestamp = PTS;

		video_packet_start += size;

		if (size == 0)
			packet->flags |= CUVID_PKT_ENDOFSTREAM;

		videoPacketNr++;

		if (videoWaiting)
			SetEvent(more_videopackets_ready);
	}

	void GenerateAudioPacket(unsigned long size, CUvideotimestamp PTS)
	{
		AVPacket *packet = &audioPackets[audioPacketNr];
		packet->data = audio_packet_start;
		packet->size = size;
		packet->dts = packet->pts = PTS;
		packet->duration = 2880;
		packet->flags = 1;
		packet->stream_index = 1;

		audio_packet_start += size;

		audioPacketNr++;

		if (audioWaiting)
			SetEvent(more_audiopackets_ready);
	}

	void WriteVideoFile()
	{
		FILE *file;
		fopen_s(&file, destVideoFilename, "wb");
		if (file == NULL) MessageBoxA(NULL, destVideoFilename, "Error: cannot write file", MB_OK);

		size_t w1 = fwrite(&format, sizeof(format), 1, file);
		assert(w1 == 1);
		size_t w2 = fwrite(&videoLastPacket, sizeof(videoLastPacket), 1, file);
		assert(w2 == 1);
		size_t w3 = fwrite(videoPackets, sizeof(CUVIDSOURCEDATAPACKET), videoLastPacket, file);
		assert(w3 == videoLastPacket);
		size_t bytes = video_packet_start - video_buffer_start;
		size_t w4 = fwrite(video_buffer_start, 1, bytes, file);
		assert(w4 == bytes);

		fclose(file);
		progress_handler(3, videoLastPacket, videoLastPacket, "video packets written");
	}

	void ParseThreadMethod()
	{
		// wakk 5.6%
		video_buffer_start = video_packet_start = new uint8_t[totalSize];
		// wakk 0.15%
		audio_buffer_start = audio_packet_start = new uint8_t[1024000000];

		unsigned long video_packet_length = 0, audio_packet_length = 0;
		CUvideotimestamp video_PTS, audio_PTS;

		long long count = 0;

		while (true)
		{
			// wakk 1.0%
			uint8_t* buffer = reader->getNextPacket();
			if (buffer == NULL)
				break;

			count += 192;

			int pos = 0;

			if (buffer[pos++] != 0x47)
			{
				MessageBox(NULL, L"Missing sync byte in video file", L"Error", 0);
				continue;
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

			//Trace2("start=%d, pid=%d, payload=%d", start, pid, payload);

			if (payload)
			{
				if (pid == 0x1011)
				{
					if (start)
					{
						if (video_packet_length > 0)
						{
							GenerateVideoPacket(video_packet_length, video_PTS);
							video_packet_length = 0;
						}

						pos += 8;
						uint8_t header_length = buffer[pos++];
						pos += header_length;

						int64_t item13 = (buffer[13] & 0x0F) >> 1;
						int64_t item14 = (buffer[14] << 8 | buffer[15]) >> 1;
						int64_t item16 = (buffer[16] << 8 | buffer[17]) >> 1;
						video_PTS = (item13 << 30) | (item14 << 15) | item16;
						//printf("video_PTS=%lld\n", video_PTS);
					}

					// wakk 1.1%
					memcpy(this->video_packet_start + video_packet_length, buffer + pos, 188 - pos);
					video_packet_length += 188 - pos;
				}
				else if (pid == 0x1100)
				{
					if (start)
					{
						if (audio_packet_length > 0)
						{
							GenerateAudioPacket(audio_packet_length, audio_PTS);
							audio_packet_length = 0;
						}

						pos += 8;
						uint8_t header_length = buffer[pos++];
						pos += header_length;

						int64_t item13 = (buffer[13] & 0x0F) >> 1;
						int64_t item14 = (buffer[14] << 8 | buffer[15]) >> 1;
						int64_t item16 = (buffer[16] << 8 | buffer[17]) >> 1;
						audio_PTS = (item13 << 30) | (item14 << 15) | item16;
					}

					memcpy(audio_packet_start + audio_packet_length, buffer + pos, 188 - pos);
					audio_packet_length += 188 - pos;
				}
			}
		}
		GenerateAudioPacket(audio_packet_length, audio_PTS);

		GenerateVideoPacket(video_packet_length, video_PTS);
		GenerateVideoPacket(0, 0);

		videoLastPacket = videoPacketNr;
		SetEvent(more_videopackets_ready);

		audioLastPacket = audioPacketNr;
		SetEvent(more_audiopackets_ready);

		// wakk 0.7%
		WriteVideoFile();
	}
};