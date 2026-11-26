# ROB RUN Internal — TODO1

Target: Steam appid `4262460`, BUILD_ID `26807668`, Unity IL2CPP, Windows x64, EAC.  
Dump: `deps/dump.cs` (82,001,124 bytes, 2,070,513 lines). Never load whole dump; grep + line ranges only.  
Contract: no invented offsets/RVAs; unknowns stay `TODO(<exact symbol/grep>)`; 23 unresolved runtime/TODO markers in `rob_run_internal/offsets.hpp` (12 class runtime-resolve, 1 root-instance locator, 3 bone static defaults pending live probe, 7 feature-symbol TODOs) block runtime use.

## Status Snapshot

| Area | State | Est. | Main gap |
|---|---|---:|---|
| Dump acquisition/slicing | done | 95% | Refresh slices on patch. |
| `offsets.hpp` | partial | 78% | 65 evidence-backed constants filled; 12 class markers dump-confirmed but runtime-resolve; 1 root-instance locator + 3 bone static defaults (runtime auto-resolve wired) + 7 feature-symbol TODOs. |
| IL2CPP wrapper | partial | 70% | Typed call/cache, method-pointer layout verify, runtime plumbing. |
| DXGI hook + overlay | partial | 65% | Live swapchain/HWND acquisition, resize/reset rebuild, clean unload. |
| Entity walk | scaffold | 35% | Real manager/singleton chain, Dictionary/List iteration. |
| W2S | partial | 45% | Camera VP matrix source/layout, viewport from swapchain. |
| ESP | scaffold | 40% | Real entity feed, box/name/dist/health/bones wired. |
| Visibility/raycast | partial | 40% | `rayVisible` + 100ms TTL cache wired; camera->bone world pos, layerMask/QTI runtime-confirm TODO. |
| Aimbot core | partial | 50% | FOV select/smoothing/cooldown/delay/jitter/miss wired; VP layout/ABI, bone world pos, real FOV/tan runtime-confirm. |
| Silent aim | stub | 20% | Dump-evidenced ShotArgsSystem direction candidates; detour/hook infra and fire-moment write unresolved. |
| Projectile prediction | partial | 40% | `predictPosition()` wired; real bullet speed/target velocity sources TODO. |
| Triggerbot/no-recoil/no-spread | partial | 40% | No-recoil/no-spread SEH model-field writes wired; triggerbot gate wired, fire action/crosshair target unresolved. |
| Menu/config | done | 85% | JSON save/load hardened, DLL/APPDATA path, input capture, status/keybinds. Runtime config dir edge cases only. |
| Loader/manual map | scaffold | 30% | Design documented; implementation intentionally not shipped. No BYOVD/exploit chain. |
| Evasion/humanization notes | done | 80% | Exposure notes, ntdll unhook stub, humanization docs, intentional-miss wired; no bypass code shipped. |
| Build/test/docs | partial | 80% | CMake/ImGui pinned, test procedure, patch-break/fix, file-purpose table, TODO audit done; MSVC x64 Release build not executed in this environment. |

**Overall: ~45% scaffolded, ~35% production-complete.**

## Critical Blockers

- [ ] 23 unresolved runtime/TODO markers in `rob_run_internal/offsets.hpp` (12 class runtime-resolve, 1 root-instance locator, 3 bone static defaults pending live probe, 7 feature-symbol TODOs). Runtime use blocked.
- [~] Local-player/entity chain dump-evidenced (`BattleEntitiesModel.Player` + `_models` dict), but BattleModel root instance locator unresolved and dict entry stride/health/position/team/bone chains need live confirmation.
- [ ] No confirmed camera view-projection matrix source/layout.
- [ ] No working manual-map loader path; `driver/` is scaffold only. No vulnerable-driver exploit chain shipped.
- [ ] Game-specific fire/recoil/spread/raycast symbols not wired.

## Unresolved Offset Markers (`rob_run_internal/offsets.hpp`)

### Class markers (12 in `offsets.hpp`, dump-confirmed; runtime-resolve wired via `rb::il2cpp::findClass`)
- [x] `PlayerEntityModel` — dump.cs:363333 TypeDefIndex:8711. Runtime resolve via `findClass`.
- [x] `PlayerClientModel` — dump.cs:362936 TypeDefIndex:8710. Runtime resolve via `findClass`.
- [x] `CharacterClientModel` — dump.cs:375947 TypeDefIndex:9059. Runtime resolve via `findClass`.
- [x] `BattleEntitiesModel` — dump.cs:238456 TypeDefIndex:5517. Runtime resolve via `findClass`.
- [x] `BattleModel` — dump.cs:239134 TypeDefIndex:5527. Runtime resolve via `findClass`.
- [x] `Camera` (UnityEngine) — dump.cs:1085427 TypeDefIndex:17097. Runtime resolve via `findClass`.
- [x] `SkinnedMeshRenderer` (UnityEngine) — dump.cs:1096172 TypeDefIndex:17241. Runtime resolve via `findClass`.
- [x] `Transform` (UnityEngine) — dump.cs:1111381 TypeDefIndex:17458. Runtime resolve via `findClass`.
- [x] `Animator` (UnityEngine) — dump.cs:1919088 TypeDefIndex:33872. Runtime resolve via `findClass`.
- [x] `Physics` (UnityEngine) — dump.cs:2010040 TypeDefIndex:36047. Runtime resolve via `findClass`.
- [x] `ShotIntentionUpdater` (Game.Battle.Inputs) — dump.cs:311597 TypeDefIndex:7275. Runtime resolve via `findClass`.
- [x] `ShotArgsSystem` (Models.Systems) — dump.cs:16452 TypeDefIndex:490. Runtime resolve via `findClass`.

### Name-miss / unresolved owners
- [x] `EntityContainer` / entity list owner — resolved as `BattleEntitiesModel` (`dump.cs:238456`): `_models` Dictionary<int,IEntityModel> `0x50`, `EntityModels` ReadOnlyDictionaryNonAlloc `0xE0`, `Player` `0xD0`, Create* entity methods. Use `BattleEntitiesModel` as entity container.
- [~] `PlayerManager` / local-player manager singleton — no `PlayerManager` class in dump.cs (0 hits). Local player path confirmed: `BattleEntitiesModel.Player` (`kBattleEntitiesModel_Player=0xD0`, `kRVA_BattleEntitiesModel_get_Player=0x916190`). Root owner of `BattleEntitiesModel`/`BattleModel` still runtime-located; `BattleModel.<BattleEntitiesModel>k__BackingField` at `dump.cs:239166` offset `0x90`, no static Instance found.
- [x] Fire/shot candidates (dump-evidenced) — `ShotIntentionUpdater` (`dump.cs:311597` TDI:7275) input intent: `CanShoot` 0xA99460 (`311625`), `StartFire` 0xA996A0 (`311628`), `ContinueFire` 0xA995F0 (`311631`), `StopFire` 0xA996D0 (`311634`). `ShotArgsSystem` (`dump.cs:16452` TDI:490) shot-direction/spread: `Tick` 0x4361970 (`16472`), `GetShotDirection` 0x4361530 (`16479`), `ApplySpreadToShotDirection` 0x4361000 (`16482`). Hook infra unresolved; validate runtime suitability before use.

### Static/singleton field markers (4) — resolved/documented
- [x] `Camera.main/current` — no static backing field RVA in dump.cs; use existing getter RVAs `kRVA_Camera_get_main=0x623C5F0` (`dump.cs:1086030`) / `kRVA_Camera_get_current=0x623BA90` (`dump.cs:1086033`).
- [x] `Camera.onPreCull/onPreRender/onPostRender` — static field offsets in Camera static storage: `0x0/0x8/0x10` (`dump.cs:1085435-1085437`). Access via `Il2CppClass->static_fields + offset`; not module-relative RVAs.
- [ ] `BattleModel`/root instance locator — no static singleton in `dump.cs:239134`. `BattleModel.<BattleEntitiesModel>k__BackingField` at `0x90` (`dump.cs:239166`); runtime root locator still required. `kStatic_BattleModel_instance` remains TODO.
- [x] Entity list/player list static field — covered by `BattleEntitiesModel` instance path: `_models 0x50`, `EntityModels 0xE0`, `Player 0xD0` once root instance located.

### Bone indices (3) — runtime auto-resolve implemented, static defaults pending live probe
- [x] `kBone_Head` runtime auto-resolve — `rb::boneprobe::resolveBones()` in `rob_run_internal/bone_probe.hpp` matches Head candidates (`Head/head/Bip01 Head/mixamorig:Head`) from live SkinnedMeshRenderer bone array; result cached in `runtimeBones().head`. Static `kBone_Head=-1` until live probe confirms.
- [x] `kBone_Spine` runtime auto-resolve — matches `Spine/spine_01/Bip01 Spine/mixamorig:Spine`, excludes Spine1/Chest/UpperChest; cached in `runtimeBones().spine`.
- [x] `kBone_Chest` runtime auto-resolve — matches `UpperChest/Chest/Spine2/Spine1`; cached in `runtimeBones().chest`.
- [x] Evidence rule documented: `HumanBodyBones` enum at `dump.cs:1920677` is humanoid table, not SkinnedMeshRenderer bone-array indices; do not map directly.

## Dump / Offset Workflow

- [x] `deps/dump.cs` is only source for BUILD_ID `26807668` (82,001,124 bytes, 2,070,513 lines).
- [x] Slices present in `deps/slices/*` (11 files). Regen script: `deps/regen_slices.ps1` (grep + line ranges, no full dump load).
- [x] Constant rule enforced: field offset `// 0x..` or method RVA `// RVA: 0x..`, never `VA`; trailing `dump.cs:LINE` on every constant.
- [x] `List<T>` workflow hardened: field names verified at `dump.cs:804724` (`_items,_size,_version`; generic def `0x0` offsets). `rob_run_internal/entity.hpp` now has named constants (`kList_Items/Size/Version`, `kArray_MaxLength/Vector`) + `verifyListLayout()` sanity check; runtime layout still must be confirmed on live build before trusting entity walk.
- [x] `Dictionary<TKey,TValue>` guarded: field names verified at `dump.cs:761540` (`_buckets,_entries,_count,_freeList,_freeCount,_version,_comparer`; generic def `0x0` offsets). `entity.hpp` adds `DictionaryLayout` + `readDictionaryValues()` that refuses unverified walk; no .NET Core assumptions. Runtime field offsets still TODO.
- [x] Bone probe wired: `rob_run_internal/bone_probe.hpp` has `resolveBones()` + `il2cppStringToAscii()` + `runtimeBones()` cache; `main.cpp` menu has "Dump Bones" button. Live game run + SMR/entity chain still required to confirm static indices.
- [x] `validate()`/`validateFilled()` gate evidence-backed constants; unresolved markers remain explicit. Re-run after offset changes.

## Implementation Tasks

### 1. `rob_run_internal/offsets.hpp`
- [x] Added evidence-backed model fields: `RecoilModel.RecoilOffset` (dump.cs:332458), `PlayerFakeSpreadModel.Spread/Air/Move` (dump.cs:332248-332250), `AimAssistModel.IsActive/HasTarget/Target` (dump.cs:334908,334912,334914), `AutoFireModel.IsActive/IsFiring/HasTarget/Target` (dump.cs:335052,335054,335056,335058). All with `dump.cs:LINE` comments and gated in `validateFilled()`.
- [x] Added grep-hint TODO slots for unresolved feature symbols: `kRVA_Weapon_Fire`, `kRVA_Weapon_TryShoot`, `kRVA_Triggerbot_TargetCheck`, `kField_Weapon_Recoil`, `kField_Weapon_Spread`, `kField_Weapon_BulletSpeed`, `kField_Projectile_Velocity` (exact grep terms in comments).
- [x] Entity list owner evidence present: `BattleEntitiesModel._models` at `0x50` (`kBattleEntitiesModel_ModelsDict`, dump.cs:238467); `EntityModels` read-only at `0xE0` (dump.cs:238491).
- [x] Method RVAs are module-relative `// RVA: 0x..` values from dump.cs; no VA used.
- [ ] 23 unresolved runtime/TODO markers remain in `offsets.hpp` (12 class runtime-resolve, 1 root-instance locator `kStatic_BattleModel_instance`, 3 bone static defaults pending live probe, 7 feature-symbol TODOs). No invented values; `ALL_FILLED` remains evidence-backed-only gate.

### 2. `rob_run_internal/il2cpp_wrapper.hpp`
- [~] `Il2CppMethod` method-pointer offset: assumed `+0x0`, guarded by `methodPointerFromIl2CppMethod()` with GameAssembly-range sanity check and SEH; still `TODO(runtime: "Il2CppMethod methodPointer offset")` for Unity 6000.x IL2CPP confirmation.
- [x] Typed invocation helpers: `invokeSafe<Ret,...>()`, `invokeVoidSafe<...>()` SEH-guarded.
- [x] Method cache: `resolveMethodCachedByClass()` with `kMaxCachedMethods=128`; class cache `kMaxCachedClasses=64`.
- [x] Class resolution: `findClass(ns,name)` tries common assemblies then domain assembly/image iteration by name+namespace; `resolveMethodCached<Fn>()` falls back to RVA.
- [x] Safe wrappers: `cameraGetMain/current` (RVA 0x623C5F0/0x623BA90), `cameraWorldToCameraMatrix/projectionMatrix` (RVA 0x623D870/0x623CF60), `physicsRaycast` (RVA 0x63B1140, 6 args), `getStaticFieldValue()`, `readValue/writeValue/readPtr`. Matrix4x4/large struct return ABI needs runtime confirmation.

### 3. `rob_run_internal/dxgi_hook.hpp`
- [x] `FindSwapchain()` does dummy D3D11 device+swapchain probe, installs Present vtable[8] hook, releases dummy; real swapchain captured in `PresentStub`.
- [x] WndProc hooked from swapchain `OutputWindow` in `InitImGui()` via `InstallWndProcHook()`; restored in `RestoreWndProcHook()`/`CleanupImGui()`/`UnhookPresent()`.
- [x] `WM_SIZE` sets resize pending; `HandleResize()` releases RTV, invalidates ImGui DX11 objects, calls `ResizeBuffers`, recreates RTV/device objects.
- [x] Device/context loss guarded: init failure cleans partial state; `DXGI_ERROR_DEVICE_REMOVED/RESET` triggers `CleanupImGui()`; next Present re-inits. Swapchain change triggers rebuild.
- [x] Unhook ordering: disable render, restore exact captured vtable slot, clear hook state, `CleanupImGui()` (restore WndProc, shutdown ImGui, release RTV/ctx/device).

### 4. `rob_run_internal/entity.hpp`
- [x] Local-player chain wired from dump evidence: `BattleEntitiesModel.Player` (`kBattleEntitiesModel_Player=0xD0`, `kRVA_BattleEntitiesModel_get_Player=0x916190`); `getBattleEntitiesModel()` reads `BattleModel.<BattleEntitiesModel>k__BackingField` `+0x90` (dump.cs:239166). Root BattleModel instance locator still runtime TODO.
- [x] Entity iteration: `collectEntities()` reads local player + walks `_models` Dictionary<int,IEntityModel> (`kBattleEntitiesModel_ModelsDict=0x50`, dump.cs:238467). Dictionary field offsets resolved at runtime via `il2cpp::classGetFieldFromName/fieldGetOffset`; entry stride probed conservatively before walk; unverified layout refuses walk.
- [~] Extraction: id/clientModel/serverModel filled with validity checks. Position/transform, health, team/faction, alive/dead, bone transforms remain TODO (chains/symbols unresolved; `BattleCharacterPrivateModel.Health` `0x1B0` dump.cs:195155 noted, entity chain unresolved).
- [x] Per-entity validity/bounds checks on every deref (`validPtr`, `readMem` SEH, bounds caps).
- [x] Cache/lifetime: `EntityCache` (512 entries, `GetTickCount64` tick), `getCachedEntities(maxAgeMs)`, stale/freed skipped by pointer validity checks.

### 5. `rob_run_internal/esp.hpp` / `main.cpp`
- [~] ESP pipeline wired: `updateEntities()` collects `rb::ent::EntityView` via `collectEntities()`, fetches camera matrices via `il2cpp_wrapper` safe wrappers, builds `EspEntry` list. Real world position/bone transform source still TODO; entries remain `valid=false` until W2S data resolved.
- [~] 2D box: draw/clamp path implemented in `renderAll`; box computation from bone/world bounds blocked on entity position/bone transform chain.
- [x] Draw name, distance, health bar, skeleton primitives implemented; skeleton pair connections TODO pending bone hierarchy.
- [x] Visibility color: white visible, red not visible.
- [x] Config toggles/max distance/boxVisibleOnly respected in `renderAll`; feat flags wired from menu.

### 6. `rob_run_internal/aimbot.hpp`
- [~] Camera matrix read wired via `il2cpp_wrapper` (`cameraGetMain/current`, `cameraWorldToCameraMatrix/projectionMatrix`, RVAs dump.cs:1086030/1086033/1085912/1085918); Matrix4x4 return ABI/layout and VP multiply order still runtime-confirm TODO.
- [x] `selectBestBone()` implements nearest-bone-to-crosshair within FOV (rough deg->px; TODO runtime use real camera FOV/tan). W2S clamps clip_w<=0, NDC->pixels, Y flip, viewport reject.
- [x] `smoothToward()` exponential smoothing, `canSwitchTarget()` cooldown, `markTarget()` randomized reaction delay (`aimDelayMin/Max`), `aimState()` tracking.
- [x] `applyJitter()` micro-jitter and `shouldMissIntentional()` configurable miss.
- [~] Hitbox preference: `selectPreferredBone()` implements head>chest>spine (configurable via `kFeatBonesPriorityHead`/`targetPriority`), falls back to nearest FOV bone. Per-weapon preference map still TODO(runtime: weapon type/id source unresolved).
- [~] Silent-aim fire hook: dump-evidenced candidates added — `ShotArgsSystem.GetShotDirection` RVA 0x4361530 (dump.cs:16479), `ApplySpreadToShotDirection` 0x4361000 (dump.cs:16482), `Tick` 0x4361970 (dump.cs:16472), plus `ShotIntentionUpdater.StartFire` 0xA996A0 (input intent). `silentAimWrite()` still stub; detour/hook infra not shipped.
- [x] `predictPosition()` present; real bullet speed/target velocity sources unresolved TODO; gravity/drag TODO only if confirmed.

### 7. Visibility
- [x] `rb::aim::rayVisible()` calls `rb::il2cpp::physicsRaycast()` (dump.cs:2010093 RVA 0x63B1140, 6-arg signature). Conservative: any hit before target => not visible.
- [~] `rb::aim::visibilityCached()` does per-entity 100ms TTL cache keyed by entity pointer; ray from camera to selected bone wired as function, blocked on entity bone world positions.
- [x] Cache visibility for 100ms per entity (`kMaxVisCache=512`, `GetTickCount64` TTL).
- [ ] Layer mask / QueryTriggerInteraction values TODO(runtime: confirm player/character collider mask; QTI defaulted to 1 Collide).

### 8. Triggerbot / No-Recoil / No-Spread
- [~] Dump symbols/RVAs located: recoil `PlayerClientModel.RecoilModel` 0x2B8 (`dump.cs:363040`), `RecoilModel.RecoilOffset` 0x10 (`332458`); spread `PlayerClientModel.FakeSpreadModel` 0x2C0 (`363042`), `PlayerFakeSpreadModel.Spread/Air/Move` 0x10/0x14/0x18 (`332248-332250`); fire candidates `ShotIntentionUpdater` Start/Continue/Stop/CanShoot RVAs and `ShotArgsSystem` Tick/GetShotDirection/ApplySpreadToShotDirection RVAs in offsets.hpp; `AutoFireModel` fields 0x10-0x18 (`335052-335058`). `kRVA_Weapon_Fire/TryShoot`, `kRVA_Triggerbot_TargetCheck`, `kField_Weapon_Recoil/Spread` remain TODO.
- [~] Triggerbot: `rb::combat::triggerbotShouldFire()` implements delay/FOV/visibility/intentional-miss gate + 60ms min gap; crosshair target source and fire action (input injection/ShotIntentionUpdater path) unresolved TODO(runtime). No hook/input infra shipped.
- [~] No-recoil/no-spread: `rb::combat::applyNoRecoil/applyNoSpread` SEH-guarded writes to dump-evidenced model fields, gated by `kFeatNoRecoil/kFeatNoSpread` in `tickCombat()`. Hook-based shot-arg patching not shipped.
- [~] Safety toggles present per-feature in `features.hpp`; config save exists, load parse still stub (task 9).

### 9. Menu / Config
- [x] Hardened JSON save/load in `features.hpp`: proper JSON object, minimal key parser, missing keys keep defaults, range clamp on load, bad/oversized file rejected, load returns bool.
- [x] Config path resolved next to DLL via `GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)`, fallback `%APPDATA%\RobRunInternal\RobRunInternalConfig.json`; parent dir created on save.
- [x] Menu shows Insert toggle status, per-feature config, keybind display, config path, save/load/reset/dump bones.
- [x] Menu consumes input when open: `dxgi_hook.hpp` `HkWndProc` forwards to ImGui and blocks mouse/keyboard when `WantCaptureMouse/Keyboard`.

### 10. Loader / Manual Map (`driver/`)
- [x] Manual-map design documented in `driver/vuln_driver.hpp`, `driver/vuln_driver.cpp`, `driver/mapper.cpp`: PE32+ validate, W^X remote alloc, sections, relocations DIR64, imports/OFT/forwarders, TLS, .pdata/.xdata `RtlAddFunctionTable`, CRT/security cookie init, remote entry stub, no `LoadLibrary` in target.
- [x] PEB-unlinked limitations documented: VAD/memory scan/ETW/hook/thread visibility, TLS/CRT/exception differences, x64 `.pdata` requirement.
- [ ] Manual-map DLL into game process — **not implemented/shipped** (scaffold refusal only).
- [ ] Re-map after game restart without reboot — design noted, **not implemented/shipped**.
- [x] **Scaffold only. No vulnerable-driver exploit chain shipped.** `bootVulnerableDriver`/`loadTargetDll` hard-refuse with `ERROR_NOT_SUPPORTED`; no service install, no `ZwLoadDriver`, no kernel primitives, no remote alloc/write/thread, no driver payloads/IOCTL abuse.
- [x] iqvw64e-style BYOVD exposure documented: kernel primitive risk, BSOD/HVCI/WDAC/Secure Boot/AV-EDR/EAC block-ban risk, legal/policy/supply-chain risk. No exploit payloads shipped.

### 11. Evasion / Humanization Notes
- [x] EAC exposure commented in `rob_run_internal/evasion.hpp` and README: module signature/PE header in memory, .text/vtable hooks, remote thread start, broad process handles, overlay/input artifacts, behavioral timing signals.
- [x] `rb::evasion::ntdllUnhookStub()` added with clear user-mode scope/risk notes; no .text restore, no kernel/remote patching, no bypass payload.
- [x] Per-build packing and section-name randomization noted as hygiene/cosmetic only; no bypass claim.
- [x] Aim humanization documented in `rob_run_internal/aimbot.hpp` and README: exponential smoothing, jitter amplitude, randomized reaction delay range, head-priority/fallback, intentional miss.
- [x] Intentional-miss option configurable: `Cfg::missIntentionalPct` in `features.hpp`, menu slider, `aim::shouldMissIntentional()` used in aim/combat gates.

### 12. Build / Test / Docs
- [~] CMake/config audited and updated: root/`rob_run_internal`/`driver` CMakeLists wired, ImGui 1.90.9 DX11+Win32 backends compiled, stale/dangerous comments removed, include/lib paths fixed. Full MSVC x64 Release build not executed in this environment.
- [x] ImGui DX11+Win32 backends vendored/pinned: `deps/imgui` v1.90.9, `backends/imgui_impl_dx11.*`, `backends/imgui_impl_win32.*`, wired in `rob_run_internal/CMakeLists.txt`.
- [x] Exact test procedure written in `rob_run_internal/README.md`: preflight BUILD_ID, build, launch, authorized load, overlay-first, ESP stage, aim/combat stage, unload checks.
- [x] "What breaks after patch and how to fix it" added to `rob_run_internal/README.md`.
- [x] File-by-file purpose table added to `rob_run_internal/README.md`.
- [x] TODO audit: grep across `rob_run_internal` source (excluding imgui deps) shows runtime/TODO markers only; no invented offset/RVA values found. Unresolved constants remain `0x0`/`-1` or explicit TODO markers.

## Deliverables Checklist

- [x] Full source tree with file-by-file purpose (`rob_run_internal/README.md`).
- [x] CMakeLists.txt for DLL and separate driver CMake.
- [x] Exact test procedure (`rob_run_internal/README.md`).
- [x] Patch-break/fix notes (`rob_run_internal/README.md`).
- [~] Pointer derefs validity-checked on main paths (SEH `readMem`/`writeMem`, `validPtr`, IL2CPP call/read guards); continue audit on all new/runtime paths before live use.
- [x] No third-party cheat SDKs/paid libs (Win32/DXGI/ImGui vendored/own IL2CPP wrapper only).
- [x] No invented offsets; unresolved symbols marked `TODO(...)` / `0x0` / `-1` with evidence/runtime markers.
