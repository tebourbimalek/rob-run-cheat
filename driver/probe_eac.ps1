$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\eackill.log'
Set-Content -Path $log -Value '== kill game' -Encoding utf8
Get-Process -Name 'ROB RUN' -ErrorAction SilentlyContinue | ForEach-Object {
  ('killing pid ' + $_.Id) | Out-File -Append -FilePath $log -Encoding utf8
  & taskkill.exe /PID $_.Id /F /T 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
}
Start-Sleep -Seconds 3
'== stop EAC kernel driver' | Out-File -Append -FilePath $log -Encoding utf8
sc.exe stop EasyAntiCheat_EOSSys 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
Start-Sleep -Seconds 3
sc.exe query EasyAntiCheat_EOSSys 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
'== retry intel load' | Out-File -Append -FilePath $log -Encoding utf8
sc.exe delete iqvw64etest 2>&1 | Out-Null
sc.exe create iqvw64etest type= kernel start= demand binPath= "$repo\driver\out\intel_extracted.sys" 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
sc.exe start iqvw64etest 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
('=== exit ' + $LASTEXITCODE) | Out-File -Append -FilePath $log -Encoding utf8
sc.exe query iqvw64etest 2>&1 | Out-File -Append -FilePath $log -Encoding utf8
'== done' | Out-File -Append -FilePath $log -Encoding utf8
Start-Sleep -Seconds 2
