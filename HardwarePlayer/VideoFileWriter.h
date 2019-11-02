#pragma once

class VideoFileWriter
{
private:
	progresshandler progress_handler;
	H264Parser* parser;
	char* destFilename;

	void WriteThreadMethod()
	{
		FILE *file;
		fopen_s(&file, destFilename, "wb");

		while (true)
		{
			CUVIDSOURCEDATAPACKET *packet = parser->getNextVideoPacket();
			if (packet == NULL)
				break;

			// fixme: write via larger memory buffer ???
			//Trace2("Write(filename=%s, size=%ld, PTS=%lld, data[0]=%d, data[1]=%d, data[2]=%d, data[3]=%d, data[4]=%d", 
			//	destFilename, packet->payload_size, packet->timestamp, packet->payload[0], packet->payload[1], packet->payload[2], packet->payload[3], packet->payload[4]);

			fwrite(&packet->timestamp,    sizeof(CUvideotimestamp), 1, file);
			fwrite(&packet->flags,        sizeof(unsigned long), 1, file);
			fwrite(&packet->payload_size, sizeof(unsigned long), 1, file);
			fwrite(packet->payload, 1, packet->payload_size, file);
		}

		fclose(file);
	}

	static DWORD WINAPI WriteThreadProc(LPVOID lpThreadParameter)
	{
		VideoFileWriter *writer = (VideoFileWriter*)lpThreadParameter;
		writer->WriteThreadMethod();
		return 0;
	}

public:
	VideoFileWriter(char* destFilename, H264Parser* parser, progresshandler progress_handler)
	{
		this->progress_handler = progress_handler;
		this->parser = parser;
		this->destFilename = destFilename;
	}

	void StartThread()
	{
		CreateThread(0, 0, WriteThreadProc, (LPVOID)this, 0, 0);
	}
};