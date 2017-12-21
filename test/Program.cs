using System;


namespace ManagedClient
{
    class Program
    {
        static void Main(string[] args)
        {
            NativeVideo.Init();
            NativeVideo.CreateVideoWindow(event_handler, time_handler, @"C:\PhotoFinish\Meets2\2017-12-15\Track1-Finish-17-52-13.MTS", 2);
            NativeVideo.EventLoop();
            while (true);
        }

        private static void time_handler(int start, int now)
        {
        }

        private static void event_handler(int event_type)
        {
        }
    }
}
