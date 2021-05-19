#pragma once

using namespace std::chrono;

#define BLOCK_SIZE   102400000 // 100MB

#define min(a,b)            (((a) < (b)) ? (a) : (b))

class SDCardReader
{
private:
	int num_files;
	progresshandler progress_handler;
	bool waiting = false;
	uint8_t* buffer_start, * client_posn, * buffer_read;
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
		auto t1 = high_resolution_clock::now();

		client_posn = buffer_read = buffer_start = new unsigned char[totalSize];

		auto t2 = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
		Trace2("allocate[%lld] = %f seconds\n", totalSize, time_span.count());

		more_packets_ready = CreateEvent(0, FALSE, FALSE, 0);
		return CreateThread(0, 0, sourceThreadProc, (LPVOID)this, 0, 0);
	}

	int counter = 0;

	long lost = 0;

	uint8_t* recoverAfterError()
	{
		while (true)
		{
			if (client_posn + 192 + 192 + 192 <= buffer_read)
			{
				for (int i = 0; i < 192; i++)
					if (client_posn[i] == 0x47 && client_posn[i + 192] == 0x47 && client_posn[i + 192 + 192] == 0x47)
					{
						uint8_t* current = client_posn + i;
						client_posn = current + 192;
						return current;
					}

				client_posn += 192;
			}
			else
			{
				if (end_of_file)
				{
					progress_handler(1, buffer_read - buffer_start, buffer_read - buffer_start, "packets");
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

	uint8_t* getNextPacket()
	{
		while (true)
		{
			if (end_of_file && client_posn >= buffer_read)
			{
				progress_handler(1, buffer_read - buffer_start, buffer_read - buffer_start, "packets");
				return NULL;
			}
			else if (client_posn + 188 <= buffer_read)
			{
				if (counter++ == 1000000)
				{
					progress_handler(1, client_posn - buffer_start, buffer_read - buffer_start, "packets");
					counter = 0;
				}

				uint8_t* current = client_posn;
				client_posn += 192;

				if (*current == 0x47)
					return current;
				else
					return recoverAfterError();
			}
			else
			{
				waiting = true;
				WaitForSingleObject(more_packets_ready, INFINITE);
				waiting = false;
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
		auto t1 = high_resolution_clock::now();

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
				// wakk 11.5%
				size_t bytes_read = fread(buffer_read, 1, min(BLOCK_SIZE, file_size - file_read), file);
				total_read += bytes_read;
				file_read += bytes_read;
				buffer_read += bytes_read;

				progress_handler(0, total_read, totalSize, "read from SD card");

				if (waiting && bytes_read > 0)
					SetEvent(more_packets_ready);
			}

			fclose(file);
		}

		assert(total_read == totalSize);
		end_of_file = true;

		auto t2 = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
		Trace2("SDCardReader.SourceThreadMethod() = %f seconds = %f MB/sec\n", time_span.count(), totalSize / time_span.count() / 1000000);
	}
};