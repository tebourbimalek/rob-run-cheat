#pragma once

// aimbot.hpp — W2S, FOV, aim smoothing, prediction, silent-aim hook point.
// W2S: read view-projection matrix from camera, multiply, reject clip_w <= 0,
// convert NDC to pixels, clamp to viewport. Y is flipped (NDC +Y up, screen +Y down).
//
// Humanization (behavioral exposure notes):
// - Smoothing curve: exponential `smoothToward(current, desired, factor)`; lower factor = slower.
//   Config: smoothingFactor/silentSmooth in [0.001,1]. Avoid factor=1 snaps.
// - Jitter amplitude: `applyJitter(dir, amplitudeDeg)` small random tangent perturbation;
//   config jitterAmpl degrees, gated by microJitter. Keep sub-degree for subtlety.
// - Reaction delay range: randomized per-target acquire delay in [aimDelayMin, aimDelayMax] seconds;
//   applied by markTarget()/reactionReady()/triggerbotShouldFire(). Avoid zero-delay perfect reactions.
// - Headshot ratio target: prefer head via kFeatBonesPriorityHead/targetPriority, but do not force
//   100% head; use missIntentionalPct and bone fallback to vary hit distribution.
// - Intentional miss: `shouldMissIntentional()` uses missIntentionalPct [0,100]; wire at fire decision.

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

#include "features.hpp"
#include "il2cpp_wrapper.hpp"

namespace rb::aim {

struct Vec3 { float x=0, y=0, z=0; };
struct Mat4 { float m[4][4]{}; }; // row-major storage; verify layout against Camera matrix dump.

inline Vec3 sub(const Vec3& a, const Vec3& b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3 add(const Vec3& a, const Vec3& b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 mul(const Vec3& a, float s) { return {a.x*s, a.y*s, a.z*s}; }
inline float dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline float len(const Vec3& v) { return std::sqrt(dot(v,v)); }
inline Vec3 norm(const Vec3& v) { float l=len(v); return l>1e-6f ? mul(v,1.f/l) : Vec3{}; }

inline bool worldToScreen(const Mat4& vp, const Vec3& world, int vpW, int vpH, float& outX, float& outY) {
  // clip = M * [x,y,z,1]^T. Matrix layout is assumed row-major; verify with live camera matrix.
  const auto& c = vp.m;
  const float clipX = world.x*c[0][0] + world.y*c[1][0] + world.z*c[2][0] + c[3][0];
  const float clipY = world.x*c[0][1] + world.y*c[1][1] + world.z*c[2][1] + c[3][1];
  const float clipW = world.x*c[0][3] + world.y*c[1][3] + world.z*c[2][3] + c[3][3];
  if (clipW <= 1e-6f) return false; // behind camera / invalid

  const float ndcX = clipX / clipW;
  const float ndcY = clipY / clipW;
  outX = (ndcX * 0.5f + 0.5f) * static_cast<float>(vpW);
  outY = (-ndcY * 0.5f + 0.5f) * static_cast<float>(vpH); // Y flip

  if (outX < -50.f || outX > vpW + 50.f || outY < -50.f || outY > vpH + 50.f) return false;
  return true;
}

inline float fovDistancePx(float x, float y, int vpW, int vpH) {
  const float cx = vpW * 0.5f;
  const float cy = vpH * 0.5f;
  const float dx = x - cx;
  const float dy = y - cy;
  return std::sqrt(dx*dx + dy*dy);
}

struct AimTarget {
  Vec3 world{};
  float screenX=0, screenY=0;
  float fovPx=1e9f;
  float distance=1e9f;
  int boneId=-1;
  bool valid=false;
};

inline Vec3 smoothToward(const Vec3& currentDir, const Vec3& desiredDir, float factor) {
  // Exponential smoothing: factor in (0,1], lower = slower/smoother.
  const float t = std::clamp(factor, 0.001f, 1.f);
  return norm(add(mul(currentDir, 1.f - t), mul(desiredDir, t)));
}

inline float randomFloat(float minV, float maxV) {
  static thread_local std::mt19937 rng{GetTickCount()};
  std::uniform_real_distribution<float> dist(minV, maxV);
  return dist(rng);
}

inline Vec3 applyJitter(const Vec3& dir, float amplitudeDeg) {
  if (amplitudeDeg <= 0.f) return dir;
  const float rad = amplitudeDeg * 3.14159265f / 180.f;
  const float jx = randomFloat(-rad, rad);
  const float jy = randomFloat(-rad, rad);
  // Small-angle jitter in local tangent-ish space; good enough for stub, tune in runtime.
  Vec3 j = {dir.x + jx, dir.y + jy, dir.z};
  return norm(j);
}

inline Vec3 predictPosition(const Vec3& targetPos, const Vec3& targetVel, float distance, float bulletSpeed) {
  if (bulletSpeed <= 1e-3f) return targetPos;
  const float t = distance / bulletSpeed;
  return add(targetPos, mul(targetVel, t));
}

struct AimState {
  ULONGLONG lastTargetSwitchMs = 0;
  ULONGLONG lastAcquireMs = 0;
  ULONGLONG reactionReadyMs = 0;
  int lastTargetId = -1;
  int lastBoneId = -1;
  bool hasTarget = false;
};

inline AimState& aimState() { static AimState s; return s; }

inline bool canSwitchTarget(int newTargetId, ULONGLONG nowMs, ULONGLONG cooldownMs = 250) {
  AimState& s = aimState();
  if (!s.hasTarget || s.lastTargetId == newTargetId) return true;
  return (nowMs - s.lastTargetSwitchMs) >= cooldownMs;
}

inline void markTarget(int targetId, int boneId, ULONGLONG nowMs) {
  AimState& s = aimState();
  if (s.lastTargetId != targetId) {
    s.lastTargetSwitchMs = nowMs;
    s.lastTargetId = targetId;
    s.hasTarget = true;
    const auto& c = cfg();
    const float delayS = randomFloat(c.aimDelayMin, c.aimDelayMax);
    s.reactionReadyMs = nowMs + static_cast<ULONGLONG>(delayS * 1000.f);
  }
  s.lastBoneId = boneId;
  s.lastAcquireMs = nowMs;
}

inline bool reactionReady(ULONGLONG nowMs) { return nowMs >= aimState().reactionReadyMs; }

inline bool shouldMissIntentional() {
  const int pct = cfg().missIntentionalPct;
  if (pct <= 0) return false;
  if (pct >= 100) return true;
  return randomFloat(0.f, 100.f) < static_cast<float>(pct);
}

inline AimTarget selectBestBone(const Vec3* boneWorld, const int* boneIds, int boneCount,
                                const Mat4& vp, int vpW, int vpH, float fovDegMax) {
  AimTarget best{};
  const float maxPx = (fovDegMax / 90.f) * (static_cast<float>(vpW) * 0.5f); // rough deg->px; TODO(runtime: use real camera FOV/tan)
  const float cx = vpW * 0.5f;
  const float cy = vpH * 0.5f;
  for (int i = 0; i < boneCount; ++i) {
    float sx = 0, sy = 0;
    if (!worldToScreen(vp, boneWorld[i], vpW, vpH, sx, sy)) continue;
    const float dx = sx - cx, dy = sy - cy;
    const float dpx = std::sqrt(dx * dx + dy * dy);
    if (dpx > maxPx) continue;
    if (dpx < best.fovPx) {
      best.world = boneWorld[i];
      best.screenX = sx; best.screenY = sy;
      best.fovPx = dpx;
      best.boneId = boneIds ? boneIds[i] : i;
      best.valid = true;
    }
  }
  return best;
}

// Hitbox preference: head > chest > spine by default; configurable via feat/kFeatBonesPriorityHead.
// TODO(runtime): per-weapon preference map once weapon type/id source resolved.
inline AimTarget selectPreferredBone(const Vec3* boneWorld, const int* boneIds, int boneCount,
                                     const Mat4& vp, int vpW, int vpH, float fovDegMax,
                                     int headIdx, int chestIdx, int spineIdx) {
  const bool headFirst = cfg().feat[kFeatBonesPriorityHead] || cfg().targetPriority == 0;
  int order[3];
  if (headFirst) { order[0] = headIdx; order[1] = chestIdx; order[2] = spineIdx; }
  else { order[0] = chestIdx; order[1] = spineIdx; order[2] = headIdx; }

  const float maxPx = (fovDegMax / 90.f) * (static_cast<float>(vpW) * 0.5f); // TODO(runtime: real camera FOV/tan)
  const float cx = vpW * 0.5f, cy = vpH * 0.5f;
  for (int k = 0; k < 3; ++k) {
    const int want = order[k];
    if (want < 0) continue;
    for (int i = 0; i < boneCount; ++i) {
      const int bid = boneIds ? boneIds[i] : i;
      if (bid != want) continue;
      float sx = 0, sy = 0;
      if (!worldToScreen(vp, boneWorld[i], vpW, vpH, sx, sy)) continue;
      const float dx = sx - cx, dy = sy - cy;
      const float dpx = std::sqrt(dx * dx + dy * dy);
      if (dpx > maxPx) continue;
      AimTarget t{}; t.world = boneWorld[i]; t.screenX = sx; t.screenY = sy; t.fovPx = dpx; t.boneId = bid; t.valid = true;
      return t;
    }
  }
  return selectBestBone(boneWorld, boneIds, boneCount, vp, vpW, vpH, fovDegMax);
}

// Silent aim hook point: write shot direction at fire moment, do not move camera.
// Candidates (dump-evidenced RVAs in offsets.hpp):
//   ShotArgsSystem.GetShotDirection RVA 0x4361530 (dump.cs:16479) - computes shot direction; strong hook candidate.
//   ShotArgsSystem.ApplySpreadToShotDirection RVA 0x4361000 (dump.cs:16482) - spread application.
//   ShotArgsSystem.Tick RVA 0x4361970 (dump.cs:16472) - system tick.
//   ShotIntentionUpdater.StartFire RVA 0xA996A0 (dump.cs:311628) - input intent only.
// TODO(runtime): implement detour/hook infra (MinHook/equivalent or manual trampoline), hook chosen method,
// overwrite shot direction at fire moment, camera unchanged, toggleable. No hook infra shipped yet.
inline bool silentAimWrite(void* fireContext, const Vec3& shotDir) {
  (void)fireContext; (void)shotDir;
  return false;
}

// Visibility check through game Physics.Raycast.
// dump.cs:2010093 RVA 0x63B1140: Raycast(Vector3 origin, Vector3 direction, out RaycastHit hitInfo, float maxDistance, int layerMask, QueryTriggerInteraction queryTriggerInteraction)
// TODO(runtime: confirm layerMask for player/character colliders; confirm QueryTriggerInteraction value - defaulting 1 Collide until confirmed)
inline bool rayVisible(const Vec3& origin, const Vec3& target, int layerMask, int queryTriggerInteraction = 1) {
  Vec3 dir = sub(target, origin);
  const float dist = len(dir);
  if (dist <= 1e-4f) return true;
  dir = mul(dir, 1.f / dist);
  il2cpp::RaycastHitBuf hit{};
  const bool gotHit = il2cpp::physicsRaycast({origin.x, origin.y, origin.z}, {dir.x, dir.y, dir.z}, &hit, dist, layerMask, queryTriggerInteraction);
  // If raycast returns true, something was hit before target - not visible.
  // TODO(runtime: verify hit collider belongs to target entity for precise visibility; current is conservative).
  return !gotHit;
}

struct VisEntry { uintptr_t key = 0; bool visible = false; ULONGLONG tick = 0; };
inline constexpr size_t kMaxVisCache = 512;
inline VisEntry& visSlot(size_t i) { static VisEntry slots[kMaxVisCache]{}; return slots[i]; }

inline bool visibilityCached(uintptr_t entityKey, const Vec3& origin, const Vec3& boneWorld, int layerMask,
                             int queryTriggerInteraction = 1, ULONGLONG ttlMs = 100) {
  const ULONGLONG now = GetTickCount64();
  if (entityKey) {
    for (size_t i = 0; i < kMaxVisCache; ++i) {
      VisEntry& e = visSlot(i);
      if (e.key == entityKey && (now - e.tick) <= ttlMs) return e.visible;
    }
  }
  const bool vis = rayVisible(origin, boneWorld, layerMask, queryTriggerInteraction);
  if (entityKey) {
    VisEntry* slot = nullptr;
    for (size_t i = 0; i < kMaxVisCache; ++i) { if (visSlot(i).key == entityKey || visSlot(i).key == 0) { slot = &visSlot(i); break; } }
    if (!slot) slot = &visSlot(static_cast<size_t>(entityKey % kMaxVisCache));
    slot->key = entityKey; slot->visible = vis; slot->tick = now;
  }
  return vis;
}

} // namespace rb::aim
