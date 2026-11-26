#pragma once

// il2cpp_wrapper.hpp — typed IL2CPP resolver/call surface.
// Contract:
// - no hardcoded absolute addresses; module base + RVA or export resolve only.
// - walk PEB->Ldr for module base, EnumProcessModulesEx fallback.
// - resolve RVA first, export table second.
// - required exports: il2cpp_domain_get, il2cpp_domain_assembly_open,
//   il2cpp_class_from_name, il2cpp_class_get_method_from_name.
// - every dereference/range checked before use.

#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include <intrin.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "Psapi.lib")

namespace rb::il2cpp {

struct Il2CppDomain {};
struct Il2CppAssembly {};
struct Il2CppImage {};
struct Il2CppClass {};
struct Il2CppMethod {};
struct Il2CppField {};
struct Il2CppType {};

using DomainGetFn = Il2CppDomain*(__cdecl*)();
using DomainGetAssembliesFn = Il2CppAssembly**(__cdecl*)(Il2CppDomain*, size_t*);
using DomainAssemblyOpenFn = Il2CppAssembly*(__cdecl*)(Il2CppDomain*, const char*);
using AssemblyGetImageFn = Il2CppImage*(__cdecl*)(Il2CppAssembly*);
using ImageGetClassCountFn = size_t(__cdecl*)(Il2CppImage*);
using ImageGetClassFn = Il2CppClass*(__cdecl*)(Il2CppImage*, size_t);
using ClassFromNameFn = Il2CppClass*(__cdecl*)(Il2CppImage*, const char*, const char*);
using ClassGetMethodFromNameFn = Il2CppMethod*(__cdecl*)(Il2CppClass*, const char*, int);
using ClassGetFieldFromNameFn = Il2CppField*(__cdecl*)(Il2CppClass*, const char*);
using FieldGetOffsetFn = size_t(__cdecl*)(Il2CppField*);
using FieldStaticGetValueFn = void(__cdecl*)(Il2CppField*, void*);
using MethodGetNameFn = const char*(__cdecl*)(Il2CppMethod*);
using ClassGetNameFn = const char*(__cdecl*)(Il2CppClass*);
using ClassGetNamespaceFn = const char*(__cdecl*)(Il2CppClass*);

struct Api {
  HMODULE module = nullptr;

  DomainGetFn domain_get = nullptr;
  DomainGetAssembliesFn domain_get_assemblies = nullptr;
  DomainAssemblyOpenFn domain_assembly_open = nullptr;
  AssemblyGetImageFn assembly_get_image = nullptr;
  ImageGetClassCountFn image_get_class_count = nullptr;
  ImageGetClassFn image_get_class = nullptr;
  ClassFromNameFn class_from_name = nullptr;
  ClassGetMethodFromNameFn class_get_method_from_name = nullptr;
  ClassGetFieldFromNameFn class_get_field_from_name = nullptr;
  FieldGetOffsetFn field_get_offset = nullptr;
  FieldStaticGetValueFn field_static_get_value = nullptr;
  MethodGetNameFn method_get_name = nullptr;
  ClassGetNameFn class_get_name = nullptr;
  ClassGetNamespaceFn class_get_namespace = nullptr;

  bool ok() const {
    return domain_get && domain_assembly_open && class_from_name &&
           class_get_method_from_name;
  }
};

inline size_t imageSizeOf(HMODULE module) {
  const uintptr_t base = reinterpret_cast<uintptr_t>(module);
  if (!base) return 0;
  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
  const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
  if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
  if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) return 0;
  return nt->OptionalHeader.SizeOfImage;
}

inline bool rvaInImage(uintptr_t base, size_t imageSize, uintptr_t rva, size_t bytes) {
  (void)base;
  return rva <= imageSize && bytes <= imageSize - rva;
}

inline bool nameAtRvaEquals(uintptr_t base, size_t imageSize, uintptr_t rva, const char* wanted) {
  if (!rvaInImage(base, imageSize, rva, 1) || !wanted) return false;
  const char* text = reinterpret_cast<const char*>(base + rva);
  const size_t remaining = imageSize - rva;
  for (size_t i = 0; i < remaining; ++i) {
    if (text[i] == '\0') return std::strcmp(text, wanted) == 0;
  }
  return false;
}

inline void* resolveForwarder(HMODULE module, const char* forwarder) {
  const char* dot = std::strchr(forwarder, '.');
  if (!dot || dot == forwarder) return nullptr;
  char dllName[MAX_PATH]{};
  const size_t dllLen = static_cast<size_t>(dot - forwarder);
  if (dllLen >= sizeof(dllName)) return nullptr;
  std::memcpy(dllName, forwarder, dllLen);
  dllName[dllLen] = '\0';
  HMODULE target = GetModuleHandleA(dllName);
  if (!target) target = LoadLibraryA(dllName);
  return target ? reinterpret_cast<void*>(GetProcAddress(target, dot + 1)) : nullptr;
}

inline void* findExportInModule(HMODULE module, const char* wanted) {
  const uintptr_t base = reinterpret_cast<uintptr_t>(module);
  const size_t imageSize = imageSizeOf(module);
  if (!base || !imageSize || !wanted) return nullptr;

  const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
  const IMAGE_DATA_DIRECTORY& exportDir =
      nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
  if (!exportDir.VirtualAddress || !exportDir.Size) return nullptr;
  if (!rvaInImage(base, imageSize, exportDir.VirtualAddress, exportDir.Size)) return nullptr;

  const auto* exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + exportDir.VirtualAddress);
  if (!exports->AddressOfFunctions || !exports->AddressOfNames || !exports->AddressOfNameOrdinals ||
      !exports->NumberOfFunctions || !exports->NumberOfNames) return nullptr;

  const DWORD* nameRvas = reinterpret_cast<const DWORD*>(base + exports->AddressOfNames);
  const WORD* ordinals = reinterpret_cast<const WORD*>(base + exports->AddressOfNameOrdinals);
  const DWORD* functions = reinterpret_cast<const DWORD*>(base + exports->AddressOfFunctions);
  for (DWORD i = 0; i < exports->NumberOfNames; ++i) {
    if (!nameAtRvaEquals(base, imageSize, nameRvas[i], wanted)) continue;
    const WORD ordinal = ordinals[i];
    if (ordinal >= exports->NumberOfFunctions) return nullptr;
    const DWORD functionRva = functions[ordinal];
    if (!functionRva || !rvaInImage(base, imageSize, functionRva, 1)) return nullptr;
    if (functionRva >= exportDir.VirtualAddress &&
        functionRva < exportDir.VirtualAddress + exportDir.Size) {
      return resolveForwarder(module, reinterpret_cast<const char*>(base + functionRva));
    }
    return reinterpret_cast<void*>(base + functionRva);
  }
  return nullptr;
}

inline HMODULE moduleByNamePebLdr(const wchar_t* wanted) {
  if (!wanted) return nullptr;
#ifdef _M_X64
  auto* peb = reinterpret_cast<PTEB>(__readgsqword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)))->ProcessEnvironmentBlock;
#else
  auto* peb = reinterpret_cast<PTEB>(__readfsdword(reinterpret_cast<DWORD_PTR>(&static_cast<NT_TIB*>(nullptr)->Self)))->ProcessEnvironmentBlock;
#endif
  if (!peb || !peb->Ldr) return nullptr;
  auto* ldr = peb->Ldr;
  auto* head = &ldr->InMemoryOrderModuleList;
  for (auto* entry = head->Flink; entry && entry != head; entry = entry->Flink) {
    auto* mod = CONTAINING_RECORD(entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    if (!mod->BaseDllName.Buffer) continue;
    if (_wcsicmp(mod->BaseDllName.Buffer, wanted) == 0) return reinterpret_cast<HMODULE>(mod->DllBase);
  }
  return nullptr;
}

inline HMODULE moduleByNameEnumProcessModules(const wchar_t* wanted) {
  if (!wanted) return nullptr;
  HMODULE modules[1024]{};
  DWORD needed = 0;
  if (!EnumProcessModulesEx(GetCurrentProcess(), modules, sizeof(modules), &needed, LIST_MODULES_ALL)) return nullptr;
  const size_t count = (needed / sizeof(HMODULE) < 1024) ? (needed / sizeof(HMODULE)) : 1024;
  for (size_t i = 0; i < count; ++i) {
    wchar_t name[MAX_PATH]{};
    if (!GetModuleBaseNameW(GetCurrentProcess(), modules[i], name, MAX_PATH)) continue;
    if (_wcsicmp(name, wanted) == 0) return modules[i];
  }
  return nullptr;
}

inline HMODULE moduleByName(const wchar_t* wanted) {
  if (HMODULE m = moduleByNamePebLdr(wanted)) return m;
  return moduleByNameEnumProcessModules(wanted);
}

inline uintptr_t moduleBaseByName(const wchar_t* name) {
  return reinterpret_cast<uintptr_t>(moduleByName(name));
}

inline void* resolveRva(HMODULE module, uintptr_t rva) {
  const uintptr_t base = reinterpret_cast<uintptr_t>(module);
  const size_t imageSize = imageSizeOf(module);
  if (!base || !rva) return nullptr;
  if (!rvaInImage(base, imageSize, rva, 1)) return nullptr;
  return reinterpret_cast<void*>(base + rva);
}

inline void* resolveExportOrRva(HMODULE module, const char* exportName, uintptr_t rva) {
  if (!module) return nullptr;
  if (void* byRva = resolveRva(module, rva)) return byRva; // RVA first
  return findExportInModule(module, exportName);            // export table second
}

inline Api buildApi() {
  Api a;
  a.module = moduleByName(L"GameAssembly.dll");
  if (!a.module) a.module = moduleByName(L"UnityPlayer.dll");

  a.domain_get = reinterpret_cast<DomainGetFn>(resolveExportOrRva(a.module, "il2cpp_domain_get", 0));
  a.domain_get_assemblies = reinterpret_cast<DomainGetAssembliesFn>(resolveExportOrRva(a.module, "il2cpp_domain_get_assemblies", 0));
  a.domain_assembly_open = reinterpret_cast<DomainAssemblyOpenFn>(resolveExportOrRva(a.module, "il2cpp_domain_assembly_open", 0));
  a.assembly_get_image = reinterpret_cast<AssemblyGetImageFn>(resolveExportOrRva(a.module, "il2cpp_assembly_get_image", 0));
  a.image_get_class_count = reinterpret_cast<ImageGetClassCountFn>(resolveExportOrRva(a.module, "il2cpp_image_get_class_count", 0));
  a.image_get_class = reinterpret_cast<ImageGetClassFn>(resolveExportOrRva(a.module, "il2cpp_image_get_class", 0));
  a.class_from_name = reinterpret_cast<ClassFromNameFn>(resolveExportOrRva(a.module, "il2cpp_class_from_name", 0));
  a.class_get_method_from_name = reinterpret_cast<ClassGetMethodFromNameFn>(resolveExportOrRva(a.module, "il2cpp_class_get_method_from_name", 0));
  a.class_get_field_from_name = reinterpret_cast<ClassGetFieldFromNameFn>(resolveExportOrRva(a.module, "il2cpp_class_get_field_from_name", 0));
  a.field_get_offset = reinterpret_cast<FieldGetOffsetFn>(resolveExportOrRva(a.module, "il2cpp_field_get_offset", 0));
  a.field_static_get_value = reinterpret_cast<FieldStaticGetValueFn>(resolveExportOrRva(a.module, "il2cpp_field_static_get_value", 0));
  a.method_get_name = reinterpret_cast<MethodGetNameFn>(resolveExportOrRva(a.module, "il2cpp_method_get_name", 0));
  a.class_get_name = reinterpret_cast<ClassGetNameFn>(resolveExportOrRva(a.module, "il2cpp_class_get_name", 0));
  a.class_get_namespace = reinterpret_cast<ClassGetNamespaceFn>(resolveExportOrRva(a.module, "il2cpp_class_get_namespace", 0));

  if (!a.ok()) OutputDebugStringA("[rb] il2cpp API incomplete; required exports missing.\n");
  return a;
}

inline const Api& api() {
  static const Api resolved = buildApi();
  return resolved;
}

inline Il2CppDomain* getDomain() {
  const Api& a = api();
  return a.domain_get ? a.domain_get() : nullptr;
}

inline Il2CppAssembly* openAssembly(Il2CppDomain* domain, const char* assemblyName) {
  const Api& a = api();
  if (!a.ok() || !domain || !assemblyName) return nullptr;
  return a.domain_assembly_open(domain, assemblyName);
}

inline Il2CppImage* imageFromAssembly(Il2CppAssembly* assembly) {
  const Api& a = api();
  if (!a.assembly_get_image || !assembly) return nullptr;
  return a.assembly_get_image(assembly);
}

inline Il2CppClass* classFromName(Il2CppImage* image, const char* ns, const char* className) {
  const Api& a = api();
  if (!a.class_from_name || !image || !ns || !className) return nullptr;
  return a.class_from_name(image, ns, className);
}

inline Il2CppMethod* classGetMethodFromName(Il2CppClass* klass, const char* name, int paramCount) {
  const Api& a = api();
  if (!a.class_get_method_from_name || !klass || !name || paramCount < 0) return nullptr;
  return a.class_get_method_from_name(klass, name, paramCount);
}

inline Il2CppField* classGetFieldFromName(Il2CppClass* klass, const char* name) {
  const Api& a = api();
  if (!a.class_get_field_from_name || !klass || !name) return nullptr;
  return a.class_get_field_from_name(klass, name);
}

inline size_t fieldGetOffset(Il2CppField* field) {
  const Api& a = api();
  if (!a.field_get_offset || !field) return 0;
  return a.field_get_offset(field);
}

inline void* methodPointerFromIl2CppMethod(Il2CppMethod* m) {
  if (!m) return nullptr;
  // Il2CppMethod/MethodInfo layout is version-dependent. methodPointer is commonly at +0x0.
  // TODO(runtime: "Il2CppMethod methodPointer offset") verify for Unity 6000.x IL2CPP build.
  void* ptr = nullptr;
  __try { ptr = *reinterpret_cast<void**>(m); } __except (EXCEPTION_EXECUTE_HANDLER) { ptr = nullptr; }
  if (!ptr) return nullptr;
  // Sanity: pointer should be inside GameAssembly image when resolved from method info.
  if (HMODULE ga = moduleByName(L"GameAssembly.dll")) {
    const uintptr_t base = reinterpret_cast<uintptr_t>(ga);
    const size_t sz = imageSizeOf(ga);
    const uintptr_t p = reinterpret_cast<uintptr_t>(ptr);
    if (p >= base && p < base + sz) return ptr;
  }
  // Fallback: accept non-null pointer; caller should prefer fallbackRva on mismatch.
  return ptr;
}

template <typename Fn>
inline Fn resolveMethod(Il2CppClass* klass, const char* name, int paramCount, uintptr_t fallbackRva = 0) {
  if (Il2CppMethod* m = classGetMethodFromName(klass, name, paramCount)) {
    void* methodPointer = methodPointerFromIl2CppMethod(m);
    if (methodPointer) return reinterpret_cast<Fn>(methodPointer);
  }
  if (fallbackRva) {
    if (HMODULE mod = moduleByName(L"GameAssembly.dll")) {
      if (void* p = resolveRva(mod, fallbackRva)) return reinterpret_cast<Fn>(p);
    }
  }
  return nullptr;
}

inline bool readPtr(void* obj, uintptr_t off, void** out) {
  if (!obj || !out) return false;
  __try {
    *out = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(obj) + off);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    *out = nullptr;
    return false;
  }
}

template <typename T>
inline bool readValue(void* obj, uintptr_t off, T* out) {
  if (!obj || !out) return false;
  __try {
    *out = *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + off);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    *out = T{};
    return false;
  }
}

template <typename T>
inline bool writeValue(void* obj, uintptr_t off, const T& value) {
  if (!obj) return false;
  __try {
    *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + off) = value;
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

// ---------------------------------------------------------------------------
// Class/image resolution cache
// ---------------------------------------------------------------------------
struct ClassKey {
  const char* ns;
  const char* name;
  Il2CppClass* klass;
};

inline constexpr size_t kMaxCachedClasses = 64;
inline ClassKey& classCacheSlot(size_t i) { static ClassKey slots[kMaxCachedClasses]{}; return slots[i]; }
inline size_t& classCacheCount() { static size_t n = 0; return n; }

inline Il2CppImage* getAssemblyImage(const char* assemblyName) {
  Il2CppDomain* d = getDomain();
  if (!d) return nullptr;
  Il2CppAssembly* as = openAssembly(d, assemblyName);
  if (!as) return nullptr;
  return imageFromAssembly(as);
}

inline Il2CppClass* findClass(const char* ns, const char* name) {
  if (!ns || !name) return nullptr;
  // cache lookup
  for (size_t i = 0; i < classCacheCount(); ++i) {
    ClassKey& k = classCacheSlot(i);
    if (k.klass && k.ns && k.name && std::strcmp(k.ns, ns) == 0 && std::strcmp(k.name, name) == 0) return k.klass;
  }

  const Api& a = api();
  Il2CppClass* found = nullptr;

  // Try common assembly names in order.
  static const char* kAssemblies[] = {"Assembly-CSharp", "Assembly-CSharp.dll", "UnityEngine.CoreModule", "UnityEngine.CoreModule.dll", "UnityEngine.PhysicsModule", "UnityEngine.PhysicsModule.dll"};
  for (const char* an : kAssemblies) {
    Il2CppImage* img = getAssemblyImage(an);
    if (!img) continue;
    found = classFromName(img, ns, name);
    if (found) break;
  }

  // Fallback: iterate all assemblies/images if domain_get_assemblies available.
  if (!found && a.domain_get_assemblies && a.assembly_get_image && a.image_get_class_count && a.image_get_class && a.class_get_name) {
    Il2CppDomain* d = getDomain();
    size_t count = 0;
    Il2CppAssembly** asms = d ? a.domain_get_assemblies(d, &count) : nullptr;
    for (size_t i = 0; asms && i < count && !found; ++i) {
      Il2CppImage* img = a.assembly_get_image(asms[i]);
      if (!img) continue;
      const size_t cc = a.image_get_class_count(img);
      for (size_t c = 0; c < cc; ++c) {
        Il2CppClass* kl = a.image_get_class(img, c);
        if (!kl) continue;
        const char* cn = a.class_get_name(kl);
        const char* cns = a.class_get_namespace ? a.class_get_namespace(kl) : "";
        if (!cn) continue;
        if (std::strcmp(cn, name) != 0) continue;
        if (cns && std::strcmp(cns, ns) == 0) { found = kl; break; }
      }
    }
  }

  if (found && classCacheCount() < kMaxCachedClasses) {
    ClassKey& k = classCacheSlot(classCacheCount()++);
    k.ns = ns; k.name = name; k.klass = found;
  }
  return found;
}

inline Il2CppMethod* findMethod(Il2CppClass* klass, const char* name, int paramCount) {
  return classGetMethodFromName(klass, name, paramCount);
}

template <typename Fn>
inline Fn resolveMethodCached(const char* ns, const char* cls, const char* method, int paramCount, uintptr_t fallbackRva = 0) {
  Il2CppClass* k = findClass(ns, cls);
  if (k) {
    if (Fn f = resolveMethod<Fn>(k, method, paramCount, fallbackRva)) return f;
  }
  if (fallbackRva) {
    if (HMODULE mod = moduleByName(L"GameAssembly.dll")) {
      if (void* p = resolveRva(mod, fallbackRva)) return reinterpret_cast<Fn>(p);
    }
  }
  return nullptr;
}

inline void* getStaticFieldValue(Il2CppClass* klass, const char* fieldName, void* out, size_t outSize) {
  const Api& a = api();
  if (!klass || !fieldName || !out || outSize == 0 || !a.class_get_field_from_name || !a.field_static_get_value) return nullptr;
  Il2CppField* f = a.class_get_field_from_name(klass, fieldName);
  if (!f) return nullptr;
  std::memset(out, 0, outSize);
  __try {
    a.field_static_get_value(f, out);
    return out;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    std::memset(out, 0, outSize);
    return nullptr;
  }
}

// ---------------------------------------------------------------------------
// Method cache
// ---------------------------------------------------------------------------
struct MethodKey {
  Il2CppClass* klass = nullptr;
  const char* name = nullptr;
  int paramCount = -1;
  void* fn = nullptr;
};
inline constexpr size_t kMaxCachedMethods = 128;
inline MethodKey& methodCacheSlot(size_t i) { static MethodKey slots[kMaxCachedMethods]{}; return slots[i]; }
inline size_t& methodCacheCount() { static size_t n = 0; return n; }

template <typename Fn>
inline Fn resolveMethodCachedByClass(Il2CppClass* klass, const char* name, int paramCount, uintptr_t fallbackRva = 0) {
  if (!klass || !name) return nullptr;
  for (size_t i = 0; i < methodCacheCount(); ++i) {
    MethodKey& k = methodCacheSlot(i);
    if (k.klass == klass && k.paramCount == paramCount && k.name && std::strcmp(k.name, name) == 0 && k.fn) {
      return reinterpret_cast<Fn>(k.fn);
    }
  }
  Fn f = resolveMethod<Fn>(klass, name, paramCount, fallbackRva);
  if (f && methodCacheCount() < kMaxCachedMethods) {
    MethodKey& k = methodCacheSlot(methodCacheCount()++);
    k.klass = klass; k.name = name; k.paramCount = paramCount; k.fn = reinterpret_cast<void*>(f);
  }
  return f;
}

// ---------------------------------------------------------------------------
// Typed invocation helpers (SEH-guarded). IL2CPP instance methods take `this`
// as first arg; static methods take no this.
// ---------------------------------------------------------------------------
template <typename Ret, typename... Args>
inline bool invokeSafe(Ret* out, void* fnPtr, Args... args) {
  if (!fnPtr) return false;
  using Fn = Ret(*)(Args...);
  __try {
    Ret r = reinterpret_cast<Fn>(fnPtr)(args...);
    if (out) *out = r;
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    if (out) *out = Ret{};
    return false;
  }
}

template <typename... Args>
inline bool invokeVoidSafe(void* fnPtr, Args... args) {
  if (!fnPtr) return false;
  using Fn = void(*)(Args...);
  __try { reinterpret_cast<Fn>(fnPtr)(args...); return true; }
  __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// ---------------------------------------------------------------------------
// Minimal Unity structs for safe wrapper signatures (layout must match IL2CPP)
// ---------------------------------------------------------------------------
struct Vec3 { float x, y, z; };
struct Vec2 { float x, y; };
struct Matrix4x4 { float m[16]; };
struct Ray { Vec3 origin; Vec3 direction; };
// RaycastHit layout is Unity-version dependent; 64 bytes buffer is conservative for Unity 6.
// TODO(runtime: confirm sizeof(RaycastHit) for Unity 6000.x IL2CPP build).
struct RaycastHitBuf { unsigned char data[64]{}; };

// ---------------------------------------------------------------------------
// Safe wrappers (RVA/module-relative fallbacks from offsets.hpp)
// ---------------------------------------------------------------------------
inline void* cameraGetMain() {
  using Fn = void* (*)();
  static Fn fn = nullptr;
  if (!fn) fn = resolveMethodCached<Fn>("UnityEngine", "Camera", "get_main", 0, 0x623C5F0); // dump.cs:1086030
  void* out = nullptr;
  invokeSafe<void*>(&out, reinterpret_cast<void*>(fn));
  return out;
}

inline void* cameraGetCurrent() {
  using Fn = void* (*)();
  static Fn fn = nullptr;
  if (!fn) fn = resolveMethodCached<Fn>("UnityEngine", "Camera", "get_current", 0, 0x623BA90); // dump.cs:1086033
  void* out = nullptr;
  invokeSafe<void*>(&out, reinterpret_cast<void*>(fn));
  return out;
}

inline bool cameraWorldToCameraMatrix(void* camera, Matrix4x4* out) {
  if (!camera || !out) return false;
  using Fn = Matrix4x4(*)(void*);
  static Fn fn = nullptr;
  if (!fn) fn = resolveMethodCached<Fn>("UnityEngine", "Camera", "get_worldToCameraMatrix", 0, 0x623D870); // dump.cs:1085912
  return invokeSafe<Matrix4x4>(out, reinterpret_cast<void*>(fn), camera);
}

inline bool cameraProjectionMatrix(void* camera, Matrix4x4* out) {
  if (!camera || !out) return false;
  using Fn = Matrix4x4(*)(void*);
  static Fn fn = nullptr;
  if (!fn) fn = resolveMethodCached<Fn>("UnityEngine", "Camera", "get_projectionMatrix", 0, 0x623CF60); // dump.cs:1085918
  return invokeSafe<Matrix4x4>(out, reinterpret_cast<void*>(fn), camera);
}

inline bool physicsRaycast(Vec3 origin, Vec3 direction, RaycastHitBuf* hitInfo, float maxDistance, int layerMask, int queryTriggerInteraction) {
  // Signature: static bool Raycast(Vector3 origin, Vector3 direction, out RaycastHit hitInfo, float maxDistance, int layerMask, QueryTriggerInteraction queryTriggerInteraction)
  // dump.cs:2010093 RVA:0x63B1140
  using Fn = bool(*)(Vec3, Vec3, RaycastHitBuf*, float, int, int);
  static Fn fn = nullptr;
  if (!fn) fn = resolveMethodCached<Fn>("UnityEngine", "Physics", "Raycast", 6, 0x63B1140); // dump.cs:2010093
  bool hit = false;
  if (!invokeSafe<bool>(&hit, reinterpret_cast<void*>(fn), origin, direction, hitInfo, maxDistance, layerMask, queryTriggerInteraction)) return false;
  return hit;
}

} // namespace rb::il2cpp
