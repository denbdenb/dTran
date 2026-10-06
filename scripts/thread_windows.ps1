Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Diagnostics;

public class ThreadWinFinder {
    public delegate bool EnumThreadDelegate(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumThreadWindows(int dwThreadId, EnumThreadDelegate lpfn, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    public static void FindForProcess(int pid) {
        Process p = Process.GetProcessById(pid);
        foreach (ProcessThread t in p.Threads) {
            EnumThreadWindows(t.Id, (hWnd, lParam) => {
                StringBuilder sb = new StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                StringBuilder cls = new StringBuilder(256);
                GetClassName(hWnd, cls, 256);
                Console.WriteLine("Thread " + t.Id + " -> HWND: 0x" + hWnd.ToInt64().ToString("X8") + " | Visible: " + IsWindowVisible(hWnd) + " | Class: " + cls.ToString() + " | Title: '" + sb.ToString() + "'");
                return true;
            }, IntPtr.Zero);
        }
    }
}
"@

$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    [ThreadWinFinder]::FindForProcess($proc.Id)
}
