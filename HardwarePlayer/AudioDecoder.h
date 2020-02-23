#pragma once

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
	long bangs[10000];
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
				bangs[bang_count++] = start;
				new_bangs(which, bangs, bang_count);
				time_since_long_silence = 10000000;
			}

			pos++;
		}
	}

	void DecodeThreadMethod()
	{
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

		end_of_file = true;
		new_bangs(which, bangs, 0);

		avcodec_close(audioCodecCtx);
		avcodec_free_context(&audioCodecCtx);
		av_frame_free(&audioFrame);
	}
};
