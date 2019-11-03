#pragma once

#define BLOCK_SIZE   100000000 // 100MB

class SDCardReader
{
private:
	int num_files;
	progresshandler progress_handler;
	bool waiting = false;
	uint8_t *buffer_start, *client_posn, *buffer_read;
	BOOL end_of_file;
	HANDLE more_packets_ready;
	size_t maxSize;

public:
	char** filenames;
	SDCardReader(char** filenames, int num_files, progresshandler progress_handler, size_t maxSize)
	{
		this->end_of_file = false;
		this->filenames = filenames;
		this->num_files = num_files;
		this->progress_handler = progress_handler;
		this->maxSize = maxSize;
	}

	void StartThread()
	{
		//size_t maxSize = (1ULL << 31) * num_files;
		client_posn = buffer_read = buffer_start = new unsigned char[maxSize];
		progress_handler(0, 0, num_files, "files");
		more_packets_ready = CreateEvent(0, FALSE, FALSE, 0);
		CreateThread(0, 0, sourceThreadProc, (LPVOID)this, 0, 0);
	}

	uint8_t* getNextPacket()
	{
		if (client_posn + 192 <= buffer_read)
		{
			if ((client_posn - buffer_start) % 192000 == 0)
				progress_handler(1, (client_posn- buffer_start) / 192, (buffer_read- buffer_start) / 192, "packets");

			uint8_t* current = client_posn;
			client_posn += 192;
			return current;
		}
		else
		{
			if (end_of_file)
			{
				progress_handler(1, (buffer_read-buffer_start) / 192, (buffer_read - buffer_start) / 192, "packets");
				return NULL;
			}
			else
			{
				waiting = true;
				WaitForSingleObject(more_packets_ready, INFINITE);
				waiting = false;
				return getNextPacket();
			}
		}
	}

private:

	static DWORD WINAPI sourceThreadProc(LPVOID lpThreadParameter)
	{
		SDCardReader *buffer = (SDCardReader*)lpThreadParameter;
		buffer->SourceThreadMethod();
		return 0;
	}

	void SourceThreadMethod()
	{
		for (int i = 0; i < num_files; i++)
		{
			FILE *file = fopen(filenames[i], "rb");

			if (i == 0)
				fread(buffer_start, 1, 4, file);

			long long total = 0;

			while (true)
			{
				size_t bytes_read = fread(buffer_read, 1, BLOCK_SIZE, file);
				total += bytes_read;

				if (bytes_read == 0)
					break;
				buffer_read += bytes_read;
				if (waiting)
					SetEvent(more_packets_ready);
			}

			progress_handler(0, i+1, num_files, "files");

			fclose(file);
		}
		end_of_file = true;
	}
};