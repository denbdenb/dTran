Add-Type -ReferencedAssemblies "UIAutomationClient", "UIAutomationTypes" @"
using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Automation;
using System.Text;
using System.Diagnostics;

public class UiaWindowFinder {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenDesktop(string lpszDesktop, uint dwFlags, bool fInherit, uint dwDesiredAccess);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetThreadDesktop(IntPtr hDesktop);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);

    public delegate bool EnumThreadDelegate(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool EnumThreadWindows(int dwThreadId, EnumThreadDelegate lpfn, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    public static void InspectProcess(int pid) {
        Thread t = new Thread(() => {
            IntPtr hDesk = OpenDesktop("Default", 0, false, 0x01FF);
            if (hDesk == IntPtr.Zero) {
                Console.WriteLine("Failed to OpenDesktop: " + Marshal.GetLastWin32Error());
                return;
            }
            if (!SetThreadDesktop(hDesk)) {
                Console.WriteLine("Failed to SetThreadDesktop: " + Marshal.GetLastWin32Error());
                CloseDesktop(hDesk);
                return;
            }

            Console.WriteLine("Inside thread on Default desktop for PID " + pid);
            Process p = Process.GetProcessById(pid);
            foreach (ProcessThread pt in p.Threads) {
                EnumThreadWindows(pt.Id, (hWnd, lParam) => {
                    StringBuilder sb = new StringBuilder(256);
                    GetWindowText(hWnd, sb, 256);
                    string title = sb.ToString();
                    if (!string.IsNullOrEmpty(title)) {
                        Console.WriteLine("Found HWND 0x" + hWnd.ToInt64().ToString("X8") + " with Title: '" + title + "'");
                        try {
                            AutomationElement window = AutomationElement.FromHandle(hWnd);
                            if (window != null) {
                                Console.WriteLine("  UIA Window Name: " + window.Current.Name + ", Class: " + window.Current.ClassName);
                                var descendants = window.FindAll(TreeScope.Descendants, Condition.TrueCondition);
                                Console.WriteLine("  Total UIA elements: " + descendants.Count);
                                int count = 0;
                                foreach (AutomationElement el in descendants) {
                                    string id = el.Current.AutomationId;
                                    string name = el.Current.Name;
                                    if (!string.IsNullOrEmpty(id) || !string.IsNullOrEmpty(name)) {
                                        Console.WriteLine("    [" + el.Current.ControlType.ProgrammaticName + "] id='" + id + "' name='" + name + "'");
                                        count++;
                                        if (count > 25) {
                                            Console.WriteLine("    ... [more elements]");
                                            break;
                                        }
                                    }
                                }
                            }
                        } catch (Exception ex) {
                            Console.WriteLine("  UIA error: " + ex.Message);
                        }
                    }
                    return true;
                }, IntPtr.Zero);
            }
            CloseDesktop(hDesk);
        });
        t.Start();
        t.Join();
    }
}
"@

$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    [UiaWindowFinder]::InspectProcess($proc.Id)
} else {
    Write-Host "dTranslate not running"
}
