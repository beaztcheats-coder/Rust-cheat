#pragma once
#include "offsets.hpp"
#include "OffsetManager.hpp"
#include "vectorMath.hpp"
#include "draw.hpp"
#include <xmmintrin.h>
#include <emmintrin.h>
#include "skcrypt.hpp"
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

inline uint64_t GameAssembly;
inline uint64_t UnityPlayer;

struct monostr { char buffer[128]; };

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
        if (type >= 4) {
            static int diagType = 0;
            if (diagType < 3) { LOG("GCHANDLE: type=%d >= 4 (invalid) page_base=0x%I64X handle=0x%I64X", type, page_base, handle_ptr); diagType++; }
            return 0;
        }

        int64_t slot = (int64_t)(handle_ptr - page_base - 0x28) >> 3;
        if (slot < 0) {
            static int diagSlot = 0;
            if (diagSlot < 3) { LOG("GCHANDLE: slot=%lld < 0 page_base=0x%I64X handle=0x%I64X", (long long)slot, page_base, handle_ptr); diagSlot++; }
            return 0;
        }

        uint32_t size = read<uint32_t>(page_base + 0x1C);
        if ((uint32_t)slot >= size) {
            static int diagSize = 0;
            if (diagSize < 3) { LOG("GCHANDLE: slot=%u >= size=%u page_base=0x%I64X handle=0x%I64X", (unsigned)slot, size, page_base, handle_ptr); diagSize++; }
            return 0;
        }

        uint64_t bitmap_ptr = read<uint64_t>(page_base + 0x10);
        if (!bitmap_ptr) {
            static int diagBmp = 0;
            if (diagBmp < 3) { LOG("GCHANDLE: bitmap_ptr=0 page_base=0x%I64X handle=0x%I64X", page_base, handle_ptr); diagBmp++; }
            return 0;
        }

        uint32_t bitmask = read<uint32_t>(bitmap_ptr + 4 * ((uint32_t)slot >> 5));
        if (!((bitmask >> (slot & 0x1F)) & 1)) {
            static int diagBit = 0;
            if (diagBit < 3) { LOG("GCHANDLE: bit not set slot=%u bitmask=0x%X page_base=0x%I64X", (unsigned)slot, bitmask, page_base); diagBit++; }
            return 0;
        }

        uint64_t entry = read<uint64_t>(page_base + 8 * ((uint32_t)slot + 5));

        static int diagOk = 0;
        if (diagOk < 1) { LOG("GCHANDLE: OK handle=0x%I64X type=%d slot=%u size=%u", handle_ptr, type, (unsigned)slot, size); diagOk++; }

        if (type > 1)
            return entry;

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

    // networkable_key (client_entities): rol-xor-sub (build 24181174)
    inline uintptr_t networkable_key(uint64_t a1)
    {
        uint64_t value = read<uint64_t>(a1 + 0x18);
        if (!value) return 0;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = (x << OffsetManager::DecryptCfg.nk_rol) | (x >> (32 - OffsetManager::DecryptCfg.nk_rol)); // ROL
            uint32_t v2 = v1 ^ OffsetManager::DecryptCfg.nk_xor; // XOR
            uint32_t v3 = v2 - OffsetManager::DecryptCfg.nk_sub; // SUB
            *data = v3;
            data++;
            --count;
        } while (count);
        uintptr_t resolved = Il2cppGetHandle(value);
        if (resolved) return resolved;
        return (value & 0x7) == 0 ? value : 0;
    }

    // networkable_key2 (entity_list): rol-add-rol (build 24181174)
    inline uintptr_t networkable_key2(uint64_t a1)
    {
        uint64_t value = read<uint64_t>(a1 + 0x18);
        if (!value) return 0;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = (x << OffsetManager::DecryptCfg.nk2_rol1) | (x >> (32 - OffsetManager::DecryptCfg.nk2_rol1)); // ROL
            uint32_t v2 = v1 + OffsetManager::DecryptCfg.nk2_add; // ADD
            uint32_t v3 = (v2 << OffsetManager::DecryptCfg.nk2_rol2) | (v2 >> (32 - OffsetManager::DecryptCfg.nk2_rol2)); // ROL
            *data = v3;
            data++;
            --count;
        } while (count);
        uintptr_t resolved = Il2cppGetHandle(value);
        if (resolved) return resolved;
        return (value & 0x7) == 0 ? value : 0;
    }

    // cl_active_item: add-rol-xor, count=1 (build 24181174)
    inline uint64_t decrypt_ClActiveItem(uint64_t raw_value)
    {
        if (!raw_value) return 0;
        uint64_t value = raw_value;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 1;
        do {
            uint32_t x = *data;
            uint32_t v1 = x + OffsetManager::DecryptCfg.cla_add; // ADD
            uint32_t v2 = (v1 << OffsetManager::DecryptCfg.cla_rol) | (v1 >> (32 - OffsetManager::DecryptCfg.cla_rol)); // ROL
            uint32_t v3 = v2 ^ OffsetManager::DecryptCfg.cla_xor; // XOR
            *data = v3;
            data++;
            --count;
        } while (count);
        return value;
    }

    // player_inventory: xor-rol-xor (build 24181174)
    inline uint64_t decrypt_inventory_pointer(uint64_t raw_value)
    {
        if (!raw_value) return 0;
        uint64_t value = raw_value;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = x ^ OffsetManager::DecryptCfg.inv_xor1; // XOR
            uint32_t v2 = (v1 << OffsetManager::DecryptCfg.inv_rol) | (v1 >> (32 - OffsetManager::DecryptCfg.inv_rol)); // ROL
            uint32_t v3 = v2 ^ OffsetManager::DecryptCfg.inv_xor2; // XOR
            *data = v3;
            data++;
            --count;
        } while (count);
        return value;
    }

    // player_eyes: add-rol-xor-rol (build 24181174)
    inline uint64_t decrypt_eyes(uint64_t raw_value)
    {
        if (!raw_value) return 0;
        uint64_t value = raw_value;
        uint32_t* data = (uint32_t*)&value;
        uint32_t count = 2;
        do {
            uint32_t x = *data;
            uint32_t v1 = x + OffsetManager::DecryptCfg.ey_add; // ADD
            uint32_t v2 = (v1 << OffsetManager::DecryptCfg.ey_rol1) | (v1 >> (32 - OffsetManager::DecryptCfg.ey_rol1)); // ROL
            uint32_t v3 = v2 ^ OffsetManager::DecryptCfg.ey_xor; // XOR
            uint32_t v4 = (v3 << OffsetManager::DecryptCfg.ey_rol2) | (v3 >> (32 - OffsetManager::DecryptCfg.ey_rol2)); // ROL
            *data = v4;
            data++;
            --count;
        } while (count);
        return value;
    }

    // decrypt_fov: rol-add-xor (build 24181174)
    inline uint32_t decrypt_fov(uint32_t val) {
        val = (val << OffsetManager::DecryptCfg.fov_rol) | (val >> (32 - OffsetManager::DecryptCfg.fov_rol));
        val += OffsetManager::DecryptCfg.fov_add;
        val ^= OffsetManager::DecryptCfg.fov_xor;
        return val;
    }

    // encrypt_fov: reverse — xor-sub-ror (build 24181174)
    inline uint32_t encrypt_fov(uint32_t val) {
        val ^= OffsetManager::DecryptCfg.fov_xor;
        val -= OffsetManager::DecryptCfg.fov_add;
        val = (val >> OffsetManager::DecryptCfg.fov_rol) | (val << (32 - OffsetManager::DecryptCfg.fov_rol));
        return val;
    }

} // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt
 // namespace decrypt


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

        // Cached transform pointers — avoids re-resolving transform_internal + relation_array
        // every bone read. Refreshed every 200ms by skeleton thread (SHA Source approach).
        struct CachedTransformData {
            uint64_t relation_array = 0;
            uint64_t dependency_index_array = 0;
            int32_t index = -1;

            bool valid() const {
                return relation_array != 0 && is_valid(relation_array) &&
                       dependency_index_array != 0 && is_valid(dependency_index_array) &&
                       index >= 0 && index <= 255;
            }
        };

        class Transformation {
        public:
            // Returns cached transform pointers (relation_array, dependency_index_array, index)
            // Called once per bone per 200ms — saves 2 IOCTLs per bone position read
            CachedTransformData Get_Cached_Data() {
                CachedTransformData data;
                auto transform_internal = read<uint64_t>((uintptr_t)this + 0x10);
                if (!transform_internal || !is_valid(transform_internal)) return data;

                struct tempshit1 { uint64_t some_ptr; int32_t index; } temp;
                if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(transform_internal + 0x28), &temp, sizeof(temp))))
                    if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(transform_internal + 0x28), &temp, sizeof(temp));
                if (!temp.some_ptr || !is_valid(temp.some_ptr)) return data;
                if (temp.index < 0 || temp.index > 255) return data;

                struct tempshit2 { uint64_t relation_array; int64_t dependency_index_array; } temp2;
                if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(temp.some_ptr + 0x18), &temp2, sizeof(temp2))))
                    if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(temp.some_ptr + 0x18), &temp2, sizeof(temp2));
                if (!temp2.relation_array || !is_valid(temp2.relation_array)) return data;
                if (!temp2.dependency_index_array || !is_valid((uint64_t)temp2.dependency_index_array)) return data;

                data.relation_array = temp2.relation_array;
                data.dependency_index_array = (uint64_t)temp2.dependency_index_array;
                data.index = temp.index;
                return data;
            }

            // Fast position read using cached transform data — skips 2 IOCTLs
            // (transform_internal read + relation_array resolution)
            static Vector3 Get_Position_From_Cache(const CachedTransformData& data) {
                if (!data.valid()) return Vector3();

                __m128 xmmword_1410D1340 = { -2.f, 2.f, -2.f, 0.f };
                __m128 xmmword_1410D1350 = { 2.f, -2.f, -2.f, 0.f };
                __m128 xmmword_1410D1360 = { -2.f, -2.f, 2.f, 0.f };
                uint64_t main_addr = data.relation_array + (uint64_t)data.index * 48;
                if (!is_valid(main_addr)) return Vector3();
                auto temp_main = read<__m128>(main_addr);
                uint64_t dep_addr = data.dependency_index_array + (uint64_t)data.index * 4;
                if (!is_valid(dep_addr)) return Vector3();
                auto dependency_index = read<int32_t>(dep_addr);
                int loop_guard = 0;
                while (dependency_index >= 0 && loop_guard < 32) {
                    loop_guard++;
                    int64_t relation_index = 6 * (int64_t)dependency_index;
                    uint64_t rel_addr = data.relation_array + 8ULL * relation_index;
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
                    uint64_t next_dep_addr = data.dependency_index_array + (uint64_t)dependency_index * 4;
                    if (!is_valid(next_dep_addr)) break;
                    dependency_index = read<int32_t>(next_dep_addr);
                }
                return *reinterpret_cast<Vector3*>(&temp_main);
            }

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
            // Read raw isVisible flag at BaseEntity+0x150 (1 byte)
            // This is Unity's rendering visibility flag — set by the game's occlusion culling system
            return read<uint8_t>((uintptr_t)this + offsets::BaseEntity::isVisible) != 0;
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

            static std::unordered_map<uintptr_t, VisState>* pVisState = nullptr;
            if (!pVisState) pVisState = new std::unordered_map<uintptr_t, VisState>();
            auto& s_visState = *pVisState;
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

            int required = (st.sampleCount + 1) / 2;  // simple majority — faster response
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

            // Diagnostic: log cl_active_item decrypt result (first 5 times only)
            {
                static int claDiagCount = 0;
                if (claDiagCount < 5) {
                    claDiagCount++;
                    LOG("CLA_DIAG[%d]: raw=0x%I64X dec=0x%I64X targetUid=0x%X uidValid=%d",
                        claDiagCount, active_item_id, uiddecrypt, targetUid, (int)uidValid);
                }
            }

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
                            if (read<uint32_t>(itm + uo) == targetUid) {
                                static int claMatchCount = 0;
                                if (claMatchCount < 3) {
                                    claMatchCount++;
                                    LOG("CLA_MATCH[%d]: targetUid=0x%X matched at uid_off=0x%X", claMatchCount, targetUid, uo);
                                }
                                return (HeldItem*)itm;
                            }
                        }
                    }
                }
            }

            // Diagnostic: log if no match found (first 3 times)
            {
                static int claNoMatchCount = 0;
                if (claNoMatchCount < 3) {
                    claNoMatchCount++;
                    LOG("CLA_NOMATCH[%d]: targetUid=0x%X not found in belt", claNoMatchCount, targetUid);
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
        float health = -1.f;
        float maxHealth = -1.f;
        float hackSeconds = -1.f;
    };

    BaseEntity* LocalPlayer = NULL;
}; inline Rust* src = nullptr;

inline std::vector<Rust::BaseEntity*> entity_List;
inline std::vector<Rust::BaseEntity*> npc_List;
inline std::vector<Rust::BaseEntity*> animal_List;
inline std::vector<Rust::PrefabData> prefab_List;

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
