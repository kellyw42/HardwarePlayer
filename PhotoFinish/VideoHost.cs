using System.Runtime.InteropServices;
using System.Windows.Interop;
using System;

namespace PhotoFinish
{
    public class VideoHost : HwndHost
    {
        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            return new HandleRef(this, IntPtr.Zero);
        }
        
        protected override void DestroyWindowCore(HandleRef hwnd)
        {
        }

        protected override IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            return base.WndProc(hwnd, msg, wParam, lParam, ref handled);
        }
    }
}
