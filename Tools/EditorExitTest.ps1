# Repro harness for the editor-exit crash: focus editor, start PIE (Alt+P),
# wait, stop PIE (Shift+Esc), close the editor window, report crash count.
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win32Exit {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    public const uint WM_CLOSE = 0x0010;
}
"@

$proc = Get-Process UnrealEditor -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -match "BrainForge" } | Select-Object -First 1
if (-not $proc) { Write-Host "NO EDITOR WINDOW"; exit 1 }
Write-Host "editor window: $($proc.MainWindowTitle)"

[Win32Exit]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
Start-Sleep -Seconds 2

# start PIE
[System.Windows.Forms.SendKeys]::SendWait("%p")
Write-Host "sent Alt+P (PIE)"
Start-Sleep -Seconds 25

# stop PIE (Shift+Esc always stops PIE even when the game consumes Esc)
[Win32Exit]::SetForegroundWindow($proc.MainWindowHandle) | Out-Null
Start-Sleep -Seconds 1
[System.Windows.Forms.SendKeys]::SendWait("+({ESC})")
Write-Host "sent Shift+Esc (stop PIE)"
Start-Sleep -Seconds 6

# close the editor
[Win32Exit]::SendMessage($proc.MainWindowHandle, [Win32Exit]::WM_CLOSE, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
Write-Host "sent WM_CLOSE"

if (-not $proc.WaitForExit(120000)) { Write-Host "EDITOR DID NOT EXIT"; exit 2 }
Write-Host "editor exited, code=$($proc.ExitCode)"

Start-Sleep -Seconds 3
$crashes = (Get-ChildItem "D:\UEprojects\BrainForge\Saved\Crashes" -Directory -ErrorAction SilentlyContinue).Count
Write-Host "crash report count: $crashes"
