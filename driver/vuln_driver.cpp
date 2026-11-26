// vuln_driver.cpp — manual-map loader scaffold.
//
// SCAFFOLD ONLY. No vulnerable-driver exploit chain shipped.
// This file intentionally does not load drivers, allocate remote memory,
// copy PE sections into another process, resolve remote imports, create
// remote threads, or invoke remote DLL entry points.
//
// iqvw64e-style BYOVD exposure (documented, not shipped):
// - Loading a legitimately signed but vulnerable driver can expose arbitrary
//   kernel read/write/physical memory primitives to user mode.
// - Operational risk: BSOD, patch incompatibility, AV/EDR detection, Secure Boot
//   / HVCI / WDAC / blocklist policy failure, system instability.
// - Security/legal risk: bypassing platform protections may violate policy,
//   terms, or law depending on authorization and jurisdiction.
// - EAC/anti-cheat risk: known vulnerable-driver hashes/names/behaviors are
//   widely blocked or flagged; use can result in bans or system compromise.
// - Supply-chain risk: "drop a signed driver here" is an unsafe distribution
//   pattern. No driver payloads, IOCTL abuse code, or exploit primitives are
//   included here.

#include "vuln_driver.hpp"

#include <cstdio>

namespace drv {

bool bootVulnerableDriver(const wchar_t* /*path*/) {
  std::fprintf(stderr,
               "[drv] SCAFFOLD ONLY: vulnerable-driver loading is not implemented/shipped.\n"
               "[drv] No BYOVD, no ZwLoadDriver, no service install/start, no kernel primitives.\n");
  return false;
}

MapResult loadTargetDll(DWORD /*pid*/, const wchar_t* /*dllPath*/, HANDLE /*target*/) {
  std::fprintf(stderr,
               "[drv] SCAFFOLD ONLY: manual map is not implemented/shipped.\n"
               "[drv] Design required: PE validate, remote W^X alloc, sections, relocations,\n"
               "[drv] imports/OFT/forwarders, TLS, .pdata registration, CRT init, remote entry stub,\n"
               "[drv] PEB-unlinked limitations, re-map-on-restart handling.\n");
  return {0u, static_cast<uint32_t>(ERROR_NOT_SUPPORTED), "scaffold_only"};
}

}  // namespace drv
