param(
    [string]$ProcessName = "pes6",
    [string]$Out = "$env:TEMP\pes6_shot.png",
    [int]$Wait = 20          # seconds to wait for a usable window
)
# Captures the game window once it is mapped and has a real size. The window is
# briefly forced to the top, because a capture taken while another app is
# focused can grab that app instead.
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W {
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out R r);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
  [StructLayout(LayoutKind.Sequential)] public struct R { public int L, T, Rt, B; }
}
"@
$TOPMOST = [IntPtr](-1)
$NOTOPMOST = [IntPtr](-2)
$SWP_NOMOVE = 0x0002; $SWP_NOSIZE = 0x0001; $SWP_SHOWWINDOW = 0x0040

$deadline = (Get-Date).AddSeconds($Wait)
$h = [IntPtr]::Zero; $w = 0; $ht = 0
while ((Get-Date) -lt $deadline) {
    $p = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue |
         Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    if ($p) {
        $r = New-Object W+R
        [void][W]::GetWindowRect($p.MainWindowHandle, [ref]$r)
        $cw = $r.Rt - $r.L; $ch = $r.B - $r.T
        if ($cw -gt 100 -and $ch -gt 100 -and [W]::IsWindowVisible($p.MainWindowHandle)) {
            $h = $p.MainWindowHandle; $w = $cw; $ht = $ch
            $title = $p.MainWindowTitle
            break
        }
    }
    Start-Sleep -Milliseconds 500
}
if ($h -eq [IntPtr]::Zero) { Write-Output "no usable window for $ProcessName after ${Wait}s"; exit 1 }

[void][W]::SetWindowPos($h, $TOPMOST, 0, 0, 0, 0, ($SWP_NOMOVE -bor $SWP_NOSIZE -bor $SWP_SHOWWINDOW))
Start-Sleep -Milliseconds 500
[void][W]::SetWindowPos($h, $NOTOPMOST, 0, 0, 0, 0, ($SWP_NOMOVE -bor $SWP_NOSIZE -bor $SWP_SHOWWINDOW))
Start-Sleep -Milliseconds 500

Write-Output "window '$title' ${w}x${ht}"
$bmp = New-Object System.Drawing.Bitmap($w, $ht)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
[void][W]::PrintWindow($h, $hdc, 2)   # 2 = PW_RENDERFULLCONTENT
$g.ReleaseHdc($hdc)
$g.Dispose()
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "saved $Out"
