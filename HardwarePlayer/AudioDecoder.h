#pragma once

#include <math.h>

class AudioDecoder
{
private:
	H264Parser *parser;
	progresshandler progress_handler;
	int which;

public:
	AudioDecoder(int which, H264Parser* parser, progresshandler progress_handler)
	{
		this->which = which;
		this->parser = parser;
		this->progress_handler = progress_handler;
		bangs = new long[10000];
	}

private:
	static DWORD WINAPI DecodeThreadProc(LPVOID lpThreadParameter)
	{
		AudioDecoder *decoder = (AudioDecoder*)lpThreadParameter;
		decoder->DecodeThreadMethod();
		return 0;
	}

public:
	HANDLE StartThread()
	{
		return CreateThread(0, 0, DecodeThreadProc, this, 0, 0);
	}

	bool end_of_file = false;
	long *bangs;
	long bang_count = 0;

private:
	int silent_period = 0;
	int time_since_long_silence = 10000000;
	long pos = 0;
	long start = 0;
	const int quiet = 1100;
	const int quiet_period = 60;
	const int loud = 8000;
	const int loud_period = 200;

#define ABS(a)	((a)>0?(a):-(a))
/*
	void Analyse(float* data, int length)
	{
		for (int j = 0; j < length; j++)
		{
			float volume = ABS(data[j] * 16384);

			if (volume < quiet) // 1100
				silent_period++;
			else
			{
				// is not quiet now ...
				if (silent_period > quiet_period) // 60
				{
					// is was quiet until now, so look for very loud now ...
					time_since_long_silence = 0;
					start = pos;
				}
				silent_period = 0;
			}
			time_since_long_silence++;

			if (time_since_long_silence < loud_period && volume > loud)
			{
				if (bang_count < 10000)
				{
					bangs[bang_count++] = start;
					new_bangs(which, bangs, bang_count);
				}
				time_since_long_silence = 10000000;
			}

			pos++;
		}
	}
*/

	FILE* file;

	float previousBlock[1536];
	double previousResponse = 0;
	long previousStart = 0;

	double C, S1, S2;

	void Accumulate(float* data, int start, int end)
	{
		for (int j = start; j < end; j++)
		{
			double old = S1;
			S1 = data[j] + C * S1 - S2;
			S2 = old;
		}
	}

#define CalculateResponse (S1*S1 + S2*S2 - S1*S2*C)

	const float sampleRate = 48000;
	const int packetSize = 1536;
	const int step = 8;
	bool started = false;

	const float Packet3500 = (2.0 * cos(2.0 * M_PI * round(3500 / sampleRate * packetSize) / packetSize));
	const float Packet4000 = (2.0 * cos(2.0 * M_PI * round(4000 / sampleRate * packetSize) / packetSize));

	int Threshold = 4000;

	void Analyse(float* data, int length)
	{
		float initialC;

		if (started)
			initialC = Packet4000;
		else
			initialC = Packet3500;

		C = initialC;  S1 = S2 = 0;
		Accumulate(data, 0, length);
		double response = CalculateResponse;

		float bestResponse = 0;
		long bestPos = 0;

		if (previousResponse > 1000 || response > 1000)
			for (int start = 0; start <= packetSize; start += step)
			{
				C = initialC;
				S1 = S2 = 0;
				Accumulate(previousBlock, start, packetSize);
				Accumulate(data, 0, start);
				double response = CalculateResponse;
				if (response > bestResponse)
				{
					bestResponse = response;
					bestPos = pos - (packetSize - start);
				}
			}
		else
			bestResponse = response;

		fprintf(file, "%f: %f\n", pos / 48000.0, bestResponse);

		pos += packetSize;

		// peak was in previous block
		if (previousResponse > Threshold && previousResponse > bestResponse)  // was 20000
		{
			fprintf(file, "!!!!!!!!!!!!!!!!!!!!!!!!\n");
			fprintf(file, "%d %f: %f\n", started, previousStart / 48000.0, previousResponse);
			bangs[bang_count++] = previousStart;
			new_bangs(which, bangs, bang_count);

			if (!started)
			{
				started = true;
				Threshold = 1600;
			}

			previousResponse = 0; // ensure it is not reported again
		}
		else
			previousResponse = bestResponse;

		memcpy(previousBlock, data, sizeof(float) * length);			
		previousStart = bestPos;
	}

	void DecodeThreadMethod()
	{
		char name[256];
		sprintf(name, "bang%d.csv", which);
		file = fopen(name, "w");

		AVFrame* audioFrame = av_frame_alloc();
		AVCodec * audioCodec = avcodec_find_decoder(AV_CODEC_ID_AC3);
		AVCodecContext *audioCodecCtx = avcodec_alloc_context3(audioCodec);
		avcodec_open2(audioCodecCtx, audioCodec, NULL);

		while (true)
		{
			AVPacket *packet = parser->getNextAudioPacket();
			if (packet == NULL)
				break;

			avcodec_send_packet(audioCodecCtx, packet);
			avcodec_receive_frame(audioCodecCtx, audioFrame);

			Analyse((float*)audioFrame->extended_data[0], audioFrame->nb_samples); // should do analysis on a separate thread ??? 
		}

		fclose(file);

		end_of_file = true;
		new_bangs(which, bangs, 0);

		avcodec_close(audioCodecCtx);
		avcodec_free_context(&audioCodecCtx);
		av_frame_free(&audioFrame);
	}
};
