Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Collections.Generic;

public class LifecycleTester {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenDesktop(string lpszDesktop, uint dwFlags, bool fInherit, uint dwDesiredAccess);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetThreadDesktop(IntPtr hDesktop);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);

    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc enumProc, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindowEx(IntPtr hWndParent, IntPtr hWndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    public const uint WM_COMMAND = 0x0111;
    public const uint WM_CLOSE = 0x0010;
    public const int IDM_TRAY_SETTINGS = 3004;
    public const int IDM_TRAY_CLIPBOARD = 3002;
    public static IntPtr HWND_MESSAGE = new IntPtr(-3);

    public static IntPtr FindWindowByTitle(uint targetPid, string targetTitle) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == targetPid && IsWindowVisible(hWnd)) {
                StringBuilder sb = new StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                if (sb.ToString().Contains(targetTitle)) {
                    found = hWnd;
                    return false;
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static void Run(uint pid) {
        Thread t = new Thread(() => {
            IntPtr hDesk = OpenDesktop("Default", 0, false, 0x01FF);
            SetThreadDesktop(hDesk);
            IntPtr hMsgWnd = FindWindowEx(HWND_MESSAGE, IntPtr.Zero, "dTranslate_MessageWindow", null);

            // Close Settings window
            IntPtr hSettings = FindWindowByTitle(pid, "Settings");
            if (hSettings != IntPtr.Zero) {
                Console.WriteLine("Closing Settings window HWND: 0x" + hSettings.ToInt64().ToString("X8"));
                SendMessage(hSettings, WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
                Thread.Sleep(800);
            }

            // Verify it is gone
            IntPtr hSettingsAfter = FindWindowByTitle(pid, "Settings");
            Console.WriteLine("Settings window after WM_CLOSE: " + (hSettingsAfter == IntPtr.Zero ? "Destroyed (OK)" : "Still open"));

            // Re-open Settings
            Console.WriteLine("Re-triggering Settings...");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_SETTINGS), IntPtr.Zero);
            Thread.Sleep(1000);
            IntPtr hSettingsReopened = FindWindowByTitle(pid, "Settings");
            Console.WriteLine("Settings reopened: " + (hSettingsReopened != IntPtr.Zero ? "PASS (HWND 0x" + hSettingsReopened.ToInt64().ToString("X8") + ")" : "FAIL"));

            // Close Quick Popup window
            IntPtr hPopup = FindWindowByTitle(pid, "Quick Translation");
            if (hPopup != IntPtr.Zero) {
                Console.WriteLine("\nClosing Quick Popup window HWND: 0x" + hPopup.ToInt64().ToString("X8"));
                SendMessage(hPopup, WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
                Thread.Sleep(800);
            }

            IntPtr hPopupAfter = FindWindowByTitle(pid, "Quick Translation");
            Console.WriteLine("Popup window after WM_CLOSE: " + (hPopupAfter == IntPtr.Zero ? "Destroyed (OK)" : "Still open"));

            // Re-open Quick Popup
            Console.WriteLine("Re-triggering Quick Popup...");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_CLIPBOARD), IntPtr.Zero);
            Thread.Sleep(1000);
            IntPtr hPopupReopened = FindWindowByTitle(pid, "Quick Translation");
            Console.WriteLine("Popup reopened: " + (hPopupReopened != IntPtr.Zero ? "PASS (HWND 0x" + hPopupReopened.ToInt64().ToString("X8") + ")" : "FAIL"));

            CloseDesktop(hDesk);
        });
        t.Start();
        t.Join();
    }
}
"@

$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    [LifecycleTester]::Run([uint32]$proc.Id)
}
