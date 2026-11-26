// mapper.cpp — ROB RUN manual-map loader scaffold.
//
// SCAFFOLD ONLY. No vulnerable-driver exploit chain shipped.
// This binary intentionally refuses to load BYOVD drivers or inject/map DLLs.
//
// Intended design (not implemented):
// 1. Locate target process (ROB RUN.exe) by name/PID with explicit user confirmation.
// 2. Validate rob_run_internal.dll PE32+ x64, signatures/hashes, and build compatibility.
// 3. Manual-map DLL into target: headers/sections, relocations, imports/OFT/forwarders,
//    TLS callbacks, .pdata/.xdata registration (RtlAddFunctionTable), CRT/security cookie init.
// 4. Invoke DLL entry/StartCheat via controlled remote stub. No LoadLibrary in target.
// 5. Keep module out of PEB loader lists where feasible; document limitations:
//    - PEB-unlinked modules are still visible via VADs, memory scanners, ETW, hooks, threads.
//    - Exception handling/TLS/CRT behavior differs from normal loader; x64 .pdata registration required.
//    - Re-map after restart requires clean process state, no stale handles, no reboot assumption.
//
// iqvw64e-style mapper exposure (documented, not shipped):
// - Signed vulnerable drivers (iqvw64e-class etc.) can expose kernel primitives.
// - Risks: system instability/BSOD, HVCI/WDAC/Secure Boot blocks, AV/EDR detection,
//   anti-cheat bans, legal/policy exposure, supply-chain compromise.
// - This repo ships no driver payloads, no IOCTL abuse, no exploit chain.

#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "vuln_driver.hpp"

namespace fs = std::filesystem;

static DWORD findPidByExe(const wchar_t* exeName) {
  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE) return 0;
  DWORD pid = 0;
  PROCESSENTRY32W pe{};
  pe.dwSize = sizeof(pe);
  if (Process32FirstW(snap, &pe)) {
    do {
      if (_wcsicmp(pe.szExeFile, exeName) == 0) { pid = pe.th32ProcessID; break; }
    } while (Process32NextW(snap, &pe));
  }
  CloseHandle(snap);
  return pid;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
  const DWORD pid = findPidByExe(L"ROB RUN.exe");

  const fs::path root = fs::current_path();
  const fs::path parent = root.parent_path();
  const std::vector<fs::path> dllCands = {
    root / "rob_run_internal" / "rob_run_internal.dll",
    parent / "rob_run_internal" / "rob_run_internal.dll",
  };

  std::wstring foundDll;
  for (const auto& c : dllCands) if (fs::exists(c)) { foundDll = c.wstring(); break; }

  wchar_t msg[1024];
  _snwprintf_s(msg, _TRUNCATE,
    L"ROB RUN manual-map loader is SCAFFOLD ONLY.\n\n"
    L"No BYOVD driver loading.\n"
    L"No vulnerable-driver exploit chain.\n"
    L"No remote manual map / CreateRemoteThread.\n\n"
    L"Target PID (ROB RUN.exe): %lu\n"
    L"DLL candidate: %s\n\n"
    L"Implement authorized, safe mapping design before any runtime use.",
    static_cast<unsigned long>(pid),
    foundDll.empty() ? L"<not found>" : foundDll.c_str());

  MessageBoxW(nullptr, msg, L"Robcheat Loader — Scaffold", MB_ICONWARNING | MB_OK);

  // Hard refusal: no driver boot, no mapping.
  (void)drv::bootVulnerableDriver(L"");
  (void)drv::loadTargetDll(pid, foundDll.empty() ? L"" : foundDll.c_str(), nullptr);
  return ERROR_NOT_SUPPORTED;
}
