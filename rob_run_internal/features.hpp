#pragma once

// ---------------------------------------------------------------------------
// features.hpp — all tunables in one struct. Zero magic numbers anywhere.
// Config: JSON sidecar next to DLL, fallback %APPDATA%\RobRunInternal.
// ---------------------------------------------------------------------------

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>

namespace rb {

enum Feat {
  kFeatESP, kFeatSkeleton, kFeatHealth, kFeatBox,
  kFeatAim, kFeatSilentAim, kFeatTrigger, kFeatNoRecoil, kFeatNoSpread,
  kFeatBones, kFeatBonesPriorityHead, kFeatFogAdjust, kFeatHideOwnName,
  kFeatCount
};

inline const char* featName(int f) {
  static const char* k[] = {
    "ESP","Skeleton","Health","Box","Aim","SilentAim","Trigger","NoRecoil","NoSpread",
    "Bones","BonesPriorityHead","FogAdjust","HideOwnName"
  };
  return (f >= 0 && f < kFeatCount) ? k[f] : "Unknown";
}

struct Cfg {
  bool feat[kFeatCount] = {};

  // Aimbot
  float fovDeg = 6.5f;
  float smoothingFactor = 0.08f;
  float silentSmooth = 0.06f;
  float aimDelayMin = 0.12f;
  float aimDelayMax = 0.35f;
  bool microJitter = true;
  float jitterAmpl = 0.6f;
  int missIntentionalPct = 0;
  int targetPriority = 0;

  // ESP
  bool boxVisibleOnly = true;
  int boxAlpha = 220;
  float maxDistESP = 150.f;
  bool name = true;
  bool healthBar = true;
  bool skeleton = true;
  bool hideMenuOnHide = false;

  void reset() { *this = Cfg(); }

  static inline const char* defaultConfigPath() {
    static char path[MAX_PATH] = {};
    if (path[0]) return path;
    HMODULE hm = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&defaultConfigPath), &hm) && hm) {
      DWORD n = GetModuleFileNameA(hm, path, MAX_PATH);
      if (n > 0 && n < MAX_PATH) {
        for (DWORD i = n; i > 0; --i) {
          if (path[i - 1] == '\\' || path[i - 1] == '/') { path[i - 1] = '\0'; break; }
        }
        strncat_s(path, "\\RobRunInternalConfig.json", _TRUNCATE);
        return path;
      }
    }
    char appData[MAX_PATH] = {};
    DWORD an = GetEnvironmentVariableA("APPDATA", appData, MAX_PATH);
    if (an > 0 && an < MAX_PATH) {
      _snprintf_s(path, MAX_PATH, _TRUNCATE, "%s\\RobRunInternal\\RobRunInternalConfig.json", appData);
    } else {
      strncpy_s(path, "RobRunInternalConfig.json", _TRUNCATE);
    }
    return path;
  }

  static inline bool ensureParentDir(const char* filePath) {
    if (!filePath || !*filePath) return false;
    char dir[MAX_PATH] = {};
    strncpy_s(dir, filePath, _TRUNCATE);
    for (int i = static_cast<int>(strlen(dir)) - 1; i >= 0; --i) {
      if (dir[i] == '\\' || dir[i] == '/') { dir[i] = '\0'; break; }
    }
    if (!dir[0]) return false;
    DWORD attr = GetFileAttributesA(dir);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) return true;
    return CreateDirectoryA(dir, nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
  }

  void save(const char* p = nullptr) const {
    const char* path = (p && *p) ? p : defaultConfigPath();
    ensureParentDir(path);
    FILE* fp = nullptr;
    if (fopen_s(&fp, path, "wb") != 0 || !fp) return;
    fprintf(fp, "{\n");
    for (int i = 0; i < kFeatCount; ++i)
      fprintf(fp, "  \"feat_%d\": %s,\n", i, feat[i] ? "true" : "false");
    fprintf(fp, "  \"fovDeg\": %.6f,\n", fovDeg);
    fprintf(fp, "  \"smoothingFactor\": %.6f,\n", smoothingFactor);
    fprintf(fp, "  \"silentSmooth\": %.6f,\n", silentSmooth);
    fprintf(fp, "  \"aimDelayMin\": %.6f,\n", aimDelayMin);
    fprintf(fp, "  \"aimDelayMax\": %.6f,\n", aimDelayMax);
    fprintf(fp, "  \"microJitter\": %s,\n", microJitter ? "true" : "false");
    fprintf(fp, "  \"jitterAmpl\": %.6f,\n", jitterAmpl);
    fprintf(fp, "  \"missIntentionalPct\": %d,\n", missIntentionalPct);
    fprintf(fp, "  \"targetPriority\": %d,\n", targetPriority);
    fprintf(fp, "  \"boxVisibleOnly\": %s,\n", boxVisibleOnly ? "true" : "false");
    fprintf(fp, "  \"boxAlpha\": %d,\n", boxAlpha);
    fprintf(fp, "  \"maxDistESP\": %.6f,\n", maxDistESP);
    fprintf(fp, "  \"name\": %s,\n", name ? "true" : "false");
    fprintf(fp, "  \"healthBar\": %s,\n", healthBar ? "true" : "false");
    fprintf(fp, "  \"skeleton\": %s,\n", skeleton ? "true" : "false");
    fprintf(fp, "  \"hideMenuOnHide\": %s\n", hideMenuOnHide ? "true" : "false");
    fprintf(fp, "}\n");
    fclose(fp);
  }

private:
  static inline const char* skipWs(const char* s) { while (s && *s && static_cast<unsigned char>(*s) <= ' ') ++s; return s; }
  static inline bool parseKey(const char* s, const char* key, const char** valOut) {
    if (!s || !key) return false;
    const char* p = strstr(s, key);
    if (!p) return false;
    p += strlen(key);
    p = skipWs(p);
    if (*p != ':') return false;
    ++p;
    p = skipWs(p);
    *valOut = p;
    return true;
  }
  static inline bool parseBool(const char* s, const char* key, bool& out) {
    const char* v = nullptr;
    if (!parseKey(s, key, &v)) return false;
    if (strncmp(v, "true", 4) == 0) { out = true; return true; }
    if (strncmp(v, "false", 5) == 0) { out = false; return true; }
    return false;
  }
  static inline bool parseInt(const char* s, const char* key, int& out) {
    const char* v = nullptr;
    if (!parseKey(s, key, &v)) return false;
    char* end = nullptr;
    long val = strtol(v, &end, 10);
    if (end == v) return false;
    out = static_cast<int>(val);
    return true;
  }
  static inline bool parseFloat(const char* s, const char* key, float& out) {
    const char* v = nullptr;
    if (!parseKey(s, key, &v)) return false;
    char* end = nullptr;
    float val = strtof(v, &end);
    if (end == v) return false;
    out = val;
    return true;
  }

public:
  bool load(const char* p = nullptr) {
    const char* path = (p && *p) ? p : defaultConfigPath();
    FILE* fp = nullptr;
    if (fopen_s(&fp, path, "rb") != 0 || !fp) return false;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz <= 0 || sz > 1024 * 64) { fclose(fp); return false; }
    char* buf = static_cast<char*>(malloc(static_cast<size_t>(sz) + 1));
    if (!buf) { fclose(fp); return false; }
    size_t rd = fread(buf, 1, static_cast<size_t>(sz), fp);
    fclose(fp);
    buf[rd] = '\0';

    // Parse into a copy; only commit on meaningful parse. Missing keys keep defaults.
    Cfg next = *this;
    bool any = false;
    for (int i = 0; i < kFeatCount; ++i) {
      char k[32]; _snprintf_s(k, sizeof(k), _TRUNCATE, "\"feat_%d\"", i);
      bool v = next.feat[i];
      if (parseBool(buf, k, v)) { next.feat[i] = v; any = true; }
    }
    any = parseFloat(buf, "\"fovDeg\"", next.fovDeg) || any;
    any = parseFloat(buf, "\"smoothingFactor\"", next.smoothingFactor) || any;
    any = parseFloat(buf, "\"silentSmooth\"", next.silentSmooth) || any;
    any = parseFloat(buf, "\"aimDelayMin\"", next.aimDelayMin) || any;
    any = parseFloat(buf, "\"aimDelayMax\"", next.aimDelayMax) || any;
    any = parseBool(buf, "\"microJitter\"", next.microJitter) || any;
    any = parseFloat(buf, "\"jitterAmpl\"", next.jitterAmpl) || any;
    any = parseInt(buf, "\"missIntentionalPct\"", next.missIntentionalPct) || any;
    any = parseInt(buf, "\"targetPriority\"", next.targetPriority) || any;
    any = parseBool(buf, "\"boxVisibleOnly\"", next.boxVisibleOnly) || any;
    any = parseInt(buf, "\"boxAlpha\"", next.boxAlpha) || any;
    any = parseFloat(buf, "\"maxDistESP\"", next.maxDistESP) || any;
    any = parseBool(buf, "\"name\"", next.name) || any;
    any = parseBool(buf, "\"healthBar\"", next.healthBar) || any;
    any = parseBool(buf, "\"skeleton\"", next.skeleton) || any;
    any = parseBool(buf, "\"hideMenuOnHide\"", next.hideMenuOnHide) || any;

    // Clamp ranges to sane bounds; bad values fall back to defaults.
    if (next.fovDeg < 0.5f || next.fovDeg > 180.f) next.fovDeg = Cfg{}.fovDeg;
    if (next.smoothingFactor < 0.001f || next.smoothingFactor > 1.f) next.smoothingFactor = Cfg{}.smoothingFactor;
    if (next.silentSmooth < 0.001f || next.silentSmooth > 1.f) next.silentSmooth = Cfg{}.silentSmooth;
    if (next.aimDelayMin < 0.f || next.aimDelayMin > 5.f) next.aimDelayMin = Cfg{}.aimDelayMin;
    if (next.aimDelayMax < 0.f || next.aimDelayMax > 5.f) next.aimDelayMax = Cfg{}.aimDelayMax;
    if (next.aimDelayMax < next.aimDelayMin) next.aimDelayMax = next.aimDelayMin;
    if (next.jitterAmpl < 0.f || next.jitterAmpl > 10.f) next.jitterAmpl = Cfg{}.jitterAmpl;
    if (next.missIntentionalPct < 0 || next.missIntentionalPct > 100) next.missIntentionalPct = Cfg{}.missIntentionalPct;
    if (next.boxAlpha < 0 || next.boxAlpha > 255) next.boxAlpha = Cfg{}.boxAlpha;
    if (next.maxDistESP < 1.f || next.maxDistESP > 10000.f) next.maxDistESP = Cfg{}.maxDistESP;

    free(buf);
    if (any) *this = next;
    return any;
  }
};

inline Cfg& cfg() { static Cfg c; return c; }

}  // namespace rb
