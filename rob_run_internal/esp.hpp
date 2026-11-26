#pragma once

// esp.hpp — Player ESP: 2D box, name, distance, health bar, skeleton.
// Visibility color: red = not visible, white = visible (spec).

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace rb::esp {

struct EspEntry {
  float x = 0, y = 0, w = 0, h = 0; // screen box
  float distance = 0.f;
  float health = 1.f; // 0..1
  const char* name = nullptr;
  bool visible = false;
  bool valid = false;

  static constexpr int kMaxBones = 32;
  ImVec2 bones[kMaxBones]{};
  int boneCount = 0;
};

inline void drawBox(ImDrawList* dl, const EspEntry& e, ImU32 col) {
  if (!dl || !e.valid) return;
  dl->AddRect(ImVec2(e.x, e.y), ImVec2(e.x + e.w, e.y + e.h), col, 0.f, 0, 1.5f);
}

inline void drawHealth(ImDrawList* dl, const EspEntry& e) {
  if (!dl || !e.valid) return;
  const float h = std::clamp(e.health, 0.f, 1.f);
  const float barW = 3.f;
  const float x0 = e.x - barW - 2.f;
  const float y0 = e.y;
  const float y1 = e.y + e.h;
  dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + barW, y1), IM_COL32(20, 20, 20, 220));
  const float fillTop = y1 - (y1 - y0) * h;
  dl->AddRectFilled(ImVec2(x0, fillTop), ImVec2(x0 + barW, y1), IM_COL32(80, 220, 80, 230));
}

inline void drawNameDistance(ImDrawList* dl, const EspEntry& e, ImU32 col) {
  if (!dl || !e.valid) return;
  char buf[128]{};
  snprintf(buf, sizeof(buf), "%s | %.1fm", e.name ? e.name : "player", e.distance);
  dl->AddText(ImVec2(e.x, e.y - 14.f), col, buf);
}

inline void drawSkeleton(ImDrawList* dl, const EspEntry& e, ImU32 col) {
  if (!dl || !e.valid || e.boneCount < 2) return;
  for (int i = 0; i < e.boneCount; ++i) {
    dl->AddCircleFilled(e.bones[i], 2.f, col, 6);
  }
  // TODO(dump: "SkinnedMeshRenderer bone array hierarchy"): connect actual bone pairs.
}

inline void renderAll(const std::vector<EspEntry>& entries, bool drawBoxEnabled, bool drawNames, bool drawHealthBar, bool drawSkeletonLines,
                       bool boxVisibleOnly, float maxDist) {
  ImDrawList* dl = ImGui::GetBackgroundDrawList();
  if (!dl) return;
  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  for (const auto& e : entries) {
    if (!e.valid) continue;
    if (maxDist > 0.f && e.distance > maxDist) continue;
    if (boxVisibleOnly && !e.visible) continue;
    const ImU32 col = e.visible ? IM_COL32(255, 255, 255, 230) : IM_COL32(255, 40, 40, 230);
    // clamp to viewport
    if (e.x + e.w < 0 || e.y + e.h < 0 || e.x > disp.x || e.y > disp.y) continue;
    if (drawBoxEnabled) drawBox(dl, e, col);
    if (drawHealthBar) drawHealth(dl, e);
    if (drawNames) drawNameDistance(dl, e, col);
    if (drawSkeletonLines) drawSkeleton(dl, e, col);
  }
}

} // namespace rb::esp
