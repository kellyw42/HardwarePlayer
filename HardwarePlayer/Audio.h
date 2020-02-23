#include <math.h>

extern "C" __declspec(dllexport) void GetAudio(char* filename, float data[48000], int start)
{
	FILE *audioFile = fopen(filename, "rb");
	if (audioFile == NULL) MessageBoxA(NULL, filename, "Error: cannot open file", MB_OK);
	fseek(audioFile, start * sizeof(float), SEEK_SET);
	fread(data, sizeof(float), 48000, audioFile);
	fclose(audioFile);
}

struct AudioData
{
	char* filename;
	eventhandler audio_handler;
};


const int quiet = 1100;
const int quiet_period = 60;
const int loud = 8000;
const int loud_period = 200;

const int BUFFERSIZE = 1000000;

DWORD WINAPI AudioSearchThread(LPVOID lpThreadParameter)
{
	AudioData *data = (AudioData*)lpThreadParameter;

	FILE *audioFile = fopen(data->filename, "rb");
	if (audioFile == NULL) MessageBoxA(NULL, data->filename, "Error: cannot open file", MB_OK);

	float *audioData = new float[BUFFERSIZE];

	int silent_period = 0;
	int time_since_long_silence = 10000000;
	int start = -1;
	int pos = 0;

	while (1)
	{
		size_t N = fread(audioData, sizeof(float), BUFFERSIZE, audioFile);

		if (N == 0)
			break;

		for (int i = 0; i < N; i++, pos++)
		{
			float volume = abs(audioData[i] * 16384);
			if (volume < quiet) // still quiet
				silent_period++;
			else
			{
				if (silent_period > quiet_period) // end of long quiet period
				{
					time_since_long_silence = 0;
					start = pos;
				}
				silent_period = 0; // the peace is shattered
			}
			time_since_long_silence++;

			if (time_since_long_silence < loud_period && volume > loud)
			{
				data->audio_handler(start, 0);
				silent_period = 0;
				time_since_long_silence = 10000000;
			}
		}
	}
	data->audio_handler(-1, 0);
	fclose(audioFile);
	return 0;
}

extern "C" __declspec(dllexport) void AudioSearch(char* filename, eventhandler audio_handler)
{
	AudioData *data = new AudioData();
	data->filename = _strdup(filename);
	data->audio_handler = audio_handler;
	CreateThread(0, 0, AudioSearchThread, data, 0, 0);

}