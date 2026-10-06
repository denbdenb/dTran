Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

$desktop = [System.Windows.Automation.AutomationElement]::RootElement
$condition = [System.Windows.Automation.Condition]::TrueCondition
$windows = $desktop.FindAll([System.Windows.Automation.TreeScope]::Children, $condition)

Write-Host "Total top-level UI Automation windows: $($windows.Count)"
foreach ($w in $windows) {
    try {
        $name = $w.Current.Name
        $pid = $w.Current.ProcessId
        $class = $w.Current.ClassName
        if ($name -like "*Translate*" -or $class -like "*WinUI*" -or $pid -eq 28236) {
            Write-Host "Found: PID=$pid, Name='$name', Class='$class'"
        }
    } catch {}
}
