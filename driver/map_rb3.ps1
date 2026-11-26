$ErrorActionPreference = 'Continue'
$repo = 'C:\Users\GIGABYTE\Desktop\new-rob-runhack'
$log  = Join-Path $repo 'driver\out\map3.log'
Set-Content -Path $log -Value 'start' -Encoding utf8
$exe = "$repo\driver\ext\kdmapper\x64\Release\kdmapper_Release.exe"
$o   = "$repo\driver\out\map3.out"
$e   = "$repo\driver\out\map3.err"
Remove-Item $o,$e -Force -ErrorAction SilentlyContinue

$t0 = Get-Date
try {
  $p = Start-Process -FilePath $exe -ArgumentList '--free', "$repo\driver\out\rb_drv.sys" -Verb RunAs `
       -RedirectStandardOutput $o -RedirectStandardError $e -Wait:$false -PassThru
  ("child pid=$($p.Id) launched $( (Get-Date)-$t0 )") | Out-File -Append -FilePath $log -Encoding utf8
  for ($i=0;$i -lt 30;$i++) {
    Start-Sleep -Seconds 2
    $p2 = Get-Process -Id $p.Id -ErrorAction SilentlyContinue
    if (-not $p2) { break }
  }
  ('alive after loop: ' + ((Get-Process -Id $p.Id -ErrorAction SilentlyContinue) -ne $null)) | Out-File -Append -FilePath $log -Encoding utf8
} catch {
  ('START-EXCEPTION: ' + $_.Exception.Message) | Out-File -Append -FilePath $log -Encoding utf8
}
'== stdout ==' | Out-File -Append -FilePath $log -Encoding utf8
Get-Content $o -ErrorAction SilentlyContinue | Out-File -Append -FilePath $log -Encoding utf8
'== stderr ==' | Out-File -Append -FilePath $log -Encoding utf8
Get-Content $e -ErrorAction SilentlyContinue | Out-File -Append -FilePath $log -Encoding utf8
'== exit/restart? ==' | Out-File -Append -FilePath $log -Encoding utf8
(Get-CimInstance Win32_SystemDriver | Where-Object { $_.Name -eq 'iqvw64e' }).State | Out-File -Append -FilePath $log -Encoding utf8
'== device' | Out-File -Append -FilePath $log -Encoding utf8
(Test-Path '\\.\RbDrv') | Out-File -Append -FilePath $log -Encoding utf8
'== done' | Out-File -Append -FilePath $log -Encoding utf8
