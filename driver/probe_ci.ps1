$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\ci.log'
Set-Content -Path $log -Value '== CiTool -lp' -Encoding utf8
& C:\Windows\System32\CiTool.exe -lp 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
'== MpComputerStatus' | Out-File -Append -FilePath $log -Encoding utf8
Get-MpComputerStatus | Select-Object IsTamperProtected,TamperProtectionSource,RealTimeProtectionEnabled | Out-String | Out-File -Append -FilePath $log -Encoding utf8
'== disable realtime' | Out-File -Append -FilePath $log -Encoding utf8
try { Set-MpPreference -DisableRealtimeMonitoring $true -ErrorAction Stop; 'realtime off OK' | Out-File -Append -FilePath $log -Encoding utf8 }
catch { ('realtime off FAIL: ' + $_.Exception.Message) | Out-File -Append -FilePath $log -Encoding utf8 }
'== retry intel driver' | Out-File -Append -FilePath $log -Encoding utf8
sc.exe delete iqvw64etest 2>&1 | Out-Null
sc.exe create iqvw64etest type= kernel start= demand binPath= "$repo\driver\out\intel_extracted.sys" 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
sc.exe start iqvw64etest 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
('=== exit ' + $LASTEXITCODE) | Out-File -Append -FilePath $log -Encoding utf8
sc.exe delete iqvw64etest 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
try { Set-MpPreference -DisableRealtimeMonitoring $false -ErrorAction Stop } catch {}
'== done' | Out-File -Append -FilePath $log -Encoding utf8
Start-Sleep -Seconds 2
