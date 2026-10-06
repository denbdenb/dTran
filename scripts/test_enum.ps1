Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public class TestEnum {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    public static void Run() {
        int count = 0;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            uint tid = GetWindowThreadProcessId(hWnd, out pid);
            StringBuilder sb = new StringBuilder(256);
            GetWindowText(hWnd, sb, 256);
            if (count < 10) {
                Console.WriteLine("HWND: 0x" + hWnd.ToInt64().ToString("X8") + " | PID: " + pid + " | TID: " + tid + " | Title: '" + sb.ToString() + "'");
                count++;
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
[TestEnum]::Run()
