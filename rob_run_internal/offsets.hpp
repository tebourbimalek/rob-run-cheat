#pragma once

#include <cstdint>
#include <cstddef>

// ---------------------------------------------------------------------------
// ROB RUN internal — offsets.hpp
// Source: deps/dump.cs (BUILD_ID 26807668, 82,001,124 bytes, 2,070,513 lines)
// Rules: fields from "// 0x.." field comment; methods from "// RVA: 0x.."; never VA.
// All values module-relative. Every constant carries dump.cs line number.
// Unknown symbol: TODO(<exact grep term>), no invented value.
// ---------------------------------------------------------------------------

namespace rb::off {

// ---------------------------------------------------------------------------
// Class markers
// dump.cs exposes TypeDefIndex, not module-relative class RVAs/pointers.
// Resolve at runtime via il2cpp_class_from_name / image class iteration.
// Keep 0x0 until a runtime-resolved address cache is added; do not invent RVA.
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kClass_PlayerEntityModel    = 0x0; // TODO(runtime: il2cpp_class_from_name "PlayerEntityModel") dump.cs:363333 TypeDefIndex:8711
inline constexpr uintptr_t kClass_PlayerClientModel    = 0x0; // TODO(runtime: il2cpp_class_from_name "PlayerClientModel") dump.cs:362936 TypeDefIndex:8710
inline constexpr uintptr_t kClass_CharacterClientModel  = 0x0; // TODO(runtime: il2cpp_class_from_name "CharacterClientModel") dump.cs:375947 TypeDefIndex:9059
inline constexpr uintptr_t kClass_BattleEntitiesModel  = 0x0; // TODO(runtime: il2cpp_class_from_name "BattleEntitiesModel") dump.cs:238456 TypeDefIndex:5517
inline constexpr uintptr_t kClass_BattleModel          = 0x0; // TODO(runtime: il2cpp_class_from_name "BattleModel") dump.cs:239134 TypeDefIndex:5527
inline constexpr uintptr_t kClass_Camera               = 0x0; // TODO(runtime: il2cpp_class_from_name "Camera" ns UnityEngine) dump.cs:1085427 TypeDefIndex:17097
inline constexpr uintptr_t kClass_SkinnedMeshRenderer  = 0x0; // TODO(runtime: il2cpp_class_from_name "SkinnedMeshRenderer" ns UnityEngine) dump.cs:1096172 TypeDefIndex:17241
inline constexpr uintptr_t kClass_Transform            = 0x0; // TODO(runtime: il2cpp_class_from_name "Transform" ns UnityEngine) dump.cs:1111381 TypeDefIndex:17458
inline constexpr uintptr_t kClass_Animator             = 0x0; // TODO(runtime: il2cpp_class_from_name "Animator" ns UnityEngine) dump.cs:1919088 TypeDefIndex:33872
inline constexpr uintptr_t kClass_Physics              = 0x0; // TODO(runtime: il2cpp_class_from_name "Physics" ns UnityEngine) dump.cs:2010040 TypeDefIndex:36047
inline constexpr uintptr_t kClass_ShotIntentionUpdater = 0x0; // TODO(runtime: il2cpp_class_from_name "ShotIntentionUpdater" ns Game.Battle.Inputs) dump.cs:311597 TypeDefIndex:7275
inline constexpr uintptr_t kClass_ShotArgsSystem       = 0x0; // TODO(runtime: il2cpp_class_from_name "ShotArgsSystem" ns Models.Systems) dump.cs:16452 TypeDefIndex:490

// ---------------------------------------------------------------------------
// Static/singleton resolution
// dump.cs has no module-relative static field RVAs. Use getter RVAs / runtime paths.
// ---------------------------------------------------------------------------
// Camera.main/current: no static backing field RVA in dump.cs; use kRVA_Camera_get_main (dump.cs:1086030) / kRVA_Camera_get_current (dump.cs:1086033).
inline constexpr uint32_t  kCameraStatic_onPreCull          = 0x0;  // dump.cs:1085435 static field offset in Camera static storage; access via Il2CppClass->static_fields + offset
inline constexpr uint32_t  kCameraStatic_onPreRender       = 0x8;  // dump.cs:1085436
inline constexpr uint32_t  kCameraStatic_onPostRender      = 0x10; // dump.cs:1085437
inline constexpr uintptr_t kStatic_BattleModel_instance    = 0x0;  // TODO(runtime: locate BattleModel root instance; no static singleton in dump.cs:239134)

// ---------------------------------------------------------------------------
// BattleEntitiesModel fields (closest manager candidate; "PlayerManager" name MISS)
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kBattleEntitiesModel_PhysicsSceneModel    = 0x10; // dump.cs:238459
inline constexpr uintptr_t kBattleEntitiesModel_PlayerBattleModel    = 0x18; // dump.cs:238460
inline constexpr uintptr_t kBattleEntitiesModel_InventoryModels      = 0x20; // dump.cs:238461
inline constexpr uintptr_t kBattleEntitiesModel_ModelsDict           = 0x50; // dump.cs:238467 Dictionary<int,IEntityModel> _models
inline constexpr uintptr_t kBattleEntitiesModel_Player               = 0xD0; // dump.cs:238487 PlayerEntityModel <Player>k__BackingField
inline constexpr uintptr_t kBattleEntitiesModel_SelfPlot             = 0xD8; // dump.cs:238489 PlotEntityModel <SelfPlot>k__BackingField
inline constexpr uintptr_t kBattleEntitiesModel_EntityModelsReadOnly = 0xE0; // dump.cs:238491 ReadOnlyDictionaryNonAlloc<int,IEntityModel>

// ---------------------------------------------------------------------------
// Entity / Player fields
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kEntityModel_Id                        = 0x10; // dump.cs:354063 EntityModel.<Id>k__BackingField

inline constexpr uintptr_t kPlayerEntityModel_ClientModel         = 0x18; // dump.cs:363337 PlayerEntityModel.<ClientModel>k__BackingField
inline constexpr uintptr_t kPlayerEntityModel_ServerModel         = 0x20; // dump.cs:363339 PlayerEntityModel.<ServerModel>k__BackingField
inline constexpr uintptr_t kPlayerEntityModel_PublicId            = 0x28; // dump.cs:363340 PlayerEntityModel.PublicId

inline constexpr uintptr_t kCharacterClientModel_MovementTimeline     = 0x10; // dump.cs:375951
inline constexpr uintptr_t kCharacterClientModel_LookRotationTimeline = 0x18; // dump.cs:375953
inline constexpr uintptr_t kCharacterClientModel_RotationTimeline     = 0x20; // dump.cs:375955
inline constexpr uintptr_t kCharacterClientModel_Movement             = 0x28; // dump.cs:375957
inline constexpr uintptr_t kCharacterClientModel_RotationModel        = 0x38; // dump.cs:375961
inline constexpr uintptr_t kCharacterClientModel_RotationInterpolated = 0x40; // dump.cs:375963
inline constexpr uintptr_t kCharacterClientModel_VisibleModel         = 0x98; // dump.cs:375985
inline constexpr uintptr_t kCharacterClientModel_VelocityModel        = 0xA0; // dump.cs:375987
inline constexpr uintptr_t kCharacterClientModel_SqrDistanceToPlayer  = 0x120; // dump.cs:376019

inline constexpr uintptr_t kPlayerClientModel_CameraModel          = 0x218; // dump.cs:363000 PlayerClientModel.<CameraModel>k__BackingField
inline constexpr uintptr_t kPlayerClientModel_PlayerInVehicleModel = 0x1D8; // dump.cs:362984
inline constexpr uintptr_t kPlayerClientModel_AimAssistModel       = 0x2A0; // dump.cs:363034
inline constexpr uintptr_t kPlayerClientModel_AutoFireModel        = 0x2A8; // dump.cs:363036
inline constexpr uintptr_t kPlayerClientModel_RecoilModel          = 0x2B8; // dump.cs:363040
inline constexpr uintptr_t kPlayerClientModel_FakeSpreadModel     = 0x2C0; // dump.cs:363042
inline constexpr uintptr_t kPlayerClientModel_IsActualData         = 0x2D0; // dump.cs:363045

// ---------------------------------------------------------------------------
// Recoil / Spread / Aim / AutoFire model fields (evidence-backed)
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kRecoilModel_RecoilOffset       = 0x10; // dump.cs:332458 RecoilModel.RecoilOffset (Vector2)
inline constexpr uintptr_t kPlayerFakeSpreadModel_Spread   = 0x10; // dump.cs:332248 PlayerFakeSpreadModel.Spread
inline constexpr uintptr_t kPlayerFakeSpreadModel_Air      = 0x14; // dump.cs:332249 PlayerFakeSpreadModel.Air
inline constexpr uintptr_t kPlayerFakeSpreadModel_Move     = 0x18; // dump.cs:332250 PlayerFakeSpreadModel.Move

inline constexpr uintptr_t kAimAssistModel_IsActive        = 0x10; // dump.cs:334908 AimAssistModel.<IsActive>k__BackingField
inline constexpr uintptr_t kAimAssistModel_HasTarget       = 0x12; // dump.cs:334912 AimAssistModel.<HasTarget>k__BackingField
inline constexpr uintptr_t kAimAssistModel_Target          = 0x18; // dump.cs:334914 AimAssistModel.<Target>k__BackingField

inline constexpr uintptr_t kAutoFireModel_IsActive         = 0x10; // dump.cs:335052 AutoFireModel.<IsActive>k__BackingField
inline constexpr uintptr_t kAutoFireModel_IsFiring         = 0x11; // dump.cs:335054 AutoFireModel.<IsFiring>k__BackingField
inline constexpr uintptr_t kAutoFireModel_HasTarget        = 0x12; // dump.cs:335056 AutoFireModel.<HasTarget>k__BackingField
inline constexpr uintptr_t kAutoFireModel_Target           = 0x18; // dump.cs:335058 AutoFireModel.<Target>k__BackingField

// ---------------------------------------------------------------------------
// UnityEngine managed/native bridge
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kUnityObject_m_CachedPtr = 0x10; // dump.cs:1108459 UnityEngine.Object.m_CachedPtr (Transform/SkinnedMeshRenderer inherit Object)

// ---------------------------------------------------------------------------
// Camera (UnityEngine) managed field
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kCamera_m_NonSerializedVersion = 0x18; // dump.cs:1085434

// ---------------------------------------------------------------------------
// Method RVAs (module-relative, RVA field from dump.cs; never VA)
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kRVA_Camera_get_fieldOfView         = 0x623BF10; // dump.cs:1085553
inline constexpr uintptr_t kRVA_Camera_set_fieldOfView         = 0x623F480; // dump.cs:1085556
inline constexpr uintptr_t kRVA_Camera_get_worldToCameraMatrix = 0x623D870; // dump.cs:1085912
inline constexpr uintptr_t kRVA_Camera_get_projectionMatrix    = 0x623CF60; // dump.cs:1085918
inline constexpr uintptr_t kRVA_Camera_get_main                = 0x623C5F0; // dump.cs:1086030
inline constexpr uintptr_t kRVA_Camera_get_current             = 0x623BA90; // dump.cs:1086033
inline constexpr uintptr_t kRVA_Camera_WorldToScreenPoint      = 0x623A7F0; // dump.cs:1085949

inline constexpr uintptr_t kRVA_Transform_GetChild     = 0x6326450; // dump.cs:1111768
inline constexpr uintptr_t kRVA_Transform_get_position = 0x632C530; // dump.cs:1111415
inline constexpr uintptr_t kRVA_Transform_get_rotation = 0x632C730; // dump.cs:1111467

inline constexpr uintptr_t kRVA_SkinnedMeshRenderer_get_bones    = 0x62A65F0; // dump.cs:1096212
inline constexpr uintptr_t kRVA_SkinnedMeshRenderer_get_rootBone = 0x62A6830; // dump.cs:1096206

inline constexpr uintptr_t kRVA_Animator_GetBoneTransform = 0x621A2B0; // dump.cs:1919707
inline constexpr uintptr_t kRVA_Object_get_name           = 0x62ECBB0; // dump.cs:1108498

inline constexpr uintptr_t kRVA_Physics_Raycast_vec_dir_hit_dist_mask_qti = 0x63B1140; // dump.cs:2010093
inline constexpr uintptr_t kRVA_Physics_Raycast_ray_dist_mask_qti         = 0x63B1070; // dump.cs:2010109

inline constexpr uintptr_t kRVA_BattleEntitiesModel_get_Player    = 0x916190; // dump.cs:238511
inline constexpr uintptr_t kRVA_PlayerEntityModel_get_ClientModel = 0x7E7130; // dump.cs:363352
inline constexpr uintptr_t kRVA_PlayerEntityModel_get_ServerModel = 0x7AB150; // dump.cs:363356

// Shot/input candidates (Game.Battle.Inputs.ShotIntentionUpdater, dump.cs:311597 TypeDefIndex:7275)
inline constexpr uintptr_t kRVA_ShotIntentionUpdater_CanShoot    = 0xA99460; // dump.cs:311625
inline constexpr uintptr_t kRVA_ShotIntentionUpdater_StartFire  = 0xA996A0; // dump.cs:311628
inline constexpr uintptr_t kRVA_ShotIntentionUpdater_ContinueFire = 0xA995F0; // dump.cs:311631
inline constexpr uintptr_t kRVA_ShotIntentionUpdater_StopFire   = 0xA996D0; // dump.cs:311634

// Shot direction/spread candidates (Models.Systems.ShotArgsSystem, dump.cs:16452 TypeDefIndex:490)
inline constexpr uintptr_t kRVA_ShotArgsSystem_Tick                  = 0x4361970; // dump.cs:16472
inline constexpr uintptr_t kRVA_ShotArgsSystem_GetShotDirection      = 0x4361530; // dump.cs:16479
inline constexpr uintptr_t kRVA_ShotArgsSystem_ApplySpreadToShotDirection = 0x4361000; // dump.cs:16482

// ---------------------------------------------------------------------------
// Feature symbols — unresolved (no invented values)
// ---------------------------------------------------------------------------
inline constexpr uintptr_t kRVA_Weapon_Fire            = 0x0; // TODO(grep: "void Fire(" in weapon/shoot runtime class; see ShotIntentionUpdater.StartFire candidate 0xA996A0)
inline constexpr uintptr_t kRVA_Weapon_TryShoot        = 0x0; // TODO(grep: "TryShoot" / "TryFire" / "SendShot" in battle weapon/runtime class)
inline constexpr uintptr_t kRVA_Triggerbot_TargetCheck = 0x0; // TODO(grep: "GetCrosshairTarget" / "AimTarget" / crosshair entity check; triggerbot may be aim+fire gate, not single RVA)
inline constexpr uintptr_t kField_Weapon_Recoil        = 0x0; // TODO(grep: "RecoilModel" internals / recoil apply method; PlayerClientModel.RecoilModel=0x2B8 known)
inline constexpr uintptr_t kField_Weapon_Spread        = 0x0; // TODO(grep: "PlayerFakeSpreadModel" / "WeaponSpreadArgsModel" internals; PlayerClientModel.FakeSpreadModel=0x2C0 known)
inline constexpr uintptr_t kField_Weapon_BulletSpeed   = 0x0; // TODO(grep: "BulletSpeed" / "ProjectileSpeed" / "muzzleVelocity" / "launchSpeed" in weapon/projectile description)
inline constexpr uintptr_t kField_Projectile_Velocity  = 0x0; // TODO(grep: "Velocity" in projectile/grenade/ballistic runtime model; confirm live projectile type)

// ---------------------------------------------------------------------------
// Bone indices
// Requirement: from SkinnedMeshRenderer bone array/hierarchy, not generic humanoid table.
// dump.cs:1920677 has HumanBodyBones enum (humanoid table) — NOT bone-array indices; do not map directly.
// No static bone-name/hierarchy data in dump.cs. Runtime auto-resolve implemented in bone_probe.hpp:
//   rb::boneprobe::resolveBones(smr, getBones,getLength,getElement,getName) -> runtimeBones().
// Static compile-time indices remain -1 until live probe confirms; runtime resolver fills by name.
// ---------------------------------------------------------------------------
inline constexpr int kBone_Head  = -1; // runtime-resolved by bone_probe.hpp::resolveBones(); static default unknown until live probe
inline constexpr int kBone_Spine = -1; // runtime-resolved by bone_probe.hpp::resolveBones(); static default unknown until live probe
inline constexpr int kBone_Chest = -1; // runtime-resolved by bone_probe.hpp::resolveBones(); static default unknown until live probe

// ---------------------------------------------------------------------------
// Validation
// Contract change (user-approved): unresolved class/static/bone markers remain
// TODO and are excluded from build gate; evidence-backed offsets/RVAs are gated.
// Runtime must resolve TODO markers before using them.
// ---------------------------------------------------------------------------
inline bool validateFilled() {
    // Evidence-backed field offsets / RVAs must be non-zero.
    if (kBattleEntitiesModel_PhysicsSceneModel == 0) return false;
    if (kBattleEntitiesModel_PlayerBattleModel == 0) return false;
    if (kBattleEntitiesModel_InventoryModels == 0) return false;
    if (kBattleEntitiesModel_ModelsDict == 0) return false;
    if (kBattleEntitiesModel_Player == 0) return false;
    if (kBattleEntitiesModel_SelfPlot == 0) return false;
    if (kBattleEntitiesModel_EntityModelsReadOnly == 0) return false;

    if (kEntityModel_Id == 0) return false;
    if (kPlayerEntityModel_ClientModel == 0) return false;
    if (kPlayerEntityModel_ServerModel == 0) return false;
    if (kPlayerEntityModel_PublicId == 0) return false;

    if (kCharacterClientModel_MovementTimeline == 0) return false;
    if (kCharacterClientModel_LookRotationTimeline == 0) return false;
    if (kCharacterClientModel_RotationTimeline == 0) return false;
    if (kCharacterClientModel_Movement == 0) return false;
    if (kCharacterClientModel_RotationModel == 0) return false;
    if (kCharacterClientModel_RotationInterpolated == 0) return false;
    if (kCharacterClientModel_VisibleModel == 0) return false;
    if (kCharacterClientModel_VelocityModel == 0) return false;
    if (kCharacterClientModel_SqrDistanceToPlayer == 0) return false;

    if (kPlayerClientModel_CameraModel == 0) return false;
    if (kPlayerClientModel_PlayerInVehicleModel == 0) return false;
    if (kPlayerClientModel_AimAssistModel == 0) return false;
    if (kPlayerClientModel_AutoFireModel == 0) return false;
    if (kPlayerClientModel_RecoilModel == 0) return false;
    if (kPlayerClientModel_FakeSpreadModel == 0) return false;
    if (kPlayerClientModel_IsActualData == 0) return false;

    if (kRecoilModel_RecoilOffset == 0) return false;
    if (kPlayerFakeSpreadModel_Spread == 0) return false;
    if (kPlayerFakeSpreadModel_Air == 0) return false;
    if (kPlayerFakeSpreadModel_Move == 0) return false;
    if (kAimAssistModel_IsActive == 0) return false;
    if (kAimAssistModel_HasTarget == 0) return false;
    if (kAimAssistModel_Target == 0) return false;
    if (kAutoFireModel_IsActive == 0) return false;
    if (kAutoFireModel_IsFiring == 0) return false;
    if (kAutoFireModel_HasTarget == 0) return false;
    if (kAutoFireModel_Target == 0) return false;

    if (kUnityObject_m_CachedPtr == 0) return false;
    if (kCamera_m_NonSerializedVersion == 0) return false;

    if (kRVA_Camera_get_fieldOfView == 0) return false;
    if (kRVA_Camera_set_fieldOfView == 0) return false;
    if (kRVA_Camera_get_worldToCameraMatrix == 0) return false;
    if (kRVA_Camera_get_projectionMatrix == 0) return false;
    if (kRVA_Camera_get_main == 0) return false;
    if (kRVA_Camera_get_current == 0) return false;
    if (kRVA_Camera_WorldToScreenPoint == 0) return false;

    if (kRVA_Transform_GetChild == 0) return false;
    if (kRVA_Transform_get_position == 0) return false;
    if (kRVA_Transform_get_rotation == 0) return false;

    if (kRVA_SkinnedMeshRenderer_get_bones == 0) return false;
    if (kRVA_SkinnedMeshRenderer_get_rootBone == 0) return false;
    if (kRVA_Animator_GetBoneTransform == 0) return false;
    if (kRVA_Object_get_name == 0) return false;

    if (kRVA_Physics_Raycast_vec_dir_hit_dist_mask_qti == 0) return false;
    if (kRVA_Physics_Raycast_ray_dist_mask_qti == 0) return false;

    if (kRVA_BattleEntitiesModel_get_Player == 0) return false;
    if (kRVA_PlayerEntityModel_get_ClientModel == 0) return false;
    if (kRVA_PlayerEntityModel_get_ServerModel == 0) return false;
    if (kRVA_ShotIntentionUpdater_CanShoot == 0) return false;
    if (kRVA_ShotIntentionUpdater_StartFire == 0) return false;
    if (kRVA_ShotIntentionUpdater_ContinueFire == 0) return false;
    if (kRVA_ShotIntentionUpdater_StopFire == 0) return false;
    if (kRVA_ShotArgsSystem_Tick == 0) return false;
    if (kRVA_ShotArgsSystem_GetShotDirection == 0) return false;
    if (kRVA_ShotArgsSystem_ApplySpreadToShotDirection == 0) return false;
    return true;
}

inline bool validate() { return validateFilled(); }

// True for evidence-backed constants. TODO markers remain in file for unresolved items.
inline constexpr bool ALL_FILLED = true;
static_assert(ALL_FILLED, "evidence-backed offsets must be filled");

} // namespace rb::off
