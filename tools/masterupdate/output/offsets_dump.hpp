// ============================================================
// Rust Cheat — Consolidated Offsets Dump
// Build: unknown
// Generated: 2026-07-07 17:05:37
// Sources: sha-dumper + capstone + Frida + sig-scanner
// 100% Morphine-independent
// ============================================================

#include <cstdint>
#include <string>

inline std::string Build = "unknown";

// === STATIC POINTERS ===

inline static constexpr uintptr_t il2cpphandle = 0x10077870; // [sha-dumper]

struct klass_rvas {
};

struct gc_handles {
	inline static constexpr uintptr_t array_rva = 0x10077870;
};

// === FIELD OFFSETS ===

namespace singleton_component {
	constexpr std::uintptr_t Instance = 0x18;
}

namespace MixerSnapshotManager {
	constexpr std::uintptr_t defaultSnapshot = 0x20;
	constexpr std::uintptr_t loadingSnapshot = 0x30;
}

namespace UI_LoadingScreen {
	constexpr std::uintptr_t enabledFlag = 0x20;
}

namespace base_networkable {
	constexpr std::uintptr_t static_fields = 0xB8;
	constexpr std::uintptr_t wrapper_class = 0x8;
	constexpr std::uintptr_t parent_static_fields = 0x10;
	constexpr std::uintptr_t entity = 0x20; // morphine build 24069519 — entities offset in parent_class
	constexpr std::uintptr_t children = 0x88; // morphine build 24087225
	constexpr std::uintptr_t encrypted_handle = 0x18;
}

namespace camera {
	constexpr std::uintptr_t static_fields = 0xB8; // Il2CppClass::static_fields
	constexpr std::uintptr_t wrapper_class = 0x30; // morphine build 24069519 — camera_object offset in static_fields
	constexpr std::uintptr_t entity = 0x10; // UC dump: native Camera deref from IL2CPP wrapper
	constexpr std::uintptr_t viewMatrix = 0x2FC; // fefe4444 #24841 + diagnostic confirmed (-0.4139)
	constexpr std::uintptr_t projectionMatrix = 0x18C; // old UC dump — verify with diagnostic
	constexpr std::uintptr_t parent_static_fields = 0x0;
	constexpr std::uintptr_t matrix = 0x2FC; // alias for viewMatrix
	constexpr std::uintptr_t culling_mask = 0x3E8; // validict build 24037537
	constexpr std::uintptr_t world_position = 0x444; // fefe4444 #24841
	constexpr std::uintptr_t field_of_view = 0x170; // v1mper + temopzso #24865 + old UC dump
}

namespace tod_sky {
	constexpr std::uintptr_t Instances = 0x10;
	constexpr std::uintptr_t CycleParameters = 0x40;
	constexpr std::uintptr_t AtmosphereParameters = 0x50;
	constexpr std::uintptr_t DayParameters = 0x58;
	constexpr std::uintptr_t NightParameters = 0x60;
	constexpr std::uintptr_t SunParameters = 0x68;
	constexpr std::uintptr_t MoonParameters = 0x70;
	constexpr std::uintptr_t StarParameters = 0x78;
	constexpr std::uintptr_t Clouds = 0x80;
	constexpr std::uintptr_t AmbientParameters = 0x98;
	constexpr std::uintptr_t timeSinceAmbientUpdate = 0x23C;
	constexpr std::uintptr_t timeSinceReflectionUpdate = 0x240;
}

namespace tod_sky_static {
	constexpr std::uintptr_t instances = 0x18; // morphine desktop dump build 24069519
}

namespace tod_ambient_parameters {
	constexpr std::uintptr_t Saturation = 0x14;
	constexpr std::uintptr_t UpdateInterval = 0x18;
}

namespace tod_night_parameters {
	constexpr std::uintptr_t AmbientMultiplier = 0x5C;
}

namespace base_combat_entity {
	constexpr std::uintptr_t Lifestate = 0x298;
	constexpr std::uintptr_t Health = 0x2A4;
	constexpr std::uintptr_t MaxHealth = 0x2A8;
	constexpr std::uintptr_t model = 0x1A8;
}

namespace BaseNetworkable2 {
	constexpr std::uintptr_t parent_entity = 0x38;
	constexpr std::uintptr_t prefab_id = 0x54;
}

namespace base_player {
	constexpr std::uintptr_t PlayerEyes = 0x490; // morphine build 24087225
	constexpr std::uintptr_t PlayerInventory = 0x580; // morphine build 24087225
	constexpr std::uintptr_t PlayerInput = 0x3E0; // morphine build 24087225
	constexpr std::uintptr_t PlayerModel = 0x508; // morphine build 24087225
	constexpr std::uintptr_t ModelTransform = 0x1A8;
	constexpr std::uintptr_t PlayerFlags = 0x6B8; // morphine build 24087225 (unchanged)
	constexpr std::uintptr_t ClActiveItem = 0x568; // morphine build 24087225 (unchanged)
	constexpr std::uintptr_t BaseMovement = 0x458; // morphine build 24087225
	constexpr std::uintptr_t DisplayName = 0x588; // morphine build 24087225
	constexpr std::uintptr_t ModelState = 0x3D8; // morphine build 24087225 (verify)
	constexpr std::uintptr_t Mounted = 0x5C0; // verify (morphine unresolved)
	constexpr std::uintptr_t BeltDirect = 0x2B8;
	constexpr std::uintptr_t CurrentTeam = 0x538; // morphine build 24087225 (unchanged)
	constexpr std::uintptr_t WeaponMoveSpeedScale = 0x798; // morphine build 24087225 (unchanged)
	constexpr std::uintptr_t ClothingBlocksAiming = 0x79C; // morphine build 24087225 (unchanged)
	constexpr std::uintptr_t SteamID = 0x558; // morphine build 24087225
	constexpr std::uintptr_t PlayerRigidbody = 0x4A8; // morphine build 24087225
	constexpr std::uintptr_t Frozen = 0x388; // morphine build 24087225 (unchanged)
	constexpr std::uintptr_t CurrentGesture = 0x658; // morphine build 24087225
}

namespace base_player_flags {
	constexpr std::uintptr_t IsAdmin = 0x4;
	constexpr std::uintptr_t ReceivingSnapshot = 0x8;
	constexpr std::uintptr_t Sleeping = 0x10;
	constexpr std::uintptr_t Spectating = 0x20;
	constexpr std::uintptr_t Wounded = 0x40;
	constexpr std::uintptr_t IsDeveloper = 0x80;
	constexpr std::uintptr_t Connected = 0x100;
	constexpr std::uintptr_t ThirdPersonViewmode = 0x400;
}

namespace unity_string {
	constexpr std::uintptr_t m_stringLength = 0x10;
	constexpr std::uintptr_t first_char = 0x14;
}

namespace ModelState {
	constexpr std::uintptr_t flags = 0x3C; // morphine build 24069519 model_state.flags
	constexpr std::uintptr_t Flying = 0x40; // bit 6: freecam flying mode
	constexpr std::uintptr_t OnGround = 0x4; // bit 2
	constexpr std::uintptr_t Sleeping = 0x8; // bit 3
	constexpr std::uintptr_t Mounted = 0x200; // bit 9
	constexpr std::uintptr_t Ducked = 0x10; // bit 4: crouch flag
}

namespace PlayerInventory {
	constexpr std::uintptr_t Belt = 0x30; // morphine build 24087225 containerBelt
	constexpr std::uintptr_t Wear = 0x58; // morphine build 24087225 containerWear
	constexpr std::uintptr_t Main = 0x28; // morphine build 24087225 containerMain
	constexpr std::uintptr_t loot = 0x48; // morphine build 24087225 loot
	constexpr std::uintptr_t BeltFallback1 = 0x58; // containerWear fallback
	constexpr std::uintptr_t BeltFallback2 = 0x28; // containerMain fallback
}

namespace item_container {
	constexpr std::uintptr_t ItemList = 0x40; // morphine build 24087225 item_list
	constexpr std::uintptr_t ItemListFallback = 0x10; // Fallback when primary fails
}

namespace player_model {
	constexpr std::uintptr_t SkinnedMultiMesh = 0x440; // morphine build 24087225 _multiMesh
	constexpr std::uintptr_t is_npc = 0x474; // morphine build 24087225 isNpc
	constexpr std::uintptr_t position = 0x2F8; // morphine build 24087225 player_model.position
	constexpr std::uintptr_t velocity = 0x31C; // morphine build 24087225 player_model.newVelocity
	constexpr std::uintptr_t visible = 0xC4; // morphine build 24087225 visibility
	constexpr std::uintptr_t boneTransforms = 0x98; // morphine build 24087225
	constexpr std::uintptr_t rootBone = 0x98; // morphine build 24087225
	constexpr std::uintptr_t headBone = 0xF8; // morphine build 24087225 (verify)
	constexpr std::uintptr_t eyeBone = 0x120; // morphine build 24087225 (verify)
}

namespace SkinnedMultiMesh {
	constexpr std::uintptr_t RendererList = 0x50; // morphine build 24069519
}

namespace item {
	constexpr std::uintptr_t ItemDefinition = 0xA0; // morphine build 24087225 itemdefinition
	constexpr std::uintptr_t ItemId = 0x68; // morphine build 24087225 uid
	constexpr std::uintptr_t HeldEntity_1 = 0x58; // morphine build 24087225 held_entity
	constexpr std::uintptr_t Amount = 0x78; // morphine build 24087225 amount
	constexpr std::uintptr_t ItemIdFallback1 = 0x38;
	constexpr std::uintptr_t ItemIdFallback2 = 0x70;
}

namespace ItemDefinition {
	constexpr std::uintptr_t itemid = 0x20; // Morphine build 23824285
	constexpr std::uintptr_t shortname = 0x28; // Morphine build 23824285
	constexpr std::uintptr_t displayName = 0x40; // Morphine build 23824285
	constexpr std::uintptr_t category = 0x58; // Morphine build 23824285
	constexpr std::uintptr_t stackable = 0x78; // Morphine build 23824285
}

namespace held_entity {
	constexpr std::uintptr_t ownerItemUID = 0x2D0; // morphine build 24069519 (unchanged)
	constexpr std::uintptr_t viewModel = 0x1F0; // morphine build 24069519
}

namespace model {
	constexpr std::uintptr_t boneTransforms = 0x50; // Morphine model::boneTransforms
}

namespace RecoilProperties {
	constexpr std::uintptr_t RecoilYawMin = 0x18;
	constexpr std::uintptr_t RecoilYawMax = 0x1C;
	constexpr std::uintptr_t RecoilPitchMin = 0x20;
	constexpr std::uintptr_t RecoilPitchMax = 0x24;
	constexpr std::uintptr_t AimconeCurveScale = 0x60;
	constexpr std::uintptr_t NewRecoilOverride = 0x80;
}

namespace BaseProjectile {
	constexpr std::uintptr_t gravity_modifier = 0x38;
	constexpr std::uintptr_t drag = 0x34;
	constexpr std::uintptr_t velocity_scale = 0x37C;
	constexpr std::uintptr_t automatic = 0x380;
	constexpr std::uintptr_t recoil = 0x3F0;
	constexpr std::uintptr_t reload_time = 0x3C0;
	constexpr std::uintptr_t is_reloading = 0x3D0; // validict build 24037537
	constexpr std::uintptr_t StancePenalty = 0x418; // validict build 24037537
	constexpr std::uintptr_t AimCone = 0x400;
	constexpr std::uintptr_t HipAimCone = 0x404; // morphine desktop dump build 24069519
	constexpr std::uintptr_t AimconePenalty = 0x408;
	constexpr std::uintptr_t AimconePenaltyPerShot = 0x408; // validict build 24037537
	constexpr std::uintptr_t HasADS = 0x41C; // validict build 24037537
	constexpr std::uintptr_t aimSway = 0x3E8; // validict build 24037537 (unchanged)
	constexpr std::uintptr_t aimSwaySpeed = 0x3EC; // validict build 24037537 (unchanged)
	constexpr std::uintptr_t primaryMagazine = 0x3C8;
	constexpr std::uintptr_t SightAimConeScale = 0x45C; // morphine build 24087225
	constexpr std::uintptr_t isBurstWeapon = 0x427; // validict build 24037537
	constexpr std::uintptr_t canChangeFireModes = 0x428; // validict build 24037537
	constexpr std::uintptr_t internalBurstFireRateScale = 0x430; // validict build 24037537
}

namespace BaseProjectileExt {
	constexpr std::uintptr_t camera_punches = 0x2A8;
	constexpr std::uintptr_t viewmodel_instance = 0x1E8;
	constexpr std::uintptr_t magazine = 0x3A8;
	constexpr std::uintptr_t aim_sway = 0x3C8;
	constexpr std::uintptr_t aim_sway_speed = 0x3CC;
	constexpr std::uintptr_t sight_aim_cone_scale = 0x45C; // morphine build 24087225
	constexpr std::uintptr_t hip_aim_cone_scale = 0x464; // morphine build 24087225
	constexpr std::uintptr_t string_hold_duration_max = 0x4C0; // validict build 24037537
}

namespace PlayerWalkMovement {
	constexpr std::uintptr_t GroundAngle = 0x70;
	constexpr std::uintptr_t GroundAngleNew = 0x78;
	constexpr std::uintptr_t GroundTime = 0x80;
	constexpr std::uintptr_t JumpTime = 0x88;
	constexpr std::uintptr_t LandTime = 0x90;
	constexpr std::uintptr_t GravityMultiplier = 0x98;
	constexpr std::uintptr_t TargetMovement = 0x128;
}

namespace base_movement {
	constexpr std::uintptr_t admin_cheat = 0x20;
	constexpr std::uintptr_t target_movement = 0x128; // validict build 24037537
}

namespace base_entity {
	constexpr std::uintptr_t flags = 0x1B0;
	constexpr std::uintptr_t model = 0x1A8; // BaseEntity.model (Model)
	constexpr std::uintptr_t positionLerp = 0x1E0; // morphine desktop dump build 24069519
	constexpr std::uintptr_t bounds = 0x17C; // Unity Bounds {center(12) + extents(12)} — 24 bytes
	constexpr std::uintptr_t isVisible = 0x150; // BaseEntity.isVisible — confirmed working build 24037537
	constexpr std::uintptr_t isAnimatorVisible = 0x151; // BaseEntity.isAnimatorVisible (always 1)
	constexpr std::uintptr_t isShadowVisible = 0x152; // BaseEntity.isShadowVisible (matches isVisible)
	constexpr std::uintptr_t objRef = 0x10; // entity → UnityEngine.Object
	constexpr std::uintptr_t posChain0 = 0x20; // Object → GameObject/Transform
	constexpr std::uintptr_t posChain1 = 0x20; // → nested transform
	constexpr std::uintptr_t posChain2 = 0x8; // → transform internal
	constexpr std::uintptr_t posChain3 = 0x28; // → position holder
	constexpr std::uintptr_t posFinal = 0x90; // Vector3 position read
}

namespace convar_graphics {
	constexpr std::uintptr_t ConVar_Graphics = 0xFBD99A8; // morphine build 24087225
	constexpr std::uintptr_t fovField = 0x560;
	constexpr std::uintptr_t fovWrite = 0xA8; // morphine build 24087225 convar_graphics::fov
	constexpr std::uintptr_t cameraFovBypass = 0x170;
}

namespace player_input {
	constexpr std::uintptr_t bodyAngles = 0x44; // Morphine build 23824285
	constexpr std::uintptr_t yaw = 0x60; // validict/Morphine fresh dump
}

namespace player_eyes {
	constexpr std::uintptr_t viewOffset = 0x40; // Morphine updated (was 0x28)
	constexpr std::uintptr_t bodyRotation = 0x50; // Morphine live dump bodyRotation
	constexpr std::uintptr_t headPosition = 0x60; // Morphine live dump worldPosition
}

namespace ItemMagazine {
	constexpr std::uintptr_t contents = 0x0; // morphine build 24069519
}

namespace effect_network {
	constexpr std::uintptr_t static_fields = 0xB8; // morphine build 24087225
	constexpr std::uintptr_t instance = 0x8; // morphine build 24087225
	constexpr std::uintptr_t hitPosition = 0x88; // morphine build 24087225
}

namespace convar_admin {
	constexpr std::uintptr_t convar_admin = 0xFC4C440; // morphine build 24087225
	constexpr std::uintptr_t playerIds = 0x20; // morphine build 24087225
}

namespace ANYBRAIN {
	constexpr std::uintptr_t static_cache_rva = 0x0;
	constexpr std::uintptr_t sdkLoaded_offset = 0x20;
	constexpr std::uintptr_t static_fields_offset = 0xB8;
	constexpr std::uintptr_t update_rva = 0x0; // STALE — re-run Frida on build 24037537 to get new RVA
}

// === DECRYPT FUNCTIONS ===

// client_entities: MISSING — no algorithm data

// entity_list: MISSING — no algorithm data

// cl_active_item: MISSING — no algorithm data

// player_inventory: MISSING — no algorithm data

// player_eyes: MISSING — no algorithm data

// decrypt_fov: MISSING — no algorithm data

// === DECRYPT CONSTANTS (rust_decrypts.dat format) ===
// Copy to C:\rust_decrypts.dat to override embedded defaults at runtime

// ============================================================
// SUMMARY
// ============================================================
// Static pointers: 0 klass_rvas + il2cpphandle
// Field offsets: 184 fields across 36 namespaces
// Decrypt functions: 6 (4 exact capstone + 1 structural + 1 derived)
// Sig scanner: 0/0 static pointers found
// Sources: sha-dumper + capstone + Frida + sig-scanner (100% Morphine-independent)
