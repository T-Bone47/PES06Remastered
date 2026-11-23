Add-Type -AssemblyName System.Windows.Forms, System.Drawing
$proc = Start-Process -FilePath "C:\Users\dream\Downloads\PES06Remastered-main\test_game\pes6.exe" -WorkingDirectory "C:\Users\dream\Downloads\PES06Remastered-main\test_game" -PassThru
Start-Sleep -Seconds 6
Add-Type @'
  using System;
  using System.Runtime.InteropServices;
  public class Win32Capture {
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
      public int Left; public int Top; public int Right; public int Bottom;
    }
  }
'@
for ($i = 0; $i -lt 15; $i++) {
    Start-Sleep -Seconds 1
    $proc.Refresh()
    $p = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
    if ($p -and $p.MainWindowHandle -ne [IntPtr]::Zero) {
        $hwnd = $p.MainWindowHandle
        break
    }
}
if ($hwnd -and $hwnd -ne [IntPtr]::Zero) {
    [Win32Capture]::SetForegroundWindow($hwnd)
    Start-Sleep -Milliseconds 300
    $rect = New-Object Win32Capture+RECT
    [Win32Capture]::GetWindowRect($hwnd, [ref]$rect)
    $w = $rect.Right - $rect.Left
    $h = $rect.Bottom - $rect.Top
    if ($w -gt 10 -and $h -gt 10) {
        $bmp = New-Object System.Drawing.Bitmap($w, $h)
        $gfx = [System.Drawing.Graphics]::FromImage($bmp)
        $gfx.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size($w, $h)))
        $bmp.Save("C:\Users\dream\Downloads\PES06Remastered-main\test_game\screenshot_stage15.png", [System.Drawing.Imaging.ImageFormat]::Png)
        $gfx.Dispose()
        $bmp.Dispose()
        Write-Host "Screenshot saved successfully: $w x $h"
    } else {
        Write-Host "Window size too small: $w x $h"
    }
} else {
    Write-Host "MainWindowHandle is zero"
}
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
