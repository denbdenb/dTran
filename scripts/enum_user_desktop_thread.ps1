Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

public class DesktopSwitcherThread {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenDesktop(string lpszDesktop, uint dwFlags, bool fInherit, uint dwDesiredAccess);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetThreadDesktop(IntPtr hDesktop);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);

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

    public static void ListWindows() {
        Thread t = new Thread(() => {
            IntPtr hDesk = OpenDesktop("Default", 0, false, 0x01FF);
            if (hDesk == IntPtr.Zero) {
                Console.WriteLine("Failed to open Default desktop. Error: " + Marshal.GetLastWin32Error());
                return;
            }

            bool ok = SetThreadDesktop(hDesk);
            if (!ok) {
                Console.WriteLine("Failed to SetThreadDesktop. Error: " + Marshal.GetLastWin32Error());
                CloseDesktop(hDesk);
                return;
            }

            Console.WriteLine("Switched new thread to Default desktop! Enumerating...");
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
                    pid == 28236) {
                    Console.WriteLine("PID: " + pid + " | HWND: 0x" + hWnd.ToInt64().ToString("X8") + " | Visible: " + IsWindowVisible(hWnd) + " | Class: " + className + " | Title: '" + title + "'");
                }
                return true;
            }, IntPtr.Zero);

            CloseDesktop(hDesk);
        });

        t.Start();
        t.Join();
    }
}
"@

[DesktopSwitcherThread]::ListWindows()
