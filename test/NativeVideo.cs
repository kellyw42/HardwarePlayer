using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public delegate void timehandler(int start, int now);
public delegate void eventhandler(int event_type);

namespace ManagedClient
{
    class NativeVideo
    {
        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Init();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr CreateVideoWindow(eventhandler event_handler, timehandler time_handler, String filename, int monitor);

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void EventLoop();
    }
}
