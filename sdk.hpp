#pragma once
#include "offsets.hpp"
#include "OffsetManager.hpp"
#include "vectorMath.hpp"
#include "draw.hpp"
#include <xmmintrin.h>
#include <emmintrin.h>
#include "skcrypt.hpp"
#include "Occlusion.hpp"
#include <map>
#include <array>
#include <unordered_map>
#include <mutex>
#include <D3DX11tex.h>
#include <filesystem>
#include "ida.h"
#pragma comment(lib, "D3DX11.lib")
#include <cstdint>
#include "globals.h"
#include "Logger.hpp"

#define TEST_BITD(value, bit) ((value) & (1 << (bit)))
//#define LODWORD(l) ((DWORD)(l))
//typedef unsigned long _DWORD;

//inline uint64_t __PAIR64__(uint32_t high, uint32_t low) {
//    return ((uint64_t)high << 32) | low;
//}
//
//template<class T> T __ROL__(T value, int count) {
//    const uintptr_t nbits = sizeof(T) * 8;
//    if (count > 0) {
//        count %= nbits;
//        T high = value >> (nbits - count);
//        if (T(-1) < 0) high &= ~((T(-1) << count));
//        value <<= count;
//        value |= high;
//    }
//    else {
//        count = -count % nbits;
//        T low = value << (nbits - count);
//        value >>= count;
//        value |= low;
//    }
//    return value;
//}

//inline uint32_t __ROR4__(uint32_t value, int count) { return __ROL__((uint32_t)value, -count); }
inline uint64_t GameAssembly;
inline uint64_t UnityPlayer;
inline uint64_t RustClient;

inline std::string getASCIIName(std::wstring name) {
    return std::string(name.begin(), name.end());
}

struct monostr { char buffer[128]; };
struct dynamic_array { uint64_t base; uint64_t mem_id; uint64_t sz; uint64_t cap; };

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: 23810444
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: 23810444
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: sha-dumper
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: 23824285
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: 23824285
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: 23824285
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: sha-dumper
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

// ============================================================
// Auto-generated decrypt namespace patch from Morphine
// Build: 24020771
// Includes: Il2cppGetHandle, type-aware read helpers, decrypt functions
// ============================================================

namespace decrypt {

    // ============================================================
    // Runtime pointer validation
    // ============================================================

    inline bool IsValidPointer(uint64_t p)
    {
        if (!p) return false;
        if (p & 0x7) return false;                 // basic 8-byte alignment
        if (p < 0x10000) return false;             // reject null/small values
        return true;
    }

    inline bool IsInModuleRange(uint64_t p)
    {
        // Fast range check against loaded module bases.
        // Falls back to true if bases are not initialized yet.
        if (!GameAssembly && !UnityPlayer) return true;
        uint64_t ga_end = GameAssembly ? GameAssembly + 0x40000000 : 0; // ~1GB generous
        uint64_t up_end = UnityPlayer ? UnityPlayer + 0x20000000 : 0;
        if (GameAssembly && p >= GameAssembly && p <= ga_end) return true;
        if (UnityPlayer && p >= UnityPlayer && p <= up_end) return true;
        return false;
    }

    // ============================================================
    // Il2Cpp GCHandle resolver
    // Derived from il2cpp_gchandle_get_target disassembly.
    // ============================================================

    inline uint64_t Il2cppGetHandle(uint64_t handle_ptr)
    {
        if (!handle_ptr) return 0;

        uint64_t page_base = handle_ptr & 0xFFFFFFFFFFFFE000ULL;
        if (!page_base) return 0;

        uint8_t type = read<uint8_t>(page_base + 0x20);
        if (type >= 4) return 0;

        int64_t slot = (int64_t)(handle_ptr - page_base - 0x28) >> 3;
        if (slot < 0) return 0;

        uint32_t size = read<uint32_t>(page_base + 0x1C);
        if ((uint32_t)slot >= size) return 0;

        uint64_t bitmap_ptr = read<uint64_t>(page_base + 0x10);
        if (!bitmap_ptr) return 0;

        uint32_t bitmask = read<uint32_t>(bitmap_ptr + 4 * ((uint32_t)slot >> 5));
        if (!((bitmask >> (slot & 0x1F)) & 1)) return 0;

        uint64_t entry = read<uint64_t>(page_base + 8 * ((uint32_t)slot + 5));

        if (type > 1)
            return entry;

        // type 0/1: real helper returns 0 for empty slots instead of ~0
        if (!entry) return 0;
        return ~entry;
    }

    // ============================================================
    // Type-aware read helpers
    // Use these for fields classified by the updater.
    // ============================================================

    inline uint64_t ReadObjectPtr(uint64_t address)
    {
        uint64_t handle = read<uint64_t>(address);
        if (!IsValidPointer(handle)) return 0;
        return Il2cppGetHandle(handle);
    }

    inline uint64_t ReadRawPtr(uint64_t address)
    {
        uint64_t p = read<uint64_t>(address);
        return IsValidPointer(p) ? p : 0;
    }

    inline uint64_t ReadTaggedPtr(uint64_t address, uint64_t (*decrypt_fn)(uint64_t))
    {
        uint64_t encoded = read<uint64_t>(address);
        if (!encoded) return 0;
        uint64_t decoded = decrypt_fn(encoded);
        return IsValidPointer(decoded) ? decoded : 0;
    }

    inline uint64_t ReadObjectTagged(uint64_t address, uint64_t (*decrypt_fn)(uint64_t))
    {
        uint64_t encoded = read<uint64_t>(address);
        if (!encoded) return 0;
        uint64_t decoded = decrypt_fn(encoded);
        if (!decoded) return 0;
        return Il2cppGetHandle(decoded);
    }

    // ============================================================
    // Handle resolver � preserved across updates (not Morphine-generated)
    // ============================================================

    inline uint64_t resolve_possible_handle(uint64_t value)
    {
        if (!value) return 0;
        if (value & 1)
            return Il2cppGetHandle(value);
        // If not 8-byte aligned, it might be a disguised GCHandle � try Il2cppGetHandle
        if (value & 0x7) {
            uint64_t resolved = Il2cppGetHandle(value);
            if (resolved && (resolved & 0x7) == 0 && resolved > 0x10000)
                return resolved;
        }
        // Aligned value: check if pointer-to-pointer
        if ((value & 0x7) == 0 && value < 0x7FFFFFFFFFFF) {
            uint64_t deref = read<uint64_t>(value);
            if (deref && deref < 0x7FFFFFFFFFFF && (deref & 0x7) == 0 && deref != value)
                return deref;
        }
        return value;
    }

    // networkable_key (client_entities): rol-sub-xor-add
    inline uintptr_t networkable_key(uint64_t a1)
    {
        uint64_t value = read<uint64_t>(a1 + 0x18);
        if (!value) return 0;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = (x << OffsetManager::DecryptCfg.nk_rol) | (x >> (32 - OffsetManager::DecryptCfg.nk_rol)); // ROL
            uint32_t v2 = v1 - OffsetManager::DecryptCfg.nk_sub; // SUB
            uint32_t v3 = v2 ^ OffsetManager::DecryptCfg.nk_xor; // XOR
            uint32_t v4 = v3 + OffsetManager::DecryptCfg.nk_add; // ADD
            *data = v4;
            data++;
            --count;
        } while (count);
        uintptr_t resolved = Il2cppGetHandle(value);
        if (resolved) return resolved;
        return (value & 0x7) == 0 ? value : 0;
    }

    // networkable_key2 (entity_list): rol-xor-rol-xor
    inline uintptr_t networkable_key2(uint64_t a1)
    {
        uint64_t value = read<uint64_t>(a1 + 0x18);
        if (!value) return 0;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = (x << OffsetManager::DecryptCfg.nk2_rol) | (x >> (32 - OffsetManager::DecryptCfg.nk2_rol)); // ROL
            uint32_t v2 = v1 ^ OffsetManager::DecryptCfg.nk2_xor; // XOR
            uint32_t v3 = (v2 << OffsetManager::DecryptCfg.nk2_rol_2) | (v2 >> (32 - OffsetManager::DecryptCfg.nk2_rol_2)); // ROL
            uint32_t v4 = v3 ^ OffsetManager::DecryptCfg.nk2_xor_2; // XOR
            *data = v4;
            data++;
            --count;
        } while (count);
        uintptr_t resolved = Il2cppGetHandle(value);
        if (resolved) return resolved;
        return (value & 0x7) == 0 ? value : 0;
    }

    // cl_active_item: sub-rol-xor-add
    // Validict confirmed — produces UIDs matching children entities
    inline uint64_t decrypt_ClActiveItem(uint64_t raw_value)
    {
        if (!raw_value) return 0;
        uint64_t value = raw_value;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = x - OffsetManager::DecryptCfg.cla_sub; // SUB
            uint32_t v2 = (v1 << OffsetManager::DecryptCfg.cla_rol) | (v1 >> (32 - OffsetManager::DecryptCfg.cla_rol)); // ROL
            uint32_t v3 = v2 ^ OffsetManager::DecryptCfg.cla_xor; // XOR
            uint32_t v4 = v3 + OffsetManager::DecryptCfg.cla_add; // ADD
            *data = v4;
            data++;
            --count;
        } while (count);
        return value;
    }

    // player_inventory: rol-add-rol
    inline uint64_t decrypt_inventory_pointer(uint64_t raw_value)
    {
        if (!raw_value) return 0;
        uint64_t value = raw_value;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = (x << OffsetManager::DecryptCfg.inv_rol) | (x >> (32 - OffsetManager::DecryptCfg.inv_rol)); // ROL
            uint32_t v2 = v1 + OffsetManager::DecryptCfg.inv_add; // ADD
            uint32_t v3 = (v2 << OffsetManager::DecryptCfg.inv_rol_2) | (v2 >> (32 - OffsetManager::DecryptCfg.inv_rol_2)); // ROL
            *data = v3;
            data++;
            --count;
        } while (count);
        return value;
    }

    // player_eyes: sub-xor-rol-add
    inline uint64_t decrypt_eyes(uint64_t raw_value)
    {
        if (!raw_value) return 0;
        uint64_t value = raw_value;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = x - OffsetManager::DecryptCfg.ey_sub; // SUB
            uint32_t v2 = v1 ^ OffsetManager::DecryptCfg.ey_xor; // XOR
            uint32_t v3 = (v2 << OffsetManager::DecryptCfg.ey_rol) | (v2 >> (32 - OffsetManager::DecryptCfg.ey_rol)); // ROL
            uint32_t v4 = v3 + OffsetManager::DecryptCfg.ey_add; // ADD
            *data = v4;
            data++;
            --count;
        } while (count);
        return value;
    }

    // decrypt_fov: xor-add-rol-sub
    inline uint32_t decrypt_fov(uint32_t val) {
        val ^= OffsetManager::DecryptCfg.fov_xor;
        val += OffsetManager::DecryptCfg.fov_add;
        val = (val << OffsetManager::DecryptCfg.fov_rol) | (val >> (32 - OffsetManager::DecryptCfg.fov_rol));
        val -= OffsetManager::DecryptCfg.fov_sub;
        return val;
    }

    // encrypt_fov: reverse — add-ror-sub-xor
    inline uint32_t encrypt_fov(uint32_t val) {
        val += OffsetManager::DecryptCfg.fov_sub;
        val = (val >> OffsetManager::DecryptCfg.fov_rol) | (val << (32 - OffsetManager::DecryptCfg.fov_rol));
        val -= OffsetManager::DecryptCfg.fov_add;
        val ^= OffsetManager::DecryptCfg.fov_xor;
        return val;
    }

} // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt


// TODO: CONSOLESYSTEM offset unknown for this patch - admin flag feature disabled
// class ConsoleSystem_Index
// {
// public:
//     static ConsoleSystem_Index* GetConsoleSystem() { ... }
//     void BlockCommands() { ... }
// };
// inline ConsoleSystem_Index* ConsoleSystemIndex;

class Rust {
public:
    Matrix4x4 LocalPlayer_Matrix;          // kept for backwards compat (render.hpp FOV check)
    Matrix4x4 matrixBackBuffer;            // published by render thread, read by aimbot/misc threads
    mutable std::mutex matrixMutex;        // protects matrixBackBuffer + LocalPlayer_Matrix (no torn reads)

    Matrix4x4 GetMatrixSnapshot() const {
        std::lock_guard<std::mutex> lk(matrixMutex);
        return matrixBackBuffer;
    }

    void PublishMatrix(const Matrix4x4& vpT) {
        std::lock_guard<std::mutex> lk(matrixMutex);
        matrixBackBuffer = vpT;
        LocalPlayer_Matrix = vpT;
    }

    class BaseEntity {
    public:
        Vector3 Get_ObjectPosition() {
            // Direct read chain (fast, no SSE walk) � same as old working code
            uintptr_t obj = read<uint64_t>((uintptr_t)this + offsets::BaseEntity::objRef);
            if (!obj || !is_valid(obj)) return Vector3();
            for (auto off : { offsets::BaseEntity::posChain0, offsets::BaseEntity::posChain1, offsets::BaseEntity::posChain2, offsets::BaseEntity::posChain3 }) {
                obj = read<uint64_t>(obj + off);
                if (!obj || !is_valid(obj)) return Vector3();
            }
            return read<Vector3>(obj + offsets::BaseEntity::posFinal);
        }

        // Fast 2-IOCTL position via PositionLerp interpolator
        Vector3 Get_ObjectPosition_Fast() {
            uintptr_t lerp = read<uint64_t>((uintptr_t)this + offsets::BaseEntity::positionLerp);
            if (!lerp || !is_valid(lerp)) return Get_ObjectPosition();
            uintptr_t interp = read<uint64_t>(lerp + offsets::BaseEntity::posChain0);
            if (!interp || !is_valid(interp)) return Get_ObjectPosition();
            return read<Vector3>(interp + offsets::BaseEntity::posFinal);
        }

        Bounds Get_Bounds() {
            return read<Bounds>((uintptr_t)this + offsets::BaseEntity::bounds);
        }

        class Transformation {
        public:
            Vector3 Get_Position() {
                auto transform_internal = read<uint64_t>((uintptr_t)this + 0x10);
                if (!transform_internal || !is_valid(transform_internal)) return Vector3(0, 0, 0);
                struct tempshit1 { uint64_t some_ptr; int32_t index; } temp;
                if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(transform_internal + 0x28), &temp, sizeof(temp))))
                    if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(transform_internal + 0x28), &temp, sizeof(temp));
                if (!temp.some_ptr || !is_valid(temp.some_ptr)) return Vector3();
                if (temp.index < 0 || temp.index > 255) return Vector3();
                struct tempshit2 { uint64_t relation_array; int64_t dependency_index_array; } temp2;
                if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(temp.some_ptr + 0x18), &temp2, sizeof(temp2))))
                    if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(temp.some_ptr + 0x18), &temp2, sizeof(temp2));
                if (!temp2.relation_array || !is_valid(temp2.relation_array)) return Vector3();
                if (!temp2.dependency_index_array || !is_valid((uint64_t)temp2.dependency_index_array)) return Vector3();
                __m128 xmmword_1410D1340 = { -2.f, 2.f, -2.f, 0.f };
                __m128 xmmword_1410D1350 = { 2.f, -2.f, -2.f, 0.f };
                __m128 xmmword_1410D1360 = { -2.f, -2.f, 2.f, 0.f };
                uint64_t main_addr = temp2.relation_array + (uint64_t)temp.index * 48;
                if (!is_valid(main_addr)) return Vector3();
                auto temp_main = read<__m128>(main_addr);
                uint64_t dep_addr = temp2.dependency_index_array + (uint64_t)temp.index * 4;
                if (!is_valid(dep_addr)) return Vector3();
                auto dependency_index = read<int32_t>(dep_addr);
                int loop_guard = 0;
                while (dependency_index >= 0 && loop_guard < 32) {
                    loop_guard++;
                    int64_t relation_index = 6 * (int64_t)dependency_index;
                    uint64_t rel_addr = temp2.relation_array + 8ULL * relation_index;
                    if (!is_valid(rel_addr)) break;
                    struct tempshit3 { __m128 temp_2; __m128i temp_0; __m128 temp_1; } temp3;
                    if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(rel_addr), &temp3, sizeof(temp3))))
                        if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(rel_addr), &temp3, sizeof(temp3));
                    __m128 v10 = _mm_mul_ps(temp3.temp_1, temp_main);
                    __m128 v11 = _mm_castsi128_ps(_mm_shuffle_epi32(temp3.temp_0, 0));
                    __m128 v12 = _mm_castsi128_ps(_mm_shuffle_epi32(temp3.temp_0, 85));
                    __m128 v13 = _mm_castsi128_ps(_mm_shuffle_epi32(temp3.temp_0, (unsigned char)-114));
                    __m128 v14 = _mm_castsi128_ps(_mm_shuffle_epi32(temp3.temp_0, (unsigned char)-37));
                    __m128 v15 = _mm_castsi128_ps(_mm_shuffle_epi32(temp3.temp_0, (unsigned char)-86));
                    __m128 v16 = _mm_castsi128_ps(_mm_shuffle_epi32(temp3.temp_0, 113));
                    __m128 v17 = _mm_add_ps(
                        _mm_add_ps(
                            _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_sub_ps(
                                        _mm_mul_ps(_mm_mul_ps(v11, xmmword_1410D1350), v13),
                                        _mm_mul_ps(_mm_mul_ps(v12, xmmword_1410D1360), v14)),
                                    _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v10), (unsigned char)-86))),
                                _mm_mul_ps(
                                    _mm_sub_ps(
                                        _mm_mul_ps(_mm_mul_ps(v15, xmmword_1410D1360), v14),
                                        _mm_mul_ps(_mm_mul_ps(v11, xmmword_1410D1340), v16)),
                                    _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v10), 85)))),
                            _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_sub_ps(
                                        _mm_mul_ps(_mm_mul_ps(v12, xmmword_1410D1340), v16),
                                        _mm_mul_ps(_mm_mul_ps(v15, xmmword_1410D1350), v13)),
                                    _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v10), 0))),
                                v10)),
                        temp3.temp_2);
                    temp_main = v17;
                    uint64_t next_dep_addr = temp2.dependency_index_array + (uint64_t)dependency_index * 4;
                    if (!is_valid(next_dep_addr)) break;
                    dependency_index = read<int32_t>(next_dep_addr);
                }
                return *reinterpret_cast<Vector3*>(&temp_main);
            }
        };

        class HeldItem {
        public:
            std::string get_item_name() {
                if (!(uintptr_t)this) return "Empty";
                uintptr_t unk = read<uintptr_t>((uintptr_t)this + offsets::Item::ItemDefinition);
                if (!unk || !is_valid(unk)) return "Empty";
                unk = read<uintptr_t>(unk + offsets::ItemDefinition::shortname);
                if (!unk || !is_valid(unk)) return "Empty";
                return read_wstr(unk + offsets::unity_string::first_char);
            }

            bool is_weapon() {
                static const std::vector<std::string> weaponitems = {
                    (std::string)skCrypt("rifle"),
                    (std::string)skCrypt("pistol"),
                    (std::string)skCrypt("lmg"),
                    (std::string)skCrypt("shotgun"),
                    (std::string)skCrypt("smg")
                };
                auto ws = this->get_item_name();
                std::string ItemName(ws.begin(), ws.end());
                for (const auto& item : weaponitems) {
                    if (ItemName.find(skCrypt("eoka")) != std::string::npos) return false;
                    if (ItemName.find(item) != std::string::npos) return true;
                }
                return false;
            }

            bool is_melee() {
                static const std::vector<std::string> meele_items = {
                    (std::string)skCrypt("rock"),
                    (std::string)skCrypt("hatchet"),
                    (std::string)skCrypt("stone.pickaxe"),
                    (std::string)skCrypt("stonehatchet"),
                    (std::string)skCrypt("pickaxe"),
                    (std::string)skCrypt("hammer.salvage"),
                    (std::string)skCrypt("icepick.salvag"),
                    (std::string)skCrypt("axe.salvaged"),
                    (std::string)skCrypt("pitchfork"),
                    (std::string)skCrypt("mace"),
                    (std::string)skCrypt("spear.stone"),
                    (std::string)skCrypt("spear.wooden"),
                    (std::string)skCrypt("machete"),
                    (std::string)skCrypt("bone.club"),
                    (std::string)skCrypt("paddle"),
                    (std::string)skCrypt("salvaged.sword"),
                    (std::string)skCrypt("salvaged.cleav"),
                    (std::string)skCrypt("knife.combat"),
                    (std::string)skCrypt("knife.butcher"),
                    (std::string)skCrypt("knife.bone"),
                    (std::string)skCrypt("hammer"),
                    (std::string)skCrypt("torch"),
                    (std::string)skCrypt("sickle")
                };

                std::string ItemName = this->get_item_name();
                for (const auto& item : meele_items) {
                    if (ItemName.find(item) != std::string::npos) return true;
                }
                return false;
            }
        };

        std::string GetName() {
            uintptr_t player_name_ptr = read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + offsets::BasePlayer::DisplayName);
            if (!player_name_ptr) return skCrypt("?").decrypt();
            std::string player_name_wstr = read_wstr(player_name_ptr + offsets::unity_string::first_char);
            return player_name_wstr;
        }

        Vector3 GetVelocity() {
            uintptr_t PlayerModel = read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + offsets::BasePlayer::PlayerModel);
            if (!PlayerModel || !is_valid(PlayerModel)) return Vector3();
            Vector3 Velocity = read<Vector3>(PlayerModel + offsets::PlayerModel::velocity);
            return Velocity;
        }

        bool IsDead() {
            if (!(uintptr_t)this) return true;
            int lifestate = read<int>((uintptr_t)(uintptr_t)this + offsets::BaseCombatEntity::Lifestate);
            return lifestate == 1;
        }

        bool IsSleeping() {
            if (!this) return true;
            int Flags = read<int>((uintptr_t)this + offsets::BasePlayer::PlayerFlags);
            return (Flags & offsets::base_player_flags::Sleeping) != 0;
        }

        bool IsVisibleRawFlag() {
            if (!this) return false;
            uintptr_t PlayerModel = read<uintptr_t>((uintptr_t)this + offsets::BasePlayer::PlayerModel);
            if (!PlayerModel || !is_valid(PlayerModel)) return false;
            uint64_t visAddr = (uint64_t)PlayerModel + offsets::PlayerModel::visible;
            if (!is_valid(visAddr)) return false;
            struct { bool hasValue; bool value; } nb = { false, false };
            bool readOk = false;
            if (g_UseInternalReads)
                readOk = ReadMemory_Internal((PVOID)visAddr, &nb, 2);
            if (!readOk && Drv && !Drv->ioctl_blocked)
                readOk = Drv->ReadMemory_ACE((PVOID)visAddr, &nb, 2);
            if (!readOk) return false;
            return nb.hasValue ? nb.value : true;
        }

        bool IsVisibleFiltered(bool rawVisible) {
            if (!this) return false;

            if (ESP::VisMode <= 0) return rawVisible;

            struct VisState {
                uint32_t history = 0;
                int sampleCount = 0;
                uint64_t lastSeenMs = 0;
                uint64_t lastUpdateMs = 0;
                bool stableVisible = false;
            };

            static std::unordered_map<uintptr_t, VisState> s_visState;
            static uint64_t s_lastPruneMs = 0;

            uint64_t now = GetTickCount64();
            uintptr_t key = (uintptr_t)this;
            auto& st = s_visState[key];

            int samples = ESP::VisSamples;
            if (samples < 1) samples = 1;
            if (samples > 16) samples = 16;

            uint32_t mask = (samples >= 32) ? 0xFFFFFFFFu : ((1u << samples) - 1u);
            st.history = ((st.history << 1) | (rawVisible ? 1u : 0u)) & mask;
            if (st.sampleCount < samples) st.sampleCount++;
            st.lastUpdateMs = now;

            int bitCount = 0;
            for (int i = 0; i < st.sampleCount; i++) {
                if ((st.history >> i) & 1u) bitCount++;
            }

            int required = (st.sampleCount + 1) / 2;
            bool consensusVisible = bitCount >= required;
            if (consensusVisible) {
                st.stableVisible = true;
                st.lastSeenMs = now;
            }
            else {
                int holdMs = ESP::VisHoldMs;
                if (holdMs < 0) holdMs = 0;
                st.stableVisible = (holdMs > 0 && (now - st.lastSeenMs) <= (uint64_t)holdMs);
            }

            if (s_visState.size() > 4096 && now - s_lastPruneMs > 3000) {
                s_lastPruneMs = now;
                for (auto it = s_visState.begin(); it != s_visState.end();) {
                    if (now - it->second.lastUpdateMs > 15000) it = s_visState.erase(it);
                    else ++it;
                }
            }

            if (ESP::VisMode == 2) {
                return rawVisible && st.stableVisible;
            }
            return st.stableVisible;
        }

        bool IsVisible() {
            bool rawVisible = IsVisibleRawFlag();
            return IsVisibleFiltered(rawVisible);
        }

        bool IsVisibleForAimbot() {
            bool filtered = IsVisible();
            if (!AIMBOT::VisibleStrict) return filtered;
            bool rawVisible = IsVisibleRawFlag();
            return rawVisible && filtered;
        }

        Transformation* Get_Transformation(int bone) {
            uintptr_t player_model = read<uintptr_t>((uintptr_t)this + offsets::BasePlayer::ModelTransform);
            if (!player_model || player_model < 0x100000) return nullptr;
            uintptr_t boneTransforms = read<uintptr_t>(player_model + offsets::Model::boneTransforms);
            Transformation* BoneValue = read<Transformation*>((boneTransforms + (0x20 + (bone * 0x8))));
            return BoneValue;
        }

        uintptr_t Get_BoneTransforms() {
            uintptr_t player_model = read<uintptr_t>((uintptr_t)this + offsets::BasePlayer::ModelTransform);
            if (!player_model || player_model < 0x100000) return 0;
            return read<uintptr_t>(player_model + offsets::Model::boneTransforms);
        }

        Transformation* Get_Transformation_Fast(int bone, uintptr_t boneTransforms) {
            uint64_t addr = boneTransforms + (0x20 + (bone * 0x8));
            if (!is_valid(addr)) return nullptr;
            Transformation* t = read<Transformation*>(addr);
            if (!t || !is_valid((uint64_t)t)) return nullptr;
            return t;
        }

        bool Is_Wounded() {
            if (!this) return true;
            int Flags = read<int>(reinterpret_cast<uintptr_t>(this) + offsets::BasePlayer::PlayerFlags);
            return (Flags & offsets::base_player_flags::Wounded) != 0;
        }

        // Find a component by Il2CppClass name � future-proof resolution
        // Walks the GameObject component array, reads each component's class name
        static uintptr_t GetComponentByName(uintptr_t entity, const char* targetName) {
            uintptr_t native = read<uintptr_t>(entity + 0x10);
            if (!native || !is_valid(native)) return 0;
            uintptr_t go = read<uintptr_t>(native + 0x20);
            if (!go || !is_valid(go)) return 0;
            uintptr_t comps = read<uintptr_t>(go + 0x20);
            int count = read<int>(go + 0x30);
            if (!comps || count <= 0 || count >= 64) return 0;
            for (int ci = 0; ci < count; ci++) {
                uintptr_t ncomp = read<uintptr_t>(comps + ci * 0x10 + 0x8);
                if (!ncomp || !is_valid(ncomp)) continue;
                uintptr_t handle = read<uintptr_t>(ncomp + 0x18);
                if (!handle || !is_valid(handle)) continue;
                uintptr_t comp = (handle & 1) ? decrypt::Il2cppGetHandle(handle) : (handle & ~0x7ULL);
                if (!comp || !is_valid(comp)) continue;
                // Read Il2CppClass name: comp+0x00 ? klass, klass+0x10 ? name (char*)
                uintptr_t klass = read<uintptr_t>(comp);
                if (!klass || !is_valid(klass)) continue;
                uintptr_t namePtr = read<uintptr_t>(klass + 0x10);
                if (!namePtr || !is_valid(namePtr)) continue;
                char nameBuf[32] = {};
                if (!(g_UseInternalReads && ReadMemory_Internal((PVOID)namePtr, nameBuf, 31)))
                    if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE((PVOID)namePtr, nameBuf, 31);
                if (strcmp(nameBuf, targetName) == 0) return comp;
            }
            return 0;
        }

        // Resolve PlayerEyes via component walker (NOT decrypt � per sha-dumper owner)
        uintptr_t GetPlayerEyes() {
            return GetComponentByName((uintptr_t)this, "PlayerEyes");
        }

        // Resolve PlayerInventory via component walker (NOT decrypt)
        uintptr_t GetPlayerInventory() {
            return GetComponentByName((uintptr_t)this, "PlayerInventory");
        }

        uintptr_t ResolvePlayerInventory() {
            static int invMethod = -1;
            uint64_t inv_raw = read<uint64_t>((uintptr_t)this + offsets::BasePlayer::PlayerInventory);
            if (!inv_raw) return 0;

            if (invMethod == 0) {
                if (inv_raw && is_valid(inv_raw)) {
                    uint64_t enc = read<uint64_t>(inv_raw + 0x18);
                    if (enc) {
                        uint64_t dec = decrypt::decrypt_inventory_pointer(enc);
                        uintptr_t resolved = decrypt::Il2cppGetHandle(dec);
                        if (resolved && is_valid(resolved)) return resolved;
                    }
                }
                invMethod = -1;
            } else if (invMethod == 1) {
                uintptr_t comp = GetPlayerInventory();
                if (comp && is_valid(comp)) return comp;
                invMethod = -1;
            }

            uintptr_t candidates[2] = { 0 };
            int methods[2] = { 0, 1 };
            static int invLogCount = 0;

            if (inv_raw && is_valid(inv_raw)) {
                uint64_t enc = read<uint64_t>(inv_raw + 0x18);
                if (enc) {
                    uint64_t dec = decrypt::decrypt_inventory_pointer(enc);
                    uintptr_t resolved = decrypt::Il2cppGetHandle(dec);
                    if (resolved && is_valid(resolved)) {
                        candidates[0] = resolved;
                    }
                    if (invLogCount < 3) {
                        invLogCount++;
                        LOG("INV_RESOLVE: inv_raw=0x%I64X enc=0x%I64X dec=0x%I64X resolved=0x%I64X",
                            inv_raw, enc, dec, (uint64_t)resolved);
                    }
                } else if (invLogCount < 3) {
                    invLogCount++;
                    LOG("INV_RESOLVE: inv_raw=0x%I64X enc=0 (no handle at +0x18)", inv_raw);
                }
            }

            candidates[1] = GetPlayerInventory();

            for (int i = 0; i < 2; i++) {
                if (!candidates[i] || !is_valid(candidates[i])) continue;
                uint64_t belt = read<uint64_t>(candidates[i] + offsets::PlayerInventory::Belt);
                if (!belt || !is_valid(belt)) continue;
                uint64_t list = read<uint64_t>(belt + offsets::ItemContainer::ItemList);
                if (!list || !is_valid(list)) continue;
                int sz = read<int>(list + 0x18);
                if (sz >= 0 && sz <= 7) {
                    invMethod = methods[i];
                    if (invLogCount < 10) {
                        const char* names[] = { "decrypt+gchandle", "component_walker" };
                        LOG("INV_METHOD: %s works (inv=0x%I64X belt=0x%I64X sz=%d)",
                            names[i], candidates[i], belt, sz);
                    }
                    return candidates[i];
                }
            }

            return 0;
        }

        // Find held weapon entity via inventory or children list
        // Uses ClActiveItem UID to identify the ACTUALLY held weapon
        uintptr_t Get_HeldWeapon() {
            static uintptr_t cachedWeapon = 0;
            static uint32_t cachedUid = 0;

            uint64_t active_item_id = read<uint64_t>((uintptr_t)this + offsets::BasePlayer::ClActiveItem);
            bool uidValid = ((uint32_t)(active_item_id >> 32) != (uint32_t)active_item_id);
            uint64_t uiddecrypt = uidValid ? decrypt::decrypt_ClActiveItem(active_item_id) : 0;
            uint32_t targetUid = uiddecrypt ? (uint32_t)(uiddecrypt & 0xFFFFFFFF) : 0;

            // Return cached weapon if clActiveItem hasn't changed and entity is still valid
            if (cachedWeapon && is_valid(cachedWeapon) && targetUid == cachedUid) {
                return cachedWeapon;
            }
            cachedWeapon = 0;
            cachedUid = targetUid;

            if (!targetUid) return 0;

            // PASS 0: ResolvePlayerInventory + belt ItemId match → HeldEntity
            uintptr_t inv = ResolvePlayerInventory();
            if (inv && is_valid(inv)) {
                for (int belt_off : { (int)offsets::PlayerInventory::Belt, (int)offsets::PlayerInventory::BeltFallback1, (int)offsets::PlayerInventory::BeltFallback2 }) {
                    uintptr_t belt = read<uintptr_t>(inv + belt_off);
                    if (!belt || !is_valid(belt)) continue;
                    for (int ilist_off : { (int)offsets::ItemContainer::ItemList, (int)offsets::ItemContainer::ItemListFallback }) {
                        uintptr_t list = read<uintptr_t>(belt + ilist_off);
                        if (!list || !is_valid(list)) continue;
                        int sz = read<int>(list + 0x18);
                        uintptr_t arr = read<uintptr_t>(list + 0x10);
                        if (!arr || !is_valid(arr) || sz <= 0 || sz > 7) continue;
                        for (int i = 0; i < sz && i <= 6; i++) {
                            uintptr_t itemRaw = read<uintptr_t>(arr + 0x20 + i * 8);
                            if (!itemRaw || !is_valid(itemRaw)) continue;
                            uintptr_t item = itemRaw;
                            if (itemRaw & 1) {
                                uintptr_t r = decrypt::Il2cppGetHandle(itemRaw);
                                if (r && r > 0x10000) item = r;
                            }
                            if (!item || !is_valid(item)) continue;
                            for (int uid_off : { (int)offsets::Item::ItemId, (int)offsets::Item::ItemIdFallback1, (int)offsets::Item::ItemIdFallback2 }) {
                                uint32_t itemUid = read<uint32_t>(item + uid_off);
                                if (itemUid == targetUid) {
                                    uintptr_t heldEntity = read<uintptr_t>(item + offsets::Item::HeldEntity_1);
                                    if (heldEntity && is_valid(heldEntity)) {
                                        cachedWeapon = heldEntity;
                                        return heldEntity;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // PASS 1: Children list — match by ownerItemUID, return on match (no recoil requirement)
            uintptr_t childrenList = read<uintptr_t>((uintptr_t)this + offsets::BaseNetworkable::children);
            if (childrenList && is_valid(childrenList)) {
                uintptr_t items = read<uintptr_t>(childrenList + 0x10);
                int count = read<int>(childrenList + 0x18);
                if (items && is_valid(items) && count > 0 && count < 32) {
                    for (int i = 0; i < count; i++) {
                        uintptr_t childRaw = read<uintptr_t>(items + 0x20 + i * 8);
                        if (!childRaw) continue;
                        uintptr_t child = (childRaw & 1) ? decrypt::Il2cppGetHandle(childRaw) : childRaw;
                        if (!child || !is_valid(child)) continue;
                        uint32_t ownerUid32 = read<uint32_t>(child + offsets::HeldEntity::ownerItemUID);
                        if (ownerUid32 == targetUid) {
                            cachedWeapon = child;
                            return child;
                        }
                    }
                }
            }
            return 0;
        }

        HeldItem* Get_HeldItem() {
            uint64_t active_item_id = read<uint64_t>((uintptr_t)this + offsets::BasePlayer::ClActiveItem);
            bool uidValid = ((uint32_t)(active_item_id >> 32) != (uint32_t)active_item_id);
            uint64_t uiddecrypt = uidValid ? decrypt::decrypt_ClActiveItem(active_item_id) : 0;
            if (!uiddecrypt) return nullptr;
            uint32_t targetUid = (uint32_t)(uiddecrypt & 0xFFFFFFFF);

            uintptr_t player_inventory = ResolvePlayerInventory();
            if (!player_inventory || !is_valid(player_inventory)) return nullptr;

            // Try multiple belt offsets — belt may be at different offsets or empty
            for (int belt_off : { (int)offsets::PlayerInventory::Belt, (int)offsets::PlayerInventory::BeltFallback1, (int)offsets::PlayerInventory::BeltFallback2 }) {
                uint64_t belt = read<uint64_t>(player_inventory + belt_off);
                if (!belt || !is_valid(belt)) continue;

                for (int ilo : { (int)offsets::ItemContainer::ItemList, (int)offsets::ItemContainer::ItemListFallback }) {
                    uint64_t list = read<uint64_t>(belt + ilo);
                    if (!list || !is_valid(list)) continue;
                    int sz = read<int>(list + 0x18);
                    if (sz <= 0 || sz > 7) continue;
                    uint64_t arr = read<uint64_t>(list + 0x10);
                    if (!arr || !is_valid(arr)) continue;

                    for (int i = 0; i < sz && i <= 6; i++) {
                        uint64_t itemRaw = read<uint64_t>(arr + 0x20 + i * 8);
                        if (!itemRaw || !is_valid(itemRaw)) continue;
                        uint64_t itm = itemRaw;
                        if (itemRaw & 1) {
                            uint64_t r = decrypt::Il2cppGetHandle(itemRaw);
                            if (r && r > 0x10000) itm = r;
                        }
                        if (!itm || !is_valid(itm)) continue;
                        for (int uo : { (int)offsets::Item::ItemId, (int)offsets::Item::ItemIdFallback1, (int)offsets::Item::ItemIdFallback2 }) {
                            if (read<uint32_t>(itm + uo) == targetUid)
                                return (HeldItem*)itm;
                        }
                    }
                }
            }

            return nullptr;
        }


        std::vector<HeldItem*> Get_Hotbar_list() {
            std::vector<HeldItem*> item;
            uintptr_t inv = ResolvePlayerInventory();
            if (!inv || !is_valid(inv)) return item;

            for (int belt_off : { (int)offsets::PlayerInventory::Belt, (int)offsets::PlayerInventory::BeltFallback1, (int)offsets::PlayerInventory::BeltFallback2 }) {
                uint64_t belt = read<uint64_t>(inv + belt_off);
                if (!belt || !is_valid(belt)) continue;

                for (int ilo : { (int)offsets::ItemContainer::ItemList, (int)offsets::ItemContainer::ItemListFallback }) {
                    uint64_t list = read<uint64_t>(belt + ilo);
                    if (!list || !is_valid(list)) continue;
                    int sz = read<int>(list + 0x18);
                    if (sz <= 0 || sz > 7) continue;
                    uint64_t arr = read<uint64_t>(list + 0x10);
                    if (!arr || !is_valid(arr)) continue;
                    for (int i = 0; i < sz && i <= 6; i++) {
                        uint64_t itemRaw = read<uint64_t>(arr + 0x20 + i * 8);
                        if (!itemRaw || !is_valid(itemRaw)) continue;
                        uint64_t itm = itemRaw;
                        if (itemRaw & 1) {
                            uint64_t r = decrypt::Il2cppGetHandle(itemRaw);
                            if (r && r > 0x10000) itm = r;
                        }
                        if (itm && is_valid(itm))
                            item.push_back((HeldItem*)itm);
                    }
                    if (!item.empty()) break;
                }
                if (!item.empty()) break;
            }
            return item;
        }
    };

    struct PrefabData {
        std::string name;
        Vector3 Position;
        ImColor Color;
    };

    BaseEntity* LocalPlayer = NULL;
}; inline Rust* src = nullptr;

inline std::vector<Rust::BaseEntity*> entity_List;
inline std::vector<Rust::BaseEntity*> npc_List;
inline std::vector<Rust::BaseEntity*> animal_List;
inline std::vector<Rust::PrefabData> prefab_List;

inline std::map<std::string, std::tuple<float, float, float, float>> recoil_values = {
    { "rifle.ak", { 1.5f, 2.5f, -2.5f, -3.5f } },
    { "rifle.ak.ice", { 1.5f, 2.5f, -2.5f, -3.5f } },
    { "rifle.ak.diver", { 1.5f, 2.5f, -2.5f, -3.5f } },
    { "smg.mp5", { -1.0f, 1.0f, -1.0f, -3.0f } },
    { "lmg.m249", { 1.25f, 2.25f, -3.0f, -4.0f } },
    { "pistol.semiauto", { -1.0f, 1.0f, -2.0f, -2.5f } },
    { "hmlmg", { -1.25f, -2.5f, -3.0f, -4.0f } },
    { "rifle.m39", { 1.5f, 2.5f, -3.0f, -4.0f } },
    { "rifle.lr300", { -1.0f, 1.0f, -2.0f, -3.0f } },
    { "smg.thompson", { -1.0f, 1.0f, -1.5f, -2.0f } },
    { "smg.2", { -1.0f, 1.0f, -1.5f, -2.0f } },
    { "rifle.semiauto", { -0.5f, 0.5f, -2.0f, -3.0f } },
    { "pistol.m92", { -1.0f, 1.0f, -7.0f, -8.0f } }
};

inline std::map<std::string, std::tuple<float, float, float, float>> recoil_values2 = {
    { "shotgun.double", { 4.0f, 8.0f, -10.0f, -14.0f } },
    { "pistol.revolver", { -1.0f, 1.0f, -3.0f, -6.0f } },
    { "pistol.prototype17", { -1.0f, 2.0f, -15.0f, -16.0f } },
    { "pistol.python", { -2.0f, 2.0f, -15.0f, -16.0f } },
    { "pistol.nailgun", { -1.0f, 1.0f, -3.0f, -6.0f } },
    { "rifle.bolt", { -4.0f, 4.0f, -2.0f, -3.0f } },
    { "rifle.l96", { -2.0f, 2.0f, -1.0f, -1.5f } },
    { "shotgun.spas12", { 4.0f, 8.0f, -10.0f, -14.0f } },
    { "shotgun.waterpipe", { 4.0f, 8.0f, -10.0f, -14.0f } },
    { "shotgun.pump", { 4.0f, 8.0f, -10.0f, -14.0f } }
};

inline std::map<std::wstring, float> bullets = {
    { L"ammo.rifle", 1.0f },
    { L"ammo.rifle.hv", 1.2f },
    { L"ammo.rifle.explosive", 0.49f },
    { L"ammo.rifle.incendiary", 0.55f },
    { L"ammo.pistol", 1.0f },
    { L"ammo.pistol.hv", 1.33333f },
    { L"ammo.pistol.fire", 0.75f },
    { L"arrow.wooden", 1.0f },
    { L"arrow.hv", 1.6f },
    { L"arrow.fire", 0.8f },
    { L"arrow.bone", 0.9f },
    { L"ammo.handmade.shell", 1.0f },
    { L"ammo.shotgun.slug", 2.25f },
    { L"ammo.shotgun.fire", 1.0f },
    { L"ammo.shotgun", 2.25f },
    { L"ammo.nailgun.nails", 1.f }
};

inline std::wstring magazine_shortname = L"";

const inline float __fastcall ProjectileSpeed_Normal(Rust::BaseEntity::HeldItem* item, float dist) {
    auto weaponshortnamea = item->get_item_name();
    std::wstring weaponshortname(weaponshortnamea.begin(), weaponshortnamea.end());

    const float default_speed = 300.0f;
    const float bullet_multiplier = bullets[magazine_shortname];
    if (weaponshortname == std::wstring(skCrypt(L"rifle.ak")) || weaponshortname == std::wstring(skCrypt(L"rifle.ak.ice"))) {
        if (dist <= 0.f) return 375.9f * bullet_multiplier;
        else if (dist <= 150.f) return 386.f * bullet_multiplier;
        else if (dist <= 200.f) return 390.f * bullet_multiplier;
        else if (dist <= 250.f) return 400.f * bullet_multiplier;
        else if (dist <= 300.0f) return 410.f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"hmlmg"))) return 500 * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"rifle.lr300"))) return 340.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"rifle.bolt"))) return 579.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"rifle.l96"))) return 1126.0f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"rifle.m39"))) return 445.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"rifle.semiauto"))) return 358.0f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"lmg.m249"))) return 450.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"smg.thompson"))) {
        if (dist <= 180.f) return 270.f * bullet_multiplier;
        else if (dist <= 204.f) return 250.f * bullet_multiplier;
        else if (dist <= 250.f) return 245.f * bullet_multiplier;
        else if (dist <= 350.f) return 230.f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"smg.2"))) {
        if (dist <= 160.f) return 220.f * bullet_multiplier;
        else if (dist <= 200.f) return 195.f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"smg.mp5"))) return 240.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"pistol.prototype17"))) {
        if (dist <= 110.f) return 300.f * bullet_multiplier;
        else if (dist <= 169.f) return 290.f * bullet_multiplier;
        else if (dist <= 170.f) return 280.f * bullet_multiplier;
        else if (dist <= 180.f) return 275.f * bullet_multiplier;
        else if (dist <= 190.f) return 270.f * bullet_multiplier;
        else if (dist <= 200.f) return 265.f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"pistol.python"))) return 300.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"pistol.semiauto"))) return 300.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"pistol.revolver"))) return 270.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"pistol.m92"))) return 300.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"pistol.eoka"))) return 90.f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"pistol.nailgun"))) {
        if (dist <= 60.f) return 59.f * bullet_multiplier;
        else if (dist <= 85.f) return 58.3f * bullet_multiplier;
        else return 57.8f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"crossbow"))) {
        if (dist <= 83.f) return 90.f * bullet_multiplier;
        else if (dist <= 100.f) return 88.f * bullet_multiplier;
        else if (dist <= 150.f) return 86.f * bullet_multiplier;
        else return 86.f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"bow.compound"))) {
        if (dist <= 90.f) return 120.f * bullet_multiplier;
        else if (dist <= 150.F) return 115.76f * bullet_multiplier;
        else return 115.5f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"bow.hunting"))) {
        if (dist <= 41.f) return 60.f * bullet_multiplier;
        else if (dist <= 82.f) return 58.f * bullet_multiplier;
        else if (dist <= 102.f) return 57.5f * bullet_multiplier;
        else if (dist <= 112.f) return 57.3f * bullet_multiplier;
        else if (dist <= 127.f) return 57.f * bullet_multiplier;
        else if (dist <= 146.f) return 56.5f * bullet_multiplier;
        else if (dist <= 153.f) return 56.3f * bullet_multiplier;
        else if (dist <= 163.f) return 56.f * bullet_multiplier;
        else if (dist <= 172.f) return 55.7f * bullet_multiplier;
        else if (dist <= 178.f) return 55.5f * bullet_multiplier;
        else if (dist <= 184.f) return 55.3f * bullet_multiplier;
        else if (dist <= 189.f) return 55.1f * bullet_multiplier;
        else if (dist <= 196.f) return 54.9f * bullet_multiplier;
        else if (dist <= 201.f) return 54.7f * bullet_multiplier;
        else if (dist <= 206.f) return 54.5f * bullet_multiplier;
        else if (dist <= 210.f) return 54.3f * bullet_multiplier;
        else if (dist <= 215.f) return 54.1f * bullet_multiplier;
        else if (dist <= 220.f) return 53.9f * bullet_multiplier;
        else if (dist <= 225.1f) return 53.7f * bullet_multiplier;
        else if (dist <= 230.1f) return 53.5f * bullet_multiplier;
        else if (dist <= 233.1f) return 53.3f * bullet_multiplier;
        else if (dist <= 237.1f) return 53.1f * bullet_multiplier;
        else if (dist <= 241.1f) return 52.9f * bullet_multiplier;
        else if (dist <= 244.1f) return 52.7f * bullet_multiplier;
        else if (dist <= 248.1f) return 52.5f * bullet_multiplier;
        else if (dist <= 252.1f) return 52.3f * bullet_multiplier;
        else if (dist <= 255.1f) return 52.1f * bullet_multiplier;
        else if (dist <= 500.f) return 50.f * bullet_multiplier;
    }
    if (weaponshortname == std::wstring(skCrypt(L"shotgun.pump"))) return 100.0f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"shotgun.spas12"))) return 100.0f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"shotgun.waterpipe"))) return 100.0f * bullet_multiplier;
    if (weaponshortname == std::wstring(skCrypt(L"shotgun.doublebarrel"))) return 100.0f * bullet_multiplier;
    return default_speed;
}

const inline Vector3 __fastcall Prediction2(Rust::BaseEntity::HeldItem* item, const Vector3& LP_Pos, Vector3 Velocity, Vector3 BonePos) {
    const float __fastcall Dist = Calc3D_Dist(LP_Pos, BonePos);
    if (Dist > 0.001f) {
        const float __fastcall BulletTime = Dist / ProjectileSpeed_Normal(item, Dist);
        const Vector3 __fastcall vel = Velocity;
        Vector3 __fastcall PredictVel;
        PredictVel.x = vel.x * BulletTime * 0.75f;
        PredictVel.y = vel.y * BulletTime * 0.75f;
        PredictVel.z = vel.z * BulletTime * 0.75f;

        BonePos.x += PredictVel.x;
        BonePos.y += PredictVel.y;
        BonePos.z += PredictVel.z;
        BonePos.y += (4.905f * BulletTime * BulletTime);
    }
    return BonePos;
}

//inline std::string GetName(uintptr_t a) {
//    uintptr_t player_name_ptr = read<uintptr_t>(a + 0x270);
//    if (!player_name_ptr) return skCrypt("?").decrypt();
//    std::string player_name_wstr = read_wstr(player_name_ptr + 0x14);
//    return player_name_wstr;
//}

inline void GetComponentsInChildren(std::uintptr_t game_object, std::vector<std::uintptr_t>& renderers, int depth = 0, bool hands = false) {
    if (depth > 16) return;
    const auto component_list = read<std::uint64_t>(game_object + 0x20);
    if (!component_list || !is_valid(component_list)) return;

    const auto component_size = read<int>(game_object + 0x30);
    if (!component_size || component_size >= 256) return;

    for (int idx{ 0 }; idx < component_size; ++idx) {
        const auto component = read<std::uint64_t>(component_list + (0x10 * idx + 0x8));
        if (!component) continue;

        const auto component_ptr = read<std::uint64_t>(component + 0x28);
        if (!component_ptr) continue;

        const auto component_name_ptr = read<std::uint64_t>(component_ptr + 0x0);
        if (!component_name_ptr) continue;

        const auto component_name = read<std::uint64_t>(component_name_ptr + 0x10);
        if (!component_name) continue;

        auto name = readstring(component_name);

        if (name == ("SkinnedMeshRenderer") || name == ("MeshRenderer")) renderers.push_back(component);

        if (name == ("Transform")) {
            const auto child_list = read<std::uint64_t>(component + 0x70);
            if (!child_list) continue;

            const auto child_size = read<int>(component + 0x80);
            if (!child_size) continue;

            for (int i{ 0 }; i < child_size; ++i) {
                const auto child_transform = read<std::uint64_t>(child_list + (0x8 * i));
                if (!child_transform) continue;

                const auto child_game_object = read<std::uint64_t>(child_transform + 0x20);
                if (!child_game_object) continue;

                const auto child_object_name = read<std::uint64_t>(child_game_object + 0x50);
                if (!child_object_name) continue;

                const auto child_name = readstring(child_object_name);

                if (child_name.find(("holosight")) != std::string::npos) continue;


                GetComponentsInChildren(child_game_object, renderers, depth + 1);
            }
        }
    }
}

inline void ProcessSkinnedMeshRenderer(uintptr_t renderer, int weaponmaterial) {
    if (!renderer || !is_valid(renderer)) return;

    for (std::uint32_t idx{ 0 }; idx < 2; idx++) {
        const auto renderEntry = read<uintptr_t>(renderer + 0x20 + (idx * 0x8));
        if (!renderEntry || !is_valid(renderEntry)) continue;

        const auto untity_object = read<uintptr_t>(renderEntry + 0x10);
        if (!untity_object || !is_valid(untity_object)) continue;

        const auto mat_list = read<dynamic_array>(untity_object + 0x140);
        if (mat_list.sz < 1 || mat_list.sz > 5) continue;
        if (!mat_list.base || !is_valid(mat_list.base)) continue;

        for (std::uint32_t midx{ 0 }; midx < mat_list.sz; midx++) {
            uintptr_t writeAddr = mat_list.base + (midx * 0x4);
            if (is_valid(writeAddr))
                write<unsigned int>(writeAddr, weaponmaterial);
        }
    }
}


inline bool IsPlayerScoped(Rust::BaseEntity* Player) {
    if (!Player || !is_valid((uintptr_t)Player)) return false;
    return read<bool>((uintptr_t)Player + offsets::BaseProjectile::HasADS);
}

namespace fs = std::filesystem;
inline std::vector<std::string> hotbar_list;

class TextureLoader {
public:
    std::string path;
    ID3D11Device* device;

    TextureLoader(const std::string& texturePath, ID3D11Device* d3dDevice) : path(texturePath), device(d3dDevice) {}

    bool LoadTexture(const std::string& filepath, ID3D11ShaderResourceView** textureView) {
        ID3D11Resource* textureResource = nullptr;
        HRESULT hr = D3DX11CreateTextureFromFileA(device, filepath.c_str(), nullptr, nullptr, &textureResource, nullptr);
        if (FAILED(hr)) return false;

        hr = device->CreateShaderResourceView(textureResource, nullptr, textureView);
        if (FAILED(hr)) {
            textureResource->Release();
            return false;
        }

        textureResource->Release();
        return true;
    }

    std::vector<std::string> FindSimilarFiles(const std::string& name) {
        std::vector<std::string> similarFiles;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(name) != std::string::npos) {
                    similarFiles.push_back(entry.path().string());
                }
            }
        }
        return similarFiles;
    }

    bool LoadSimilarTexture(const std::string& name, ID3D11ShaderResourceView** textureView) {
        std::vector<std::string> similarFiles = FindSimilarFiles(name);
        if (similarFiles.empty()) return false;
        for (const auto& file : similarFiles) {
            if (LoadTexture(file, textureView)) return true;
        }
        return false;
    }
};

// Global item icon cache � loads PNG icons from Rust's Bundles/items directory
// Keyed by item shortname (e.g., "rifle.ak" ? loads "rifle.ak.png")
struct ItemIconCache {
    std::unordered_map<std::string, ID3D11ShaderResourceView*> cache;
    std::string itemsPath;
    TextureLoader* loader = nullptr;

    void Init(ID3D11Device* device) {
        if (loader) return;
        // Try to find Rust install path from registry
        char steamPath[MAX_PATH] = {};
        DWORD len = MAX_PATH;
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr, (LPBYTE)steamPath, &len);
            RegCloseKey(hKey);
        }
        if (steamPath[0]) {
            itemsPath = std::string(steamPath) + "\\steamapps\\common\\Rust\\Bundles\\items";
        } else {
            itemsPath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rust\\Bundles\\items";
        }
        loader = new TextureLoader(itemsPath, device);
    }

    ID3D11ShaderResourceView* Get(const std::string& shortname) {
        if (!loader || shortname.empty()) return nullptr;
        auto it = cache.find(shortname);
        if (it != cache.end()) return it->second;
        ID3D11ShaderResourceView* srv = nullptr;
        std::string path = itemsPath + "\\" + shortname + ".png";
        if (loader->LoadTexture(path, &srv)) {
            cache[shortname] = srv;
            return srv;
        }
        // Try fuzzy match as fallback
        if (loader->LoadSimilarTexture(shortname, &srv)) {
            cache[shortname] = srv;
            return srv;
        }
        cache[shortname] = nullptr; // cache the miss
        return nullptr;
    }
};
inline ItemIconCache g_ItemIcons;
