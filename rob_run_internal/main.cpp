#include <windows.h>
#include <d3d11.h>
#include <cstdio>
#include <vector>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include "dxgi_hook.hpp"
#include "features.hpp"
#include "esp.hpp"
#include "aimbot.hpp"
#include "entity.hpp"
#include "offsets.hpp"
#include "il2cpp_wrapper.hpp"
#include "bone_probe.hpp"
#include "combat.hpp"

namespace {
bool g_menuOpen = true;
bool g_insertHeld = false;
bool g_aimHeld = false;
std::vector<rb::esp::EspEntry> g_espEntries;

void drawMenu() {
  ImGui::GetIO().MouseDrawCursor = g_menuOpen;
  if (!g_menuOpen) return;
  ImGui::SetNextWindowSize(ImVec2(420.f, 0.f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("ROB RUN Internal", &g_menuOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::End();
    return;
  }
  auto& c = rb::cfg();
  ImGui::Text("Menu: %s (Insert)", g_menuOpen ? "OPEN" : "CLOSED");
  ImGui::Separator();
  if (ImGui::CollapsingHeader("ESP", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("ESP Master", &c.feat[rb::kFeatESP]);
    ImGui::Checkbox("Box", &c.feat[rb::kFeatBox]);
    ImGui::Checkbox("Skeleton", &c.feat[rb::kFeatSkeleton]);
    ImGui::Checkbox("Health Bar", &c.feat[rb::kFeatHealth]);
    ImGui::Checkbox("Names", &c.name);
    ImGui::Checkbox("Health Text/Bar", &c.healthBar);
    ImGui::Checkbox("Skeleton Lines", &c.skeleton);
    ImGui::Checkbox("Visible Only", &c.boxVisibleOnly);
    ImGui::SliderFloat("Max Distance", &c.maxDistESP, 10.f, 1000.f, "%.0fm");
    ImGui::SliderInt("Box Alpha", &c.boxAlpha, 20, 255);
  }
  if (ImGui::CollapsingHeader("Aim / Combat", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Checkbox("Aimbot", &c.feat[rb::kFeatAim]);
    ImGui::Checkbox("Silent Aim", &c.feat[rb::kFeatSilentAim]);
    ImGui::Checkbox("Triggerbot", &c.feat[rb::kFeatTrigger]);
    ImGui::Checkbox("No Recoil", &c.feat[rb::kFeatNoRecoil]);
    ImGui::Checkbox("No Spread", &c.feat[rb::kFeatNoSpread]);
    ImGui::Checkbox("Bones Priority Head", &c.feat[rb::kFeatBonesPriorityHead]);
    ImGui::SliderFloat("FOV (deg)", &c.fovDeg, 1.f, 30.f, "%.1f");
    ImGui::SliderFloat("Smoothing", &c.smoothingFactor, 0.01f, 1.f, "%.3f");
    ImGui::SliderFloat("Silent Smooth", &c.silentSmooth, 0.01f, 1.f, "%.3f");
    ImGui::SliderFloat("Delay Min (s)", &c.aimDelayMin, 0.f, 1.f, "%.3f");
    ImGui::SliderFloat("Delay Max (s)", &c.aimDelayMax, 0.f, 1.f, "%.3f");
    ImGui::Checkbox("Micro Jitter", &c.microJitter);
    ImGui::SliderFloat("Jitter (deg)", &c.jitterAmpl, 0.f, 2.f, "%.2f");
    ImGui::SliderInt("Miss %", &c.missIntentionalPct, 0, 100);
    ImGui::SliderInt("Target Priority", &c.targetPriority, 0, 2);
  }
  if (ImGui::CollapsingHeader("Config / Keys")) {
    ImGui::TextDisabled("Config: %s", rb::Cfg::defaultConfigPath());
    if (ImGui::Button("Save Config")) c.save();
    ImGui::SameLine();
    if (ImGui::Button("Load Config")) (void)c.load();
    ImGui::SameLine();
    if (ImGui::Button("Reset Defaults")) c.reset();
    if (ImGui::Button("Dump Bones")) {
      auto& b = rb::boneprobe::runtimeBones();
      rb::boneprobe::logf("[rb] bone_probe: manual dump requested (head=%d spine=%d chest=%d resolved=%d)\n",
                          b.head, b.spine, b.chest, b.resolved ? 1 : 0);
      // TODO(runtime): wire to local player SkinnedMeshRenderer once entity chain + SMR access resolved.
    }
    ImGui::Separator();
    ImGui::TextDisabled("Insert: toggle menu");
    ImGui::TextDisabled("StopCheat export: unload");
    ImGui::TextDisabled("Menu captures input when open");
  }
  ImGui::End();
}

void updateEntities() {
  g_espEntries.clear();
  auto& c = rb::cfg();
  if (!c.feat[rb::kFeatESP]) return;

  // TODO(runtime: resolve BattleModel root instance -> BattleEntitiesModel; no static singleton in dump.cs:239134)
  void* battleEntitiesModel = nullptr;
  if (!battleEntitiesModel) return;

  rb::ent::EntityView views[512]{};
  const size_t n = rb::ent::collectEntities(battleEntitiesModel, views, 512);

  for (size_t i = 0; i < n; ++i) {
    if (views[i].local && views[i].clientModel) { rb::combat::tickCombat(views[i].clientModel); break; }
  }

  // TODO(runtime: camera matrix source/layout - use rb::il2cpp::cameraGetMain/current + cameraWorldToCameraMatrix/projectionMatrix; confirm Matrix4x4 ABI)
  void* cam = rb::il2cpp::cameraGetMain();
  if (!cam) cam = rb::il2cpp::cameraGetCurrent();
  rb::il2cpp::Matrix4x4 view{}, proj{};
  const bool hasMatrix = cam && rb::il2cpp::cameraWorldToCameraMatrix(cam, &view) && rb::il2cpp::cameraProjectionMatrix(cam, &proj);
  (void)hasMatrix;

  const ImVec2 disp = ImGui::GetIO().DisplaySize;
  for (size_t i = 0; i < n; ++i) {
    const auto& v = views[i];
    if (v.local) continue;
    if (!v.entity) continue;

    rb::esp::EspEntry e{};
    e.name = "player"; // TODO(runtime: resolve player name from entity model chain)
    e.health = v.health;
    e.visible = false; // TODO(runtime: Physics.Raycast visibility with 100ms per-entity cache)
    e.distance = 0.f;

    // TODO(runtime: resolve world position/bone transforms; compute 2D box via W2S)
    // W2S requires confirmed matrix layout and entity world position source.
    e.valid = false;
    (void)disp;
    (void)hasMatrix;
    g_espEntries.push_back(e);
  }
}

void updateAim() {
  auto& c = rb::cfg();
  if (!c.feat[rb::kFeatAim] && !c.feat[rb::kFeatSilentAim]) return;

  void* cam = rb::il2cpp::cameraGetMain();
  if (!cam) cam = rb::il2cpp::cameraGetCurrent();
  if (!cam) return;

  rb::il2cpp::Matrix4x4 view{}, proj{};
  if (!rb::il2cpp::cameraWorldToCameraMatrix(cam, &view) || !rb::il2cpp::cameraProjectionMatrix(cam, &proj)) return;
  // TODO(runtime): confirm Matrix4x4 layout/ABI and multiply order for VP; aimbot::Mat4 expects row-major m[4][4].
  // TODO(runtime): build VP, get entity bone world positions, select best bone in FOV.
  // TODO(runtime): apply reaction delay (rb::aim::reactionReady), target-switch cooldown (canSwitchTarget),
  //   smoothing (smoothToward), jitter (applyJitter), intentional miss (shouldMissIntentional).
  // TODO(runtime): silent aim - hook fire method and write shot dir at fire moment only (silentAimWrite stub).
  // TODO(runtime): prediction - predictPosition with real bullet speed and target velocity; gravity/drag TODO if confirmed.
  (void)view; (void)proj;
}

} // namespace

void rb_renderOverlay() {
  const bool insertDown = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
  if (insertDown && !g_insertHeld) g_menuOpen = !g_menuOpen;
  g_insertHeld = insertDown;

  updateEntities();
  auto& c = rb::cfg();
  rb::esp::renderAll(g_espEntries, c.feat[rb::kFeatBox] || c.feat[rb::kFeatESP], c.feat[rb::kFeatESP] && c.name,
                     c.feat[rb::kFeatHealth] && c.healthBar, c.feat[rb::kFeatSkeleton] && c.skeleton,
                     c.boxVisibleOnly, c.maxDistESP);
  updateAim();
  drawMenu();
}

void rb_start() {
  OutputDebugStringA("[rb] rb_start\n");
  rb::hook::SetRender(true);
  rb::hook::FindSwapchain(); // dummy probe installs Present hook; real swapchain captured in PresentStub
}

void rb_unload() {
  rb::hook::SetRender(false);
  rb::hook::UnhookPresent(); // restores vtable, WndProc, ImGui, refs
  OutputDebugStringA("[rb] unloaded\n");
}

extern "C" __declspec(dllexport) void StartCheat() { rb_start(); }
extern "C" __declspec(dllexport) void StopCheat() { rb_unload(); }

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(GetModuleHandleW(nullptr));
  } else if (reason == DLL_PROCESS_DETACH) {
    rb_unload();
  }
  return TRUE;
}
