using namespace std::chrono;

void CHECK(CUresult result)
{
	if (result != CUDA_SUCCESS)
	{

		char msg[256];
		const char* e = msg;
		cuGetErrorName(result, &e);
		printf("Error %d %s\n", result, e);
		exit(1);
	}
}

int CUDAAPI HandleVideoData(void *userData, CUVIDSOURCEDATAPACKET *pPacket);
int CUDAAPI HandlePictureDecode(void *decoder, CUVIDPICPARAMS * pPicParams);
int CUDAAPI HandlePictureDisplay(void *a, CUVIDPARSERDISPINFO *p);

class VideoDecoder
{
public:

	FrameQueue* frameQueue;
	VideoSource *source;
	CUvideoparser parser;
	CUvideodecoder decoder;
	CUVIDEOFORMAT format;

	CUVIDEOFORMAT OpenVideo(char* filename)
	{
		frameQueue = new FrameQueue();

		CUVIDSOURCEPARAMS params;
		memset(&params, 0, sizeof(CUVIDSOURCEPARAMS));
		params.pUserData = this;
		params.pfnVideoDataHandler = HandleVideoData;

		source = new VideoSource(filename, params);

		CUvideosource src;
		CHECK(cuvidCreateVideoSource(&src, filename, &params));
		CHECK(cuvidGetSourceVideoFormat(src, &format, 0));

		return format;
	}

	CUVIDPARSERPARAMS parserParams;
	CUVIDDECODECREATEINFO   decoderParams;

	void Init()
	{
		CUVIDEOFORMATEX formatEx;
		memset(&formatEx, 0, sizeof(CUVIDEOFORMATEX));
		formatEx.format = format;

		memset(&parserParams, 0, sizeof(CUVIDPARSERPARAMS));
		parserParams.pUserData = this;
		parserParams.CodecType = format.codec;
		parserParams.pExtVideoInfo = &formatEx;
		parserParams.pfnDecodePicture = HandlePictureDecode;
		parserParams.pfnDisplayPicture = HandlePictureDisplay;
		parserParams.pfnSequenceCallback = 0;
		parserParams.ulClockRate = 0;
		parserParams.ulErrorThreshold = 0;
		parserParams.ulMaxDisplayDelay = 2;  // this flag is needed so the parser will push frames out to the decoder as quickly as it can
		parserParams.ulMaxNumDecodeSurfaces = 20;

		memset(&decoderParams, 0, sizeof(CUVIDDECODECREATEINFO));
		decoderParams.bitDepthMinus8 = format.bit_depth_luma_minus8;
		decoderParams.ChromaFormat = format.chroma_format;
		decoderParams.CodecType = format.codec;
		decoderParams.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive; // ???
		decoderParams.OutputFormat = cudaVideoSurfaceFormat_NV12;
		decoderParams.ulCreationFlags = cudaVideoCreate_PreferCUVID;
		decoderParams.ulIntraDecodeOnly = 0;
		decoderParams.ulNumDecodeSurfaces = 20;
		decoderParams.ulNumOutputSurfaces = 2;
		decoderParams.vidLock = 0;
		decoderParams.ulWidth = format.coded_width;
		decoderParams.ulHeight = format.coded_height;
		decoderParams.ulTargetHeight = format.display_area.bottom - format.display_area.top;
		decoderParams.ulTargetWidth = format.display_area.right - format.display_area.left;
		decoderParams.display_area.left = format.display_area.left;
		decoderParams.display_area.right = format.display_area.right;
		decoderParams.display_area.top = format.display_area.top;
		decoderParams.display_area.bottom = format.display_area.bottom;
		decoderParams.target_rect.left = format.display_area.left;
		decoderParams.target_rect.right = format.display_area.right;
		decoderParams.target_rect.top = format.display_area.top;
		decoderParams.target_rect.bottom = format.display_area.bottom;

		Create();
	}

	void Create()
	{
		CHECK(cuvidCreateVideoParser(&parser, &parserParams));
		CHECK(cuvidCreateDecoder(&decoder, &decoderParams));
	}

	void Destroy()
	{
		cuvidDestroyVideoParser(parser);
		cuvidDestroyDecoder(decoder);
	}

	void Start()
	{
		Trace("VideoDecoder::Start();");
		Create(); // needed?
		source->Start();
	}

	void Stop()
	{
		Trace("VideoDecoder::Stop();");
		frameQueue->Stop();
		source->Stop();
		Destroy(); // needed?
		frameQueue->Start();
	}

	void Goto(CUvideotimestamp pts)
	{
		Trace("VideoDecoder::Goto(%ld);", pts);
		Stop();
		source->Goto(pts);
		Start();
		while (1)
		{
			CUvideotimestamp actual = frameQueue->Peek();
			if (actual > 0)
			{
				//printf("first decoded = %lld\n", actual);
				break;
			}
		}
	}

	CUVIDPARSERDISPINFO FetchFrame()
	{
		Trace("VideoDecoder::FetchFrame();");
		CUVIDPARSERDISPINFO info;
		while (1)
		{
			if (frameQueue->dequeue(&info))
				return info;
		}
	}

	void releaseFrame(CUVIDPARSERDISPINFO *info)
	{
		frameQueue->releaseFrame(info->picture_index);
	}
};

int CUDAAPI HandleVideoData(void *userData, CUVIDSOURCEDATAPACKET *pPacket)
{
	//Trace("HandleVideoData();");
	VideoDecoder *container = (VideoDecoder*)userData;
	CHECK(cuvidParseVideoData(container->parser, pPacket));
	return true;
}

int CUDAAPI HandlePictureDecode(void *userData, CUVIDPICPARAMS * pPicParams)
{
	//Trace("HandlePictureDecode(%d);", pPicParams->CurrPicIdx);
	VideoDecoder *container = (VideoDecoder*)userData;
	container->frameQueue->waitUntilFrameAvailable(pPicParams->CurrPicIdx);
	CHECK(cuvidDecodePicture(container->decoder, pPicParams));
	return true;
}

int CUDAAPI HandlePictureDisplay(void *userData, CUVIDPARSERDISPINFO *p)
{
	//Trace("HandlePictureDisplay(%ld);", p->timestamp);
	VideoDecoder *container = (VideoDecoder*)userData;
	//printf("enqueue PTS = %lld\n", p->timestamp);
	container->frameQueue->enqueue(p);
	return true;
}