$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\map2.log'
Set-Content -Path $log -Value '== cleanup test service' -Encoding utf8
sc.exe stop iqvw64etest 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
sc.exe delete iqvw64etest 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
Start-Sleep -Seconds 2

'== kdmapper' | Out-File -Append -FilePath $log -Encoding utf8
$exe = "$repo\driver\ext\kdmapper\x64\Release\kdmapper_Release.exe"
$out = & $exe --free "$repo\driver\out\rb_drv.sys" 2>&1
$out | Out-File -Append -FilePath $log -Encoding utf8
('=== kdmapper exit ' + $LASTEXITCODE) | Out-File -Append -FilePath $log -Encoding utf8

'== device present' | Out-File -Append -FilePath $log -Encoding utf8
(Test-Path '\\.\RbDrv') | Out-File -Append -FilePath $log -Encoding utf8
'== done' | Out-File -Append -FilePath $log -Encoding utf8
Start-Sleep -Seconds 2
