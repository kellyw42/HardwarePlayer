#include <windows.h>
#include <assert.h>

class FrameQueue
{
public:
	static const unsigned int cnMaximumSize = 20;

	FrameQueue() : hEvent_(0), nReadPosition_(0), nFramesInQueue_(0), bEndOfDecode_(0)
	{
		hEvent_ = CreateEvent(NULL, false, false, NULL);
		InitializeCriticalSection(&oCriticalSection_);
		memset(aDisplayQueue_, 0, cnMaximumSize * sizeof(CUVIDPARSERDISPINFO));
		memset((void *)aIsFrameInUse_, 0, cnMaximumSize * sizeof(int));
	}

	virtual ~FrameQueue()
	{
		DeleteCriticalSection(&oCriticalSection_);
		CloseHandle(hEvent_);
	}

	void enqueue(const CUVIDPARSERDISPINFO *pPicParams)
	{
		// Mark the frame as 'in-use' so we don't re-use it for decoding until it is no longer needed
		// for display
		aIsFrameInUse_[pPicParams->picture_index] = true;

		// Wait until we have a free entry in the display queue (should never block if we have enough entries)
		do
		{
			bool bPlacedFrame = false;
			EnterCriticalSection(&oCriticalSection_);

			if (nFramesInQueue_ < (int)FrameQueue::cnMaximumSize)
			{
				int iWritePosition = (nReadPosition_ + nFramesInQueue_) % cnMaximumSize;
				aDisplayQueue_[iWritePosition] = *pPicParams;
				nFramesInQueue_++;
				bPlacedFrame = true;
			}

			LeaveCriticalSection(&oCriticalSection_);

			if (bPlacedFrame) // Done
				break;

			Sleep(1);   // Wait a bit
		} while (!bEndOfDecode_);

		SetEvent(hEvent_);
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
			nReadPosition_ = (iEntry + 1) % cnMaximumSize;
			nFramesInQueue_--;
			bHaveNewFrame = true;
		}

		LeaveCriticalSection(&oCriticalSection_);

		return bHaveNewFrame;
	}

	void releaseFrame(const CUVIDPARSERDISPINFO *pPicParams)
	{
		aIsFrameInUse_[pPicParams->picture_index] = false;
	}

	bool isInUse(int nPictureIndex) const
	{
		return (0 != aIsFrameInUse_[nPictureIndex]);
	}

	bool isEndOfDecode() const
	{
		return (0 != bEndOfDecode_);
	}

	void endDecode()
	{
		bEndOfDecode_ = true;
		SetEvent(hEvent_);  // Signal for the display thread
	}

	bool isEmpty() 
	{ 
		return nFramesInQueue_ == 0; 
	}

	bool waitUntilFrameAvailable(int nPictureIndex)
	{
		while (isInUse(nPictureIndex))
		{
			Sleep(1);   // Decoder is getting too far ahead from display

			if (isEndOfDecode())
				return false;
		}

		return true;
	}

private:
	HANDLE hEvent_;
	CRITICAL_SECTION    oCriticalSection_;
	volatile int        nReadPosition_;
	volatile int        nFramesInQueue_;
	CUVIDPARSERDISPINFO aDisplayQueue_[cnMaximumSize];
	volatile int        aIsFrameInUse_[cnMaximumSize];
	volatile int        bEndOfDecode_;
};