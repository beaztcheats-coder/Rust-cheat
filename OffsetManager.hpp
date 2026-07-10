#pragma once
#include <windows.h>
#include <string>
#include "Logger.hpp"
#include "Driver.hpp"
#include "offsets.hpp"
#include "RuntimePaths.hpp"

namespace OffsetManager {

    struct DecryptConfig {
        // networkable_key (client_entities): add-rol-add (build 24091435)
        uint32_t nk_add1 = 0x16A2149;
        uint32_t nk_rol = 0x7;
        uint32_t nk_add2 = 0x42D649F;

        // networkable_key2 (entity_list): rol-sub-xor-add (build 24091435)
        uint32_t nk2_rol = 0x14;
        uint32_t nk2_sub = 0x1CC81122;
        uint32_t nk2_xor = 0xAACADDEB;
        uint32_t nk2_add = 0x63DFF63C;

        // decrypt_ClActiveItem (cl_active_item): xor-rol-add, count=1 (build 24091435)
        uint32_t cla_xor = 0x320F4C7D;
        uint32_t cla_rol = 0x1F;
        uint32_t cla_add = 0x62E951E6;

        // decrypt_inventory_pointer (player_inventory): rol-xor-add-rol (build 24091435)
        uint32_t inv_rol1 = 0x7;
        uint32_t inv_xor = 0xEC9526A3;
        uint32_t inv_add = 0xAC5D1D91;
        uint32_t inv_rol2 = 0xF;

        // decrypt_eyes (player_eyes): sub-xor-add-rol (build 24091435)
        uint32_t ey_sub = 0x2C865414;
        uint32_t ey_xor = 0x873FAFE5;
        uint32_t ey_add = 0x3FD966E1;
        uint32_t ey_rol = 0x6;

        // decrypt_fov: add-rol-sub (build 24091435)
        uint32_t fov_add1 = 0xCA34BF6D;
        uint32_t fov_rol = 0x1D;
        uint32_t fov_sub = 0xB723318;
    };

    inline DecryptConfig DecryptCfg;

    inline bool LoadDecryptConfig() {
        const char* paths[] = {
            RuntimePaths::DecryptsPath(),
            "C:\\rust_decrypts.dat",
            nullptr
        };
        HANDLE h = INVALID_HANDLE_VALUE;
        const char* loadedFrom = nullptr;
        for (int i = 0; paths[i]; ++i) {
            h = CreateFileA(paths[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                loadedFrom = paths[i];
                break;
            }
        }
        if (h == INVALID_HANDLE_VALUE) {
            LOG("No rust_decrypts.dat found. Using embedded decrypt defaults.");
            return false;
        }
        DWORD size = GetFileSize(h, NULL);
        if (size == 0 || size > 4096) { CloseHandle(h); return false; }
        char* data = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
        if (!data) { CloseHandle(h); return false; }
        DWORD rd = 0;
        ReadFile(h, data, size, &rd, NULL);
        CloseHandle(h);
        data[rd] = 0;

        char* line = data;
        for (DWORD i = 0; i <= rd; i++) {
            if (data[i] == '\r' || data[i] == '\n' || data[i] == 0) {
                data[i] = 0;
                if (line < data + i) {
                    char* eq = nullptr;
                    for (char* p = line; *p; p++) { if (*p == '=') { eq = p; break; } }
                    if (eq) {
                        *eq = 0;
                        char* key = line;
                        char* val = eq + 1;
                        while (*val == ' ') val++;
                        uint32_t iv = (uint32_t)strtoul(val, nullptr, 0);

                        #define LOAD_DEC(k, v) if (lstrcmpA(key, k) == 0) { v = iv; }

LOAD_DEC("nk_add1", DecryptCfg.nk_add1);
                        LOAD_DEC("nk_rol", DecryptCfg.nk_rol);
                        LOAD_DEC("nk_add2", DecryptCfg.nk_add2);
                        // networkable_key: add-rol-add
                        LOAD_DEC("nk2_rol", DecryptCfg.nk2_rol);
                        LOAD_DEC("nk2_sub", DecryptCfg.nk2_sub);
                        LOAD_DEC("nk2_xor", DecryptCfg.nk2_xor);
                        LOAD_DEC("nk2_add", DecryptCfg.nk2_add);
                        // networkable_key2: rol-sub-xor-add
                        LOAD_DEC("cla_xor", DecryptCfg.cla_xor);
                        LOAD_DEC("cla_rol", DecryptCfg.cla_rol);
                        LOAD_DEC("cla_add", DecryptCfg.cla_add);
                        // decrypt_ClActiveItem: xor-rol-add
                        LOAD_DEC("inv_rol1", DecryptCfg.inv_rol1);
                        LOAD_DEC("inv_xor", DecryptCfg.inv_xor);
                        LOAD_DEC("inv_add", DecryptCfg.inv_add);
                        LOAD_DEC("inv_rol2", DecryptCfg.inv_rol2);
                        // decrypt_inventory_pointer: rol-xor-add-rol
                        LOAD_DEC("ey_sub", DecryptCfg.ey_sub);
                        LOAD_DEC("ey_xor", DecryptCfg.ey_xor);
                        LOAD_DEC("ey_add", DecryptCfg.ey_add);
                        LOAD_DEC("ey_rol", DecryptCfg.ey_rol);
                        // decrypt_eyes: sub-xor-add-rol
                        LOAD_DEC("fov_add1", DecryptCfg.fov_add1);
                        LOAD_DEC("fov_rol", DecryptCfg.fov_rol);
                        LOAD_DEC("fov_sub", DecryptCfg.fov_sub);

                        #undef LOAD_DEC
                    }
                }
                line = data + i + 1;
            }
        }
        HeapFree(GetProcessHeap(), 0, data);

        LOG("Decrypt config loaded from %s", loadedFrom);
        return true;
    }

    inline void SaveDecryptConfig() {
        // No-op — disabled for anti-cheat stealth. Embedded defaults used instead.
    }

    inline int ScanAndUpdate(uintptr_t gameAssemblyBase) {
        int resolved = 0;
        char buf[256];

        LOG("[OffsetMgr] Pattern scanning disabled for external cheat — using embedded defaults.");

        auto probe = [&](uint64_t rva, const char* name) -> bool {
            uintptr_t ptr = gameAssemblyBase + rva;
            uintptr_t val = read<uintptr_t>(ptr);
            if (val > 0x100000 && val < 0x7FFFFFFFFFFF) {
                wsprintfA(buf, "[OffsetMgr] %s resolved at RVA 0x%IX -> 0x%IX", name, rva, val);
                LOG(buf);
                return true;
            }
            wsprintfA(buf, "[OffsetMgr] %s is NULL at RVA 0x%llX (may populate later — not a failure)", name, rva);
            LOG(buf);
            return false;
        };

        if (probe(offsets::basenetworkable_pointer, "BaseNetworkable")) resolved++;
        else LOG("[OffsetMgr] BaseNetworkable pointer validation FAILED.");
        if (probe(offsets::camera_pointer, "BaseCamera")) resolved++;
        else LOG("[OffsetMgr] BaseCamera pointer validation FAILED.");
        if (probe(offsets::Class_TOD_Sky_Static, "TOD_Sky")) resolved++;
        else LOG("[OffsetMgr] TOD_Sky pointer validation FAILED.");

        return resolved;
    }

    inline void Initialize(uintptr_t gameAssemblyBase) {
        LOG("=== OffsetManager Initializing ===");

        LoadDecryptConfig();
        int resolved = ScanAndUpdate(gameAssemblyBase);
        char buf[128];
        wsprintfA(buf, "[OffsetMgr] Initialization complete. %d pointers validated.", resolved);
        LOG(buf);
    }
}
