#pragma once

// entity.hpp — local player / entity walk.
// IL2CPP System.Collections.Generic.List<T> (Unity/IL2CPP, common layout):
//   +0x10  T[]   _items   (Il2CppArray*)
//   +0x18  int   _size    (count)
//   +0x1C  int   _version
// Verify against dump.cs: grep "class List`1" in mscorlib/Unity dump or inspect fields.
// Every dereference is null/bounds checked.

#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <vector>

#include "il2cpp_wrapper.hpp"
#include "offsets.hpp"

namespace rb::ent {

struct Il2CppArrayHeader {
  void* klass;
  void* monitor;
  void* bounds;
  size_t max_length; // 0x18 on x64
};

inline bool validPtr(void* p) {
  const uintptr_t v = reinterpret_cast<uintptr_t>(p);
  return v > 0x10000 && v < 0x0000800000000000ULL;
}

template <typename T>
inline bool readMem(void* addr, T* out) {
  if (!validPtr(addr) || !out) return false;
  __try {
    *out = *reinterpret_cast<T*>(addr);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    *out = T{};
    return false;
  }
}

template <typename T>
inline bool writeMem(void* addr, const T& value) {
  if (!validPtr(addr)) return false;
  __try {
    *reinterpret_cast<T*>(addr) = value;
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

inline void* readPtrField(void* obj, uintptr_t off) {
  void* v = nullptr;
  if (!obj) return nullptr;
  readMem<void*>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + off), &v);
  return validPtr(v) ? v : nullptr;
}

inline int readIntField(void* obj, uintptr_t off) {
  int v = 0;
  if (!obj) return 0;
  readMem<int>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(obj) + off), &v);
  return v;
}

// IL2CPP List<T> layout (runtime-assumed; verify on live build).
// dump.cs:804724 shows fields _items/_size/_version but generic def offsets are 0x0.
inline constexpr uintptr_t kList_Items   = 0x10;
inline constexpr uintptr_t kList_Size    = 0x18;
inline constexpr uintptr_t kList_Version = 0x1C;

// Il2CppArray (x64): klass 0x0, monitor 0x8, bounds 0x10, max_length 0x18, vector 0x20.
inline constexpr uintptr_t kArray_MaxLength = 0x18;
inline constexpr uintptr_t kArray_Vector    = 0x20;

inline bool verifyListLayout(void* listObj) {
  if (!listObj) return false;
  void* items = readPtrField(listObj, kList_Items);
  int count = readIntField(listObj, kList_Size);
  int ver = readIntField(listObj, kList_Version);
  if (count < 0 || count > 1000000) return false;
  if (ver < 0) return false;
  if (count == 0) return items == nullptr || validPtr(items);
  if (!validPtr(items)) return false;
  size_t maxLen = 0;
  readMem<size_t>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(items) + kArray_MaxLength), &maxLen);
  if (maxLen > 10000000ULL) return false;
  return static_cast<size_t>(count) <= maxLen;
}

// List<T> walk. Returns number of elements written to out.
inline size_t readList(void* listObj, void** out, size_t outCap) {
  if (!listObj || !out || outCap == 0) return 0;
  void* items = readPtrField(listObj, kList_Items);
  int count = readIntField(listObj, kList_Size);
  if (!items || count <= 0) return 0;
  if (count > 4096) count = 4096; // bounds cap

  size_t maxLen = 0;
  readMem<size_t>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(items) + kArray_MaxLength), &maxLen);
  if (maxLen == 0 || maxLen > 100000) return 0;
  if (static_cast<size_t>(count) > maxLen) count = static_cast<int>(maxLen);

  const size_t n = static_cast<size_t>(count) < outCap ? static_cast<size_t>(count) : outCap;
  for (size_t i = 0; i < n; ++i) {
    void* elem = readPtrField(items, kArray_Vector + i * sizeof(void*));
    out[i] = elem; // may be null; caller checks
  }
  return n;
}

// Dictionary<TKey,TValue> layout (IL2CPP/Unity) — resolve at runtime via field offsets.
// dump.cs:761540 field names: _buckets, _entries, _count, _freeList, _freeCount, _version, _comparer.
// Generic def offsets are 0x0 in dump. Never assume .NET Core layout.
struct DictionaryLayout {
  uintptr_t buckets = 0;   // int[]
  uintptr_t entries = 0;   // Entry[]
  uintptr_t count = 0;     // int
  uintptr_t version = 0;   // int
  uintptr_t entriesKey = 0;   // Entry.key offset within entry
  uintptr_t entriesValue = 0; // Entry.value offset within entry
  uintptr_t entriesHash = 0;  // Entry.hashCode offset within entry
  uintptr_t entriesNext = 0;  // Entry.next offset within entry
  uintptr_t entryStride = 0;  // sizeof(Entry) - runtime probed
  bool verified = false;
};

inline DictionaryLayout& dictLayout() {
  static DictionaryLayout l;
  return l;
}

inline bool resolveDictionaryLayoutAtRuntime(void* dictObj) {
  DictionaryLayout& l = dictLayout();
  if (l.verified) return true;
  if (!dictObj) return false;

  auto* klass = *reinterpret_cast<il2cpp::Il2CppClass**>(dictObj); // Il2CppObject.klass at 0x0
  if (!validPtr(klass)) return false;

  auto resolveField = [&](const char* name) -> uintptr_t {
    il2cpp::Il2CppField* f = il2cpp::classGetFieldFromName(klass, name);
    return f ? static_cast<uintptr_t>(il2cpp::fieldGetOffset(f)) : 0;
  };

  const uintptr_t buckets = resolveField("_buckets");
  const uintptr_t entries = resolveField("_entries");
  const uintptr_t count = resolveField("_count");
  const uintptr_t version = resolveField("_version");
  if (!buckets || !entries || !count) return false;

  l.buckets = buckets;
  l.entries = entries;
  l.count = count;
  l.version = version;

  // Entry struct layout is harder to get from field reflection; probe conservatively.
  // Common IL2CPP Dictionary<int, object> entry: hashCode(0x0), next(0x4), key(0x8), value(0x10) on x64.
  // We verify by checking that key/value look plausible on first live entry.
  l.entriesHash = 0x0;
  l.entriesNext = 0x4;
  l.entriesKey = 0x8;
  l.entriesValue = 0x10;
  l.entryStride = 0x18; // conservative default for int key + ref value on x64; probed below

  // Probe: if dict has entries, confirm at least one valid slot looks sane.
  int countVal = readIntField(dictObj, l.count);
  void* entriesArr = readPtrField(dictObj, l.entries);
  if (countVal < 0 || countVal > 100000) { l.verified = false; return false; }
  if (countVal == 0) { l.verified = true; return true; } // empty dict, offsets ok
  if (!entriesArr) { l.verified = false; return false; }
  size_t maxLen = 0;
  readMem<size_t>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(entriesArr) + kArray_MaxLength), &maxLen);
  if (maxLen == 0 || maxLen > 100000) { l.verified = false; return false; }

  const size_t scan = maxLen < 64 ? maxLen : 64;
  bool ok = false;
  for (size_t i = 0; i < scan; ++i) {
    const uintptr_t entryBase = reinterpret_cast<uintptr_t>(entriesArr) + kArray_Vector + i * l.entryStride;
    int hashCode = 0;
    if (!readMem<int>(reinterpret_cast<void*>(entryBase + l.entriesHash), &hashCode)) continue;
    if (hashCode < 0) continue;
    void* value = nullptr;
    if (!readMem<void*>(reinterpret_cast<void*>(entryBase + l.entriesValue), &value)) continue;
    if (value == nullptr || validPtr(value)) { ok = true; break; }
  }
  l.verified = ok;
  return l.verified;
}

inline size_t readDictionaryValues(void* dictObj, void** out, size_t outCap) {
  if (!dictObj || !out || outCap == 0) return 0;
  DictionaryLayout& l = dictLayout();
  if (!l.verified) {
    if (!resolveDictionaryLayoutAtRuntime(dictObj)) return 0;
  }

  int count = readIntField(dictObj, l.count);
  if (count <= 0 || count > 4096) return 0;
  void* entriesArr = readPtrField(dictObj, l.entries);
  if (!entriesArr) return 0;

  size_t maxLen = 0;
  readMem<size_t>(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(entriesArr) + kArray_MaxLength), &maxLen);
  if (maxLen == 0 || maxLen > 100000) return 0;

  const size_t scan = maxLen < 4096 ? maxLen : 4096;
  size_t written = 0;
  for (size_t i = 0; i < scan && written < outCap; ++i) {
    const uintptr_t entryBase = reinterpret_cast<uintptr_t>(entriesArr) + kArray_Vector + i * l.entryStride;
    int hashCode = 0;
    if (!readMem<int>(reinterpret_cast<void*>(entryBase + l.entriesHash), &hashCode)) continue;
    if (hashCode < 0) continue; // empty/deleted slot
    void* value = nullptr;
    if (!readMem<void*>(reinterpret_cast<void*>(entryBase + l.entriesValue), &value)) continue;
    if (!validPtr(value)) continue;
    out[written++] = value;
  }
  return written;
}

struct EntityView {
  void* entity = nullptr;
  void* clientModel = nullptr;
  void* serverModel = nullptr;
  void* transform = nullptr; // TODO(runtime: resolve player/character transform source)
  int id = 0;
  float health = 0.f;       // TODO(runtime: resolve health source; BattleCharacterPrivateModel.Health at 0x1B0 dump.cs:195155, chain unresolved)
  bool isDead = false;      // TODO(runtime)
  int team = 0;             // TODO(runtime: resolve team/faction if present)
  bool local = false;
};

struct EntityCache {
  EntityView entries[512];
  size_t count = 0;
  ULONGLONG tick = 0;
};

inline EntityCache& entityCache() { static EntityCache c; return c; }

inline void* getLocalPlayer(void* battleEntitiesModel) {
  if (!battleEntitiesModel) return nullptr;
  return readPtrField(battleEntitiesModel, off::kBattleEntitiesModel_Player);
}

// Resolve BattleEntitiesModel from BattleModel instance if known.
// TODO(runtime: locate BattleModel root instance; no static singleton in dump.cs:239134).
inline void* getBattleEntitiesModel(void* battleModel) {
  if (!battleModel) return nullptr;
  // BattleModel.<BattleEntitiesModel>k__BackingField at dump.cs:239166 offset 0x90
  return readPtrField(battleModel, 0x90);
}

inline bool fillEntityView(void* entity, bool isLocal, EntityView& v) {
  if (!entity) return false;
  v = {};
  v.entity = entity;
  v.local = isLocal;
  v.id = readIntField(entity, off::kEntityModel_Id);
  v.clientModel = readPtrField(entity, off::kPlayerEntityModel_ClientModel);
  v.serverModel = readPtrField(entity, off::kPlayerEntityModel_ServerModel);
  // Health/team/position/bones require runtime-confirmed chains; keep zero/TODO.
  // TODO(runtime: health - BattleCharacterPrivateModel.Health 0x1B0 dump.cs:195155, chain from entity unknown)
  // TODO(runtime: team/faction - no dump-evidenced field on PlayerEntityModel/CharacterClientModel)
  // TODO(runtime: position/transform - resolve via CharacterClientModel/Movement or Unity Transform component)
  return true;
}

inline size_t collectEntities(void* battleEntitiesModel, EntityView* out, size_t outCap) {
  if (!battleEntitiesModel || !out || outCap == 0) return 0;
  size_t written = 0;

  void* local = getLocalPlayer(battleEntitiesModel);
  if (local && written < outCap) {
    fillEntityView(local, true, out[written]);
    ++written;
  }

  // Walk _models Dictionary<int, IEntityModel> at kBattleEntitiesModel_ModelsDict (dump.cs:238467).
  void* dict = readPtrField(battleEntitiesModel, off::kBattleEntitiesModel_ModelsDict);
  if (dict) {
    void* vals[512]{};
    const size_t n = readDictionaryValues(dict, vals, 512);
    for (size_t i = 0; i < n && written < outCap; ++i) {
      void* e = vals[i];
      if (!e || e == local) continue;
      // Basic validity: must be readable entity with id field
      int id = readIntField(e, off::kEntityModel_Id);
      (void)id;
      fillEntityView(e, false, out[written]);
      ++written;
    }
  }

  // Update cache
  EntityCache& c = entityCache();
  c.count = written < 512 ? written : 512;
  for (size_t i = 0; i < c.count; ++i) c.entries[i] = out[i];
  c.tick = GetTickCount64();
  return written;
}

inline const EntityCache& getCachedEntities(ULONGLONG maxAgeMs = 250) {
  EntityCache& c = entityCache();
  const ULONGLONG now = GetTickCount64();
  if (c.count > 0 && (now - c.tick) <= maxAgeMs) return c;
  return c; // stale allowed; caller can trigger refresh
}

} // namespace rb::ent
