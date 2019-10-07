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
			fwrite(&packet->timestamp, sizeof(CUvideotimestamp), 1, file);
			fwrite(&packet->flags, sizeof(unsigned long), 1, file);
			fwrite(&packet->payload_size, sizeof(unsigned long), 1, file);
			fwrite(&packet->payload, sizeof(unsigned char), packet->payload_size, file);
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