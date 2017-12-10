#include <stdio.h>

FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) { return _iob; }

#include "VideoWindow.h"

#define name "C:\\PhotoFinish\\Meets2\\2017-09-12\\Track1-Finish-17-57-34.MTS"
//#define name "C:\\PhotoFinish\\Meets2\\2017-12-08\\Track1-Finish-08-26-44.MTS"

int main(int argc, char* argv[])
{
	CUresult OK0 = cuInit(0);

	glutInit(&argc, argv);

	//VideoWindow *startWindow = new VideoWindow("C:\\PhotoFinish\\Meets2\\2017-12-08\\Track1-Start-17-44-39.MTS", 1);

	VideoWindow *finishWindow = new VideoWindow("C:\\PhotoFinish\\Meets2\\2017-12-08\\Track1-Finish-17-55-51.MTS", 2);

	glutMainLoop();
}