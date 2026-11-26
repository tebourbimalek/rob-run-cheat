#pragma once

// bone_probe.hpp — runtime SkinnedMeshRenderer bone array dump.
// Use during driver/inject pass to resolve kBone_Head/Spine/Chest from real hierarchy.
// Not dump.cs-evidenced; outputs index -> bone name for manual mapping.

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdarg>

namespace rb::boneprobe {

struct UnityObjectLike {
  void* klass;
  void* monitor;
  void* m_CachedPtr; // 0x10
};

inline void log(const char* msg) {
  if (msg) OutputDebugStringA(msg);
}

inline void logf(const char* fmt, ...) {
  char buf[512]{};
  va_list args;
  va_start(args, fmt);
  _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
  va_end(args);
  OutputDebugStringA(buf);
}

// Caller supplies IL2CPP-resolved accessors at runtime:
// get_bones: Transform[] (SkinnedMeshRenderer*)
// get_name:  const char*/Il2CppString* (UnityEngine.Object*) depending on wrapper; adapt as needed.
// This probe is intentionally minimal: it only validates pointers/count bounds before reads.
struct BoneIndices {
  int head = -1;
  int spine = -1;
  int chest = -1;
  bool resolved = false;
};

inline BoneIndices& runtimeBones() {
  static BoneIndices b;
  return b;
}

// Reads Il2CppString* (Unity System.String) into caller buffer as narrow ASCII.
// Il2CppString layout (common): +0x10 length (int32), +0x14 chars (UTF-16LE).
inline const char* il2cppStringToAscii(void* strObj, char* buf, int bufSz) {
  if (!strObj || !buf || bufSz < 2) return nullptr;
  __try {
    const int len = *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(strObj) + 0x10);
    const auto* chars = reinterpret_cast<const wchar_t*>(reinterpret_cast<uintptr_t>(strObj) + 0x14);
    if (len < 0 || len > 1024) return nullptr;
    int n = len < (bufSz - 1) ? len : (bufSz - 1);
    for (int i = 0; i < n; ++i) {
      wchar_t c = chars[i];
      buf[i] = (c >= 32 && c < 127) ? static_cast<char>(c) : '?';
    }
    buf[n] = '\0';
    return buf;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    if (bufSz > 0) buf[0] = '\0';
    return nullptr;
  }
}

inline bool nameContains(const char* hay, const char* needle) {
  if (!hay || !needle) return false;
  for (const char* h = hay; *h; ++h) {
    const char* p = h;
    const char* n = needle;
    while (*p && *n) {
      char pc = *p; char nc = *n;
      if (pc >= 'A' && pc <= 'Z') pc = static_cast<char>(pc - 'A' + 'a');
      if (nc >= 'A' && nc <= 'Z') nc = static_cast<char>(nc - 'A' + 'a');
      if (pc != nc) break;
      ++p; ++n;
    }
    if (*n == '\0') return true;
  }
  return false;
}

inline int scoreBoneName(const char* name, int kind /*0=head,1=spine,2=chest*/) {
  if (!name) return -1;
  if (kind == 0) {
    if (nameContains(name, "head")) return 100;
  } else if (kind == 1) {
    if (nameContains(name, "spine1") || nameContains(name, "chest") || nameContains(name, "upperchest")) return -1;
    if (nameContains(name, "spine") || nameContains(name, "hips") || nameContains(name, "pelvis")) return 100;
  } else if (kind == 2) {
    if (nameContains(name, "upperchest")) return 120;
    if (nameContains(name, "chest")) return 110;
    if (nameContains(name, "spine2")) return 100;
    if (nameContains(name, "spine1")) return 90;
  }
  return -1;
}

template <typename GetBonesFn, typename GetLengthFn, typename GetElementFn, typename GetNameFn>
inline void dumpBones(void* skinnedMeshRenderer, GetBonesFn getBones, GetLengthFn getLength, GetElementFn getElement, GetNameFn getName) {
  if (!skinnedMeshRenderer) { log("[rb] bone_probe: null SkinnedMeshRenderer\n"); return; }
  void* bonesArray = getBones(skinnedMeshRenderer);
  if (!bonesArray) { log("[rb] bone_probe: null bones array\n"); return; }

  int len = getLength(bonesArray);
  if (len <= 0 || len > 4096) { logf("[rb] bone_probe: bad bone count=%d\n", len); return; }

  logf("[rb] bone_probe: bone_count=%d\n", len);
  for (int i = 0; i < len; ++i) {
    void* bone = getElement(bonesArray, i);
    if (!bone) { logf("[rb] bone[%d]=<null>\n", i); continue; }
    const char* name = getName(bone);
    logf("[rb] bone[%d]=%s\n", i, name ? name : "<noname>");
  }
}

template <typename GetBonesFn, typename GetLengthFn, typename GetElementFn, typename GetNameFn>
inline BoneIndices resolveBones(void* skinnedMeshRenderer, GetBonesFn getBones, GetLengthFn getLength, GetElementFn getElement, GetNameFn getName) {
  BoneIndices out;
  if (!skinnedMeshRenderer) return out;
  void* bonesArray = getBones(skinnedMeshRenderer);
  if (!bonesArray) return out;
  int len = getLength(bonesArray);
  if (len <= 0 || len > 4096) return out;

  int bestHead = -1, bestSpine = -1, bestChest = -1;
  int hs = -1, ss = -1, cs = -1;
  for (int i = 0; i < len; ++i) {
    void* bone = getElement(bonesArray, i);
    if (!bone) continue;
    const char* name = getName(bone);
    if (!name) continue;
    int s0 = scoreBoneName(name, 0);
    int s1 = scoreBoneName(name, 1);
    int s2 = scoreBoneName(name, 2);
    if (s0 > hs) { hs = s0; bestHead = i; }
    if (s1 > ss) { ss = s1; bestSpine = i; }
    if (s2 > cs) { cs = s2; bestChest = i; }
  }
  out.head = bestHead;
  out.spine = bestSpine;
  out.chest = bestChest;
  out.resolved = (out.head >= 0 || out.spine >= 0 || out.chest >= 0);
  if (out.resolved) {
    runtimeBones() = out;
    logf("[rb] bone_resolve: head=%d spine=%d chest=%d\n", out.head, out.spine, out.chest);
  }
  return out;
}

} // namespace rb::boneprobe
