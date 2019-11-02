#pragma once

#define MAXFRAMES	300000

class VideoIndex
{
private:
	size_t frames = 0;
	char* filename;
	__int64* DTS;

public:
	VideoIndex(char* filename)
	{
		this->filename = filename;
		//CreateThread(NULL, 0, LoadFunction, this, 0, 0);
		//Sleep(100);
		LoadMethod();
	}

	static DWORD WINAPI LoadFunction(LPVOID lpThreadParameter)
	{
		((VideoIndex*)lpThreadParameter)->LoadMethod();
		return 0;
	}

	void LoadMethod()
	{
		FILE* DTS_file;

		DTS = new __int64[MAXFRAMES]();

		char name[256];
		sprintf_s(name, "%s.dts", filename);

		if (_access(name, 0) != -1)
		{
			fopen_s(&DTS_file, name, "rb");
			frames = fread(DTS, sizeof(__int64), MAXFRAMES, DTS_file);
			fclose(DTS_file);
		}
		else
		{
			uint8_t buffer[TS_PACKET_SIZE * BATCH];

			FILE *file;
			fopen_s(&file, filename, "rb");

			__int64 seek = 4;
			_fseeki64(file, seek, SEEK_SET);

			while (1)
			{
				size_t bytes_read = fread(buffer, 1, TS_PACKET_SIZE * BATCH, file);
				if (bytes_read == 0)
					break;

				uint8_t *buf = buffer;

				for (int read = 0; read < bytes_read; read += TS_PACKET_SIZE)
				{
					uint8_t item1 = buf[1];   // bit 0x40 = start indicator, bits 0x1F (5 bits) = PID (most significant) 
					uint8_t item2 = buf[2];   // PID (least significant)
					uint8_t item3 = buf[3];   // flags: 0x10 = 0001 0000 = PTS (adaptation field?)
					uint8_t item11 = buf[11]; // DTS indicator
					uint8_t item33 = buf[33]; // IFrame

					bool start = 0x40 & item1;
					bool payload = 0x10 & item3; 
					bool pid = (0x1F & item1) == 0x10 && (item2 == 0x11);
					bool DTS_indicator = (item11 & 0xC0) == 0xC0; // idr_pic_id
					bool IFrame = item33 == 0x27;

					if (start && pid && payload && DTS_indicator && IFrame)
					{
						int64_t item13 = (buf[13] & 0x0F) >> 1;
						int64_t item14 = (buf[14] << 8 | buf[15]) >> 1;
						int64_t item16 = (buf[16] << 8 | buf[17]) >> 1;
						uint64_t pts = (item13 << 30) | (item14 << 15) | item16;

						DTS[pts / TIME_PER_FRAME] = (seek - 4) / TS_PACKET_SIZE;
						frames++;
					}

					seek += TS_PACKET_SIZE;
					buf += TS_PACKET_SIZE;
				}
			}

			fclose(file);

			fopen_s(&DTS_file, name, "wb");
			fwrite(DTS, sizeof(__int64), MAXFRAMES, DTS_file);
			fclose(DTS_file);
		}
	}

	__int64 Lookup(CUvideotimestamp pts)
	{
		Trace("VideoIndex::Lookup(%ld);", pts);
		__int64 index = pts / TIME_PER_FRAME;
		while (index > 0 && DTS[index] == 0)
			index--;

		//printf("lookup %lld, closest = %lld ", pts, index * TIME_PER_FRAME);

		return 4 + DTS[index] * TS_PACKET_SIZE;
	}
};