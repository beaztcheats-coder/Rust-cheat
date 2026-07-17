// validict on discord
// game_base @ runtime: 0x7FFC4A7D0000  (image size 0x10EED000)
// Dump generated on: 2026-07-13 03:08:51 PM UTC+3

#include <cstdint>
#include <string>

inline std::string Build = "24181174";
namespace GameAssembly
{
	constexpr std::uintptr_t timestamp = 0x6A54B05A;
	constexpr std::uintptr_t gc_handles = 0x10088B40;
	constexpr std::uintptr_t type_info_definition_table = 0x100894C0;
	constexpr std::uintptr_t il2cpp_resolve_icall = 0x843550;
	constexpr std::uintptr_t il2cpp_array_new = 0x843570;
	constexpr std::uintptr_t il2cpp_assembly_get_image = 0x3CB0;
	constexpr std::uintptr_t il2cpp_class_from_name = 0x82D760;
	constexpr std::uintptr_t il2cpp_class_get_method_from_name = 0x843970;
	constexpr std::uintptr_t il2cpp_class_get_type = 0x71D970;
	constexpr std::uintptr_t il2cpp_domain_get = 0x844290;
	constexpr std::uintptr_t il2cpp_domain_get_assemblies = 0x8442B0;
	constexpr std::uintptr_t il2cpp_gchandle_get_target = 0x8449C0;
	constexpr std::uintptr_t il2cpp_gchandle_new = 0x844970;
	constexpr std::uintptr_t il2cpp_gchandle_free = 0x844A60;
	constexpr std::uintptr_t il2cpp_method_get_name = 0xC150;
	constexpr std::uintptr_t il2cpp_object_new = 0x845300;
	constexpr std::uintptr_t il2cpp_type_get_object = 0x846430;
}
struct il2cpp_api
{
	inline static constexpr uintptr_t domain_get = 0x844290;
	inline static constexpr uintptr_t domain_get_assemblies = 0x8442B0;
	inline static constexpr uintptr_t domain_assembly_open = 0x8442A0;
	inline static constexpr uintptr_t assembly_get_image = 0x3CB0;
	inline static constexpr uintptr_t class_from_name = 0x82D760;
	inline static constexpr uintptr_t image_get_class_count = 0x2B10;
	inline static constexpr uintptr_t image_get_class = 0x846710;
	inline static constexpr uintptr_t class_get_methods = 0x8438E0;
	inline static constexpr uintptr_t class_get_method_from_name = 0x843970;
	inline static constexpr uintptr_t class_get_fields = 0x843690;
	inline static constexpr uintptr_t class_get_nested_types = 0x843710;
	inline static constexpr uintptr_t class_get_type = 0x71D970;
	inline static constexpr uintptr_t class_get_name = 0xC0C0;
	inline static constexpr uintptr_t class_get_namespace = 0xC150;
	inline static constexpr uintptr_t class_get_parent = 0x23130;
	inline static constexpr uintptr_t class_get_image = 0x3CB0;
	inline static constexpr uintptr_t class_get_flags = 0x8439C0;
	inline static constexpr uintptr_t class_get_static_field_data = 0x2860;
	inline static constexpr uintptr_t class_from_il2cpp_type = 0x843670;
	inline static constexpr uintptr_t type_get_object = 0x846430;
	inline static constexpr uintptr_t type_get_class_or_element_class = 0x846450;
	inline static constexpr uintptr_t type_get_name = 0x846480;
	inline static constexpr uintptr_t type_get_attrs = 0x8466A0;
	inline static constexpr uintptr_t method_get_param_count = 0x844F30;
	inline static constexpr uintptr_t method_get_name = 0xC150;
	inline static constexpr uintptr_t method_get_param = 0x844F40;
	inline static constexpr uintptr_t method_get_return_type = 0xC250;
	inline static constexpr uintptr_t method_get_class = 0xC240;
	inline static constexpr uintptr_t method_get_flags = 0x844FF0;
	inline static constexpr uintptr_t field_get_offset = 0x7D34B0;
	inline static constexpr uintptr_t field_get_type = 0x442E60;
	inline static constexpr uintptr_t field_get_parent = 0xC0C0;
	inline static constexpr uintptr_t field_get_name = 0x3CB0;
	inline static constexpr uintptr_t field_get_flags = 0x844530;
	inline static constexpr uintptr_t field_static_get_value = 0x844690;
	inline static constexpr uintptr_t object_get_class = 0x3CB0;
	inline static constexpr uintptr_t object_new = 0x845300;
	inline static constexpr uintptr_t resolve_icall = 0x843550;
	inline static constexpr uintptr_t gchandle_get_target = 0x8449C0;
	inline static constexpr uintptr_t gchandle_new = 0x844970;
	inline static constexpr uintptr_t gchandle_free = 0x844A60;
	inline static constexpr uintptr_t array_new = 0x843570;
	inline static constexpr uintptr_t string_new = 0x845420;
};
inline static constexpr uintptr_t il2cpphandle = 0x10088B40;
struct gc_handles
{
	inline static constexpr uintptr_t array_rva = 0x10088B40;
};
struct klass_rvas
{
	inline static constexpr uintptr_t BaseNetworkable = 0xFB71410;
	inline static constexpr uintptr_t BaseEntity = 0xFC1E710;
	inline static constexpr uintptr_t BaseCombatEntity = 0xFC1A178;
	inline static constexpr uintptr_t BasePlayer = 0xFBE98B0;
	inline static constexpr uintptr_t BaseNpc = 0xFC7B490;
	inline static constexpr uintptr_t BaseVehicle = 0xFB65CF0;
	inline static constexpr uintptr_t DroppedItemContainer = 0xFB6B4C8;
	inline static constexpr uintptr_t OreResourceEntity = 0xFC0E9D8;
	inline static constexpr uintptr_t CollectibleEntity = 0xFB6AD80;
	inline static constexpr uintptr_t BuildingBlock = 0xFB87C50;
	inline static constexpr uintptr_t BuildingPrivlidge = 0xFBC6868;
	inline static constexpr uintptr_t Door = 0xFB7E9E0;
	inline static constexpr uintptr_t WorldItem = 0xFC16060;
	inline static constexpr uintptr_t Signage = 0xFB87288;
	inline static constexpr uintptr_t MainCamera = 0xFBE49A0;
	inline static constexpr uintptr_t PlayerEyes = 0xFD387E0;
	inline static constexpr uintptr_t PlayerModel = 0xFC3A518;
	inline static constexpr uintptr_t TOD_Sky = 0xFC0CE40;
	inline static constexpr uintptr_t Item = 0xFBB91C8;
	inline static constexpr uintptr_t ItemId = 0xFBB90B0;
	inline static constexpr uintptr_t ConsoleSystem_Command = 0xFBAAC68;
	inline static constexpr uintptr_t PlayerInventory_typenav = 0xFD38800;
	inline static constexpr uintptr_t BaseNetworkable_TypeInfo = 0xFB71410;
	inline static constexpr uintptr_t BaseEntity_TypeInfo = 0xFC1E710;
	inline static constexpr uintptr_t BasePlayer_TypeInfo = 0xFBE98B0;
	inline static constexpr uintptr_t PlayerEyes_TypeInfo = 0xFD387E0;
	inline static constexpr uintptr_t PlayerInventory_TypeInfo = 0xFD38800;
	inline static constexpr uintptr_t PlayerModel_TypeInfo = 0xFC3A518;
	inline static constexpr uintptr_t ModelState_TypeInfo = 0x1009BB00;
	inline static constexpr uintptr_t PlayerInput_TypeInfo = 0xFC20980;
	inline static constexpr uintptr_t BaseProjectile_TypeInfo = 0xFC027F0;
	inline static constexpr uintptr_t HeldEntity_TypeInfo = 0xFB6B2C8;
	inline static constexpr uintptr_t BaseViewModel_TypeInfo = 0xFC8D8A0;
	inline static constexpr uintptr_t AutoTurret_TypeInfo = 0x0;
	inline static constexpr uintptr_t PlayerCorpse_TypeInfo = 0x10090798;
	inline static constexpr uintptr_t LootableCorpse_TypeInfo = 0x1008DD00;
	inline static constexpr uintptr_t GameManager_TypeInfo = 0x100940C0;
	inline static constexpr uintptr_t GameManager_Static_TypeInfo = 0xFC074B0;
};
namespace PhysX
{
	constexpr std::uintptr_t type_info = 0xFB84618;
	constexpr std::uintptr_t static_fields = 0xb8;
}
namespace game_manager
{
	constexpr std::uintptr_t game_manager = 0xFC074B0;
	constexpr std::uintptr_t static_fields = 0xb8;
}
namespace console_system
{
	constexpr std::uintptr_t find = 0x3766510;
	constexpr std::uintptr_t get_override = 0x78;
	constexpr std::uintptr_t set_override = 0x30;
	constexpr std::uintptr_t call = 0x90;
}
namespace klass_layout
{
	constexpr std::uintptr_t name_ptr = 0x10;
	constexpr std::uintptr_t static_fields = 0xb8;
}
namespace base_networkable
{
	constexpr std::uintptr_t static_fields = 0xb8;
	constexpr std::uintptr_t wrapper_class_ptr = 0x8;
	constexpr std::uintptr_t parent_static_fields = 0x10;
	constexpr std::uintptr_t hv_offset = 0x18;
	constexpr std::uintptr_t entities = 0x10;
	constexpr std::uint32_t buffer_list_array = 0x10;
	constexpr std::uint32_t buffer_list_size = 0x18;
	constexpr std::uintptr_t client_entities_decryption = 0x1163F00;
	constexpr std::uintptr_t entity_list_wrapper = 0x45C1A30;
	constexpr std::uintptr_t entity_list_decryption = 0x125F320;
	constexpr std::uintptr_t entity = 0x10;
	constexpr std::uintptr_t buffer = 0x10;
	constexpr std::uintptr_t prefabID = 0x54;
	constexpr std::uintptr_t parentEntity = 0x38;
	constexpr std::uintptr_t children = 0x88;
	constexpr std::uintptr_t net = 0xb8;
	constexpr std::uintptr_t globalBroadcast = 0x58;
	constexpr std::uintptr_t networkRange = 0x64;
}
namespace camera
{
	constexpr std::uintptr_t camera_static = 0xb8;
	constexpr std::uintptr_t static_fields = 0xb8;
	constexpr std::uintptr_t camera_object = 0x38;
	constexpr std::uintptr_t instance = 0x38;
	constexpr std::uintptr_t buffer = 0x38;
	constexpr std::uintptr_t entity = 0x10;
	constexpr std::uintptr_t position = 0x444;
	constexpr std::uintptr_t viewMatrix = 0x2fc;
	constexpr std::uintptr_t projectionMatrix = 0x18c;
	constexpr std::uint32_t projection_layout = 0x1;
	constexpr std::uintptr_t fieldOfView = 0x170;
	constexpr std::uintptr_t aspect = 0x4e0;
	constexpr std::uintptr_t nearClip = 0x3ec;
	constexpr std::uintptr_t farClip = 0x3f8;
	constexpr std::uintptr_t viewProjectionMatrix = 0x2fc;
	constexpr std::uintptr_t worldToCameraMatrix = 0x70;
	constexpr std::uintptr_t cullingMask = 0x3e8;
}
namespace BasePlayer
{
	constexpr std::uintptr_t clActiveItem = 0x568;
	constexpr std::uintptr_t PlayerEyes = 0x380;
	constexpr std::uintptr_t PlayerInventory = 0x548;
	constexpr std::uintptr_t current_team = 0x538;
	constexpr std::uintptr_t base_movement = 0x4f8;
	constexpr std::uintptr_t player_model = 0x6d8;
	constexpr std::uintptr_t player_flags = 0x6b8;
	constexpr std::uintptr_t userIDString = 0x0;
	constexpr std::uintptr_t display_name = 0x3e0;
	constexpr std::uintptr_t player_input = 0x2d0;
	constexpr std::uintptr_t mounted = 0x5c0;
	constexpr std::uintptr_t weaponMoveSpeedScale = 0x798;
	constexpr std::uintptr_t clothingBlocksAiming = 0x79c;
	constexpr std::uintptr_t player_rigidbody = 0x490;
	constexpr std::uintptr_t frozen = 0x388;
	constexpr std::uintptr_t current_gesture = 0x520;
}
namespace BaseCombatEntity
{
	constexpr std::uintptr_t skeletonProperties = 0x220;
	constexpr std::uintptr_t baseProtection = 0x228;
	constexpr std::uintptr_t lifestate = 0x298;
	constexpr std::uintptr_t markAttackerHostile = 0x29e;
	constexpr std::uintptr_t model = 0x1a8;
	constexpr std::uintptr_t _health = 0x2a4;
	constexpr std::uintptr_t _maxHealth = 0x2a8;
}
namespace BaseEntity
{
	constexpr std::uintptr_t bounds = 0x17c;
	constexpr std::uintptr_t model = 0x1a8;
	constexpr std::uintptr_t flags = 0x1b0;
	constexpr std::uintptr_t triggers = 0x98;
	constexpr std::uintptr_t positionLerp = 0x1c8;
}
namespace BaseEntityFlags
{
	constexpr std::uintptr_t Placeholder = 0x1;
	constexpr std::uintptr_t On = 0x2;
	constexpr std::uintptr_t OnFire = 0x4;
	constexpr std::uintptr_t Open = 0x8;
	constexpr std::uintptr_t Locked = 0x10;
	constexpr std::uintptr_t Debugging = 0x20;
	constexpr std::uintptr_t Disabled = 0x40;
	constexpr std::uintptr_t Reserved1 = 0x80;
	constexpr std::uintptr_t Reserved2 = 0x100;
	constexpr std::uintptr_t Reserved3 = 0x200;
	constexpr std::uintptr_t Reserved4 = 0x400;
	constexpr std::uintptr_t Reserved5 = 0x800;
}
namespace base_player_flags
{
	constexpr std::uintptr_t Unused1 = 0x1;
	constexpr std::uintptr_t Unused2 = 0x2;
	constexpr std::uintptr_t IsAdmin = 0x4;
	constexpr std::uintptr_t ReceivingSnapshot = 0x8;
	constexpr std::uintptr_t Sleeping = 0x10;
	constexpr std::uintptr_t Spectating = 0x20;
	constexpr std::uintptr_t Wounded = 0x40;
	constexpr std::uintptr_t IsDeveloper = 0x80;
	constexpr std::uintptr_t Connected = 0x100;
	constexpr std::uintptr_t ThirdPersonViewmode = 0x400;
}
namespace ModelState
{
	constexpr std::uintptr_t flags = 0x0;
	constexpr std::uintptr_t waterLevel = 0x0;
	constexpr std::uintptr_t Flying = 0x40;
	constexpr std::uintptr_t Sleeping = 0x8;
	constexpr std::uintptr_t Mounted = 0x200;
	constexpr std::uintptr_t OnGround = 0x4;
}
namespace ItemContainer
{
	constexpr std::uintptr_t ItemList = 0x48;
	constexpr std::uintptr_t flags = 0x7c;
}
namespace ItemDefinition
{
	constexpr std::uintptr_t itemid = 0x20;
	constexpr std::uintptr_t shortname = 0x28;
	constexpr std::uintptr_t displayName = 0x40;
	constexpr std::uintptr_t category = 0x58;
	constexpr std::uintptr_t stackable = 0x78;
	constexpr std::uintptr_t iconSprite = 0x50;
	constexpr std::uintptr_t rarity = 0x94;
	constexpr std::uintptr_t condition = 0xb8;
	constexpr std::uintptr_t ItemModWearable = 0x188;
}
namespace item
{
	constexpr std::uintptr_t info = 0xb0;
	constexpr std::uintptr_t itemdefinition = 0xb0;
	constexpr std::uintptr_t uid = 0xa0;
	constexpr std::uintptr_t amount = 0xe0;
	constexpr std::uintptr_t shortname = 0x28;
	constexpr std::uintptr_t category = 0x58;
	constexpr std::uintptr_t HeldEntity = 0x78;
}
namespace BaseProjectile
{
	constexpr std::uintptr_t aimCone = 0x400;
	constexpr std::uintptr_t hipAimCone = 0x404;
	constexpr std::uintptr_t aimSway = 0x3e8;
	constexpr std::uintptr_t aimSwaySpeed = 0x3ec;
	constexpr std::uintptr_t recoil = 0x3f0;
	constexpr std::uintptr_t projectileVelocityScale = 0x37c;
	constexpr std::uintptr_t automatic = 0x380;
	constexpr std::uintptr_t reloadTime = 0x3c0;
	constexpr std::uintptr_t primaryMagazine = 0x3c8;
	constexpr std::uintptr_t magazine = 0x3c8;
	constexpr std::uintptr_t isReloading = 0x3d0;
	constexpr std::uintptr_t stancePenalty = 0x418;
	constexpr std::uintptr_t aimconePenalty = 0x408;
	constexpr std::uintptr_t sightAimConeScale = 0x45c;
	constexpr std::uintptr_t sight_aim_cone_scale = 0x45c;
	constexpr std::uintptr_t hipAimConeScale = 0x464;
	constexpr std::uintptr_t hip_aim_cone_scale = 0x464;
	constexpr std::uintptr_t fractionalReload = 0x3d0;
	constexpr std::uintptr_t aimconeCurve = 0x3f8;
	constexpr std::uintptr_t noAimingWhileCycling = 0x41d;
	constexpr std::uintptr_t cachedModHash = 0x458;
	constexpr std::uintptr_t hipAimConeOffset = 0x468;
	constexpr std::uintptr_t sightAimConeOffset = 0x460;
	constexpr std::uintptr_t aimconePenaltyPerShot = 0x408;
	constexpr std::uintptr_t hasADS = 0x41c;
	constexpr std::uintptr_t isBurstWeapon = 0x427;
	constexpr std::uintptr_t canChangeFireModes = 0x428;
	constexpr std::uintptr_t internalBurstFireRateScale = 0x430;
	constexpr std::uintptr_t gravity_modifier = 0x38;
	constexpr std::uintptr_t drag = 0x34;
	constexpr std::uintptr_t string_hold_duration_max = 0x4c0;
}
namespace BaseMelee
{
	constexpr std::uintptr_t damageProperties = 0x378;
	constexpr std::uintptr_t maxDistance = 0x390;
	constexpr std::uintptr_t attackRadius = 0x394;
	constexpr std::uintptr_t blockSprintOnAttack = 0x399;
	constexpr std::uintptr_t gathering = 0x3d0;
	constexpr std::uintptr_t canThrowAsProjectile = 0x370;
}
namespace ItemModProjectile
{
	constexpr std::uintptr_t projectileObject = 0x20;
	constexpr std::uintptr_t ammoType = 0x30;
	constexpr std::uintptr_t projectileSpread = 0x3c;
	constexpr std::uintptr_t projectileVelocity = 0x40;
	constexpr std::uintptr_t projectileVelocitySpread = 0x44;
	constexpr std::uintptr_t useCurve = 0x48;
	constexpr std::uintptr_t spreadScalar = 0x50;
	constexpr std::uintptr_t category = 0x68;
}
namespace server_projectile
{
	constexpr std::uintptr_t drag = 0x34;
	constexpr std::uintptr_t gravityModifier = 0x38;
	constexpr std::uintptr_t speed = 0x3c;
	constexpr std::uintptr_t radius = 0x5c;
}
namespace RecoilProperties
{
	constexpr std::uintptr_t recoilYawMin = 0x18;
	constexpr std::uintptr_t recoilYawMax = 0x1c;
	constexpr std::uintptr_t recoilPitchMin = 0x20;
	constexpr std::uintptr_t recoilPitchMax = 0x24;
	constexpr std::uintptr_t timeToTakeMin = 0x28;
	constexpr std::uintptr_t timeToTakeMax = 0x2c;
	constexpr std::uintptr_t ADSScale = 0x30;
	constexpr std::uintptr_t movementPenalty = 0x34;
	constexpr std::uintptr_t clampPitch = 0x38;
	constexpr std::uintptr_t pitchCurve = 0x40;
	constexpr std::uintptr_t yawCurve = 0x48;
	constexpr std::uintptr_t newRecoilOverride = 0x80;
	constexpr std::uintptr_t overrideAimconeWithCurve = 0x5c;
	constexpr std::uintptr_t aimconeProbabilityCurve = 0x70;
	constexpr std::uintptr_t aimconeCurveScale = 0x60;
}
namespace FlintStrikeWeapon
{
	constexpr std::uintptr_t successFraction = 0x4a8;
	constexpr std::uintptr_t strikeRecoil = 0x4b0;
	constexpr std::uintptr_t _didSparkThisFrame = 0x4b8;
}
namespace PlayerEyes
{
	constexpr std::uintptr_t viewOffset = 0x40;
	constexpr std::uintptr_t bodyRotation = 0x50;
	constexpr std::uintptr_t world_position = 0x60;
	constexpr std::uintptr_t worldPosition = 0x60;
}
namespace PlayerInput
{
	constexpr std::uintptr_t state = 0x28;
	constexpr std::uintptr_t bodyAngles = 0x44;
	constexpr std::uintptr_t yaw = 0x60;
}
namespace PlayerModel
{
	constexpr std::uintptr_t boneTransforms = 0x98;
	constexpr std::uintptr_t _multiMesh = 0x3d0;
	constexpr std::uintptr_t collision = 0xd0;
	constexpr std::uintptr_t fullMask = 0x208;
	constexpr std::uintptr_t newVelocity = 0x31c;
	constexpr std::uintptr_t isNpc = 0x468;
	constexpr std::uintptr_t visible = 0xc4;
	constexpr std::uintptr_t position = 0x2f8;
}
namespace SkinnedMultiMesh
{
	constexpr std::uintptr_t RendererList = 0x58;
	constexpr std::uintptr_t Renderers = 0x58;
}
namespace chams
{
	constexpr std::uintptr_t player_model_multi_mesh = 0x3d0;
	constexpr std::uintptr_t multi_mesh_renderer_list = 0x58;
}
namespace PlayerInventory
{
	constexpr std::uintptr_t loot = 0x48;
	constexpr std::uintptr_t containerBelt = 0x38;
	constexpr std::uintptr_t containerMain = 0x30;
	constexpr std::uintptr_t containerWear = 0x58;
}
namespace base_movement
{
	constexpr std::uintptr_t adminCheat = 0x20;
	constexpr std::uintptr_t Owner = 0x28;
}
namespace PlayerWalkMovement
{
	constexpr std::uintptr_t GroundAngle = 0x70;
	constexpr std::uintptr_t GroundAngleNew = 0x78;
	constexpr std::uintptr_t GroundTime = 0x80;
	constexpr std::uintptr_t JumpTime = 0x88;
	constexpr std::uintptr_t LandTime = 0x90;
	constexpr std::uintptr_t GravityMultiplier = 0x98;
	constexpr std::uintptr_t TargetMovement = 0x128;
	constexpr std::uintptr_t capsule = 0xf0;
	constexpr std::uintptr_t ladder = 0xd8;
	constexpr std::uintptr_t modify = 0x1b8;
}
namespace WorldItem
{
	constexpr std::uintptr_t item = 0x1f8;
	constexpr std::uintptr_t allowPickup = 0x1f0;
}
namespace AutoTurret
{
	constexpr std::uintptr_t authorizedPlayers = 0x3a0;
	constexpr std::uintptr_t lastYaw = 0x430;
	constexpr std::uintptr_t muzzlePos = 0x4a0;
	constexpr std::uintptr_t gun_yaw = 0x4b8;
	constexpr std::uintptr_t gun_pitch = 0x4c0;
	constexpr std::uintptr_t sightRange = 0x4c8;
}
namespace PlayerCorpse
{
	constexpr std::uintptr_t clientClothing = 0x340;
}
namespace LootableCorpse
{
	constexpr std::uintptr_t playerSteamID = 0x308;
	constexpr std::uintptr_t _playerName = 0x318;
}
namespace HackableLockedCrate
{
	constexpr std::uint32_t hackSeconds = 0x400;
	constexpr std::uintptr_t timerText = 0x3f0;
}
namespace HeldEntity
{
	constexpr std::uintptr_t ownerItemUID = 0x2d0;
	constexpr std::uintptr_t viewModel = 0x240;
	constexpr std::uint32_t viewModel_wrapper_kind = 0x4;
	constexpr std::uint32_t viewModel_inner_off = 0x28;
	constexpr std::uintptr_t _punches = 0x230;
}
namespace ItemIcon
{
	constexpr std::uintptr_t item_icon_c = 0xFC1D100;
	constexpr std::uintptr_t static_fields = 0xb8;
	constexpr std::uintptr_t backgroundImage = 0xe8;
}
namespace ViewModel
{
	constexpr std::uintptr_t instance = 0x28;
	constexpr std::uintptr_t viewmodel_instance = 0x28;
}
namespace BaseViewModel
{
	constexpr std::uintptr_t useViewModelCamera = 0x40;
	constexpr std::uintptr_t model = 0x0;
	constexpr std::uintptr_t bob = 0xd0;
	constexpr std::uintptr_t lower = 0xb8;
	constexpr std::uintptr_t sway = 0xc8;
	constexpr std::uintptr_t ironSights = 0xa8;
}
namespace model
{
	constexpr std::uintptr_t rootBone = 0x28;
	constexpr std::uintptr_t headBone = 0x30;
	constexpr std::uintptr_t eyeBone = 0x38;
	constexpr std::uintptr_t boneTransforms = 0x50;
	constexpr std::uintptr_t boneNames = 0x58;
	constexpr std::uintptr_t bone_transform = 0x50;
	constexpr std::uintptr_t pelvis_bone_idx = 0x0;
	constexpr std::uintptr_t l_hip_bone_idx = 0x1;
	constexpr std::uintptr_t l_knee_bone_idx = 0x3;
	constexpr std::uintptr_t l_foot_bone_idx = 0x4;
	constexpr std::uintptr_t r_hip_bone_idx = 0xe;
	constexpr std::uintptr_t r_knee_bone_idx = 0x10;
	constexpr std::uintptr_t r_foot_bone_idx = 0x11;
	constexpr std::uintptr_t spine2_bone_idx = 0x15;
	constexpr std::uintptr_t spine4_bone_idx = 0x17;
	constexpr std::uintptr_t l_clavicle_bone_idx = 0x18;
	constexpr std::uintptr_t l_upperarm_bone_idx = 0x19;
	constexpr std::uintptr_t l_forearm_bone_idx = 0x1a;
	constexpr std::uintptr_t l_hand_bone_idx = 0x1d;
	constexpr std::uintptr_t r_clavicle_bone_idx = 0x3c;
	constexpr std::uintptr_t r_upperarm_bone_idx = 0x3d;
	constexpr std::uintptr_t r_forearm_bone_idx = 0x3e;
	constexpr std::uintptr_t r_hand_bone_idx = 0x41;
	constexpr std::uintptr_t neck_bone_idx = 0x34;
	constexpr std::uintptr_t head_bone_idx = 0x35;
	constexpr std::uintptr_t bone_array_count_off = 0x18;
	constexpr std::uintptr_t bone_array_data_off = 0x20;
	constexpr std::uintptr_t bone_array_elem_size = 0x8;
}
namespace EffectNetwork
{
	constexpr std::uintptr_t type_info = 0xFBF9080;
	constexpr std::uintptr_t static_type_info = 0xFBFD480;
	constexpr std::uintptr_t static_fields = 0xb8;
	constexpr std::uintptr_t instance = 0x8;
	constexpr std::uintptr_t hitPosition = 0x84;
}
namespace singleton_typeinfos
{
	constexpr std::uintptr_t SingletonComponent_UI_LoadingScreen = 0xFC76D38;
	constexpr std::uintptr_t SingletonComponent_MixerSnapshotManager = 0xFC32768;
}
namespace TOD_Sky_Static
{
	constexpr std::uintptr_t tod_sky_c = 0xFC0CE40;
	constexpr std::uintptr_t static_fields = 0xb8;
	constexpr std::uintptr_t instance = 0x20;
}
namespace TOD_Sky
{
	constexpr std::uintptr_t Cycle = 0x40;
	constexpr std::uintptr_t Atmosphere = 0x50;
	constexpr std::uintptr_t Day = 0x58;
	constexpr std::uintptr_t Night = 0x60;
	constexpr std::uintptr_t Stars = 0x78;
	constexpr std::uintptr_t Clouds = 0x80;
	constexpr std::uintptr_t Ambient = 0x98;
	constexpr std::uintptr_t timeSinceAmbientUpdate = 0x23c;
	constexpr std::uintptr_t timeSinceReflectionUpdate = 0x240;
}
namespace TOD_CycleParameters
{
	constexpr std::uintptr_t Hour = 0x10;
	constexpr std::uintptr_t Day = 0x14;
	constexpr std::uintptr_t Month = 0x18;
	constexpr std::uintptr_t Year = 0x1c;
}
namespace tod_ambient_parameters
{
	constexpr std::uintptr_t Saturation = 0x14;
}
namespace tod_night_parameters
{
	constexpr std::uintptr_t AmbientMultiplier = 0x5c;
}
namespace unity_transform_native
{
	constexpr std::uintptr_t access_struct_off = 0x28;
}
namespace transform_data
{
	constexpr std::uintptr_t pos_base_off = 0x18;
	constexpr std::uintptr_t indices_b_off = 0x20;
	constexpr std::uintptr_t root_pos_off = 0x90;
	constexpr std::uintptr_t root_quat_off = 0xa0;
	constexpr std::uintptr_t matrix_stride = 0x30;
	constexpr std::uintptr_t walker_max_depth = 0x10;
	constexpr std::uintptr_t slot_pad_off = 0xc;
	constexpr std::uintptr_t slot_quat_off = 0x10;
	constexpr std::uintptr_t slot_scale_off = 0x20;
	constexpr std::uintptr_t slot_pos_quat_size = 0x20;
}
namespace Object
{
	constexpr std::uintptr_t m_CachedPtr = 0x10;
}
namespace unity_object
{
	constexpr std::uintptr_t cached_ptr = 0x10;
}
namespace unity_component
{
	constexpr std::uintptr_t game_object = 0x20;
	constexpr std::uintptr_t m_GameObject = 0x20;
}
namespace unity_game_object
{
	constexpr std::uintptr_t components = 0x20;
	constexpr std::uintptr_t component_count = 0x30;
	constexpr std::uintptr_t component_stride = 0x10;
	constexpr std::uintptr_t component_ptr_in_entry = 0x8;
	constexpr std::uintptr_t component_handle_in_native = 0x18;
}
namespace unity_transform
{
	constexpr std::uintptr_t indirect_ptr_off = 0x28;
	constexpr std::uintptr_t world_pos_off = 0x90;
}
namespace unity_string
{
	constexpr std::uintptr_t m_stringLength = 0x10;
	constexpr std::uintptr_t first_char = 0x14;
}
namespace system_list
{
	constexpr std::uintptr_t array = 0x10;
	constexpr std::uintptr_t size = 0x18;
	constexpr std::uintptr_t array_first_element = 0x20;
}
namespace convar_graphics
{
	constexpr std::uintptr_t convar_graphics = 0xFBB2578;
	constexpr std::uintptr_t convar_graphics_instance = 0xb8;
	constexpr std::uintptr_t fov = 0xb8;
}
namespace convar_admin
{
	constexpr std::uintptr_t convar_admin = 0xFC2A3E0;
	constexpr std::uintptr_t playerIds = 0x20;
}
namespace admin_movement
{
	constexpr std::uintptr_t ground_angle = 0x70;
	constexpr std::uintptr_t ground_angle_new = 0x78;
	constexpr std::uintptr_t GroundTime = 0x80;
	constexpr std::uintptr_t JumpTime = 0x88;
	constexpr std::uintptr_t LandTime = 0x90;
	constexpr std::uintptr_t GravityMultiplier = 0x98;
	constexpr std::uintptr_t TargetMovement = 0x128;
}

uintptr_t decryption::client_entities(uint64_t a1)
{
	std::uintptr_t rax = driver.read<std::uintptr_t>(a1 + 0x18);
	std::uint32_t* rdx = (std::uint32_t*)&rax;
	std::uint32_t r8d = 0x2;
	std::uint32_t eax, ecx;
	do {
		ecx = *(std::uint32_t*)(rdx);
		eax = *(std::uint32_t*)(rdx);
		rdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);
		eax = ecx;
		ecx = ecx << 0x15;
		eax = eax >> 0xB;
		ecx = ecx | eax;
		ecx = ecx ^ 0xDA053B90;
		ecx = ecx - 0x4D1B2722;
		*((std::uint32_t*)rdx - 1) = ecx;
		--r8d;
	} while (r8d);
	return il2cpp_get_handle(rax);
}

uintptr_t decryption::entity_list(uint64_t a1)
{
	std::uintptr_t rax = driver.read<std::uintptr_t>(a1 + 0x18);
	std::uint32_t* rdx = (std::uint32_t*)&rax;
	std::uint32_t r8d = 0x2;
	std::uint32_t eax, ecx;
	do {
		ecx = *(std::uint32_t*)(rdx);
		eax = *(std::uint32_t*)(rdx);
		rdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);
		eax = ecx;
		ecx = ecx << 0x6;
		eax = eax >> 0x1A;
		ecx = ecx | eax;
		ecx = ecx + 0xC52F0932;
		eax = ecx;
		ecx = ecx << 0x16;
		eax = eax >> 0xA;
		ecx = ecx | eax;
		*((std::uint32_t*)rdx - 1) = ecx;
		--r8d;
	} while (r8d);
	return il2cpp_get_handle(rax);
}

uintptr_t decryption::clActiveItem(uint64_t a1)
{
	std::uint32_t* rdx = (std::uint32_t*)&a1;
	std::uint32_t r9d = 0x1;
	std::uint32_t eax, edx;
	do {
		edx = *(std::uint32_t*)(rdx);
		eax = *(std::uint32_t*)(rdx);
		rdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);
		edx = edx + 0x5A0D1F9B;
		edx = (edx << 0x19) | (edx >> 0x7);
		edx = edx ^ 0xAFFC2B31;
		*((std::uint32_t*)rdx - 1) = edx;
		--r9d;
	} while (r9d);
	return a1;
}

uintptr_t decryption::PlayerInventory(uint64_t a1)
{
	std::uintptr_t rax = driver.read<std::uintptr_t>(a1 + 0x18);
	std::uint32_t* rdx = (std::uint32_t*)&rax;
	std::uint32_t r8d = 0x2;
	std::uint32_t eax, ecx;
	do {
		ecx = *(std::uint32_t*)(rdx);
		eax = *(std::uint32_t*)(rdx);
		rdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);
		ecx = ecx ^ 0x6562778;
		eax = ecx;
		ecx = ecx << 0x6;
		eax = eax >> 0x1A;
		ecx = ecx | eax;
		ecx = ecx ^ 0x7EC38BFC;
		*((std::uint32_t*)rdx - 1) = ecx;
		--r8d;
	} while (r8d);
	return il2cpp_get_handle(rax);
}

uintptr_t decryption::PlayerEyes(uint64_t a1)
{
	std::uintptr_t rax = driver.read<std::uintptr_t>(a1 + 0x18);
	std::uint32_t* rdx = (std::uint32_t*)&rax;
	std::uint32_t r8d = 0x2;
	std::uint32_t eax, ecx;
	do {
		ecx = *(std::uint32_t*)(rdx);
		eax = *(std::uint32_t*)(rdx);
		rdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);
		ecx = ecx + 0xBCFC6DA8;
		eax = ecx;
		ecx = ecx << 0x13;
		eax = eax >> 0xD;
		ecx = ecx | eax;
		ecx = ecx ^ 0x73437527;
		eax = ecx;
		ecx = ecx << 0xA;
		eax = eax >> 0x16;
		ecx = ecx | eax;
		*((std::uint32_t*)rdx - 1) = ecx;
		--r8d;
	} while (r8d);
	return il2cpp_get_handle(rax);
}

inline uint32_t decryption::decrypt_fov(uint32_t val) {
	val = (val << 0x3) | (val >> 0x1D);
	val += 0x1180A466;
	val ^= 0x91BFA9C2;
	return val;
}

inline uint32_t decryption::encrypt_fov(uint32_t val) {
	val ^= 0x91BFA9C2;
	val -= 0x1180A466;
	val = (val >> 0x3) | (val << 0x1D);
	return val;
}
