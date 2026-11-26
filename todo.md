# ROB RUN Internal — Offset Pass TODO

## 0) Blocker
- [x] Installed Steam `BUILD_ID` confirmed: `26807668` (appid `4262460`, `appmanifest_4262460.acf`). Matching folder only; no fallback.

## 1) Acquire Dump
- [x] Fetch `dump.cs` for matching `BUILD_ID` only:
  - `https://raw.githubusercontent.com/InsideExploit/rob-run-sdk/main/26807668/dump.cs`
- [x] Save outside source tree: `deps/dump.cs`
- [x] Report file size and line count: `82,001,124 bytes`, `2,070,513 lines`
- [x] Fetch succeeded; local Il2CppDumper fallback not required.

## 2) Slice `dump.cs` (no full-file load)
- [x] Grep and extract hit ranges + 40 lines above into `deps/slices/`
- [x] Patterns:
  - [x] `class PlayerManager` — MISS (0 hits). Retarget candidates: `BattleEntitiesModel` (238456), `PlayerEntityModel` (363333), `PlayerClientModel` (362936).
  - [x] `class Player\b` — MISS (0 hits). Retargeted to PlayerEntityModel/PlayerClientModel.
  - [x] `class Camera` — UnityEngine `Camera : Behaviour` at 1085427 sliced.
  - [x] `class SkinnedMeshRenderer` — 1096172 sliced.
  - [x] `GetBoneTransform` — Animator 1919708; RVA 0x621A2B0 captured.
  - [x] `Physics::Raycast|Physics.Raycast` — Physics 2010040 sliced.
  - [x] `static PlayerManager|static Camera` — static PlayerManager MISS; static Camera property getters present.
- [x] Slices written: `Camera_UnityEngine.txt`, `SkinnedMeshRenderer.txt`, `Animator_GetBoneTransform.txt`, `Physics_Raycast.txt`, `Transform.txt`, `EntityModel.txt`, `CharacterClientModel.txt`, `PlayerClientModel.txt`, `PlayerEntityModel.txt`, `BattleEntitiesModel.txt`, `BattleModel.txt`.
- [x] Misses reported; retarget applied as best-effort, unconfirmed by user.

## 3) Fill `offsets.hpp`
- [x] Field offsets from `// 0x..` comment; method RVAs from `// RVA: 0x..`; never `VA`; all module-relative.
- [x] Every constant gets trailing comment with `dump.cs` line number.
- [x] Bone indices: no SkinnedMeshRenderer bone array/hierarchy data in dump.cs; HumanBodyBones enum is generic humanoid table and not used. Left as `TODO(<exact grep term>)` (runtime/asset required).
- [x] Added filled constants to `off::validateFilled()`/`validate()`; evidence-backed gate enforced.
- [x] User-approved contract change: `ALL_FILLED=true` for evidence-backed constants; 16 unresolved markers remain as explicit TODOs (not build-gated, must be resolved before use).
- [x] Unknown symbol: `TODO(<exact grep term>)`; no invented values.
- [ ] Remaining TODOs (16): 9 class markers (TypeDefIndex only; no class RVA in dump.cs), 4 static/singleton markers, 3 bone indices (runtime/asset required).

## 4) Carried Fixes
- [x] `il2cpp_wrapper.hpp`
  - [x] No `ZwQuerySystemInformation(SystemModuleInformation)`
  - [x] Walk `PEB->Ldr`; `EnumProcessModulesEx` fallback
  - [x] Real export names; `resolveExportOrRva` resolves RVA first, export table second
  - [x] Required exports: `il2cpp_domain_get`, `il2cpp_domain_assembly_open`, `il2cpp_class_from_name`, `il2cpp_class_get_method_from_name`
- [x] `dxgi_hook.hpp`
  - [x] PE32+ only; magic `0x20B`, reject `0x10B`
  - [x] Capture current `vtable[8]` at init; call through captured
  - [x] Restore exactly captured

## 5) Exit Gate
- [x] Deliver finished `offsets.hpp` in full
- [x] Deliver symbol -> value -> dump.cs line table for spot-checking
- [x] Stop and wait; next pass is driver

## Rules
- [x] No invented values
- [x] No suspicious-number adjustment; unresolved kept as TODO
- [x] Bounds-check every read in wrappers
- [x] No new features