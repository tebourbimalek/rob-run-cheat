# RobRun — robrun

C++20, x64, Release, Windows. External RPM.

## Build

```
cmake -A x64 -S . -B build
cmake --build build --config Release
```

## Offset refresh — chain links

```
build\Release\robrun.exe --dump
```

ROB RUN up. One verdict line per chain link — GWorld, OwningGameInstance,
LocalPlayers, PlayerController, AcknowledgedPawn, PlayerCameraManager,
CameraCache, PersistentLevel, Actors. Paste the printed addresses into
offsets.h, drop the `// UNVERIFIED` tags, re-run until every line is PASS
and the POV + 5 actors print live values.

## Sens / radius / distance calibration

```
build\Release\robrun.exe
```

ROB RUN up. Hold VK_XBUTTON1 while watching `targets / targets-in-fov` —
adjust `Cfg.fovPx`, `Cfg.sens`, `Cfg.smooth`, `Cfg.headOffset`,
`Cfg.maxDist` — the tunables in main.cpp.
