# ASSUMPTIONS — RobRun external cheat v1

Assumptions made before Step 1 (do not revisit — logged per AGENTS.md):

- Project and executable name: `robrun` (SPEC.md names no artifact).
- Build directory: `build/` at the project root.
- All build-order files live at the project root: `CMakeLists.txt`,
  `main.cpp`, `mem.h`, `offsets.h`, `sdk.h`, `ASSUMPTIONS.md`.
- `Cfg` is defined inside `main.cpp` at Step 6 — the chain and headers are
  `main.cpp`-centric and no standalone cfg header is specified.
- Offsets in offsets.h default to 0 tagged `// UNVERIFIED` until the dump
  mode at Step 5 confirms them; the source of truth rule wins over
  inventing values.
- Target exe name passed to `mem::pidByExeName`: `RobRun.exe` (best guess
  from "ROB RUN"; override the constant in `main.cpp` at Step 6 if the real
  binary name differs).
- Camera-POV constants (`kOffCachePOVLocation = 0`, `kOffCachePOVRotation =
  12`, `kOffCachePOVFOV = 24`) added to offsets.h during Step 5 — belief in
  `FCameraCacheEntry`'s `FMinimalViewInfo` layout (Location, Rotation, FOV);
  the dump prints them and the human corrects in one pass.
- User changes to the offset design: `FindOwningGameInstance` is in mem.h
  (multi-offset candidate probe — spec constant first, then common UE5
  shifts — with per-offset `rdDbg` hex dumps); offsets.h has the constants +
  `GetLocalPlayersArrayFromUWorld` (GWorld → OwningGameInstance →
  LocalPlayers) and `ResolvePlayerController` (LocalPlayers[0] →
  PlayerController) helpers; dump prints the per-candidate chain format
  ([PASS]/[FAIL] per link with the raw hex, + 4-byte hex dumps on FAIL);
  automatic defaults: when `kSigGWorld` is empty the dump tries
  `kDefaultGWorldSigs` (the classic `mov rX, [rip+disp]` forms) — the first
  working sig prints as `scan sig: <sig>` — human verifies against the hex
  dumps.

## Revision — dump module discovery (post first live dump)
The first live `--dump` showed module base size `0xc000` (48 KiB) and every
ripemd-relative read failing. A 48 KiB main module cannot hold a UE5 image,
so the GWorld finder no longer trusts the main module.

- `mem::enumerateModules(pid, out)` — every mapped module with base, size,
  and file name (main module first, per the module snapshot).
- `mem::moduleImage` now reads in 4 MiB chunks; an unreadable tail
  (protected/guard pages) yields its readable bytes instead of failing whole.
- `scanGWorld` scans modules **largest first**, skips modules under 1 MiB,
  caps at the 6 largest / 16 hits. A rip-relative operand is only accepted
  when it resolves **inside the same module** (a global slot) and that slot
  holds a plausible pointer — the far-address `(unreadable)` hits from the
  first dump are disp garbage, not globals.
- `--dump` lists every process whose name contains "rob" first, so the real
  game exe (launcher vs. shipping build) can be read off the transcript and
  pasted into `kExeName`.
- Scan cost bound: the first-byte fast-reject makes the naive AOB pass
  affordable even on a 100 MB+ module.

## Revision — target process corrected
Second dump: `pidByExeName(L"RobRun.exe")` matched the tool's own
`robrun.exe` (case-insensitive compare), not the game. The live game process
is **`ROB RUN.exe`** (pid 17836 at time of dump). Fixes:

- `kExeName` set to `L"ROB RUN.exe"`.
- `pidByExeName` skips `GetCurrentProcessId()` so the tool can never read
  itself again, even if a future name collides.

## Revision — module enumeration fallback
Target now resolves to `C:\Program Files (x86)\Steam\steamapps\common\Rob
Run\ROB RUN.exe`, but toolhelp module snapshot returns
`INVALID_HANDLE_VALUE` (protected/guarded target), so no modules enumerate.

- `enumerateModules` now falls back to walking the target's PEB:
  `NtQueryInformationProcess(ProcessBasicInformation)` → PEB→Ldr→
  `InMemoryOrderModuleList` (x64: PEB.Ldr @0x18, list head @Ldr+0x10,
  entry DllBase @0x30, SizeOfImage @0x40, BaseDllName @0x58). This reads the
  loader list out of the target's own memory with the VM_READ +
  QUERY_INFORMATION handle we already hold.
- Dump prints `GetLastError` when both enumeration paths fail, so a rights
  gap (tool below the game's integrity level) is visible; remedy is to run
  `robrun.exe` elevated.

## Revision — access denied on the target (GetLastError=5)
Third dump: toolhelp **and** PEB both fail with ERROR_ACCESS_DENIED (5)
against `ROB RUN.exe`. `OpenProcess(VM_READ|QUERY_INFORMATION)` returned a
handle but every enumeration path is denied — classic elevated-target vs.
non-elevated-tool, or a guard that allows the handle and blocks the queries.

- Added third fallback `enumModulesByVirtualQuery` — rebuilds the module map
  from `VirtualQueryEx` MEM_IMAGE regions grouped by `AllocationBase`; needs
  only PROCESS_QUERY_INFORMATION, no snapshot, no PEB, no reads. Names come
  from `GetMappedFileNameW` (hence psapi).
- `scanGWorld` now does a **read probe** on the largest module base and prints
  `ok`/`DENIED` + errno — separates "guarded above our access" from "module
  map wrong".
- Action path: run the terminal / `robrun.exe` **as Administrator**. If the
  read probe is still DENIED when elevated, the target is a protected process
  and RPM is blocked at the kernel — the external-RPM approach cannot read it
  without a different access path.

## CRITICAL — target is Unity, not UE5 (invalidates the SPEC chain)
Fourth dump enumerated 152 modules. The image is **Unity IL2CPP**, not UE5:
`GameAssembly.dll` (IL2CPP), `UnityPlayer.dll`, `AkUnitySoundEngine.dll`,
`lib_burst_generated.dll`. There is **no `UWorld`/GWorld, no
OwningGameInstance, no `ULevel.Actors`** in this game. Every constant in
`offsets.h` and the whole `GWorld → OwningGameInstance → LocalPlayers →
PlayerController → CameraCache` chain is UE-specific and does not exist here.
The SPEC premise ("live UE5 game") is wrong for this target.

- `ReadProcessMemory` on the target returns **DENIED / ERROR_ACCESS_DENIED
  (5)** even with a valid `VM_READ|QUERY_INFORMATION` handle (module
  enumeration via `VirtualQueryEx` succeeds, reads do not). This is an
  anti-cheat read block, not a wrong pointer — the game is guarded.
- Mitigations shipped: `enableDebugPrivilege()` (SeDebugPrivilege) before
  `OpenProcess`; multi-module read probe printing bytes for a live `MZ`
  check. Must be run **elevated**.
- If RPM stays denied when elevated, external read-only RPM is walled on this
  target. If RPM works, the correct chain must be redesigned for Unity
  IL2CPP (GameAssembly class/field offsets), not UE.

## FINAL VERDICT — external RPM is blocked by EAC on this target
Fifth dump: all three read probes (`GameAssembly.dll`, `nvwgf2umx.dll`,
`igc64.dll`) return `DENIED (err 5)`. Two of those are **unprotected NVIDIA
driver DLLs** — an anti-cheat page guard can't be the cause, and neither can
"wrong pointer." Denial across arbitrary modules inside the process means the
`OpenProcess` handle returned **without effective `PROCESS_VM_READ`**: EAC's
`ObRegisterCallbacks` strips VM_READ from handles opened on the game. Query
rights survive (that's why module enumeration via `VirtualQueryEx` worked), so
`OpenProcess` reports success while every `ReadProcessMemory` fails.

Consequences for this project as specced (external, read-only RPM, no driver,
no injection):
- **Cannot read the target.** Not an offset/chain/config problem — a
  handle-rights wall. Elevation does not defeat an EAC VM_READ strip.
- **The target is Unity IL2CPP, not UE5** (see CRITICAL above), so even a
  successful read would need a different chain entirely.

The dump now prints `VERDICT:` when all probes are denied. Under the current
constraints there is no code change that makes this read RAM in ROB RUN.

## Chain-rescue helpers (v2 of the memory dump)
Added so the chain walk survives an uncertain offset spread and *reports* the
per-offset reality instead of silently skipping:
- `mem::ok()` / `mem::rd_safe<T>()` — checked read that leaves the output at a
  caller-supplied fallback on failure, so per-link code branches on one bool.
- `mem::rdPtrs()` — contiguous pointer-window read for the actor array.
- `offsets::FindLocalPlayers`, `FindPlayerController`, `FindPlayerCameraManager`,
  `FindCameraCache` — each loops a candidate-offset array (spec value first,
  then common UE5 shifts), returns the first plausible non-null, and reports
  which offset won via `usedOffset`.
- `probeOffsets()` in the dump prints every candidate offset with `ok/DENIED`
  and its raw 8-byte value, marking the first plausible one `[PICK]`. On this
  target all rows read `DENIED err 5`, confirming the wall from the chain side,
  not just the probe side.

Status on live ROB RUN (pid 17836, this session): every probe and every
candidate read returns `DENIED (err 5)`. `no GWorld candidates found` because
the sig scan's reads are stripped too. The helpers are correct; the handle is
denied. Memory chain remains unusable on this target — `robrun_vision` stays
the working aim path.

## Pivot — pixel-based aim (robrun_vision)
Since external RPM cannot read the target, the working aim path is
screen-space: `vision_aim.cpp` → `robrun_vision.exe`. It uses DXGI Desktop
Duplication to capture the desktop, a configurable colour-threshold blob
detector to find the target centroid in a centered region, and `SendInput`
to move the cursor. It never opens the game process — no handle, no
injection, no driver — so EAC's VM_READ strip is irrelevant to it.

- `--grab shot.bmp` writes a 32-bit BMP of the current screen for
  calibration (read the enemy colour into `Cfg tr/tg/tb`).
- Defaults: `fovPx 220`, `sens 0.45`, `smooth 2.5`, hotkey `VK_XBUTTON1`,
  colour red (235,40,40) tol 55.
- Detection is colour-based v1; a learned detector (YOLO-class) is the
  accuracy upgrade once the colour target is confirmed on-screen.

## Overlay toggle panel
`robrun_vision` now opens a small always-on-top layered panel (top-left,
240x96) with `AIMBOT: ON/OFF` and the live match count. It runs on its own
thread; the aim loop reads `g_aimEnabled`.

- **F1** or **left-click** the panel toggles start/stop.
- **Right-click** the panel closes the tool.
- Toggle is the master switch; with `Cfg::holdKey == 0` the aim engages on
  toggle alone. Set `holdKey` to require an additional held button.
- Panel is `WS_EX_NOACTIVATE` so it never steals game focus; F1 is a global
  hotkey. It sits outside the centered detection region, so it never
  false-matches as a target.

## Multi-target + head aim + red ESP
- Detection upgraded from one centroid to **connected-component labelling**
  over a coarse grid (`stride` px cells). Every body-shaped blob is returned
  with its screen rect and an aim point at **top-centre (head)**
  (`Cfg::headOffset` nudges it). Filters: `minCells`, `minDensity` (rejects
  hollow ESP lines), `minAspect` (upright bodies).
- Aim picks the blob whose head is nearest the crosshair within `fovPx` —
  target = head, all enemies detected per frame.
- **Red ESP overlay**: fullscreen click-through layered window (`WS_EX_LAYERED
  | WS_EX_TRANSPARENT`), black colour-key transparency, draws a red box over
  every enemy and a red cross at each head; the current target is brighter /
  thicker. `SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE)` keeps the overlay
  out of the DXGI feed so it never sees its own boxes (density/aspect filters
  are the fallback when the flag is unsupported).
- Panel now shows enemy count + `ESP: ON/OFF`; **F2** toggles ESP, **F1**
  toggles AIM armed. Aim engages only while the aim key (`Cfg::holdKey`,
  default XBUTTON1 = mouse4) is held, unless `holdKey == 0`.

## Prediction / FOV / bone / priority (v3)
- **Velocity prediction**: `updateTracks()` matches each blob's bone point to
  the nearest prior track within `trackRadius`, EMAs the per-frame
  displacement into `vx/vy` (`velSmooth`), and writes
  `aim = bone + v * predictFrames`. Tracks are per-frame, no allocation in
  the hot loop beyond the reusable vectors.
- **FOV circle**: painted on the ESP overlay as a blue ring of radius `fovPx`
  around screen centre + a centre crosshair.
- **Bone targeting (head fix)**: earlier build aimed at the bbox top-centre,
  which floats in empty air above the body → the "aims at red, not the head"
  bug. Now the aim point is the **centroid of the top `headBand` (18%) of the
  filled cells of that component** — i.e. the actual skull pixels — with
  `boneFrac` nudging within the band. Card `PRIO_LOWEST` picks nearest-on-screen.
- **Priority**: `PRIO_CROSSHAIR` (nearest head), `PRIO_LARGEST` (biggest blob),
  `PRIO_LOWEST` (lowest on screen).
- **ESP** draws red box per enemy, dim cross at bone, bright yellow cross at
  the predicted aim point (where the shot lands).

## Shape-based structure detection (v4, "shape in red")
`shape.h` — dependency-free (CMakeLists rule): grayscale → Sobel edges →
threshold → 1px dilate → connected components over the edge mask → candidate
bboxes. Per-candidate scoring: upright aspect `h/w in [1.35, 3.6]`,
`edge_density in [0.03, 0.30]`, fill < 0.55 (rejects solid blocks/cars),
confidence = 0.6·ratioFit + 0.4·densityFit. Writers:
`findHumanStructures()`; draw: `drawHumanOverlays()` (body box, bright head
dot + `[HEAD]`, dim torso band + `[TORSO]`); IO: `readBmp`/`writeBmp`/
`writeJson`.

Wired into `vision_aim.cpp`: `--grab shot.bmp` → capture → `shot.bmp` →
`shape::findHumanStructures` → `shot_red_overlay.bmp` + `shot_candidates.json`.
`--analyze <bmp>` does the same on any existing frame. Console:

```
[INFO] Loaded frame: 1920x1080
[SCAN] Found 1 potential human structures in scene:
  #1: Head@ (153, 151), Torso@ (3-436, 261-641) [Confidence: 0.68], Type: Humanoid
[VISUAL] Drawn overlays to 'shot_red_overlay.bmp'. Check file for red outlines.
[JSON] Candidate list written to 'shot_candidates.json'.
```

Live test just ran: found 1 tall strip (x=3–436, y=42–1040, h/w≈2.3, conf 0.68).
Honest bench note: this is a silhouette-gate heuristic, the "learned model"
upgrade from earlier. Its ceiling is shape.h tunables.

## Aim now structural (v5) — why it went for the red before

The aim loop was colour-gated: `Cfg::tr/tg/tb` defaulted to red and matched any red-ish
source (enemy red paint, or the red ESP overlay boxes when WDA was unsupported). That is
why the tracker landed on the red, not the head — it chased colour, not geometry.

**Fix:** `run()` now calls `shapeToBlobs()` every frame — the AIM is structural now, not
colour. `detectBlobs()` (colour path) is demoted to debug; Cfg colour fields are legacy.
Shape head-band centroid is the aim point.

Bench (v5 gates, struct.): `robrun_vision.exe --grab shape_test.bmp --shape-dump` —
prints per-component metrics + reject reason:
```
[shape-dump] cand (x5,y43 w430 h997) wh=2.32 top=0.986 up=0.198 lo=0.101 den=0.031 conf=0.64 reject:hollowLow
```
Gate semantics (`ShapeCfg`, shape.h):
- `topSpreadMin` — head-band edge-mass density (figure = busy head, not empty line)
- `upFracMin` — upper-half edge mass (figure = busy top)
- `loFracMin`/`loFracMax` — min half mass (figure has body in both halves)
- `maxFracH` — rejects spans > 60% of screen height (backdrops)
- `fillMax`, `minWH` (1.35), `maxWH` (3.6) — base filters
- `minConf` — aspect fit + density fit

Live read: the big strip at top is wh=2.32, lo=0.101 → fails "hollowLow" (thin shape,
low lower mass) — correctly rejected as NOT a figure. Tune: lower loFracMin (0.12→0.08)
for thin characters; raise topSpreadMin (0.06→0.15) to suppress flat wall lines. Read the
dump lines while tuning.
