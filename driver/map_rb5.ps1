$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\map5.log'
Set-Content -Path $log -Value 'start' -Encoding utf8
$exe = "$repo\driver\ext\kdmapper\x64\Release\kdmapper_Release.exe"
'== precheck ==' | Out-File -Append -FilePath $log -Encoding utf8
("exe exists: " + (Test-Path $exe) + " sys exists: " + (Test-Path "$repo\driver\out\rb_drv.sys")) | Out-File -Append -FilePath $log -Encoding utf8
$out = & $exe --free "$repo\driver\out\rb_drv.sys" 2>&1 | Tee-Object -FilePath "$repo\driver\out\map5.raw"
('exit=' + $LASTEXITCODE) | Out-File -Append -FilePath $log -Encoding utf8
'== raw ==' | Out-File -Append -FilePath $log -Encoding utf8
$out | Out-File -Append -FilePath $log -Encoding utf8
'== device ==' | Out-File -Append -FilePath $log -Encoding utf8
(Test-Path '\\.\RbDrv') | Out-File -Append -FilePath $log -Encoding utf8
'== iqvw64e ==' | Out-File -Append -FilePath $log -Encoding utf8
(Get-CimInstance Win32_SystemDriver -ErrorAction SilentlyContinue | Where-Object { $_.Name -eq 'iqvw64e' }).State | Out-File -Append -FilePath $log -Encoding utf8
'== done ==' | Out-File -Append -FilePath $log -Encoding utf8
