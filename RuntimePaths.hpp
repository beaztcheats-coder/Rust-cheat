#pragma once
#include <windows.h>
#include <string>
#include <cctype>

namespace RuntimePaths {

    inline std::string DllDirectory() {
        char path[MAX_PATH] = {};
        HMODULE self = nullptr;
        if (!GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&DllDirectory,
                &self)) {
            return "";
        }
        if (!GetModuleFileNameA(self, path, MAX_PATH)) {
            return "";
        }
        std::string s(path);
        size_t pos = s.find_last_of('\\');
        if (pos == std::string::npos) return "";
        return s.substr(0, pos + 1);
    }

    inline std::string CurrentModuleNameLower() {
        HMODULE self = nullptr;
        if (!GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&CurrentModuleNameLower,
                &self)) {
            return "";
        }

        char path[MAX_PATH] = {};
        if (!GetModuleFileNameA(self, path, MAX_PATH)) {
            return "";
        }

        const char* file = path;
        for (const char* p = path; *p; ++p) {
            if (*p == '\\' || *p == '/') file = p + 1;
        }

        std::string lower(file);
        for (char& ch : lower) ch = (char)std::tolower((unsigned char)ch);
        return lower;
    }

    inline bool IsBomza() {
#ifdef BOMZA
        return true;
#else
        const std::string name = CurrentModuleNameLower();
        return name.find("bomza") != std::string::npos;
#endif
    }

    inline const char* DefaultConfigPath() {
        static std::string path;
        if (path.empty()) {
            path = DllDirectory() + (IsBomza() ? "rustcfg_bomza.dat" : "rustcfg.dat");
        }
        return path.c_str();
    }

    inline const char* ConfigPrefix() {
        if (IsBomza()) return "rustcfg_bomza";
        return "rustcfg";
    }

    inline const char* DefaultLogPath() {
        static std::string path;
        if (path.empty()) {
            path = DllDirectory() + (IsBomza() ? "cheat_debug_bomza.log" : "cheat_debug.log");
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
