using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public delegate void timehandler(long start, long now);
public delegate void eventhandler(long event_type);

namespace PhotoFinish
{
    class NativeVideo
    {
        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Init();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr OpenVideo(String filename, eventhandler event_handler, timehandler time_handler);

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void GotoTime(IntPtr player, long pts);

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void FastForward();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void Rewind();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void PlayPause();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void NextFrame();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void PrevFrame();

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void VisualSearch(IntPtr startPlayer, IntPtr finishPlayer);

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void GetAudio(String filename, [MarshalAs(UnmanagedType.LPArray, SizeConst = 48000)] float[] data, int start);

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AudioSearch(String filename, eventhandler audio_handler);

        [DllImport("HardwarePlayer.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool UploadHeats([MarshalAs(UnmanagedType.LPWStr)]String destFile);
    }
}
