$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\map4.log'
Set-Content -Path $log -Value 'start' -Encoding utf8
$exe = "$repo\driver\ext\kdmapper\x64\Release\kdmapper_Release.exe"
$o   = "$repo\driver\out\map4.out"
Remove-Item $o -Force -ErrorAction SilentlyContinue
$p = $null
try {
  $p = Start-Process -FilePath $exe -ArgumentList '--free', "$repo\driver\out\rb_drv.sys" -Verb RunAs -RedirectStandardOutput $o -PassThru
  ('p in psm=$($p -ne $null) pid=' + (if($p){$p.Id}else{'na'})) | Out-File -Append -FilePath $log -Encoding utf8
} catch {
  ('SP-EXC: ' + $_.Exception.Message) | Out-File -Append -FilePath $log -Encoding utf8
}
Start-Sleep -Seconds 8
if ($p) { ('alive8s=' + ($p.HasExited -eq $false) + ' exitcode=' + $p.ExitCode) | Out-File -Append -FilePath $log -Encoding utf8 }
Start-Sleep -Seconds 15
if ($p -and -not $p.HasExited) { 'alive23s=yes' | Out-File -Append -FilePath $log -Encoding utf8 }
'== kdmapper out ==' | Out-File -Append -FilePath $log -Encoding utf8
Get-Content $o -ErrorAction SilentlyContinue | Out-File -Append -FilePath $log -Encoding utf8
'== device ==' | Out-File -Append -FilePath $log -Encoding utf8
(Test-Path '\\.\RbDrv') | Out-File -Append -FilePath $log -Encoding utf8
'== iqvw64e state ==' | Out-File -Append -FilePath $log -Encoding utf8
(Get-CimInstance Win32_SystemDriver -ErrorAction SilentlyContinue | Where-Object { $_.Name -eq 'iqvw64e' }).State | Out-File -Append -FilePath $log -Encoding utf8
'== done ==' | Out-File -Append -FilePath $log -Encoding utf8
