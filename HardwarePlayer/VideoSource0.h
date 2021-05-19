#pragma once



class VideoSource0
{
public:
	H264Parser* parser;

	VideoSource0(char* destFilename, int which, char** filenames, int num_files, size_t totalSize, progresshandler progress_handler)
	{
		high_resolution_clock::time_point t1 = high_resolution_clock::now();

		char **clone = new char*[num_files];
		for (int i = 0; i < num_files; i++)
			clone[i] = _strdup(filenames[i]);

		// get video file format
		CUVIDSOURCEPARAMS params;
		CUvideosource src;
		CUVIDEOFORMAT format;
		CHECK(cuvidCreateVideoSource(&src, clone[0], &params));
		CHECK(cuvidGetSourceVideoFormat(src, &format, 0));

		SDCardReader* reader = new SDCardReader(clone, num_files, progress_handler, totalSize);
		HANDLE readThread = reader->StartThread();

		parser = new H264Parser(reader, progress_handler, _strdup(destFilename), format, totalSize);
		HANDLE parseThread = parser->StartThread();

		AudioDecoder* decoder = new AudioDecoder(which, parser, progress_handler);
		HANDLE decodeThread = decoder->StartThread();

		WaitForMultipleObjects(3, new HANDLE[3]{ readThread, parseThread, decodeThread }, TRUE, INFINITE);
		//WaitForSingleObject(readThread, INFINITE);
		
		delete reader;
		delete parser;
		delete decoder;

		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
		Trace2("%s VideoSource0() %f seconds\n", destFilename, time_span.count());
	}
};