param(
    [string]$ProcessName = "pes6",
    [string]$Keys = "ENTER",
    [int]$Delay = 0,
    [int]$Gap = 140
)
# Sends keys to the game. The game polls DirectInput, so the keys are injected
# as real key events with a gap between them; a burst is easily missed.
#
#   press.ps1 -Keys "DOWN DOWN ENTER"
#   press.ps1 -Keys "VK_F1"
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class K {
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
  [DllImport("user32.dll")] public static extern uint SendInput(uint n, INPUT[] inputs, int size);
  [StructLayout(LayoutKind.Sequential)]
  public struct INPUT { public uint type; public InputUnion U; }
  [StructLayout(LayoutKind.Explicit)]
  public struct InputUnion {
    [FieldOffset(0)] public KEYBDINPUT ki;
    [FieldOffset(0)] public MOUSEINPUT mi;
    [FieldOffset(0)] public HARDWAREINPUT hi;
  }
  [StructLayout(LayoutKind.Sequential)]
  public struct KEYBDINPUT { public ushort wVk; public ushort wScan; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
  [StructLayout(LayoutKind.Sequential)]
  public struct MOUSEINPUT { public int dx; public int dy; public uint mouseData; public uint dwFlags; public uint time; public IntPtr dwExtraInfo; }
  [StructLayout(LayoutKind.Sequential)]
  public struct HARDWAREINPUT { public uint uMsg; public ushort wParamL; public ushort wParamH; }
  public const uint INPUT_KEYBOARD = 1;
  public const uint KEYEVENTF_KEYUP = 0x0002;
  public static void Tap(ushort vk) {
    INPUT[] down = new INPUT[1];
    down[0].type = INPUT_KEYBOARD;
    down[0].U.ki = new KEYBDINPUT { wVk = vk, wScan = 0, dwFlags = 0, time = 0, dwExtraInfo = IntPtr.Zero };
    INPUT[] up = new INPUT[1];
    up[0].type = INPUT_KEYBOARD;
    up[0].U.ki = new KEYBDINPUT { wVk = vk, wScan = 0, dwFlags = KEYEVENTF_KEYUP, time = 0, dwExtraInfo = IntPtr.Zero };
    SendInput(1, down, Marshal.SizeOf(typeof(INPUT)));
    System.Threading.Thread.Sleep(60);
    SendInput(1, up, Marshal.SizeOf(typeof(INPUT)));
  }
}
"@

$map = @{
    "ENTER" = 0x0D; "RETURN" = 0x0D; "ESC" = 0x1B; "ESCAPE" = 0x1B
    "SPACE" = 0x20; "UP" = 0x26; "DOWN" = 0x28; "LEFT" = 0x25; "RIGHT" = 0x27
    "TAB" = 0x09; "BACK" = 0x08; "BACKSPACE" = 0x08; "SHIFT" = 0x10
    "F1" = 0x70; "F2" = 0x71; "F3" = 0x72; "F4" = 0x73; "F5" = 0x74
    "F6" = 0x75; "F7" = 0x76; "F8" = 0x77; "F9" = 0x78; "F10" = 0x79
    "HOME" = 0x24; "END" = 0x23; "PGUP" = 0x21; "PGDN" = 0x22
}

if ($Delay -gt 0) { Start-Sleep -Seconds $Delay }
$p = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue |
     Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
if (-not $p) { Write-Output "no window for $ProcessName"; exit 1 }
[void][K]::ShowWindow($p.MainWindowHandle, 9)
[void][K]::SetForegroundWindow($p.MainWindowHandle)
Start-Sleep -Milliseconds 400

foreach ($name in ($Keys -split "\s+" | Where-Object { $_ })) {
    $n = $name.ToUpper()
    if ($map.ContainsKey($n)) {
        $vk = [byte]$map[$n]
    }
    elseif ($n -match "^VK_([0-9A-F]+)$") {
        $vk = [byte][Convert]::ToInt32($Matches[1], 16)
    }
    elseif ($n.Length -eq 1) {
        $vk = [byte][System.Windows.Forms.AVK]::FromChar($n[0])
    }
    else {
        Write-Output "unknown key '$name'"
        continue
    }
    $scan = 0
    [K]::Tap([uint16]$vk)
    Start-Sleep -Milliseconds $Gap
}
Write-Output "sent '$Keys' to '$($p.MainWindowTitle)'"
