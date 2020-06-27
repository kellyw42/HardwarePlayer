#include <stdio.h>

extern "C" __declspec(dllimport) void  GetFrame(char* filename, long long target);

int main(int argc, char* argv[])
{
	printf("start main ...\n");
	
	GetFrame((char*)"C:\\PhotoFinish\\Meets2\\2019-11-29\\Track1-Finish-17-57-42.video", 100000);

	return 0;
}