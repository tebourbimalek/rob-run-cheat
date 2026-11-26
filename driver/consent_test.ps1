$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$out = "$repo\driver\out\acceptor_cmds.log"
$elev = "$repo\driver\out\elev2.log"
Remove-Item $elev -Force -ErrorAction SilentlyContinue
$cmd = "& { `$id=[Security.Principal.WindowsIdentity]::GetCurrent(); `$p=New-Object Security.Principal.WindowsPrincipal(`$id); 'elev admin=' + `$p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator) | Out-File -FilePath '$elev' -Encoding utf8; 'done' | Out-File -Append -FilePath '$elev' -Encoding utf8 }"
$pr1 = $null; $pr2 = $null
try {
  $pr1 = Start-Process powershell -Verb RunAs -ArgumentList '-NoProfile','-Command',$cmd -PassThru -ErrorAction Stop
  ('elev spawn pid=' + $pr1.Id) | Out-File $out -Encoding utf8
} catch {
  ('elev spawn EXC: ' + $_.Exception.Message) | Out-File $out -Encoding utf8
}
try {
  $pr2 = Start-Process powershell -NoProfile -ExecutionPolicy Bypass -File "$repo\driver\acceptor.ps1" -PassThru -ErrorAction Stop
  ('acceptor spawn pid=' + $pr2.Id) | Out-File -Append $out -Encoding utf8
} catch {
  ('acceptor spawn EXC: ' + $_.Exception.Message) | Out-File -Append $out -Encoding utf8
}
Start-Sleep -Seconds 40
'== elev2.log ==' | Out-File -Append $out -Encoding utf8
Get-Content $elev -ErrorAction SilentlyContinue | Out-File -Append $out -Encoding utf8
'== acceptor.log ==' | Out-File -Append $out -Encoding utf8
Get-Content "$repo\driver\out\acceptor.log" -ErrorAction SilentlyContinue | Out-File -Append $out -Encoding utf8
'== done ==' | Out-File $out -Encoding utf8
