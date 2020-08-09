#pragma once

#define BLOCK_SIZE   102400000 // 100MB

class SDCardReader
{
private:
	int num_files;
	progresshandler progress_handler;
	bool waiting = false;
	uint8_t *buffer_start, *client_posn, *buffer_read;
	BOOL end_of_file;
	HANDLE more_packets_ready;
	size_t totalSize;

public:
	char** filenames;
	SDCardReader(char** filenames, int num_files, progresshandler progress_handler, size_t totalSize)
	{
		this->end_of_file = false;
		this->filenames = filenames;
		this->num_files = num_files;
		this->progress_handler = progress_handler;
		this->totalSize = totalSize;
	}

	~SDCardReader()
	{
		CloseHandle(more_packets_ready);
		delete[] buffer_start;
	}

	HANDLE StartThread()
	{
		client_posn = buffer_read = buffer_start = new unsigned char[totalSize];
		more_packets_ready = CreateEvent(0, FALSE, FALSE, 0);
		return CreateThread(0, 0, sourceThreadProc, (LPVOID)this, 0, 0);
	}

	uint8_t* getNextPacket()
	{
		while (true)
		{
			if (client_posn + 192 <= buffer_read)
			{
				if ((client_posn - buffer_start) % 1920000 == 0)
					progress_handler(1, (client_posn - buffer_start) / 192, (buffer_read - buffer_start) / 192, "packets");

				uint8_t* current = client_posn;
				client_posn += 192;
				return current;
			}
			else
			{
				if (end_of_file)
				{
					progress_handler(1, (buffer_read - buffer_start) / 192, (buffer_read - buffer_start) / 192, "packets");
					return NULL;
				}
				else
				{
					waiting = true;
					WaitForSingleObject(more_packets_ready, INFINITE);
					waiting = false;
				}
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
		size_t total_read = 0;

		for (int i = 0; i < num_files; i++)
		{
			FILE *file = fopen(filenames[i], "rb");
			if (file == NULL) MessageBoxA(NULL, filenames[i], "Error: cannot open file", MB_OK);
			_fseeki64(file, 0, SEEK_END);
			long long file_size = _ftelli64(file);
			_fseeki64(file, 0, SEEK_SET);

			long long file_read = 0;

			if (i == 0)
			{
				fread(buffer_start, 1, 4, file);
				total_read += 4;
				file_read += 4;
			}

			while (file_read < file_size)
			{
				size_t bytes_read = fread(buffer_read, 1, min(BLOCK_SIZE, file_size - file_read), file);
				total_read += bytes_read;
				file_read += bytes_read;
				buffer_read += bytes_read;

				progress_handler(0, total_read/1024000, totalSize/1024000, "MB");

				if (waiting && bytes_read > 0)
					SetEvent(more_packets_ready);
			}

			fclose(file);
		}

		assert(total_read == totalSize);
		end_of_file = true;
	}
};