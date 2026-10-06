Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public class WinStaChecker {
    [DllImport("user32.dll")]
    public static extern IntPtr GetProcessWindowStation();

    [DllImport("user32.dll")]
    public static extern IntPtr GetThreadDesktop(int dwThreadId);

    [DllImport("kernel32.dll")]
    public static extern int GetCurrentThreadId();

    [DllImport("user32.dll")]
    public static extern bool GetUserObjectInformation(IntPtr hObj, int nIndex, StringBuilder pvInfo, int nLength, out int lpnLengthNeeded);

    public static void Check() {
        IntPtr hWinsta = GetProcessWindowStation();
        StringBuilder sb = new StringBuilder(256);
        int needed;
        GetUserObjectInformation(hWinsta, 2, sb, 256, out needed);
        Console.WriteLine("Window Station: " + sb.ToString());

        IntPtr hDesk = GetThreadDesktop(GetCurrentThreadId());
        StringBuilder sbDesk = new StringBuilder(256);
        GetUserObjectInformation(hDesk, 2, sbDesk, 256, out needed);
        Console.WriteLine("Desktop: " + sbDesk.ToString());
    }
}
"@
[WinStaChecker]::Check()
