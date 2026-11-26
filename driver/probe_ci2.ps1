$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\ci2.log'
Set-Content -Path $log -Value '== MpComputerStatus' -Encoding utf8
Get-MpComputerStatus | Select-Object IsTamperProtected,TamperProtectionSource,RealTimeProtectionEnabled,AntivirusEnabled | Out-String | Out-File -Append -FilePath $log -Encoding utf8

'== disable realtime' | Out-File -Append -FilePath $log -Encoding utf8
try { Set-MpPreference -DisableRealtimeMonitoring $true -ErrorAction Stop; 'realtime off OK' | Out-File -Append -FilePath $log -Encoding utf8 }
catch { ('realtime off FAIL: ' + $_.Exception.Message) | Out-File -Append -FilePath $log -Encoding utf8 }

'== load intel driver' | Out-File -Append -FilePath $log -Encoding utf8
sc.exe delete iqvw64etest 2>&1 | Out-Null
sc.exe create iqvw64etest type= kernel start= demand binPath= "$repo\driver\out\intel_extracted.sys" 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
$out = sc.exe start iqvw64etest 2>&1
$out | Out-File -Append -FilePath $log -Encoding utf8
('=== sc exit ' + $LASTEXITCODE) | Out-File -Append -FilePath $log -Encoding utf8
sc.exe delete iqvw64etest 2>&1 | Out-File -Append -FilePath $log -Encoding utf8

'== system log (last 10 min)' | Out-File -Append -FilePath $log -Encoding utf8
Get-WinEvent -FilterHashtable @{LogName='System'; StartTime=(Get-Date).AddMinutes(-10)} -ErrorAction SilentlyContinue |
  Select-Object TimeCreated,Id,ProviderName,@{n='Msg';e={ ($_.Message -replace "`r?`n",' ').Substring(0,[Math]::Min(200,($_.Message -replace "`r?`n",' ').Length)) }} |
  Format-List | Out-String -Width 240 | Out-File -Append -FilePath $log -Encoding utf8

'== restore realtime' | Out-File -Append -FilePath $log -Encoding utf8
try { Set-MpPreference -DisableRealtimeMonitoring $false -ErrorAction Stop; 'realtime on OK' | Out-File -Append -FilePath $log -Encoding utf8 } catch { 'realtime on restore FAIL' | Out-File -Append -FilePath $log -Encoding utf8 }
'== done' | Out-File -Append -FilePath $log -Encoding utf8
Start-Sleep -Seconds 2
