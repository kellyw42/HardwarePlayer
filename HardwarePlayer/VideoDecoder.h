using namespace std::chrono;

int CUDAAPI HandleVideoData(void *userData, CUVIDSOURCEDATAPACKET *pPacket);
int CUDAAPI HandlePictureDecode(void *decoder, CUVIDPICPARAMS * pPicParams);
int CUDAAPI HandlePictureDisplay(void *a, CUVIDPARSERDISPINFO *p);

class VideoDecoder
{
public:
	FrameQueue* frameQueue;
	VideoSource1 *source;
	CUvideoparser parser;
	CUvideodecoder decoder;
	CUVIDPARSERPARAMS parserParams;
	CUVIDDECODECREATEINFO   decoderParams;

	void OpenVideo(VideoSource1 *source)
	{
		this->source = source;	
		frameQueue = new FrameQueue();

		source->Attach(this, HandleVideoData);
/*
		CUVIDSOURCEPARAMS params;
		memset(&params, 0, sizeof(CUVIDSOURCEPARAMS));
		params.pUserData = this;
		params.pfnVideoDataHandler = HandleVideoData;

		CUvideosource src;
		CHECK(cuvidCreateVideoSource(&src, filename, &params));
		CHECK(cuvidGetSourceVideoFormat(src, &format, 0));
*/
	}

	void Init()
	{
		CUVIDEOFORMATEX formatEx;
		memset(&formatEx, 0, sizeof(CUVIDEOFORMATEX));
		formatEx.format = source->format;

		memset(&parserParams, 0, sizeof(CUVIDPARSERPARAMS));
		parserParams.pUserData = this;
		parserParams.CodecType = source->format.codec;
		parserParams.pExtVideoInfo = &formatEx;
		parserParams.pfnDecodePicture = HandlePictureDecode;
		parserParams.pfnDisplayPicture = HandlePictureDisplay;
		parserParams.pfnSequenceCallback = 0;
		parserParams.ulClockRate = 0;
		parserParams.ulErrorThreshold = 0;
		parserParams.ulMaxDisplayDelay = 2;  // this flag is needed so the parser will push frames out to the decoder as quickly as it can
		parserParams.ulMaxNumDecodeSurfaces = 20;

		memset(&decoderParams, 0, sizeof(CUVIDDECODECREATEINFO));
		decoderParams.bitDepthMinus8 = source->format.bit_depth_luma_minus8;
		decoderParams.ChromaFormat = source->format.chroma_format;
		decoderParams.CodecType = source->format.codec;
		decoderParams.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave; // cudaVideoDeinterlaceMode_Adaptive; // ???
		decoderParams.OutputFormat = cudaVideoSurfaceFormat_NV12;
		decoderParams.ulCreationFlags = cudaVideoCreate_PreferCUVID;
		decoderParams.ulIntraDecodeOnly = 0;
		decoderParams.ulNumDecodeSurfaces = 20;
		decoderParams.ulNumOutputSurfaces = 2;
		decoderParams.vidLock = 0;
		decoderParams.ulWidth = source->format.coded_width;
		decoderParams.ulHeight = source->format.coded_height;
		
		decoderParams.display_area.left = source->format.display_area.left;
		decoderParams.display_area.right = source->format.display_area.right;
		decoderParams.display_area.top = source->format.display_area.top;
		decoderParams.display_area.bottom = source->format.display_area.bottom;

		decoderParams.target_rect.left = source->format.display_area.left;
		decoderParams.target_rect.right = source->format.display_area.right;
		decoderParams.target_rect.top = source->format.display_area.top;
		decoderParams.target_rect.bottom = source->format.display_area.bottom;

		decoderParams.ulTargetHeight = (decoderParams.target_rect.bottom - decoderParams.target_rect.top);
		decoderParams.ulTargetWidth = (decoderParams.target_rect.right - decoderParams.target_rect.left);

		//Create();
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
		Create(); // needed?
		source->Start();
	}

	void Stop()
	{
		frameQueue->Stop();
		source->Stop();
		Destroy(); // needed?
		frameQueue->Start();
	}

	CUvideotimestamp Goto(CUvideotimestamp targetPts)
	{
		Stop();
		source->Goto(targetPts);
		Start();
		while (1)
		{
			CUvideotimestamp firstPts = frameQueue->Peek();
			if (firstPts > 0)
			{
				Trace("first decoded = %lld\n", firstPts);
				assert(firstPts <= targetPts);
				return firstPts;
			}
		}
	}

	CUVIDPARSERDISPINFO FetchFrame()
	{
		CUVIDPARSERDISPINFO info;
		while (1)
		{
			if (frameQueue->dequeue(&info))
			{
				return info;
			}
		}
	}

	void releaseFrame(CUVIDPARSERDISPINFO *info)
	{
		frameQueue->releaseFrame(info->picture_index);
	}
};

int CUDAAPI HandleVideoData(void *userData, CUVIDSOURCEDATAPACKET *pPacket)
{
	Trace2("HandleVideoData(%lld)", pPacket->timestamp);
	VideoDecoder *container = (VideoDecoder*)userData;
	CHECK(cuvidParseVideoData(container->parser, pPacket));
	Trace2("return from cuvidParseVideoData (%lld)", pPacket->timestamp);
	return true;
}

int CUDAAPI HandlePictureDecode(void *userData, CUVIDPICPARAMS * pPicParams)
{
	VideoDecoder *container = (VideoDecoder*)userData;
	container->frameQueue->waitUntilFrameAvailable(pPicParams->CurrPicIdx);
	Trace2("\tHandlePictureDecode(%d)", pPicParams->CurrPicIdx);
	CHECK(cuvidDecodePicture(container->decoder, pPicParams));
	return true;
}

int CUDAAPI HandlePictureDisplay(void *userData, CUVIDPARSERDISPINFO *p)
{
	Trace2("\t\tHandlePictureDisplay(%lld)", p->timestamp);
	VideoDecoder *container = (VideoDecoder*)userData;
	container->frameQueue->enqueue(p);
	return true;
}