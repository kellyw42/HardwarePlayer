#pragma once

class H264Parser
{
private:
	uint8_t PES_video[1000000], PES_audio[1000000];
	long videoPacketNr = 0, audioPacketNr = 0;
	long videoLastPacket = -1, audioLastPacket = -1;
	long videoClientPos = 0, audioClientPos = 0;
	CUVIDSOURCEDATAPACKET videoPackets[1000000];
	AVPacket audioPackets[1000000];
	bool videoWaiting = false, audioWaiting = false;
	HANDLE more_videopackets_ready, more_audiopackets_ready;

	SDCardReader* reader;
	progresshandler progress_handler;

public:
	H264Parser(SDCardReader* reader, progresshandler progress_handler)
	{
		this->reader = reader;
		this->progress_handler = progress_handler;
	}

private:
	static DWORD WINAPI ParseThreadProc(LPVOID lpThreadParameter)
	{
		H264Parser *parser = (H264Parser*)lpThreadParameter;
		parser->ParseThreadMethod();
		return 0;
	}

public:
	void StartThread()
	{
		more_audiopackets_ready = CreateEvent(0, FALSE, FALSE, 0);
		more_videopackets_ready = CreateEvent(0, FALSE, FALSE, 0);
		CreateThread(0, 0, ParseThreadProc, this, 0, 0);
	}	

	CUVIDSOURCEDATAPACKET *getNextVideoPacket()
	{
		if (videoClientPos < videoPacketNr)
		{
			if (videoClientPos % 1000 == 0)
				progress_handler(3, videoClientPos, videoPacketNr, "video packets written");
			return &videoPackets[videoClientPos++];
		}

		if (videoLastPacket > 0 && videoClientPos >= videoLastPacket)
		{
			progress_handler(3, videoClientPos, videoLastPacket, "video packets written");
			return NULL;
		}

		videoWaiting = true;
		WaitForSingleObject(more_videopackets_ready, INFINITE);
		videoWaiting = false;

		return getNextVideoPacket();
	}

	AVPacket *getNextAudioPacket()
	{
		if (audioClientPos < audioPacketNr)
		{
			if (audioClientPos % 1000 == 0)
				progress_handler(2, audioClientPos, audioPacketNr, "audio packets");
			return &audioPackets[audioClientPos++];
		}
		if (audioLastPacket > 0 && audioClientPos >= audioLastPacket)
		{
			progress_handler(2, audioClientPos, audioLastPacket, "audio packetes");
			return NULL;
		}

		audioWaiting = true;
		WaitForSingleObject(more_audiopackets_ready, INFINITE);
		audioWaiting = false;

		return getNextAudioPacket();
	}

private:

	void GenerateVideoPacket(const uint8_t* payload, unsigned long size, CUvideotimestamp PTS)
	{
		CUVIDSOURCEDATAPACKET *packet = &videoPackets[videoPacketNr];
		uint8_t* buffer = (uint8_t*)malloc(size);
		memcpy(buffer, payload, size);
		packet->payload = buffer;
		packet->payload_size = size;
		packet->flags = CUVID_PKT_TIMESTAMP;
		packet->timestamp = PTS;

		if (!payload || size == 0)
			packet->flags |= CUVID_PKT_ENDOFSTREAM;

		videoPacketNr++;

		if (videoWaiting)
			SetEvent(more_videopackets_ready);
	}

	void GenerateAudioPacket(const uint8_t* payload, unsigned long size, CUvideotimestamp PTS)
	{
		AVPacket *packet = &audioPackets[audioPacketNr];
		uint8_t* buffer = (uint8_t*)malloc(size);
		memcpy(buffer, payload, size);
		packet->data = buffer;
		packet->size = size;
		packet->dts = packet->pts = PTS;
		packet->duration = 2880;
		packet->flags = 1;
		packet->stream_index = 1;

		audioPacketNr++;

		if (audioWaiting)
			SetEvent(more_audiopackets_ready);
	}

	void ParseThreadMethod()
	{
		unsigned long video_packet_length = 0, audio_packet_length = 0;
		CUvideotimestamp PTS;

		while (true)
		{
			uint8_t* buffer = reader->getNextPacket();
			if (buffer == NULL)
				break;

			int pos = 0;

			if (buffer[pos++] != 0x47)
			{
				MessageBox(NULL, L"Missing sync byte in video file", L"Error", 0);
				return;
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

			if (payload)
			{
				if (pid == 0x1011)
				{
					if (start)
					{
						if (video_packet_length > 0)
						{
							GenerateVideoPacket(PES_video, video_packet_length, PTS);
							video_packet_length = 0;
						}

						pos += 8;
						uint8_t header_length = buffer[pos++];
						pos += header_length;

						int64_t item13 = (buffer[13] & 0x0F) >> 1;
						int64_t item14 = (buffer[14] << 8 | buffer[15]) >> 1;
						int64_t item16 = (buffer[16] << 8 | buffer[17]) >> 1;
						PTS = (item13 << 30) | (item14 << 15) | item16;
					}

					memcpy(PES_video + video_packet_length, buffer + pos, 188 - pos);

					video_packet_length += 188 - pos;
				}
				else if (pid == 0x1100)
				{
					if (start)
					{
						if (audio_packet_length > 0)
						{
							GenerateAudioPacket(PES_audio, audio_packet_length, PTS);
							audio_packet_length = 0;
						}

						pos += 8;
						uint8_t header_length = buffer[pos++];
						pos += header_length;

						int64_t item13 = (buffer[13] & 0x0F) >> 1;
						int64_t item14 = (buffer[14] << 8 | buffer[15]) >> 1;
						int64_t item16 = (buffer[16] << 8 | buffer[17]) >> 1;
						PTS = (item13 << 30) | (item14 << 15) | item16;
					}

					memcpy(PES_audio + audio_packet_length, buffer + pos, 188 - pos);

					audio_packet_length += 188 - pos;
				}
			}
		}
		GenerateAudioPacket(PES_audio, audio_packet_length, PTS);

		GenerateVideoPacket(PES_video, video_packet_length, PTS);
		GenerateVideoPacket(0, 0, 0);

		videoLastPacket = videoPacketNr;
		SetEvent(more_videopackets_ready);

		audioLastPacket = audioPacketNr;
		SetEvent(more_audiopackets_ready);
	}
};