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
        public static extern void Test();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Init();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr CreateVideoPlayer(eventhandler event_handler, timehandler time_handler, int monitor);

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void OpenVideo(IntPtr player, String filename);
    }
}
