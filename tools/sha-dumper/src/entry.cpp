#include "resolver/resolver.hpp"

#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#pragma comment(lib, "shell32.lib")

static char g_logPath[MAX_PATH] = "C:\\rust_dumper.log";

void g_dlog(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    auto fp = std::fopen(g_logPath, "a");
    if (fp) { std::fprintf(fp, "%s\n", buf); std::fclose(fp); }
}

static void init_log_path()
{
    char desktop[MAX_PATH]{};
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktop)))
        GetEnvironmentVariableA("USERPROFILE", desktop, MAX_PATH);
    std::snprintf(g_logPath, MAX_PATH, "%s\\rust_dumper.log", desktop);
    auto fp = std::fopen(g_logPath, "w");
    if (fp) { std::fprintf(fp, "=== sha-dumper log ===\n"); std::fclose(fp); }
}

static bool safe_run(g_resolver* solver, const char* out_path)
{
    __try {
        return solver->run(out_path);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        g_dlog("[entry] solver.run() CRASHED (SEH code=0x%X)", GetExceptionCode());
        return false;
    }
}

static void safe_alloc_console()
{
    __try {
        AllocConsole();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        g_dlog("[entry] AllocConsole CRASHED");
    }
}

static DWORD WINAPI dump_thread(LPVOID)
{
    init_log_path();
    g_dlog("[entry] dump_thread started");

    safe_alloc_console();
    g_dlog("[entry] AllocConsole done");

    SetConsoleTitleA("sha-dumper");
    SetConsoleOutputCP(CP_UTF8);

    FILE* dummy = nullptr;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    g_dlog("[entry] console stdout redirected");

    char desktop[MAX_PATH]{};
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktop)))
        GetEnvironmentVariableA("USERPROFILE", desktop, MAX_PATH);

    char out[MAX_PATH]{};
    std::snprintf(out, MAX_PATH, "%s\\sha-dumper_output.txt", desktop);
    g_dlog("[entry] output path: %s", out);

    std::printf("[sha] -> %s\n", out);
    std::fflush(stdout);

    g_resolver solver;
    g_dlog("[entry] calling solver.run()...");
    bool ok = safe_run(&solver, out);
    g_dlog("[entry] solver.run() returned %d", ok);

    std::printf("[sha] press any key to close\n");
    std::fflush(stdout);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(mod);
        CreateThread(nullptr, 0, dump_thread, nullptr, 0, nullptr);
    }
    return TRUE;
}
