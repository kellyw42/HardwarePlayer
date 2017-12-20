using System;
using System.Runtime.InteropServices;

public delegate void timehandler(int start, int now);
public delegate void eventhandler(int event_type);

class Program
{
    [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void Init();

    [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr CreateVideoWindow(eventhandler event_handler, timehandler time_handler, String filename, int monitor);

    [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void EventLoop();

    static void Main(string[] args)
    {
        Console.WriteLine("Start");
        Init();
        CreateVideoWindow(event_handler, time_handler, @"C:\PhotoFinish\Meets2\2017-12-15\Track1-Finish-17-52-13.MTS", 2);
        EventLoop();
        Console.WriteLine("Done");
    }

    private static void time_handler(int start, int now)
    {
    }

    private static void event_handler(int event_type)
    {
    }
}
