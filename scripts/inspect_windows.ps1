Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public class WinHelper {
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

    public static void ListProcessWindows(uint targetPid) {
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == targetPid) {
                StringBuilder sb = new StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                StringBuilder cls = new StringBuilder(256);
                GetClassName(hWnd, cls, 256);
                Console.WriteLine(string.Format("HWND: 0x{0:X8} | Visible: {1} | Class: {2} | Title: '{3}'", 
                    hWnd.ToInt64(), IsWindowVisible(hWnd), cls.ToString(), sb.ToString()));
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@

$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    Write-Host "Process dTranslate found: PID = $($proc.Id), WorkingSet = $([math]::Round($proc.WorkingSet64 / 1MB, 2)) MB, PrivateMemory = $([math]::Round($proc.PrivateMemorySize64 / 1MB, 2)) MB"
    [WinHelper]::ListProcessWindows([uint32]$proc.Id)
} else {
    Write-Host "dTranslate is not running"
}
