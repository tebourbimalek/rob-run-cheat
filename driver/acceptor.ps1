$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\acceptor.log'
Set-Content -Path $log -Value 'acceptor start' -Encoding utf8

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class U {
  [DllImport("user32.dll")]
  public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")]
  public static extern IntPtr FindWindow(string cls, string name);
  [DllImport("user32.dll")]
  public static extern IntPtr SendMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
}
"@ -ErrorAction SilentlyContinue

$sent = $false
for ($i=0; $i -lt 150; $i++) {
  $c = Get-Process consent -ErrorAction SilentlyContinue -First 1
  if ($c -and $c.MainWindowHandle -ne 0) {
    ('found consent pid=' + $c.Id + ' hwnd=0x' + $c.MainWindowHandle.ToString('X')) | Out-File -Append -FilePath $log -Encoding utf8
    # send Enter (accept) — consent focuses Yes by default
    $r = [U]::SendMessage($c.MainWindowHandle, 0x0100, 0x0000000D, 0)  # WM_KEYDOWN VK_RETURN
    $r2 = [U]::SendMessage($c.MainWindowHandle, 0x0101, 0x0000000D, 0)  # WM_KEYUP
    'sent Enter -> 0x' + $r.ToString('X') | Out-File -Append -FilePath $log -Encoding utf8
    $sent = $true
    break
  }
  Start-Sleep -Milliseconds 100
}
if (-not $sent) { 'no consent window seen' | Out-File -Append -FilePath $log -Encoding utf8 }
'acceptor done' | Out-File -Append -FilePath $log -Encoding utf8
