Add-Type -ReferencedAssemblies "UIAutomationClient", "UIAutomationTypes" @"
using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Automation;
using System.Text;

public class UiaInspector {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr OpenDesktop(string lpszDesktop, uint dwFlags, bool fInherit, uint dwDesiredAccess);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetThreadDesktop(IntPtr hDesktop);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool CloseDesktop(IntPtr hDesktop);

    public static void InspectWindow(IntPtr hWnd) {
        Thread t = new Thread(() => {
            IntPtr hDesk = OpenDesktop("Default", 0, false, 0x01FF);
            SetThreadDesktop(hDesk);

            try {
                AutomationElement window = AutomationElement.FromHandle(hWnd);
                if (window == null) {
                    Console.WriteLine("Could not get AutomationElement for HWND 0x" + hWnd.ToInt64().ToString("X8"));
                    return;
                }
                Console.WriteLine("Window Name: " + window.Current.Name);
                Console.WriteLine("Class Name: " + window.Current.ClassName);
                Console.WriteLine("AutomationId: " + window.Current.AutomationId);

                var allElements = window.FindAll(TreeScope.Descendants, Condition.TrueCondition);
                Console.WriteLine("Total descendants: " + allElements.Count);

                int printed = 0;
                foreach (AutomationElement el in allElements) {
                    string id = el.Current.AutomationId;
                    string name = el.Current.Name;
                    string controlType = el.Current.ControlType.ProgrammaticName;
                    if (!string.IsNullOrEmpty(id) || !string.IsNullOrEmpty(name)) {
                        Console.WriteLine("  [" + controlType + "] Name='" + name + "' Id='" + id + "'");
                        printed++;
                        if (printed > 60) {
                            Console.WriteLine("  ... [truncated for brevity]");
                            break;
                        }
                    }
                }
            } catch (Exception ex) {
                Console.WriteLine("Exception in UIA: " + ex.Message);
            } finally {
                CloseDesktop(hDesk);
            }
        });
        t.Start();
        t.Join();
    }
}
"@

$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    [UiaInspector]::InspectWindow([IntPtr]0x0021003A)
}
