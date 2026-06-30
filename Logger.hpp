#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include "RuntimePaths.hpp"

#ifdef BEAZT_DEBUG
inline bool g_UpdateLoggingEnabled = true;  // Debug build: logging always on
#else
inline bool g_UpdateLoggingEnabled = false;  // Production: off by default, enable via marker file
#endif

class Logger {
public:
    static Logger& Get() {
        static Logger instance;
        return instance;
    }

    void Init(const char* path = nullptr) {
        if (!g_UpdateLoggingEnabled) return;
        if (!path) path = RuntimePaths::DefaultLogPath();
        log_path = path;
        HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }

    void Log(const char* fmt, ...) {
        if (!g_UpdateLoggingEnabled) return;
        va_list args;
        va_start(args, fmt);
        char buf[1024];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        WriteLine(buf);
    }

    void LogHex(const char* label, UINT64 value) {
        if (!g_UpdateLoggingEnabled) return;
        char buf[256];
        snprintf(buf, sizeof(buf), "%s: 0x%016llX", label, (unsigned long long)value);
        WriteLine(buf);
    }

    void LogChainStep(const char* step, UINT64 addr, UINT64 value) {
        if (!g_UpdateLoggingEnabled) return;
        char buf[256];
        snprintf(buf, sizeof(buf), "  %s: addr=0x%016llX value=0x%016llX", step, (unsigned long long)addr, (unsigned long long)value);
        WriteLine(buf);
    }

private:
    Logger() {}
    ~Logger() {}

    void WriteLine(const char* msg) {
        HANDLE h = CreateFileA(log_path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) return;
        SYSTEMTIME st;
        GetLocalTime(&st);
        char line[1280];
        snprintf(line, sizeof(line), "[%02d:%02d:%02d.%03d] %s\r\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
        DWORD written;
        WriteFile(h, line, (DWORD)strlen(line), &written, NULL);
        CloseHandle(h);
    }

    const char* log_path = nullptr;
};

#define LOG(...) do { if (g_UpdateLoggingEnabled) Logger::Get().Log(__VA_ARGS__); } while(0)
#define LOG_HEX(label, val) do { if (g_UpdateLoggingEnabled) Logger::Get().LogHex(label, val); } while(0)
#define LOG_CHAIN(step, addr, val) do { if (g_UpdateLoggingEnabled) Logger::Get().LogChainStep(step, addr, val); } while(0)
