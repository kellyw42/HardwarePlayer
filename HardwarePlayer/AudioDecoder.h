#pragma once

#include <math.h>
#include <vector>

struct BEEP
{
	long pos;
	double response;
};

struct FREQ
{
	bool started = false;
	double initialC;
	std::vector<BEEP> beeps;
	double previousResponse = 0;
	long previousStart = 0;
};

class AudioDecoder
{
private:
	H264Parser* parser;
	progresshandler progress_handler;
	int which;

public:
	AudioDecoder(int which, H264Parser* parser, progresshandler progress_handler)
	{
		this->which = which;
		this->parser = parser;
		this->progress_handler = progress_handler;
	}

private:
	static DWORD WINAPI DecodeThreadProc(LPVOID lpThreadParameter)
	{
		AudioDecoder* decoder = (AudioDecoder*)lpThreadParameter;
		decoder->DecodeThreadMethod();
		return 0;
	}

public:
	HANDLE StartThread()
	{
		return CreateThread(0, 0, DecodeThreadProc, this, 0, 0);
	}

private:
	const double sampleRate = 48000;
	const int packetSize = 1536;
	const int step = 8;

	long pos = 0;
	float previousBlock[1536];
	double C, S1, S2;
	FILE* file;

	FREQ starts, ticks;
	double syncC = (2.0 * cos(2.0 * M_PI * round(8000 / sampleRate * packetSize) / packetSize));

	void Accumulate(float* data, int start, int end)
	{
		for (int j = start; j < end; j++)
		{
			double old = S1;
			S1 = data[j] + C * S1 - S2;
			S2 = old;
		}
	}

#define CalculateResponse (S1 * S1 + S2 * S2 - S1 * S2 * C)

	void Analyse2(float* data, int length, FREQ *freq)
	{
		int Threshold = 1000;
		double initialC;

		if (freq->started)
			initialC = freq->initialC;
		else
			initialC = syncC;

		C = initialC;
		S1 = S2 = 0;
		Accumulate(data, 0, length);
		double response = CalculateResponse;

		double bestResponse = 0;
		long bestPos = 0;

		if (freq->previousResponse > 1000 || response > 1000)
			for (int start = 0; start < packetSize; start += step)
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

		// peak was in previous block
		if (freq->previousResponse > Threshold && freq->previousResponse > bestResponse)
		{
			BEEP beep;
			beep.pos = freq->previousStart;
			beep.response = freq->previousResponse;
			freq->beeps.push_back(beep);
			freq->previousResponse = 0; // ensure it is not reported again
			freq->started = true;
		}
		else
			freq->previousResponse = bestResponse;

		freq->previousStart = bestPos;
	}

	void Analyse(float* data, int length)
	{
		//for (int i = 0; i < length; i++)
		//	Trace2("which %d, data[%d] = %20.20e\n", which, i, data[i]);
			//assert(!isnan(data[i]));

		//fwrite(data, sizeof(float), length, file);
		//fflush(file);

		Analyse2(data, length, &starts);
		Analyse2(data, length, &ticks);

		pos += packetSize;

		memcpy(previousBlock, data, sizeof(float) * length);
	}

	void DecodeThreadMethod()
	{
		starts.initialC = (2.0 * cos(2.0 * M_PI * round(3000 / sampleRate * packetSize) / packetSize));
		ticks.initialC = (2.0 * cos(2.0 * M_PI * round(4000 / sampleRate * packetSize) / packetSize));

		AVFrame* audioFrame = av_frame_alloc();
		AVCodec * audioCodec = avcodec_find_decoder(AV_CODEC_ID_AC3);
		AVCodecContext *audioCodecCtx = avcodec_alloc_context3(audioCodec);
		avcodec_open2(audioCodecCtx, audioCodec, NULL);

		while (true)
		{
			AVPacket *packet = parser->getNextAudioPacket();
			if (packet == NULL)
				break;

			// wakk 2.31%
			avcodec_send_packet(audioCodecCtx, packet);
			avcodec_receive_frame(audioCodecCtx, audioFrame);

			Analyse((float*)audioFrame->extended_data[0], audioFrame->nb_samples); // should do analysis on a separate thread ??? 
		}

		char filename[128];
		sprintf(filename, "Audio%d.txt", which);
		file = fopen(filename, "w");

		fprintf(file, "starts:\n");
		for (BEEP beep : starts.beeps)
			fprintf(file, "%f,%f\n", beep.pos/48000.0, beep.response);

		fprintf(file, "ticks:\n");
			for (BEEP beep : ticks.beeps)
				fprintf(file, "%f,%f\n", beep.pos/48000.0, beep.response);
		fclose(file);

		int bang_count = ticks.beeps.size();
		long *bangs = new long[bang_count];

		for (int i = 0; i < bang_count; i++)
			//if (ticks.beeps[i].response > 10000)
				bangs[i] = ticks.beeps[i].pos;

		new_bangs(which, bangs, bang_count);

		avcodec_close(audioCodecCtx);
		avcodec_free_context(&audioCodecCtx);
		av_frame_free(&audioFrame);
	}
};
