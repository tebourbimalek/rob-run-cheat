#pragma once

// combat.hpp — triggerbot / no-recoil / no-spread.
// All writes are SEH-guarded and toggle-gated. No hook/input infra shipped.

#include <windows.h>
#include <cstdint>
#include "offsets.hpp"
#include "entity.hpp"
#include "features.hpp"
#include "aimbot.hpp"

namespace rb::combat {

struct TriggerState {
  ULONGLONG lastFireMs = 0;
  ULONGLONG reactionReadyMs = 0;
  bool hadTarget = false;
};

inline TriggerState& triggerState() { static TriggerState s; return s; }

inline bool applyNoRecoil(void* recoilModel) {
  if (!recoilModel) return false;
  // RecoilModel.RecoilOffset Vector2 at 0x10 (dump.cs:332458)
  const bool x = ent::writeMem<float>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(recoilModel) + off::kRecoilModel_RecoilOffset + 0x0), 0.f);
  const bool y = ent::writeMem<float>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(recoilModel) + off::kRecoilModel_RecoilOffset + 0x4), 0.f);
  return x && y;
}

inline bool applyNoSpread(void* fakeSpreadModel) {
  if (!fakeSpreadModel) return false;
  // PlayerFakeSpreadModel.Spread/Air/Move at 0x10/0x14/0x18 (dump.cs:332248-332250)
  bool ok = true;
  ok = ent::writeMem<float>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(fakeSpreadModel) + off::kPlayerFakeSpreadModel_Spread), 0.f) && ok;
  ok = ent::writeMem<float>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(fakeSpreadModel) + off::kPlayerFakeSpreadModel_Air), 0.f) && ok;
  ok = ent::writeMem<float>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(fakeSpreadModel) + off::kPlayerFakeSpreadModel_Move), 0.f) && ok;
  return ok;
}

// Triggerbot gate: delay/FOV/bone/visibility must be satisfied by caller.
// Fire action unresolved: no input injection/hook infra shipped.
// TODO(runtime): resolve crosshair target/entity-under-crosshair source.
// TODO(runtime): fire gate - ShotIntentionUpdater.StartFire 0xA996A0 / AutoFireModel fields / input injection; validate on live build.
inline bool triggerbotShouldFire(bool hasTarget, bool visible, float fovPx, float maxFovPx, ULONGLONG nowMs) {
  auto& c = cfg();
  if (!c.feat[kFeatTrigger]) return false;
  TriggerState& s = triggerState();
  if (!hasTarget || !visible) { s.hadTarget = false; return false; }
  if (fovPx > maxFovPx) return false;
  if (!s.hadTarget) {
    s.hadTarget = true;
    const float delayS = aim::randomFloat(c.aimDelayMin, c.aimDelayMax);
    s.reactionReadyMs = nowMs + static_cast<ULONGLONG>(delayS * 1000.f);
  }
  if (nowMs < s.reactionReadyMs) return false;
  if (aim::shouldMissIntentional()) return false;
  // Minimum gap between trigger attempts to avoid hammering.
  if ((nowMs - s.lastFireMs) < 60) return false;
  s.lastFireMs = nowMs;
  // TODO(runtime): execute fire action (input injection / ShotIntentionUpdater path). No fire infra shipped.
  return false;
}

inline void tickCombat(void* localPlayerClientModel) {
  if (!localPlayerClientModel) return;
  auto& c = cfg();
  if (c.feat[kFeatNoRecoil]) {
    void* recoil = ent::readPtrField(localPlayerClientModel, off::kPlayerClientModel_RecoilModel);
    (void)applyNoRecoil(recoil);
  }
  if (c.feat[kFeatNoSpread]) {
    void* spread = ent::readPtrField(localPlayerClientModel, off::kPlayerClientModel_FakeSpreadModel);
    (void)applyNoSpread(spread);
  }
  // Triggerbot requires per-target visibility/FOV from aim pipeline; called from aim/update loop when data available.
}

} // namespace rb::combat
