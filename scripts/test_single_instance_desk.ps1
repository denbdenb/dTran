Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Collections.Generic;
using System.Diagnostics;

public class SingleInstanceVerifier {
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

    public const uint WM_COMMAND = 0x0111;
    public const int IDM_TRAY_SETTINGS = 3004;
    public const int IDM_TRAY_CLIPBOARD = 3002;
    public static IntPtr HWND_MESSAGE = new IntPtr(-3);

    public static List<string> GetVisibleWinUIWindows(uint targetPid) {
        List<string> list = new List<string>();
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == targetPid && IsWindowVisible(hWnd)) {
                StringBuilder sb = new StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                StringBuilder cls = new StringBuilder(256);
                GetClassName(hWnd, cls, 256);
                if (cls.ToString() == "WinUIDesktopWin32WindowClass") {
                    list.Add("HWND:0x" + hWnd.ToInt64().ToString("X8") + " | Title:'" + sb.ToString() + "'");
                }
            }
            return true;
        }, IntPtr.Zero);
        return list;
    }

    public static void Run(uint pid) {
        Thread t = new Thread(() => {
            IntPtr hDesk = OpenDesktop("Default", 0, false, 0x01FF);
            if (!SetThreadDesktop(hDesk)) {
                Console.WriteLine("SetThreadDesktop failed: " + Marshal.GetLastWin32Error());
                return;
            }

            IntPtr hMsgWnd = FindWindowEx(HWND_MESSAGE, IntPtr.Zero, "dTranslate_MessageWindow", null);
            Console.WriteLine("Message window handle: 0x" + hMsgWnd.ToInt64().ToString("X8"));

            var wins0 = GetVisibleWinUIWindows(pid);
            Console.WriteLine("Initial Windows count: " + wins0.Count);
            foreach (var w in wins0) Console.WriteLine("  " + w);

            // Trigger Settings #1
            Console.WriteLine("\n[Action] Triggering Settings #1...");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_SETTINGS), IntPtr.Zero);
            Thread.Sleep(1000);
            var wins1 = GetVisibleWinUIWindows(pid);
            Console.WriteLine("Windows after Settings #1: " + wins1.Count);
            foreach (var w in wins1) Console.WriteLine("  " + w);

            // Trigger Settings #2
            Console.WriteLine("\n[Action] Triggering Settings #2 (Duplicate check)...");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_SETTINGS), IntPtr.Zero);
            Thread.Sleep(1000);
            var wins2 = GetVisibleWinUIWindows(pid);
            Console.WriteLine("Windows after Settings #2: " + wins2.Count);
            foreach (var w in wins2) Console.WriteLine("  " + w);

            bool settingsSinglePass = (wins1.Count == 2 && wins2.Count == 2);
            Console.WriteLine("Settings Single-Instance Result: " + (settingsSinglePass ? "PASS (Exact 1 Settings window)" : "FAIL"));

            // Trigger Quick Popup #1
            Console.WriteLine("\n[Action] Triggering Quick Popup #1...");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_CLIPBOARD), IntPtr.Zero);
            Thread.Sleep(1000);
            var wins3 = GetVisibleWinUIWindows(pid);
            Console.WriteLine("Windows after Popup #1: " + wins3.Count);
            foreach (var w in wins3) Console.WriteLine("  " + w);

            // Trigger Quick Popup #2
            Console.WriteLine("\n[Action] Triggering Quick Popup #2 (Duplicate check)...");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_CLIPBOARD), IntPtr.Zero);
            Thread.Sleep(1000);
            var wins4 = GetVisibleWinUIWindows(pid);
            Console.WriteLine("Windows after Popup #2: " + wins4.Count);
            foreach (var w in wins4) Console.WriteLine("  " + w);

            bool popupSinglePass = (wins3.Count == 3 && wins4.Count == 3);
            Console.WriteLine("Popup Single-Instance Result: " + (popupSinglePass ? "PASS (Exact 1 Popup window)" : "FAIL"));

            CloseDesktop(hDesk);
        });
        t.Start();
        t.Join();
    }
}
"@

$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    [SingleInstanceVerifier]::Run([uint32]$proc.Id)
} else {
    Write-Host "dTranslate not running"
}
