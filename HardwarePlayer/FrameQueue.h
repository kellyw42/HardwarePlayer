

class FrameQueue
{
public:
	static const unsigned int cnMaximumSize = 20;

	FrameQueue() : nReadPosition_(0), nFramesInQueue_(0)
	{
		InitializeCriticalSection(&oCriticalSection_);
		memset(aDisplayQueue_, 0, cnMaximumSize * sizeof(CUVIDPARSERDISPINFO));
		memset((void *)aIsFrameInUse_, 0, cnMaximumSize * sizeof(bool));
		paused = false;
	}

	~FrameQueue()
	{
		Pause();
		DeleteCriticalSection(&oCriticalSection_);
	}

	void enqueue(const CUVIDPARSERDISPINFO *pPicParams)
	{
		if (paused)
			return;

		//Trace2("\t\t\taIsFrameInUse_[%d] = true;", pPicParams->picture_index);
		aIsFrameInUse_[pPicParams->picture_index] = true;
		
		while (!paused)
		{
			bool bPlacedFrame = false;
			EnterCriticalSection(&oCriticalSection_);

			if (nFramesInQueue_ < (int)FrameQueue::cnMaximumSize)
			{
				int iWritePosition = (nReadPosition_ + nFramesInQueue_) % cnMaximumSize;
				//Trace2("write posn %d", iWritePosition);
				aDisplayQueue_[iWritePosition] = *pPicParams;
				nFramesInQueue_++;
				bPlacedFrame = true;
			}

			LeaveCriticalSection(&oCriticalSection_);

			if (bPlacedFrame) // Done
				break;

			Sleep(1);   // Wait a bit
		}
	}

	CUvideotimestamp Peek()
	{
		while (nFramesInQueue_ == 0)
			Sleep(1);

		return aDisplayQueue_[nReadPosition_].timestamp;
	}

	bool dequeue(CUVIDPARSERDISPINFO *pDisplayInfo)
	{
		pDisplayInfo->picture_index = -1;
		bool bHaveNewFrame = false;

		EnterCriticalSection(&oCriticalSection_);

		if (nFramesInQueue_ > 0)
		{
			int iEntry = nReadPosition_;
			*pDisplayInfo = aDisplayQueue_[iEntry];
			//printf("Dequeue PTS = %lld\n", pDisplayInfo->timestamp);

			nReadPosition_ = (iEntry + 1) % cnMaximumSize;
			nFramesInQueue_--;
			bHaveNewFrame = true;
		}

		LeaveCriticalSection(&oCriticalSection_);

		return bHaveNewFrame;
	}

	void releaseFrame(int nPictureIndex)
	{
		//Trace2("\t\t\treleaseFrame[%d]", nPictureIndex);
		aIsFrameInUse_[nPictureIndex] = false;
	}

	bool waitUntilFrameAvailable(int nPictureIndex)
	{
		while (aIsFrameInUse_[nPictureIndex] && !paused)
			Sleep(1);

		return !aIsFrameInUse_[nPictureIndex];
	}

	void Pause()
	{
		nFramesInQueue_ = 0;
		paused = true;
	}

	void Clear()
	{
		for (int i = 0; i < cnMaximumSize; i++)
			releaseFrame(i);

		nReadPosition_ = nFramesInQueue_ = 0;
		paused = false;
	}

private:
	bool paused;
	CRITICAL_SECTION    oCriticalSection_;
	volatile int        nReadPosition_;
	volatile int        nFramesInQueue_;
	CUVIDPARSERDISPINFO aDisplayQueue_[cnMaximumSize];
	volatile bool        aIsFrameInUse_[cnMaximumSize];
};