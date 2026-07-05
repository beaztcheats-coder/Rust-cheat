#pragma once
#include <windows.h>
#include <string>
#include <cctype>

extern HMODULE g_hSelfModule;

namespace RuntimePaths {

    inline std::string DllDirectory() {
        char path[MAX_PATH] = {};
        if (g_hSelfModule && GetModuleFileNameA(g_hSelfModule, path, MAX_PATH)) {
            std::string s(path);
            size_t pos = s.find_last_of('\\');
            if (pos != std::string::npos) return s.substr(0, pos + 1);
        }
        // Fallback: try GetModuleHandleExA
        HMODULE self = nullptr;
        if (GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&DllDirectory,
                &self)) {
            if (GetModuleFileNameA(self, path, MAX_PATH)) {
                std::string s(path);
                size_t pos = s.find_last_of('\\');
                if (pos != std::string::npos) return s.substr(0, pos + 1);
            }
        }
        // Last resort: %TEMP%
        char tempPath[MAX_PATH];
        GetTempPathA(MAX_PATH, tempPath);
        return std::string(tempPath);
    }

    inline std::string CurrentModuleNameLower() {
        char path[MAX_PATH] = {};
        if (g_hSelfModule && GetModuleFileNameA(g_hSelfModule, path, MAX_PATH)) {
            // Extract filename
            const char* file = path;
            for (const char* p = path; *p; ++p) {
                if (*p == '\\' || *p == '/') file = p + 1;
            }
            std::string lower(file);
            for (char& ch : lower) ch = (char)std::tolower((unsigned char)ch);
            return lower;
        }
        // Fallback: try GetModuleHandleExA
        HMODULE self = nullptr;
        if (GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&CurrentModuleNameLower,
                &self)) {
            if (GetModuleFileNameA(self, path, MAX_PATH)) {
                const char* file = path;
                for (const char* p = path; *p; ++p) {
                    if (*p == '\\' || *p == '/') file = p + 1;
                }
                std::string lower(file);
                for (char& ch : lower) ch = (char)std::tolower((unsigned char)ch);
                return lower;
            }
        }
        return "";
    }

    inline bool IsBomza() {
#ifdef BOMZA
        return true;
#else
        const std::string name = CurrentModuleNameLower();
        return name.find("bomza") != std::string::npos;
#endif
    }

    inline bool IsBetterCheats() {
#ifdef BETTERCHEATS
        return true;
#else
        const std::string name = CurrentModuleNameLower();
        return name.find("bettercheats") != std::string::npos;
#endif
    }

    inline const char* DefaultConfigPath() {
        static std::string path;
        if (path.empty()) {
            char tempPath[MAX_PATH];
            GetTempPathA(MAX_PATH, tempPath);
            if (IsBetterCheats())      path = std::string(tempPath) + "rustcfg_bc.dat";
            else if (IsBomza())         path = std::string(tempPath) + "rustcfg_bomza.dat";
            else                        path = std::string(tempPath) + "rustcfg.dat";
        }
        return path.c_str();
    }

    inline const char* ConfigPrefix() {
        if (IsBetterCheats()) return "rustcfg_bc";
        if (IsBomza()) return "rustcfg_bomza";
        return "rustcfg";
    }

    inline const char* DefaultLogPath() {
        static std::string path;
        if (path.empty()) {
            std::string modName = CurrentModuleNameLower();
            const char* logFile;
            if (IsBetterCheats())      logFile = "cheat_debug_bc.log";
            else if (IsBomza())         logFile = "cheat_debug_bomza.log";
            else                        logFile = "cheat_debug.log";
            if (modName.find("_debug") != std::string::npos || modName.find("bomza") != std::string::npos || modName.find("bettercheats") != std::string::npos) {
                // Debug builds: write to %TEMP% (always writable, survives DLL unload)
                char tempPath[MAX_PATH];
                GetTempPathA(MAX_PATH, tempPath);
                path = std::string(tempPath) + logFile;
            } else {
                path = DllDirectory() + logFile;
            }
        }
        return path.c_str();
    }

    inline const char* MeshPath() {
        static std::string path;
        if (path.empty()) {
            path = DllDirectory() + "rust_mesh.tri";
        }
        return path.c_str();
    }

    inline const char* DecryptsPath() {
        static std::string path;
        if (path.empty()) {
            path = DllDirectory() + "rust_decrypts.dat";
        }
        return path.c_str();
    }

    inline const char* DiagnosticMarkerPath() {
        static std::string path;
        if (path.empty()) {
            path = DllDirectory() + "cheat_dll_loaded.txt";
        }
        return path.c_str();
    }

    inline const char* SpooferSeedPath() {
        static std::string path;
        if (path.empty()) {
            path = DllDirectory() + "spoof_seed.dat";
        }
        return path.c_str();
    }

    inline const char* SpooferLogPath() {
        static std::string path;
        if (path.empty()) {
            path = DllDirectory() + "spoofer_debug.log";
        }
        return path.c_str();
    }
}
