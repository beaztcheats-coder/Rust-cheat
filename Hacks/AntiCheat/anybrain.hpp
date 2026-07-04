#pragma once
#include "../../offsets.hpp"
#include "../../Driver.hpp"
#include "../../sdk.hpp"
#include "../../Logger.hpp"

namespace Anybrain {

    static uintptr_t s_cacheTypeInfo = 0;
    static uintptr_t s_staticFields = 0;
    static ULONGLONG s_lastWrite = 0;
    static bool s_nopDone = false;
    static bool s_neutralized = false;
    static int s_failCount = 0;

    inline bool NopUpdateMethod() {
        if (s_nopDone) return true;
        if (!ANYBRAIN::update_rva) return false;
        if (!GameAssembly) return false;

        uintptr_t updateAddr = GameAssembly + ANYBRAIN::update_rva;
        if (!is_valid(updateAddr)) return false;

        uint8_t original = read<uint8_t>(updateAddr);
        if (original == 0xC3) {
            s_nopDone = true;
            return true;
        }

        write<uint8_t>(updateAddr, (uint8_t)0xC3);
        uint8_t verify = read<uint8_t>(updateAddr);
        if (verify == 0xC3) {
            LOG("ANYBRAIN: NOP Update() success at 0x%I64X (rva=0x%I64X)", updateAddr, ANYBRAIN::update_rva);
            s_nopDone = true;
            return true;
        }
        LOG("ANYBRAIN: NOP Update() FAILED at 0x%I64X (driver may not write code sections)", updateAddr);
        return false;
    }

    inline bool FindCacheClass() {
        if (s_cacheTypeInfo && is_valid(s_cacheTypeInfo)) return true;
        if (!ANYBRAIN::static_cache_rva) return false;
        if (!GameAssembly) return false;

        uintptr_t typeInfo = GameAssembly + ANYBRAIN::static_cache_rva;
        if (!is_valid(typeInfo)) return false;

        s_cacheTypeInfo = typeInfo;
        return true;
    }

    inline bool ResolveStaticFields() {
        if (!s_cacheTypeInfo) return false;

        uintptr_t sf = read<uintptr_t>(s_cacheTypeInfo + ANYBRAIN::static_fields_offset);
        if (!sf || !is_valid(sf)) return false;

        s_staticFields = sf;
        return true;
    }

    inline bool DisableSdk() {
        if (!s_staticFields) return false;

        uint8_t current = read<uint8_t>(s_staticFields + ANYBRAIN::sdkLoaded_offset);
        if (current == 0) return true;

        write<uint8_t>(s_staticFields + ANYBRAIN::sdkLoaded_offset, (uint8_t)0);
        return true;
    }

    inline void Neutralize() {
        if (!MISC::AntiAnybrain) return;
        if (g_shutting_down || g_process_dead) return;

        if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) return;
        if (g_BnStableCycles.load(std::memory_order_relaxed) < 3) return;

        ULONGLONG now = GetTickCount64();
        if (s_lastWrite && (now - s_lastWrite) < 2000) return;
        s_lastWrite = now;

        if (NopUpdateMethod()) {
            if (!s_neutralized) {
                LOG("ANYBRAIN: neutralized via NOP Update() (rva=0x%I64X)", ANYBRAIN::update_rva);
                s_neutralized = true;
            }
            s_failCount = 0;
            return;
        }

        if (FindCacheClass() && ResolveStaticFields() && DisableSdk()) {
            if (!s_neutralized) {
                LOG("ANYBRAIN: neutralized via sdkLoaded=false (typeInfo=0x%I64X sf=0x%I64X)",
                    s_cacheTypeInfo, s_staticFields);
                s_neutralized = true;
            }
            s_failCount = 0;
        } else {
            if (s_failCount < 3) {
                LOG("ANYBRAIN: NOP failed + sdkLoaded fallback unavailable (static_cache_rva=0x%I64X)",
                    ANYBRAIN::static_cache_rva);
                s_failCount++;
            }
        }
    }

    inline bool IsActive() { return s_neutralized; }

} // namespace Anybrain
