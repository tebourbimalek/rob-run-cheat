#pragma once

// ---------------------------------------------------------------------------
// unity_types.hpp — il2cpp runtime struct layouts used by the cheat.
//
// These are copies of the IL2CPP type layouts (verified against the stock
// RuntimeClass structs in GameAssembly.dll via the metadata dump). Every field
// offset is written as { 0xNN } /* field */ and re-stamped after a patch.
//
// IL2CPP_FIELDNAME = field name in dump.cs; values are the struct field offsets.
// NOTE: il2cpp-typed classes inherit Object (16 bytes: vtable ptr + klass +
// synchronized bitfield + padding), and every class ref adds 8 bytes.
// ---------------------------------------------------------------------------

#include <cstdint>

namespace rb {

// ---- stock UnityEngine.Object (IL2CPP) -------------------------------------
struct Il2CppObject {  // UnityEngine.Object
  void* vtable = nullptr;     // +0x00
  void* klass = nullptr;      // +0x08
  uint8_t sync = 0;           // +0x10
  uint8_t pad[7] = {};        // +0x11 (struct packing)
};

// ---- stock UnityEngine.Vector3 ----------------------------------------------
struct Vec3 {
  float x = 0.f, y = 0.f, z = 0.f;
  bool operator==(const Vec3& o) const {
    return x == o.x && y == o.y && z == o.z;
  }
};
static_assert(sizeof(Vec3) == 12, "Vec3 must be 12 bytes");

// ---- UnityEngine.Transform + Renderer/SPline helpers (GameObject level) -----
// UnityEngine.GameObject -> components TArray (this is stored on Object)
struct TransformChain {
  void* gameObject = nullptr;
  void* localToWorld = nullptr;  // (stock; not used directly)
  // Stock: UnityEngine.Transform extends UnityEngine.Component.
};

// ---- UnityEngine.Camera -----------------------------------------------------
struct UnityCamera {
  uint8_t pad[16];            // TODO (pad to UnityEngine.Object)
  void* mainTransform;       // TODO — UnityEngine.Camera::main accessor
  float fov;                  // TODO
  float focalLength;          // TODO
};

// ---- stock UnityEngine.RaycastHit --------------------------------------------
// (layout from dump.cs — class UnityEngine.RaycastHit(exposed))
struct RaycastHit {
  Vec3 point;
  Vec3 normal;
  void* collider;   // UnityEngine.Collider*
  uint32_t faceIndex;
  uint8_t pad[24];
};
static_assert(sizeof(RaycastHit) >= 44, "RaycastHit must fit");

// ---- UnityEngine.Physics ------------------------------------------------------
// We don't call physics statically; Raycast goes through the wrapper layer.

// ---- Rob (the game) types -----------------------------------------------------
// Generated from Il2Dumper — every field has a TODO until the dump lands.
// Copy verbatim from dump.cs once available; do NOT invent.

struct GameObjectWithRig : Il2CppObject {
  void* rootComponent;        // TODO — game RootComponent
};

struct BotEntity : Il2CppObject {
  void* playerState;          // TODO — game PlayerState
  void* rootComponent;        // TODO — game RootComponent
  Vec3 position;              // TODO — read from rootComponent->translation
  void* healthPack;           // TODO — game HealthPack (for distance/damage)
};

// ---- PlayerCamera + FOV -------------------------------------------------------
struct PlayerCamera : Il2CppObject {
  void* transform;            // TODO — UnityEngine.Transform
  float fov = 0.f;            // TODO — game fov accessor
  float nearDist = 0.01f;     // TODO
  float farDist = 1500.f;     // TODO
  void* portalCameras;        // TODO — stub
};

struct CameraCacheEntry {
  Vec3 pos = {};              // TODO — FMinimalViewInfo::Position
  Vec3 dir = {};              // TODO — FMinimalViewInfo::Rotation dir
  Vec3 up = {};               // TODO — camera up
  float fov = 0.f;            // TODO
};

}  // namespace rb
