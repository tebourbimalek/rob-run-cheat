# ROB RUN Internal — BUILD_ID 26807668

Steam appid `4262460`, Unity IL2CPP, Windows x64, EAC.  
No third-party cheat SDKs/paid libs. ImGui (vendored 1.90.9, DX11+Win32) + Win32/DXGI/own IL2CPP wrapper only.

## Build

```powershell
# From repo root
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Outputs (typical): `build/rob_run_internal.dll`, `build/mapper_main.exe` (scaffold refusal only).

Driver scaffold build (standalone):

```powershell
cmake -S driver -B driver\build -G "Visual Studio 17 2022" -A x64
cmake --build driver\build --config Release
```

ImGui: vendored at `rob_run_internal/deps/imgui` (v1.90.9). Required: `imgui*.cpp`, `backends/imgui_impl_dx11.*`, `backends/imgui_impl_win32.*`.

## Exact Test Procedure

1. **Preflight**: confirm Steam BUILD_ID `26807668` in `appmanifest_4262460.acf`. Confirm `deps/dump.cs` matches build. Do not use on mismatched patch.
2. **Build**: clean Release x64 build. Verify no invented offsets; unresolved markers remain `TODO(...)` in `offsets.hpp`.
3. **Launch game**: start ROB RUN, wait until main menu/swapchain is stable.
4. **Load DLL (authorized local test only)**: load `rob_run_internal.dll` by a lawful method you own and are authorized to use. `StartCheat` calls `rb_start()`. `driver/mapper_main.exe` is scaffold-only and refuses; it does not map.
5. **Overlay first**: press `Insert`. ImGui menu must open/close; mouse/keyboard captured when menu open. If no overlay, stop; validate DXGI Present hook/swapchain/device before enabling features.
6. **Entity/ESP stage**: only after BattleModel root locator, dict layout, camera Matrix4x4 ABI/layout, world position/transform/bone chains are runtime-validated. Enable ESP master; verify boxes/names/dist/health/skeleton against known in-game entities. Disable if invalid.
7. **Aim/combat stage**: only after raycast layerMask/QTI, camera VP, bone world positions, bullet speed/velocity sources, and fire hook candidates are runtime-validated. Test smoothing/delay/jitter/miss on private authorized environment. Silent aim/trigger remain stubbed until hook infra + fire-path validation exists.
8. **Unload**: call `StopCheat` export or unload through your authorized loader. Confirm Present vtable restored, WndProc restored, ImGui shutdown, no crash on resize/device-loss/unload.

## What Breaks After Patch / How To Fix

| Break | Symptom | Fix |
|---|---|---|
| Game update / BUILD_ID change | wrong reads, crash, empty entities, bad ESP/aim | Re-dump `dump.cs` for new BUILD_ID. Regenerate `deps/slices/*` (`deps/regen_slices.ps1`). Re-grep RVAs/offsets, update `offsets.hpp` with `dump.cs:LINE` evidence. Re-validate all runtime markers. |
| IL2CPP method pointer layout | crashes on method resolve/call | Verify `Il2CppMethod` methodPointer offset (`il2cpp_wrapper.hpp`, currently `+0x0` assumed) against live build; update `TODO(runtime: "Il2CppMethod methodPointer offset")`. |
| Camera matrix ABI/layout | W2S wrong, inverted, behind-camera false positives | Runtime-confirm `Camera.worldToCameraMatrix`/`projectionMatrix` return layout, row/column major, multiply order. Adjust `aimbot.hpp`/matrix handling; keep clip_w<=0 reject. |
| Dictionary/List layout | entity count 0, bad pointers, crash | Confirm IL2CPP Dictionary/List field offsets/entry stride on live build. `entity.hpp` probes conservatively; if probe fails, do not walk. |
| BattleModel root locator | no entities | Locate BattleModel instance/root holder at runtime; wire to `kBattleEntitiesModel` from backing field `+0x90` (`dump.cs:239166`). No static singleton in dump. |
| Bone indices / transform chain | skeleton/aim wrong bone | Use `bone_probe.hpp` runtime SMR name scan; verify bone hierarchy and transform world-position source. Do not use generic humanoid table. |
| Raycast signature/layers | visibility always false/true, crash | Confirm `Physics.Raycast` 6-arg signature, RaycastHit size/layout, layerMask, QueryTriggerInteraction on live build. |
| Fire/shot path | silent aim/trigger no-op or crash | Validate ShotArgsSystem/ShotIntentionUpdater candidates on live build. Implement hook infra only after signature/ABI/prologue safety confirmed. |
| DXGI swapchain/device | no overlay, resize crash, device-loss crash | Verify Present vtable index 8 on current swapchain; check resize/Reset/ResizeBuffers handling and RTV recreation. |

## File-by-File Purpose

| File | Purpose |
|---|---|
| `rob_run_internal/main.cpp` | DLL entry/start/unload exports, Insert menu toggle, ImGui menu, entity/ESP/aim/combat frame pipeline. |
| `rob_run_internal/offsets.hpp` | Single source for dump-evidenced RVAs/field offsets/runtime markers; `validateFilled()` gates evidence-backed constants. |
| `rob_run_internal/il2cpp_wrapper.hpp` | GameAssembly base resolve (PEB->Ldr/EnumProcessModulesEx), export/RVA resolve, class/method/field cache, safe call/read wrappers, camera/physics helpers. |
| `rob_run_internal/dxgi_hook.hpp` | DXGI Present vtable hook, ImGui DX11/Win32 init, WndProc hook/input capture, resize/device-loss handling, clean restore. |
| `rob_run_internal/entity.hpp` | IL2CPP List/Dictionary/array reads with SEH guards, runtime dict layout probe, BattleEntitiesModel/Player access, entity view collection/cache. |
| `rob_run_internal/esp.hpp` | ESP entry draw primitives: box/name/distance/health/skeleton, viewport clamp, visibility color, config gates. |
| `rob_run_internal/aimbot.hpp` | W2S, FOV/bone selection, smoothing, cooldown/reaction delay, jitter, intentional miss, prediction, raycast visibility + TTL cache, silent-aim stub. |
| `rob_run_internal/combat.hpp` | No-recoil/no-spread SEH-guarded model-field writes; triggerbot gate with delay/FOV/visibility/miss; fire action TODO. |
| `rob_run_internal/bone_probe.hpp` | Runtime bone index resolver by SkinnedMeshRenderer bone-name scan; manual dump button hook point. |
| `rob_run_internal/features.hpp` | Feature flags/tunables, JSON config save/load with defaults/clamps, DLL/APPDATA config path resolution. |
| `rob_run_internal/evasion.hpp` | EAC exposure notes, ntdll unhook user-mode stub (no bypass), per-build hygiene notes. |
| `rob_run_internal/unity_types.hpp` | Minimal Unity/IL2CPP struct/layout helpers used by wrappers. |
| `rob_run_internal/CMakeLists.txt` | Builds `rob_run_internal.dll`; compiles vendored ImGui DX11+Win32 backends. |
| `driver/mapper.cpp` | Manual-map loader **scaffold only**; reports status and hard-refuses. No BYOVD/injection chain shipped. |
| `driver/vuln_driver.hpp` | Driver/map scaffold API + design checklist; not a working exploit chain. |
| `driver/vuln_driver.cpp` | Scaffold refusal implementations; documents iqvw64e-style BYOVD exposure/risk. No kernel primitives/IOCTL abuse. |
| `driver/CMakeLists.txt` | Builds `mapper_main.exe` scaffold refusal binary. |
| `CMakeLists.txt` | Root build; adds `rob_run_internal` and `driver`. |
| `deps/dump.cs` | Il2CppDumper output for BUILD_ID 26807668; source of offsets/RVAs. Not loaded whole. |
| `deps/regen_slices.ps1` | Regenerates grep/range slices under `deps/slices/`. |
| `deps/slices/*` | Small dump slices for repeatable lookup. |
| `todo1.md` | Gap map, blockers, unresolved markers, implementation checklist. |

## Runtime Blockers / Honest State

- `offsets.hpp`: 23 unresolved runtime/TODO markers (12 class runtime-resolve, 1 root-instance locator, 3 bone static defaults pending live probe, 7 feature-symbol TODOs). Runtime use blocked until resolved/validated.
- BattleModel root-instance locator unresolved; `Dictionary<int,IEntityModel>` entry stride/layout must be confirmed live.
- Camera Matrix4x4 ABI/layout and VP multiply order must be confirmed live.
- Entity world position/transform/bone world positions, health, team/faction, alive/dead, player name unresolved.
- Visibility layerMask/QueryTriggerInteraction unresolved; raycast is conservative.
- Silent aim/triggerbot fire action unresolved; no detour/hook/input infra shipped.
- `driver/` manual-map path is scaffold only; no working vulnerable-driver/EAC bypass exploit chain shipped.

## Safety / Legality

For authorized local testing only. No vulnerable-driver exploit chain, no EAC bypass payload, no third-party cheat SDK. Use only on systems and software you own or are explicitly authorized to test.