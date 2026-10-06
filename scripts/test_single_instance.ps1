Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Collections.Generic;

public class SingleInstanceTester {
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
    public const uint WM_HOTKEY = 0x0312;
    public const int IDM_TRAY_SETTINGS = 3004;
    public const int IDM_TRAY_CLIPBOARD = 3002;
    public const int HOTKEY_ID_SELECTION = 2001;

    public static IntPtr HWND_MESSAGE = new IntPtr(-3);

    public static List<string> GetWindowsForPid(uint targetPid) {
        List<string> result = new List<string>();
        IntPtr hDesk = OpenDesktop("Default", 0, false, 0x01FF);
        SetThreadDesktop(hDesk);
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == targetPid && IsWindowVisible(hWnd)) {
                StringBuilder sb = new StringBuilder(256);
                GetWindowText(hWnd, sb, 256);
                StringBuilder cls = new StringBuilder(256);
                GetClassName(hWnd, cls, 256);
                string title = sb.ToString();
                string className = cls.ToString();
                if (className == "WinUIDesktopWin32WindowClass") {
                    result.Add("HWND:0x" + hWnd.ToInt64().ToString("X8") + " | Title:'" + title + "'");
                }
            }
            return true;
        }, IntPtr.Zero);
        CloseDesktop(hDesk);
        return result;
    }

    public static void RunTest(uint pid) {
        Thread t = new Thread(() => {
            IntPtr hMsgWnd = FindWindowEx(HWND_MESSAGE, IntPtr.Zero, "dTranslate_MessageWindow", null);
            Console.WriteLine("Message window handle: 0x" + hMsgWnd.ToInt64().ToString("X8"));
            if (hMsgWnd == IntPtr.Zero) {
                Console.WriteLine("ERROR: Could not find dTranslate_MessageWindow!");
                return;
            }

            Console.WriteLine("\n--- Initial Windows ---");
            var wins0 = GetWindowsForPid(pid);
            foreach (var w in wins0) Console.WriteLine("  " + w);

            Console.WriteLine("\n--- Step 1: Open Settings First Time ---");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_SETTINGS), IntPtr.Zero);
            Thread.Sleep(800);
            var wins1 = GetWindowsForPid(pid);
            foreach (var w in wins1) Console.WriteLine("  " + w);
            int settingsCount1 = wins1.FindAll(x => x.Contains("Settings")).Count;
            Console.WriteLine("Settings windows detected: " + settingsCount1);

            Console.WriteLine("\n--- Step 2: Trigger Settings Again (Should NOT create second window) ---");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_SETTINGS), IntPtr.Zero);
            Thread.Sleep(800);
            var wins2 = GetWindowsForPid(pid);
            foreach (var w in wins2) Console.WriteLine("  " + w);
            int settingsCount2 = wins2.FindAll(x => x.Contains("Settings")).Count;
            Console.WriteLine("Settings windows detected after 2nd trigger: " + settingsCount2);

            if (settingsCount1 == 1 && settingsCount2 == 1 && wins1.Count == wins2.Count) {
                Console.WriteLine("PASS: Settings Single-Instance Verification Successful!");
            } else {
                Console.WriteLine("FAIL: Settings Single-Instance Failed!");
            }

            Console.WriteLine("\n--- Step 3: Trigger Quick Popup First Time ---");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_CLIPBOARD), IntPtr.Zero);
            Thread.Sleep(800);
            var wins3 = GetWindowsForPid(pid);
            foreach (var w in wins3) Console.WriteLine("  " + w);
            int popupCount1 = wins3.Count - wins2.Count;
            Console.WriteLine("Popup window opened: count diff = " + popupCount1);

            Console.WriteLine("\n--- Step 4: Trigger Quick Popup Again (Should NOT duplicate) ---");
            PostMessage(hMsgWnd, WM_COMMAND, new IntPtr(IDM_TRAY_CLIPBOARD), IntPtr.Zero);
            Thread.Sleep(800);
            var wins4 = GetWindowsForPid(pid);
            foreach (var w in wins4) Console.WriteLine("  " + w);
            int popupCount2 = wins4.Count - wins2.Count;
            Console.WriteLine("Popup count diff after 2nd trigger: " + popupCount2);

            if (popupCount1 == 1 && popupCount2 == 1 && wins3.Count == wins4.Count) {
                Console.WriteLine("PASS: Quick Popup Single-Instance Verification Successful!");
            } else {
                Console.WriteLine("FAIL: Quick Popup Single-Instance Failed!");
            }
        });
        t.Start();
        t.Join();
    }
}
"@

$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    [SingleInstanceTester]::RunTest([uint32]$proc.Id)
} else {
    Write-Host "dTranslate not running"
}
