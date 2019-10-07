#pragma once



class VideoSource0
{
public:
	VideoSource0(char* destFilename, char* directory, int which, char** filenames, int num_files, progresshandler progress_handler)
	{
		char **clone = new char*[num_files];
		for (int i = 0; i < num_files; i++)
			clone[i] = _strdup(filenames[i]);

		SDCardReader* reader = new SDCardReader(clone, num_files, progress_handler);
		reader->StartThread();

		H264Parser* parser = new H264Parser(reader, progress_handler);
		parser->StartThread();

		AudioDecoder* decoder = new AudioDecoder(_strdup(directory), which, parser, progress_handler);
		decoder->StartThread();

		VideoFileWriter* writer = new VideoFileWriter(_strdup(destFilename), parser, progress_handler);
		writer->StartThread();
	}
};