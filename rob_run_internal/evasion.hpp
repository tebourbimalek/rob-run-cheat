#pragma once

// evasion.hpp — exposure notes and defensive stubs.
// SCOPE: documentation + user-mode hygiene stubs. No EAC bypass, no kernel
// primitives, no vulnerable driver, no signature forgery, no exploit chain.

#include <windows.h>
#include <cstdint>
#include <cstdio>

namespace rb::evasion {

// ---------------------------------------------------------------------------
// EAC exposure surface (operator awareness)
// ---------------------------------------------------------------------------
// - Module signature / PE header: manually mapped or injected modules can be
//   found by memory scans, VAD walks, PE-header signatures in private memory,
//   and hash/section heuristics. PEB-unlinked does not erase VAD evidence.
// - .text hook: Present/vtable detours and inline trampolines patch executable
//   pages; EAC can detect code-integrity diffs, unexpected hooks, and suspicious
//   call targets. dxgi_hook.hpp restores captured vtable entry on unhook.
// - Thread start: CreateRemoteThread/remote stub starts are high-signal events.
//   Start address outside known module, suspicious stack, or missing loader
//   metadata increases detection risk. loader is scaffold-only and refuses.
// - Handle: OpenProcess with broad access (PROCESS_ALL_ACCESS/VM_WRITE/etc.)
//   is detectable. Keep handles minimal, close promptly, avoid unnecessary
//   cross-process handle duplication.
// - Overlay/input: ImGui overlay and WndProc hook are user-mode artifacts;
//   they are visible to integrity/user-mode scans. Menu input capture is for UX,
//   not stealth.
// - Timing/behavior: inhuman aim cadence, perfect visibility, zero recoil/spread
//   writes, and impossible reaction distributions are behavioral signals.

// ---------------------------------------------------------------------------
// ntdll unhook stub
// ---------------------------------------------------------------------------
// SCOPE: user-mode only. Intended for environments where YOU own the machine
// and have explicit authorization to test integrity-hook behavior.
// RISK: may destabilize process, break EDR/EAC hooks, trigger integrity checks,
// or violate terms/policy/law when used without authorization. Not a bypass.
// This stub does NOT map a clean ntdll, does NOT patch remote processes, and
// does NOT touch kernel.

struct NtdllUnhookStatus {
  bool attempted = false;
  bool restored = false;
  uint32_t bytesConsidered = 0;
  const char* note = "stub_only";
};

inline NtdllUnhookStatus ntdllUnhookStub() {
  NtdllUnhookStatus s{};
  s.attempted = false;
  s.restored = false;
  s.bytesConsidered = 0;
  s.note = "stub_only: no .text restore performed. Implement only with explicit authorization, "
           "clean-disk ntdll validation, per-function scope, and rollback.";
  std::fprintf(stderr, "[rb][evasion] ntdllUnhookStub: %s\n", s.note);
  return s;
}

// ---------------------------------------------------------------------------
// Per-build hygiene notes (not stealth guarantees)
// ---------------------------------------------------------------------------
// - Packing: optional per-build packer/compressor can change signatures, but
//   packed stubs/unpack behavior are themselves detectable. Do not claim bypass.
// - Section-name randomization: cosmetic; scanners key on entropy, behavior,
//   headers, and memory patterns, not names alone.
// - Build randomization: vary build path/timestamps/pdb stripping in authorized
//   test builds; keep reproducible release builds for debugging.
// - Config/artifacts: store config in user profile/DLL dir as normal file;
//   avoid suspicious hidden locations in production test tooling.

} // namespace rb::evasion
