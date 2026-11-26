#pragma once

// vuln_driver.hpp — manual-map loader scaffold (NOT a working exploit chain).
//
// STATUS: SCAFFOLD ONLY. No vulnerable-driver exploit chain shipped.
// No BYOVD loading, no kernel primitives, no remote injection performed.

#include <windows.h>
#include <cstdint>

namespace drv {

struct MapResult {
  uintptr_t remoteBase = 0;
  uint32_t lastError = 0;
  const char* reason = nullptr;
};

// Design-only entry points. Current implementation refuses to operate.
bool bootVulnerableDriver(const wchar_t* path);
MapResult loadTargetDll(DWORD pid, const wchar_t* dllPath, HANDLE target);

// Manual-map design checklist (implementation required before any runtime use):
// - Validate PE32+ x64, section alignment, size bounds.
// - Allocate remote image memory (no RWX; W^X preferred, finalize protections per section).
// - Copy headers/sections into remote buffer.
// - Apply relocations (IMAGE_REL_BASED_DIR64).
// - Resolve imports by walking OFT/INT, writing IAT; handle ordinal/import-forwarder cases.
// - Handle TLS callbacks/directory if present.
// - Register exception handlers (RtlAddFunctionTable for .pdata/.xdata on x64).
// - Initialize CRT/security cookie as required by target DLL.
// - Invoke DLL_PROCESS_ATTACH entry in target via controlled remote stub; no loader list entry.
// - Support re-map after restart: detect stale pid/process, rebuild clean state, no reboot assumption.
// - Keep module out of PEB loader lists where feasible (limitations documented in mapper.cpp).

}  // namespace drv
