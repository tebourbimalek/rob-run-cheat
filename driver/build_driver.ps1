# build_driver.ps1 — deterministic WDM build for driver\rb_drv.cpp
#
# No CMake: CMake cannot produce a native WDM image, and a hand-rolled
# cl/link line is reproducible and pinned to whatever WDK/VS is installed.
#
#   powershell -ExecutionPolicy Bypass -File driver\build_driver.ps1
#
# Output: driver\out\rb_drv.sys
# Exit codes: 0 built, 2 toolchain missing, 3 compile failed, 4 link failed.

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$out  = Join-Path $here 'out'

# --- locate Visual Studio (BuildTools or IDE) -------------------------------
$vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = $null
if (Test-Path $vswhere) {
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2>$null
}
if (-not $vsPath) {
    foreach ($p in @(
        'C:\Program Files\Microsoft Visual Studio\2022\BuildTools',
        'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools')) {
        if (Test-Path $p) { $vsPath = $p; break }
    }
}
if (-not $vsPath) { Write-Host '[build] FAIL: no Visual Studio with C++ toolset found' -ForegroundColor Red; exit 2 }
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { Write-Host "[build] FAIL: vcvars64.bat missing at $vcvars" -ForegroundColor Red; exit 2 }

# --- locate WDK kernel headers/libs ----------------------------------------
# Source 1: an installed WDK (Program Files\Windows Kits\10).
# Source 2: the WDK NuGet payload extracted to driver\ext\wdk (no admin needed).
# Both layouts expose Include\<ver>\km and Lib\<ver>\km<x64>, so one probe works.
$kitRoot = 'C:\Program Files (x86)\Windows Kits\10'
$kmInc = $null
$kmLib = $null
$wdkVer = $null

if (Test-Path "$kitRoot\Lib") {
    foreach ($v in (Get-ChildItem "$kitRoot\Lib" -Directory | Sort-Object Name -Descending)) {
        $lib = Join-Path $v.FullName 'km\x64\ntoskrnl.lib'
        $inc = Join-Path $kitRoot "Include\$($v.Name)\km\ntifs.h"
        if ((Test-Path $lib) -and (Test-Path $inc)) {
            $wdkVer = $v.Name
            $kmInc = Join-Path $kitRoot "Include\$wdkVer\km"
            $kmLib = Join-Path $v.FullName 'km\x64'
            break
        }
    }
}

if (-not $kmInc) {
    $nupkgDir = Join-Path $here 'ext\wdk\c'
    if (Test-Path "$nupkgDir\Lib") {
        foreach ($v in (Get-ChildItem "$nupkgDir\Lib" -Directory | Sort-Object Name -Descending)) {
            $lib = Join-Path $v.FullName 'km\x64\ntoskrnl.lib'
            $inc = Join-Path $nupkgDir "Include\$($v.Name)\km\ntifs.h"
            if ((Test-Path $lib) -and (Test-Path $inc)) {
                $wdkVer = $v.Name
                $kmInc = Join-Path $nupkgDir "Include\$wdkVer\km"
                $kmLib = Join-Path $v.FullName 'km\x64'
                Write-Host '[build] WDK source: NuGet payload at driver\ext\wdk' -ForegroundColor Yellow
                break
            }
        }
    }
}

if (-not $kmInc) {
    Write-Host '[build] FAIL: WDK kernel headers/libs not found.' -ForegroundColor Red
    Write-Host '[build] Either install WDK 10.0.26100.6584 (VS Installer -> Individual components ->' -ForegroundColor Yellow
    Write-Host '[build]   Windows Driver Kit), or drop the NuGet payload at driver\ext\wdk\c :' -ForegroundColor Yellow
    Write-Host '[build]   Invoke-WebRequest -UseBasicParsing -OutFile wdk.nupkg `' -ForegroundColor Yellow
    Write-Host '[build]     https://www.nuget.org/api/v2/package/Microsoft.Windows.WDK.x64/10.0.26100.6584' -ForegroundColor Yellow
    Write-Host '[build]   Expand-Archive wdk.nupkg -DestinationPath driver\ext\wdk' -ForegroundColor Yellow
    exit 2
}

New-Item -ItemType Directory -Force -Path $out | Out-Null

$src = Join-Path $here 'rb_drv.cpp'
$obj = Join-Path $out 'rb_drv.obj'
$sys = Join-Path $out 'rb_drv.sys'

$cflags = @(
    '/nologo', '/c', '/kernel', '/GS-', '/GR-', '/EHs-c-', '/Ox', '/W4',
    '/D_WIN64', '/DAMD64', '/D_AMD64_', '/DPOOL_NX_OPTIN=1', '/DNDEBUG',
    "/I`"$kmInc`"",
    "/Fo`"$obj`"",
    "/Fd`"$(Join-Path $out 'rb_drv.pdb')`"",
    "`"$src`""
)

$lflags = @(
    '/nologo', '/DRIVER', '/SUBSYSTEM:NATIVE', '/MACHINE:X64',
    '/ENTRY:DriverEntry', '/NODEFAULTLIB:libcntpr',
    "/LIBPATH:`"$kmLib`"",
    "/OUT:`"$sys`"",
    "`"$obj`"", 'ntoskrnl.lib'
)

$cmdFile = Join-Path $out 'build.cmd'
@"
@echo off
call "$vcvars" >nul
cl $($cflags -join ' ')
if errorlevel 1 exit /b 3
link $($lflags -join ' ')
if errorlevel 1 exit /b 4
exit /b 0
"@ | Set-Content -Path $cmdFile -Encoding ASCII

Write-Host "[build] VS:  $vsPath"
Write-Host "[build] WDK: $wdkVer"
Write-Host "[build] cl/link via $cmdFile"
& cmd.exe /c "`"$cmdFile`""
$code = $LASTEXITCODE
if ($code -eq 3) { Write-Host '[build] FAIL: compile' -ForegroundColor Red; exit 3 }
if ($code -eq 4) { Write-Host '[build] FAIL: link' -ForegroundColor Red; exit 4 }
if ($code -ne 0) { Write-Host "[build] FAIL: exit $code" -ForegroundColor Red; exit $code }

if (Test-Path $sys) {
    $len = (Get-Item $sys).Length
    Write-Host "[build] OK: $sys ($len bytes)" -ForegroundColor Green
} else {
    Write-Host '[build] FAIL: no image produced' -ForegroundColor Red
    exit 4
}
