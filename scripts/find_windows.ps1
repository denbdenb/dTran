Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public class WinFinder {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    public static void FindAll() {
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            StringBuilder sb = new StringBuilder(256);
            GetWindowText(hWnd, sb, 256);
            StringBuilder cls = new StringBuilder(256);
            GetClassName(hWnd, cls, 256);
            string title = sb.ToString();
            string className = cls.ToString();
            if (title.IndexOf("Translate", StringComparison.OrdinalIgnoreCase) >= 0 ||
                className.IndexOf("WinUI", StringComparison.OrdinalIgnoreCase) >= 0 ||
                className.IndexOf("Xaml", StringComparison.OrdinalIgnoreCase) >= 0) {
                Console.WriteLine("HWND: 0x" + hWnd.ToInt64().ToString("X8") + " | PID: " + pid + " | Visible: " + IsWindowVisible(hWnd) + " | Class: " + className + " | Title: '" + title + "'");
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@

[WinFinder]::FindAll()
