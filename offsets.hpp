#pragma once
#include <Windows.h>
#include <iostream>
#include "imgui/imgui.h"
#include <vector>

namespace offsets {
	inline uint64_t basenetworkable_pointer = 0x0FC72970; // morphine build 24091435
	inline uint64_t camera_pointer = 0x0FC68100;           // morphine build 24091435 (MainCamera)
	inline uint64_t Il2cppGetHandle = 0x10132020;           // morphine build 24091435 (gc_handles::array_rva)
	inline uint64_t TOD_Sky_TypeInfo = 0x0FD0A5D0;         // morphine build 24091435
	inline uint64_t Class_TOD_Sky_Static = 0x0FD0A5D0;    // morphine build 24091435 (same as TypeInfo)
	inline uint64_t EffectNetwork_Pointer = 0xFCABFC0;   // morphine build 24091435 (type_info)
	inline uint64_t Class_SingletonComponent_UI_LoadingScreen = 0xFC7D388; // morphine build 24091435
	inline uint64_t Class_SingletonComponent_MixerSnapshotManager__c = 0xFD0D1D0; // morphine build 24091435
	// Diagnostic camera scan TypeInfo RVAs (only used when kCacheVerboseLogs is on)
	inline uint64_t cam_typeinfo_singleton = 0xE2D5CC8;    // SingletonComponent<MainCamera>
	inline uint64_t cam_typeinfo_camera_c = 0xE2C3EC8;      // Camera_c

	namespace SingletonComponent
	{
		inline uint64_t Instance = 0x18;
	}

	namespace MixerSnapshotManager
	{
		inline uint64_t defaultSnapshot = 0x20;
		inline uint64_t loadingSnapshot = 0x30;
	}

	namespace UI_LoadingScreen
	{
		inline uint64_t enabledFlag = 0x20;
	}

	namespace BaseNetworkable
	{
		inline uint64_t  static_fields = 0xB8;
		inline uint64_t  wrapper_class = 0x8;
		inline uint64_t  parent_static_fields = 0x10;
		inline uint64_t  entity = 0x20;         // morphine build 24091435 — entities offset in parent_class
		inline uint64_t  children = 0x70;      // morphine build 24091435
		inline uint64_t  encrypted_handle = 0x18;
	}

  namespace BaseCamera
  {
  	inline uint64_t  static_fields = 0xB8;   // Il2CppClass::static_fields
  	inline uint64_t  wrapper_class = 0x38;   // morphine build 24091435 — camera_object/instance offset in static_fields
  	inline uint64_t  entity = 0x10;          // UC dump: native Camera deref from IL2CPP wrapper
  	inline uint64_t  viewMatrix = 0x2FC;     // fefe4444 #24841 + diagnostic confirmed (-0.4139)
  	inline uint64_t  projectionMatrix = 0x18C; // old UC dump — verify with diagnostic
  	inline uint64_t  parent_static_fields = 0x0;
  	inline uint64_t  matrix = 0x2FC;          // alias for viewMatrix
  		inline uint64_t  culling_mask = 0x3E8;     // validict build 24037537
  	inline uint64_t  world_position = 0x444;  // fefe4444 #24841
  	inline uint64_t  field_of_view = 0x170;   // v1mper + temopzso #24865 + old UC dump
  }

  	namespace TOD_Sky
  	{
 		inline uint64_t Instances = 0x10;
 		inline uint64_t CycleParameters = 0x40;
		inline uint64_t AtmosphereParameters = 0x50;
		inline uint64_t DayParameters = 0x58;
 		inline uint64_t NightParameters = 0x60;
		inline uint64_t SunParameters = 0x68;
		inline uint64_t MoonParameters = 0x70;
		inline uint64_t StarParameters = 0x78;
		inline uint64_t Clouds = 0x80;
		inline uint64_t AmbientParameters = 0x98;
		inline uint64_t timeSinceAmbientUpdate = 0x23C;
		inline uint64_t timeSinceReflectionUpdate = 0x240;
  	}

 	namespace TOD_Sky_Static
 	{
  		inline uint64_t instances = 0x18; // morphine desktop dump build 24069519
 	}

	namespace TOD_AmbientParameters
	{
		inline uint64_t Saturation = 0x14;
	}

	namespace TOD_NightParameters
	{
		inline uint64_t AmbientMultiplier = 0x5C;
	}

namespace BaseCombatEntity {
	inline uint64_t Lifestate = 0x00000298;
	inline uint64_t Health = 0x000002A4;
	inline uint64_t MaxHealth = 0x000002A8;
	inline uint64_t model = 0x1A8;
}

	namespace BaseNetworkable2
	{
		inline uint64_t parent_entity = 0x38;
		inline uint64_t prefab_id = 0x54;
	}

namespace BasePlayer {
	inline uint64_t PlayerEyes = 0x490;        // morphine build 24091435
	inline uint64_t PlayerInventory = 0x3a0;   // morphine build 24091435
	inline uint64_t PlayerInput = 0x338;       // morphine build 24091435
	inline uint64_t PlayerModel = 0x3d8;       // morphine build 24091435
	inline uint64_t ModelTransform = 0x1A8;
	inline uint64_t PlayerFlags = 0x6B8;       // morphine build 24091435 (unchanged)
	inline uint64_t ClActiveItem = 0x568;      // morphine build 24091435 (unchanged)
	inline uint64_t BaseMovement = 0x5b8;      // morphine build 24091435
	inline uint64_t DisplayName = 0x2e8;       // morphine build 24091435
	inline uint64_t ModelState = 0x518;  // morphine build 24091435 (verify)
	inline uint64_t Mounted = 0x5c0;           // morphine build 24091435
	inline uint64_t BeltDirect = 0x7b8;        // morphine build 24091435
	inline uint64_t CurrentTeam = 0x538;       // morphine build 24091435 (unchanged)
	inline uint64_t WeaponMoveSpeedScale = 0x798; // morphine build 24091435 (unchanged)
	inline uint64_t ClothingBlocksAiming = 0x79C; // morphine build 24091435 (unchanged)
	inline uint64_t SteamID = 0x558;           // morphine build 24091435 (verify)
	inline uint64_t PlayerRigidbody = 0x520;   // morphine build 24091435
	inline uint64_t Frozen = 0x388;            // morphine build 24091435 (unchanged)
	inline uint64_t CurrentGesture = 0x3f0;    // morphine build 24091435
  }

// Morphine: base_player_flags — used for admin/sleeping/wounded checks
namespace base_player_flags {
    constexpr uint64_t IsAdmin = 0x4;
    constexpr uint64_t ReceivingSnapshot = 0x8;
    constexpr uint64_t Sleeping = 0x10;
    constexpr uint64_t Spectating = 0x20;
    constexpr uint64_t Wounded = 0x40;
    constexpr uint64_t IsDeveloper = 0x80;
    constexpr uint64_t Connected = 0x100;
    constexpr uint64_t ThirdPersonViewmode = 0x400;
}

// Morphine: unity_string — Il2Cpp string layout
namespace unity_string {
    constexpr uint64_t m_stringLength = 0x10;
    constexpr uint64_t first_char = 0x14;
}

  namespace ModelState
  {
      constexpr uint64_t flags = 0x0;            // morphine build 24087225 model_state.flags
      constexpr uint64_t Flying    = 0x40;      // bit 6: freecam flying mode
      constexpr uint64_t OnGround  = 0x04;      // bit 2
      constexpr uint64_t Sleeping  = 0x08;      // bit 3
      constexpr uint64_t Mounted   = 0x200;     // bit 9
      constexpr uint64_t Ducked    = 0x10;      // bit 4: crouch flag
  }

  namespace PlayerInventory
  {
      inline uint64_t Belt = 0x58;        // morphine build 24091435 containerBelt
      inline uint64_t Wear = 0x78;        // morphine build 24091435 containerWear
      inline uint64_t Main = 0x30;        // morphine build 24091435 containerMain
      inline uint64_t loot = 0x48;        // morphine build 24091435 loot
      constexpr uint64_t BeltFallback1 = 0x78;  // containerWear fallback
      constexpr uint64_t BeltFallback2 = 0x30;  // containerMain fallback
  }

  namespace ItemContainer
  {
      constexpr uint64_t ItemList = 0x60; // morphine build 24091435 item_list
      constexpr uint64_t ItemListFallback = 0x10; // Fallback when primary fails
  }

    namespace PlayerModel
   	{
    	inline uint64_t SkinnedMultiMesh = 0x420; // morphine build 24091435 _multiMesh
       		inline uint64_t is_npc = 0x00000490;           // morphine build 24091435 isNpc
      		inline uint64_t position = 0x000002F8;         // morphine build 24087225 player_model.position
      		inline uint64_t velocity = 0x0000031C;         // morphine build 24087225 player_model.newVelocity
      		inline uint64_t visible = 0x000000C4;           // morphine build 24087225 visibility
      		inline uint64_t boneTransforms = 0x00000098;    // morphine build 24087225
      		inline uint64_t rootBone = 0x00000098;          // morphine build 24087225
      		inline uint64_t headBone = 0x000000F8;          // morphine build 24087225 (verify)
      		inline uint64_t eyeBone = 0x00000120;          // morphine build 24087225 (verify)
   	}

  	namespace SkinnedMultiMesh
  	{
   		constexpr uint64_t RendererList = 0x50; // morphine build 24069519
  	}

    namespace Item
    {
        constexpr uint64_t ItemDefinition = 0xA8;       // morphine build 24091435 itemdefinition
        constexpr uint64_t ItemId = 0xB8;               // morphine build 24091435 uid
        constexpr uint64_t HeldEntity_1 = 0xA8;         // morphine build 24091435 held_entity
        constexpr uint64_t Amount = 0x24;               // morphine build 24091435 amount
        constexpr uint64_t ItemIdFallback1 = 0x38;
        constexpr uint64_t ItemIdFallback2 = 0x70;
    }

   namespace ItemDefinition
   {
       constexpr uint64_t itemid = 0x20;               // Morphine build 23824285
       constexpr uint64_t shortname = 0x28;            // Morphine build 23824285
       constexpr uint64_t displayName = 0x40;          // Morphine build 23824285
       constexpr uint64_t category = 0x58;             // Morphine build 23824285
       constexpr uint64_t stackable = 0x78;            // Morphine build 23824285
   }

  namespace HeldEntity
  {
      constexpr uint64_t ownerItemUID = 0x000002D0;        // morphine build 24069519 (unchanged)
       constexpr uint64_t viewModel = 0x000002C8;           // morphine build 24091435
  }

  namespace Model
  {
      constexpr uint64_t boneTransforms = 0x50;       // Morphine model::boneTransforms
  }

  namespace RecoilProperties
  {
      constexpr uint64_t RecoilYawMin = 0x18;
      constexpr uint64_t RecoilYawMax = 0x1C;
      constexpr uint64_t RecoilPitchMin = 0x20;
      constexpr uint64_t RecoilPitchMax = 0x24;
      constexpr uint64_t AimconeCurveScale = 0x60;
      constexpr uint64_t NewRecoilOverride = 0x80;
  }

  namespace BaseProjectile
  {
      constexpr uint64_t gravity_modifier = 0x38;
      constexpr uint64_t drag = 0x34;
      constexpr uint64_t velocity_scale = 0x0000037C;
      constexpr uint64_t automatic = 0x00000380;
      constexpr uint64_t recoil = 0x000003F0;
      constexpr uint64_t reload_time = 0x000003C0;
      constexpr uint64_t is_reloading = 0x3D0;           // validict build 24037537
      constexpr uint64_t StancePenalty = 0x418;    // validict build 24037537
      constexpr uint64_t AimCone = 0x00000400;
      constexpr uint64_t HipAimCone = 0x00000404;  // morphine desktop dump build 24069519
      constexpr uint64_t AimconePenalty = 0x408;
      constexpr uint64_t AimconePenaltyPerShot = 0x408; // validict build 24037537
      constexpr uint64_t HasADS = 0x41C;    // validict build 24037537
      constexpr uint64_t aimSway = 0x000003E8;              // validict build 24037537 (unchanged)
      constexpr uint64_t aimSwaySpeed = 0x000003EC;         // validict build 24037537 (unchanged)
      constexpr uint64_t primaryMagazine = 0x000003C8;
        constexpr uint64_t SightAimConeScale = 0x45C;    // morphine build 24087225
      constexpr uint64_t isBurstWeapon = 0x427;    // validict build 24037537
      constexpr uint64_t canChangeFireModes = 0x428; // validict build 24037537
      constexpr uint64_t internalBurstFireRateScale = 0x430; // validict build 24037537
  }

  namespace BaseProjectileExt
  {
      constexpr uint64_t camera_punches = 0x2A8;
      constexpr uint64_t viewmodel_instance = 0x1E8;
      constexpr uint64_t magazine = 0x3A8;
      constexpr uint64_t aim_sway = 0x3C8;
      constexpr uint64_t aim_sway_speed = 0x3CC;
       constexpr uint64_t sight_aim_cone_scale = 0x45C;  // morphine build 24087225
       constexpr uint64_t hip_aim_cone_scale = 0x464;   // morphine build 24087225
      constexpr uint64_t string_hold_duration_max = 0x4C0; // validict build 24037537
  }

  namespace PlayerWalkMovement
  {
      constexpr uint64_t GroundAngle = 0x70;
      constexpr uint64_t GroundAngleNew = 0x78;
      constexpr uint64_t GroundTime = 0x80;
      constexpr uint64_t JumpTime = 0x88;
      constexpr uint64_t LandTime = 0x90;
      constexpr uint64_t GravityMultiplier = 0x98;
      constexpr uint64_t TargetMovement = 0x128;
  }

  namespace BaseMovement2
  {
      constexpr uint64_t admin_cheat = 0x20;
      constexpr uint64_t target_movement = 0x128;  // validict build 24037537
  }

  namespace BaseEntity
  {
      constexpr uint64_t flags = 0x1B0;
      constexpr uint64_t model = 0x1A8;         // BaseEntity.model (Model)
      constexpr uint64_t positionLerp = 0x1c8;    // morphine build 24087225
      constexpr uint64_t bounds = 0x17C;         // Unity Bounds {center(12) + extents(12)} — 24 bytes
      constexpr uint64_t isVisible = 0x150;      // BaseEntity.isVisible — confirmed working build 24037537
      constexpr uint64_t isAnimatorVisible = 0x151; // BaseEntity.isAnimatorVisible (always 1)
      constexpr uint64_t isShadowVisible = 0x152;    // BaseEntity.isShadowVisible (matches isVisible)
      // Position chain offsets used by Get_ObjectPosition() — object → transform hierarchy
      constexpr uint64_t objRef = 0x10;          // entity → UnityEngine.Object
      constexpr uint64_t posChain0 = 0x20;       // Object → GameObject/Transform
      constexpr uint64_t posChain1 = 0x20;       // → nested transform
      constexpr uint64_t posChain2 = 0x8;        // → transform internal
      constexpr uint64_t posChain3 = 0x28;       // → position holder
      constexpr uint64_t posFinal = 0x90;        // Vector3 position read
  }

  namespace FOV
  {
      inline uint64_t ConVar_Graphics = 0xFCF2C10;  // morphine build 24091435
      constexpr uint64_t fovField = 0x560;
      constexpr uint64_t fovWrite = 0x110;           // morphine build 24091435 convar_graphics::fov
      constexpr uint64_t cameraFovBypass = 0x170;
	}

  namespace PlayerInput
  {
      constexpr uint64_t bodyAngles = 0x44; // Morphine build 23824285
      constexpr uint64_t yaw = 0x60;        // validict/Morphine fresh dump
  }

  namespace PlayerEyes
  {
      constexpr uint64_t viewOffset = 0x40;    // Morphine updated (was 0x28)
      constexpr uint64_t bodyRotation = 0x50;  // Morphine live dump bodyRotation
      constexpr uint64_t headPosition = 0x60;  // Morphine live dump worldPosition
  }

  namespace ItemMagazine
  {
       constexpr uint64_t contents = 0x1c; // morphine build 24087225
  }

  namespace EffectNetwork
  {
      constexpr uint64_t static_fields = 0xB8;  // morphine build 24087225
      constexpr uint64_t instance = 0x8;        // morphine build 24087225
       constexpr uint64_t hitPosition = 0x90;    // morphine build 24091435
  }

  namespace convar_admin
  {
       constexpr uint64_t convar_admin = 0xFC816A8; // morphine build 24091435
      constexpr uint64_t playerIds = 0x20;          // morphine build 24087225
  }

  namespace TOD_AmbientParameters
  {
      constexpr uint64_t UpdateInterval = 0x18;
  }
	inline float AnimalBoxDist = 300.f;
	inline float AnimalNameDist = 300.f;
	inline float AnimalDistTextDist = 300.f;
	inline float AnimalHealthBarDist = 300.f;
	inline float AnimalSnaplineDist = 300.f;
}











#define RGBAs(r, g, b, a) ImVec4((r) / 255.f, (g) / 255.f, (b) / 255.f, (a) / 255.f) 
#define Create(name,value) uintptr_t m_##name = value

	namespace ESP {
	inline bool Box = true;
	inline bool ESPEnabled = true;      // Master toggle for Player ESP
	inline bool hotbar_text = false;
	inline bool HotbarIcons = false;
		// Removed: weapon chams (legacy material handle path was unreliable)
		inline bool OFFArrows = false;
	inline bool FilledBox = false;
	inline bool DrawTextBackground = false;
	inline bool CornerBox = false;
	inline bool Distance = true;
	inline bool SnapLines = true;
	inline bool HeadCircle = false;
	inline bool Weapon = true;
	inline bool Name = true;
	inline bool Skeleton = true;
	inline bool RemoveWounded = false;
	inline bool HighlightWounded = false;
	inline bool HealthBar = false; // health is server-side only in Rust
	inline bool VisCheck = false;
	inline int VisGhostMs = 0;
	inline bool VisGhost = false;
	inline int VisMode = 0;            // 0=Raw (no std::unordered_map — safe for manually mapped DLL)
	inline int VisSamples = 3;         // 3 samples = max 200ms response (was 8 = 600ms)
	inline int VisHoldMs = 0;          // no sticky visible — instant transition
	inline float OccluderDist = 200.f; // max distance to read occluder AABBs
	inline bool TeamID = false;
	inline int SnaplineMode = 0;
	inline bool MovementTrails = false;
	inline bool BulletTracers = false;
	inline bool ShowVisibility = false;
	inline float TracerThickness = 1.5f;
	inline ImColor TracerColor = { 97, 138, 200, 255 };
	inline bool TracerOutline = true;
	inline float EspFontSize = 1.0f;     // 0.5�2.0 scale
	inline int DistanceBracketStyle = 0;  // 0=(100m) 1=[100m] 2={100m} 3=100m
	inline bool ReloadBar = false;
		// Removed: legacy chams material tables/ids.
	inline bool RemoveTeam = false;
	inline float draw_distance = 300.0f;

	inline bool RemoveSleepers = false;
	inline bool HighlightSleepers = false;

	inline bool ESPAdvanced = false;
	inline float BoxDist = 300.f;
	inline float SkeletonDist = 300.f;
	inline float NameDist = 300.f;
	inline float WeaponDist = 300.f;
	inline float HealthBarDist = 300.f;
	inline float SnaplineDist = 300.f;
	inline float HeadCircleDist = 200.f;
	inline float OFFArrowDist = 300.f;

	// Visual enhancements
	inline bool SkeletonOutline = true;
	inline int SkeletonThickness = 1;
	inline bool BoxOutline = true;
	inline int BoxThickness = 1;
	inline bool SnaplineOutline = false;
	inline int SnaplineThickness = 1;
	inline bool Clothing = false;
	inline bool ItemList = false;
	inline bool AmmoBar = false;
	inline bool PlayerInventoryPanel = false;

		namespace color {
			inline ImColor Box = { 220,220,220,255 };
			inline ImColor FilledBox = { 220,220,220,20 };
		inline ImColor Distance = { 150,170,200,255 };
		inline ImColor Snaplines = { 80,120,180,150 };
		inline ImColor HeadCircle = { 100,150,255,255 };
		inline ImColor Weapon = { 180,190,200,255 };
		inline ImColor Name = { 200,200,210,255 };
		inline ImColor Skeleton = { 100,150,220,255 };
		inline ImColor Wounded = { 200,80,80,255 };
		inline ImColor Sleepers = { 100,100,110,255 };
		inline ImColor Team = { 100,160,255,255 };
			inline ImColor OFFArrowColor = { 200,200,220,255 };
			inline ImColor Visible = { 255,59,59,255 };
			inline ImColor Invisible = { 120,120,140,255 };
			inline ImColor SkeletonVisible = { 255,59,59,255 };
			inline ImColor SkeletonInvisible = { 120,120,140,255 };
			inline ImColor HealthBar = { 0,255,0,255 };
			inline ImColor Ghost = { 100,100,110,180 };
			inline ImColor Teammate = { 0,200,100,255 };
		}
}
namespace WORLD {
	inline bool Hemp = true;
	inline bool Distance = false;
	inline float draw_distance = 1500.0f;
	inline float draw_hemp = 200.f;
	inline float draw_stone = 200.f;
	inline float draw_metal = 200.f;
	inline float draw_sulfur = 200.f;
	inline float draw_stash = 300.f;
	inline float draw_corpse = 200.f;
	inline float draw_turret = 200.f;
	inline float draw_shotguntrap = 200.f;
	inline float draw_minicopter = 400.f;
	inline float draw_backpack = 200.f;
	inline float draw_bradley = 500.f;
	inline float draw_dropped = 200.f;
	// Per-entity distance variables (independent sliders)
	inline float draw_samsite = 300.f;
	inline float draw_beartrap = 300.f;
	inline float draw_landmine = 300.f;
	inline float draw_flameturret = 300.f;
	inline float draw_rowboat = 400.f;
	inline float draw_rhib = 400.f;
	inline float draw_kayak = 400.f;
	inline float draw_submarine = 400.f;
	inline float draw_tugboat = 400.f;
	inline float draw_transportheli = 500.f;
	inline float draw_attackheli = 500.f;
	inline float draw_balloon = 400.f;
	inline float draw_motorbike = 400.f;
	inline float draw_snowmobile = 400.f;
	inline float draw_barrelbeige = 300.f;
	inline float draw_barrelblue = 300.f;
	inline float draw_barrelfuel = 300.f;
	inline float draw_crate_normal = 300.f;
	inline float draw_crate_military = 300.f;
	inline float draw_crate_elite = 300.f;
	inline float draw_crate_locked = 300.f;
	inline float draw_lockers = 300.f;
	inline float draw_bags = 300.f;
	inline float draw_beds = 300.f;
	inline float draw_tc = 300.f;
	inline float draw_vending = 300.f;
	inline bool Stash = false;
	inline bool BodyBag = false;
	inline bool Turret = true;
	inline bool Stone = true;
	inline bool Metal = true;
	inline bool Sulfer = true;
	inline bool DroppedItem = false;
	inline bool ShotGunTrap = true;
	inline bool MiniCopter = false;
	inline bool Backpack = false;
	inline bool BradlyAPC = false;
	// --- New world entities ---
	inline bool StonePickup = false;
	inline bool MetalPickup = false;
	inline bool SulfurPickup = false;
	inline bool WoodPickup = false;
	inline bool SamSite = true;
	inline bool BearTrap = true;
	inline bool Landmine = true;
	inline bool FlameTurret = true;
	inline bool Lockers = false;
	inline bool Bags = false;
	inline bool Beds = false;
	inline bool TC = false;
	inline bool Vending = false;
	inline bool Workbench = false;
	inline bool LargeStorage = false;
	inline bool Ladder = false;
	inline bool Generator = false;
	inline bool Battery = false;
	inline bool BarrelBeige = false;
	inline bool BarrelBlue = false;
	inline bool BarrelFuel = false;
	inline bool CrateNormal = false;
	inline bool CrateMilitary = false;
	inline bool CrateElite = false;
	inline bool CrateLocked = false;
	inline bool CrateMedical = false;
	inline bool CrateFood = false;
	inline bool Rowboat = false;
	inline bool RHIB = false;
	inline bool Kayak = false;
	inline bool Tugboat = false;
	inline bool Submarine = false;
	inline bool TransportHeli = false;
	inline bool AttackHeli = false;
	inline bool Balloon = false;
	inline bool Motorbike = false;
	inline bool MotorbikeSidecar = false;
	inline bool Trike = false;
	inline bool Bicycle = false;
	inline bool Snowmobile = false;
	inline bool Shark = false;
	inline bool CargoShip = false;
	inline bool SupplyDrop = false;
	inline float draw_supplydrop = 400.f;
	namespace color {
		inline ImColor Distance_Color = { 255,255,255,255 };
		inline ImColor Hemp_Color = { 80,180,80,255 };
		inline ImColor BodyBag_Color = { 255,255,255,255 };
		inline ImColor Stone_Color = { 220,220,220,255 };
		inline ImColor Metal_Color = { 220,160,60,255 };
		inline ImColor Sulfer_Color = { 220,200,60,255 };
		inline ImColor Turret_Color = { 255,255,255,255 };
		inline ImColor Stash_Color = { 255,255,255,255 };
		inline ImColor DroppedItem_Color = { 255,255,255,255 };
		inline ImColor ShotGunTrap_Color = { 255,255,255,255 };
		inline ImColor MiniCopter_Color = { 255,255,255,255 };
		inline ImColor Backpack_Color = { 255,255,255,255 };
		inline ImColor BradlyAPC = { 255,255,255,255 };
		// --- New world colors ---
		inline ImColor StonePickup = { 180,170,150,255 };
		inline ImColor MetalPickup = { 220,160,60,255 };
		inline ImColor SulfurPickup = { 220,200,60,255 };
		inline ImColor WoodPickup = { 139,90,43,255 };
		inline ImColor SamSite = { 220,80,60,255 };
		inline ImColor BearTrap = { 180,60,60,255 };
		inline ImColor Landmine = { 200,120,40,255 };
		inline ImColor FlameTurret = { 255,140,0,255 };
		inline ImColor Lockers = { 100,180,100,255 };
		inline ImColor Bags = { 80,140,220,255 };
		inline ImColor Beds = { 120,180,220,255 };
		inline ImColor TC = { 160,120,80,255 };
		inline ImColor Vending = { 100,200,180,255 };
		inline ImColor Workbench = { 140,180,200,255 };
		inline ImColor LargeStorage = { 180,160,100,255 };
		inline ImColor Ladder = { 150,130,90,255 };
		inline ImColor Generator = { 220,200,50,255 };
		inline ImColor Battery = { 180,200,50,255 };
		inline ImColor BarrelBeige = { 180,160,120,255 };
		inline ImColor BarrelBlue = { 80,120,220,255 };
		inline ImColor BarrelFuel = { 220,60,60,255 };
		inline ImColor NormalCrate = { 140,100,60,255 };
		inline ImColor ToolCrate = { 200,140,80,255 };
		inline ImColor MilitaryCrate = { 60,140,60,255 };
		inline ImColor EliteCrate = { 180,80,180,255 };
		inline ImColor LockedCrate = { 220,80,80,255 };
		inline ImColor MedicalCrate = { 80,220,80,255 };
		inline ImColor FoodCrate = { 220,180,80,255 };
		inline ImColor Rowboat = { 160,200,220,255 };
		inline ImColor RHIB = { 180,210,230,255 };
		inline ImColor Kayak = { 140,200,200,255 };
		inline ImColor Tugboat = { 200,180,160,255 };
		inline ImColor Submarine = { 200,200,100,255 };
		inline ImColor TransportHeli = { 220,220,220,255 };
		inline ImColor AttackHeli = { 180,180,200,255 };
		inline ImColor Balloon = { 220,100,100,255 };
		inline ImColor Motorbike = { 200,160,120,255 };
		inline ImColor Sidecar = { 200,180,140,255 };
		inline ImColor Trike = { 180,150,120,255 };
		inline ImColor Bicycle = { 100,200,100,255 };
		inline ImColor Snowmobile = { 200,200,220,255 };
		inline ImColor Shark = { 120,120,130,255 };
		inline ImColor CargoShip = { 200,180,80,255 };
		inline ImColor SupplyDrop = { 220,180,60,255 };
	}

	inline bool AnyEspEnabled() {
		return
			Hemp || Stash || BodyBag || Turret || Stone || Metal || Sulfer || DroppedItem ||
			ShotGunTrap || MiniCopter || Backpack || BradlyAPC ||
			StonePickup || MetalPickup || SulfurPickup || WoodPickup || SamSite || BearTrap || Landmine || FlameTurret ||
			Lockers || Bags || Beds || TC || Vending || Workbench || LargeStorage || Ladder || Generator || Battery ||
			BarrelBeige || BarrelBlue || BarrelFuel || CrateNormal || CrateMilitary || CrateElite || CrateLocked ||
			CrateMedical || CrateFood || Rowboat || RHIB || Kayak || Tugboat || Submarine || TransportHeli ||
			AttackHeli || Balloon || Motorbike || MotorbikeSidecar || Trike || Bicycle || Snowmobile || Shark || CargoShip || SupplyDrop;
	}
}
namespace NPC_ESP {
	inline bool Enabled = true;  // NPCs classified by default
	inline bool ESPAdvanced = false;
	inline bool Box = false;
	inline bool Name = true;
	inline bool Distance = true;
	inline bool HealthBar = false; // NPC health is server-side only in Rust
	inline bool Skeleton = false;
	inline bool HeldItem = false;
	inline bool SnapLines = false;
	inline int SnaplineMode = 0;
	inline float draw_distance = 300.0f;
	inline bool Scientists = true;
	inline bool TunnelDweller = true;
	inline bool UnderwaterDweller = true;
	namespace color {
		inline ImColor Box = { 200,140,60,255 };
		inline ImColor Name = { 200,180,150,255 };
		inline ImColor Distance = { 150,170,200,255 };
		inline ImColor Skeleton = { 200,160,80,255 };
		inline ImColor Snaplines = { 200,160,80,100 };
	}
	inline float NPCBoxDist = 300.f;
	inline float NPCNameDist = 300.f;
	inline float NPCDistTextDist = 300.f;
	inline float NPCHealthBarDist = 300.f;
	inline float NPCSkeletonDist = 300.f;
	inline float NPCHeldItemDist = 250.f;
	inline float NPCSnaplineDist = 300.f;
}
namespace ANIMAL_ESP {
	inline bool Enabled = true;
	inline bool ESPAdvanced = false;
	inline bool Box = false;
	inline bool Name = false;
	inline bool Distance = false;
	inline bool HealthBar = false;
	inline bool SnapLines = false;
	inline int SnaplineMode = 0;
	inline float draw_distance = 200.0f;
	inline bool Bear = true;
	inline bool Wolf = true;
	inline bool Boar = true;
	inline bool Stag = true;
	inline bool Chicken = true;
	inline bool Horse = true;
	inline bool PolarBear = true;
	inline bool Panther = true;
	inline bool Tiger = true;
	inline bool Snake = true;
	namespace color {
		inline ImColor Box = { 200,180,80,255 };
		inline ImColor Name = { 200,190,160,255 };
		inline ImColor Distance = { 150,170,200,255 };
		inline ImColor Snaplines = { 200,200,80,100 };
	}
	inline float AnimalBoxDist = 300.f;
	inline float AnimalNameDist = 300.f;
	inline float AnimalDistTextDist = 300.f;
	inline float AnimalHealthBarDist = 300.f;
	inline float AnimalSnaplineDist = 300.f;
}
namespace RADAR {
	inline bool Enabled = false;
	inline float Size = 250.0f;
	inline float Scale = 1.0f;
	inline float DotSize = 4.0f;
	inline bool ShowPlayers = true;
	inline bool ShowNPCs = true;
	inline bool ShowAnimals = true;
	inline bool HideSleepers = true;
	inline bool RemoveTeam = false;
	inline int Position = 0;
	inline bool ShowGrid = true;
	inline bool Rotate = true;
	inline float BackgroundOpacity = 0.78f;
	inline ImColor PlayerColor = { 255, 50, 50, 255 };
	inline ImColor TeammateColor = { 0, 200, 100, 255 };
	inline ImColor NpcColor = { 255, 165, 0, 255 };
	inline ImColor AnimalColor = { 255, 255, 0, 255 };
	inline ImColor BorderColor = { 50, 80, 150, 255 };
}
namespace BATTLE {
	inline bool Players = true;
	inline bool Aimbot = true;
	inline bool World = false;
	inline bool NPC = false;
	inline bool Animals = false;
	inline bool Radar = false;
}
namespace AIMBOT {
	inline bool Memory = false;
	inline bool FovCircle = false;
	inline bool TargetLine = false;
	inline bool TargetText = false;
	inline bool TargetLineFromMuzzle = true;
	inline int FovSize = 100;
	inline float SMOOTHING = 5.f;
	inline float TargetLineThickness = 2.0f;
	inline bool KeepTarget = false;
	inline int BoneSelector = 53;
	inline bool Silent = false;       // BodyRotation quaternion � no visible aim change
	inline int BonePriority = 0;     // 0=Head, 1=Neck, 2=Chest, 3=Pelvis, 4=ClosestToCrosshair, 5=Smart
	inline float PredictionScale = 0.0f;
	inline bool SpinWrites = true;
	inline bool VisibleOnly = false;
	inline bool VisibleStrict = false;
	inline bool IgnoreTeam = false;
	inline bool IgnoreWounded = false;
	inline bool IgnoreSleepers = true;
	inline float MaxDistanceAim = 300.f;
	inline bool DynamicFov = false;
	inline bool WeaponCheck = true;
	inline bool FovOutline = false;
	inline ImColor FovColor = { 255, 255, 255, 60 };
	inline bool PredictionIndicator = true;
	inline bool HumanizeEnabled = false;
	inline float JitterAmount = 1.0f;
	inline float OvershootAmount = 2.0f;
	inline float SmoothingVariance = 0.15f;
	inline float MissProbability = 0.02f;
	namespace color {
		inline ImColor TargetLine = { 255,255,255,255 };
		inline ImColor TargetText = { 255,255,255,255 };
	}
}

namespace MISC {
	inline bool CheatEnabled = true;
	inline bool ShutdownRequested = false;
	inline bool NoMeleeCoolDown = false;
	inline bool ChangeBurst = false;
	inline bool QuickBurst = false;
	inline float BURSTAMOUNT = 0.4f;
	inline bool RecoilEnabled = true;
	inline float RecoilModifier = 100.f;
	inline bool RecoilVariance = false;
	inline float RecoilFloor = 0.25f;
	inline bool AntiAnybrain = true;
	inline bool SilentShot = false;
	inline bool FovChanger = false;
	inline float FovAmount = 100.0f;
	inline bool Zoom = false;
	inline float ZoomAmount = 100.0f;
	inline bool usetouchUnderWater = false;
	inline bool MiniGunMagicBullet = false;
	inline bool Timechanger = false;
	inline bool BrightNight = false;
	inline float lightIntensity = 25.f;
	inline float ambientMultiplier = 4.f;
	inline float AmbientSaturation = 1.0f;
	inline int timevalue = 12;

	// --- Environment modifiers ---
	inline bool Rayleigh = false;
	inline float RayleighVal = 1.0f;
	inline bool Mie = false;
	inline float MieVal = 1.0f;
	inline bool BrightnessEnv = false;
	inline float BrightnessEnvVal = 1.0f;
	inline bool StarMod = false;
	inline float StarSize = 1.0f;
	inline float StarBrightness = 1.0f;
	inline bool MeshMod = false;
	inline float MeshSize = 1.0f;
	inline float MeshBrightness = 1.0f;
	inline bool RemoveTrees = false;
	inline bool RemoveGrass = false;
	inline bool RemoveWater = false;
	inline bool RemoveSky = false;
	inline bool RemoveLayersActive = false;
	inline bool AspectRatio = false;
	inline float AspectRatioVal = 1.777f;
	// TOD color changers
	inline bool TodColors = false;
	inline bool TodSunMoon = false;   inline ImColor TodSunMoonCol = { 255,200,100,255 };
	inline bool TodLight = false;     inline ImColor TodLightCol = { 255,255,200,255 };
	inline bool TodRay = false;       inline ImColor TodRayCol = { 100,180,255,255 };
	inline bool TodSky = false;       inline ImColor TodSkyCol = { 80,150,255,255 };
	inline bool TodCloud = false;     inline ImColor TodCloudCol = { 200,200,220,255 };
	inline bool TodFog = false;       inline ImColor TodFogCol = { 180,190,200,255 };
	inline bool TodAmbient = false;   inline ImColor TodAmbientCol = { 100,120,160,255 };
	// Removed: fast_bullet (legacy path wrote default velocity_scale only).
	inline bool DebugCamera = false;
	inline float DebugCameraTimer = 0.0f;
	inline bool AdminFlags = false;        // sets PlayerFlags.IsAdmin every frame
	inline bool DebugCamAdminFlags = false; // sets Flying + freezes body (used with DebugCamera)
	inline bool CombatMode = false;
	inline int CrosshairStyle = 0;
	inline float CrosshairSize = 4.f;
	inline ImColor CrosshairColor = { 0, 255, 0, 255 };
	inline bool ShowServerTime = false;
	inline bool DrawFlags = false;
	inline bool ShowEspRangeOverlay = true;
	inline ImVec4 UsernameBackgroundColour = RGBAs(255, 255, 255, 255);
	inline ImVec4 DistanceBackgroundColor = RGBAs(255, 255, 255, 255);

	// --- Weapon mods (ported from pre-Unity6) ---
	inline bool NoSpread = false;
	inline float SpreadScale = 100.f;   // 100=no change, 0=no spread
	inline bool Automatic = false;
	inline bool Aimsway = false;
	inline bool NoPunch = false;
	inline bool RapidFire = false;
	inline float FireRateScale = 1.f;   // fire rate multiplier
	inline bool SuperMelee = false;
	inline bool InstantBow = false;
	inline bool HitSound = false;

	// --- New weapon exploits ---
	inline bool InstantEoka = false;
	inline bool FastBow = false;
	inline bool NoPullback = false;
	inline bool InstantCompound = false;
	inline bool Longhand = false;
	inline bool NoHeavySlow = false;

	// --- ViewModel exploits ---
	inline bool NoAnim = false;
	inline bool NoBob = false;
	inline bool NoSway = false;
	inline bool NoLower = false;

	// --- Movement / util (ported from pre-Unity6) ---
	inline bool AlwaysSprint = false;
	inline bool omniSprint = false;
	inline bool JumpShot = false;
	inline bool WalkonWalls = false;
	inline bool NoFall = false;
	inline bool ShootInAir = false;
	inline bool ReducedGravity = false;
	inline float GravityValue = 0.5f;
	inline bool MoonJump = false;
	inline bool Spinbot = false;
	inline bool mincoptershoot = false;
	inline bool AimWhileHeavy = false;
	inline bool FASTHORSE = false;
	inline bool Fly = false;
	inline int FlyKey = VK_MENU;
	inline float FlySpeed = 5.0f;
	inline bool FlySafeMode = false;
	inline bool HighJump = false;
	inline float HighJumpAngle = 85.0f;
	inline bool SpeedHack = false;
	inline int SpeedKey;
	inline float SpeedValue = 1.25f;
	// --- New movement exploits ---
	inline bool Spiderman = false;
	inline bool WalkOnWater = false;
	inline bool InfJump = false;
	inline bool NoWearOverlay = false;
}
namespace SETTINGS {
	inline bool Vsync = false;
	inline bool BattleMode = false;
	inline bool Crosshair = false;
	inline float ROTSPEED = 0.3f;
	inline bool AdminFlag = false;
	inline bool MenuOpen = false;
	inline int Language = 0; // 0=English, 1=French
}

namespace MENU_UI {
	inline int UiScale = 100;
	inline float MenuOpacity = 0.96f;
	inline float BorderGlow = 0.50f;
	inline float AnimationSpeed = 1.00f;
	inline bool CompactMode = false;
	inline bool AdvancedMode = false;
	inline bool Tooltips = true;
	inline bool ShowWatermark = true;
	inline bool ShowFps = true;
	inline bool KeybindsList = false;
	inline bool PerformanceMode = false;
	inline bool DisableAnimations = false;
	inline bool ConfirmBeforeReset = true;
	inline int Theme = 0;
	inline ImColor AccentColor = { 59, 140, 255, 255 };
}

namespace MENU_FX {
	inline bool AnimatedBackground = true;
	inline bool MatrixParticles = true;
	inline bool LightningFx = false;
	inline float FxIntensity = 0.75f;
	inline float MatrixDensity = 0.65f;
	inline float BackgroundOpacity = 0.55f;
}

namespace ANYBRAIN {
	inline uint64_t static_cache_rva = 0x0;
	constexpr uint64_t sdkLoaded_offset = 0x20;
	constexpr uint64_t static_fields_offset = 0xB8;
	inline uint64_t update_rva = 0x0; // STALE — re-run Frida on build 24037537 to get new RVA
	inline bool neutralized = false;
}
