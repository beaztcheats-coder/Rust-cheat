#pragma once
#include <windows.h>
#include <string>
#include "Logger.hpp"
#include "Driver.hpp"
#include "offsets.hpp"
#include "RuntimePaths.hpp"

namespace OffsetManager {

    struct DecryptConfig {
        // networkable_key (client_entities): rol-sub-xor-add
        uint32_t nk_rol = 0x16;
        uint32_t nk_sub = 0x512FB7E6;
        uint32_t nk_xor = 0x3C25B628;
        uint32_t nk_add = 0x606330A1;

        // networkable_key2 (entity_list): rol-xor-rol-xor
        uint32_t nk2_rol = 0x12;
        uint32_t nk2_xor = 0xE54E9BFF;
        uint32_t nk2_rol_2 = 0x8;
        uint32_t nk2_xor_2 = 0xCECB4770;

        // decrypt_ClActiveItem (cl_active_item): xor-add-rol-sub
        uint32_t cla_xor = 0x8041A4D4;
        uint32_t cla_add = 0x2270CDAC;
        uint32_t cla_rol = 0x1D;
        uint32_t cla_sub = 0x3BA7A498;

        // decrypt_inventory_pointer (player_inventory): rol-add-rol
        uint32_t inv_rol = 0x8;
        uint32_t inv_add = 0x18E53C82;
        uint32_t inv_rol_2 = 0x1;

        // decrypt_eyes (player_eyes): sub-xor-rol-add
        uint32_t ey_sub = 0x6FB58358;
        uint32_t ey_xor = 0x6DC93C8F;
        uint32_t ey_rol = 0x15;
        uint32_t ey_add = 0x4E3D6061;

        // decrypt_fov: xor-add-sub
        uint32_t fov_xor = 0x8041A4D4;
        uint32_t fov_add = 0x2270CDAC;
        uint32_t fov_sub = 0x3BA7A498;
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

LOAD_DEC("nk_rol", DecryptCfg.nk_rol);
                        LOAD_DEC("nk_sub", DecryptCfg.nk_sub);
                        LOAD_DEC("nk_xor", DecryptCfg.nk_xor);
                        LOAD_DEC("nk_add", DecryptCfg.nk_add);
                        // networkable_key
                        LOAD_DEC("nk2_rol", DecryptCfg.nk2_rol);
                        LOAD_DEC("nk2_xor", DecryptCfg.nk2_xor);
                        LOAD_DEC("nk2_rol_2", DecryptCfg.nk2_rol_2);
                        LOAD_DEC("nk2_xor_2", DecryptCfg.nk2_xor_2);
                        // networkable_key2
                        LOAD_DEC("cla_xor", DecryptCfg.cla_xor);
                        LOAD_DEC("cla_add", DecryptCfg.cla_add);
                        LOAD_DEC("cla_rol", DecryptCfg.cla_rol);
                        LOAD_DEC("cla_sub", DecryptCfg.cla_sub);
                        // decrypt_ClActiveItem
                        LOAD_DEC("inv_rol", DecryptCfg.inv_rol);
                        LOAD_DEC("inv_add", DecryptCfg.inv_add);
                        LOAD_DEC("inv_rol_2", DecryptCfg.inv_rol_2);
                        // decrypt_inventory_pointer
                        LOAD_DEC("ey_sub", DecryptCfg.ey_sub);
                        LOAD_DEC("ey_xor", DecryptCfg.ey_xor);
                        LOAD_DEC("ey_rol", DecryptCfg.ey_rol);
                        LOAD_DEC("ey_add", DecryptCfg.ey_add);
                        // decrypt_eyes
                        LOAD_DEC("fov_xor", DecryptCfg.fov_xor);
                        LOAD_DEC("fov_add", DecryptCfg.fov_add);
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
