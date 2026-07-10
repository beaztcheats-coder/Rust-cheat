#include <windows.h>
#include <mutex>
#include <string>
#include <vector>
#include <TlHelp32.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <conio.h>
#include "Logger.hpp"
#include "Config.hpp"
#include "Overlay.hpp"
#include "OffsetManager.hpp"
#include "RuntimePaths.hpp"

#if !defined(BETTERCHEATS)
#include "Hacks/Misc/spoofer.hpp"
#endif
#if !defined(BETTERCHEATS)
#include "Hacks/Misc/cleaner.hpp"
#endif

extern std::atomic<uint64_t> g_CacheHeartbeatMs;
extern std::atomic<uint32_t> g_CacheThreadEpoch;
extern std::atomic<int> g_BnStableCycles;
extern std::atomic<int> g_CacheStep;
extern std::atomic<uint64_t> g_FastRefreshHeartbeatMs;
extern std::atomic<uint32_t> g_FastRefreshEpoch;
extern std::atomic<uint64_t> g_SkeletonHeartbeatMs;
extern std::atomic<uint32_t> g_SkeletonEpoch;

#pragma comment(lib, "ntdll.lib")

HANDLE driver_handle = nullptr;
HMODULE g_hSelfModule = nullptr;
static PVOID g_vehHandle = nullptr;

LONG WINAPI CrashVEH(EXCEPTION_POINTERS* ep) {
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        uintptr_t ip = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;

        HMODULE hSelf = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)ip, &hSelf);
        uintptr_t base = (uintptr_t)hSelf;
        if (!base) {
            MEMORY_BASIC_INFORMATION mbi = {};
            if (VirtualQuery((LPCVOID)ip, &mbi, sizeof(mbi)))
                base = (uintptr_t)mbi.AllocationBase;
        }
        uintptr_t crashRVA = ip - base;

        LOG("VEH: ACCESS_VIOLATION at RVA=0x%IX (base=0x%IX IP=0x%IX)",
            crashRVA, base, (uintptr_t)ep->ExceptionRecord->ExceptionAddress);
        if (ep->ExceptionRecord->NumberParameters >= 2) {
            LOG("VEH: access type=%s at 0x%IX",
                ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "READ" : "WRITE",
                ep->ExceptionRecord->ExceptionInformation[1]);
        }

        CONTEXT* ctx = ep->ContextRecord;
        LOG("VEH: RAX=0x%IX RCX=0x%IX RDX=0x%IX RBX=0x%IX",
            ctx->Rax, ctx->Rcx, ctx->Rdx, ctx->Rbx);
        LOG("VEH: RSP=0x%IX RBP=0x%IX RSI=0x%IX RDI=0x%IX",
            ctx->Rsp, ctx->Rbp, ctx->Rsi, ctx->Rdi);
        LOG("VEH: R8=0x%IX R9=0x%IX R10=0x%IX R11=0x%IX",
            ctx->R8, ctx->R9, ctx->R10, ctx->R11);
        LOG("VEH: R12=0x%IX R13=0x%IX R14=0x%IX R15=0x%IX",
            ctx->R12, ctx->R13, ctx->R14, ctx->R15);
        LOG("VEH: RIP=0x%IX", ctx->Rip);

        void* frames[64];
        USHORT nFrames = RtlCaptureStackBackTrace(0, 64, frames, NULL);
        LOG("VEH: stack trace (%u frames):", (unsigned)nFrames);
        for (USHORT i = 0; i < nFrames; i++) {
            HMODULE hFrame = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)frames[i], &hFrame);
            uintptr_t frameBase = (uintptr_t)hFrame;
            if (!frameBase) {
                MEMORY_BASIC_INFORMATION mbi = {};
                if (VirtualQuery((LPCVOID)frames[i], &mbi, sizeof(mbi)))
                    frameBase = (uintptr_t)mbi.AllocationBase;
            }
            uintptr_t frameRVA = (uintptr_t)frames[i] - frameBase;
            LOG("VEH: [%u] 0x%IX (RVA=0x%IX base=0x%IX)",
                (unsigned)i, (uintptr_t)frames[i], frameRVA, frameBase);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void ConsolePrint(const char* msg) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h && h != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteConsoleA(h, msg, (DWORD)lstrlenA(msg), &written, NULL);
    }
}

void ConsolePrintHex(const char* label, UINT64 val) {
    char buf[256];
    wsprintfA(buf, "%s: 0x%I64X\n", label, val);
    ConsolePrint(buf);
}

void SafeCLS() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hOut || hOut == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written;
    COORD home = { 0, 0 };
    FillConsoleOutputCharacterA(hOut, ' ', cells, home, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(hOut, home);
}

void SafePause() {
    ConsolePrint("Press any key to continue...\n");
    while (true) {
        for (int k = 0x08; k <= 0xFE; k++) {
            if (GetAsyncKeyState(k) & 1) return;
        }
        Sleep(50);
    }
}

void CreateConsole()
{
    if (!AllocConsole()) {
        AttachConsole(ATTACH_PARENT_PROCESS);
    }
    SetConsoleTitleA("Rust External");

    HANDLE hOut = CreateFileA("CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    HANDLE hIn = CreateFileA("CONIN$", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hOut && hOut != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, hOut);
        SetStdHandle(STD_ERROR_HANDLE, hOut);
    }
    if (hIn && hIn != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_INPUT_HANDLE, hIn);
    }

    HWND conWnd = GetConsoleWindow();
    if (conWnd) {
        SetWindowPos(conWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
        SetForegroundWindow(conWnd);
    }
}

void MainThreadImpl();

static LONG s_already_running = 0;
static std::mutex g_cache_worker_lock;

static void StartCacheWorker(const char* reason)
{
    std::lock_guard<std::mutex> lk(g_cache_worker_lock);
    uint32_t epoch = g_CacheThreadEpoch.fetch_add(1, std::memory_order_acq_rel) + 1;
    g_CacheHeartbeatMs.store(GetTickCount64(), std::memory_order_release);
    LOG("CACHE: starting worker epoch=%u (%s)", epoch, reason ? reason : "n/a");
    std::thread([]() {
        __try {
            if (cac) {
                cac->do_Cache();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG("CRASH in cache worker thread! Exception code: 0x%X (last entity=%d, step=%d)", GetExceptionCode(), g_CacheLastEntity.load(std::memory_order_relaxed), g_CacheStep.load(std::memory_order_relaxed));
            Sleep(250);
        }
    }).detach();
}

static void StartFastRefreshWorker(const char* reason)
{
    std::lock_guard<std::mutex> lk(g_cache_worker_lock);
    uint32_t epoch = g_FastRefreshEpoch.fetch_add(1, std::memory_order_acq_rel) + 1;
    g_FastRefreshHeartbeatMs.store(GetTickCount64(), std::memory_order_release);
    LOG("POS: starting position refresh worker epoch=%u (%s)", epoch, reason ? reason : "n/a");
    std::thread([]() {
        __try {
            if (cac) {
                cac->do_PositionRefresh();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG("CRASH in position refresh thread! Exception code: 0x%X (last entity=%d)", GetExceptionCode(), g_CacheLastEntity.load(std::memory_order_relaxed));
            Sleep(250);
        }
    }).detach();
}

static void StartSkeletonWorker(const char* reason)
{
    std::lock_guard<std::mutex> lk(g_cache_worker_lock);
    uint32_t epoch = g_SkeletonEpoch.fetch_add(1, std::memory_order_acq_rel) + 1;
    g_SkeletonHeartbeatMs.store(GetTickCount64(), std::memory_order_release);
    LOG("SKEL: starting skeleton refresh worker epoch=%u (%s)", epoch, reason ? reason : "n/a");
    std::thread([]() {
        __try {
            if (cac) {
                cac->do_SkeletonRefresh();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            LOG("CRASH in skeleton refresh thread! Exception code: 0x%X (last entity=%d)", GetExceptionCode(), g_CacheLastEntity.load(std::memory_order_relaxed));
            Sleep(250);
        }
    }).detach();
}

static void StartCacheWatchdog()
{
    std::thread([]() {
        uint64_t lastRestartMs = 0;
        uint64_t lastFastRestartMs = 0;
        bool cacheStaleLogged = false;
        while (!MISC::ShutdownRequested) {
            Sleep(1000);
            uint64_t now = GetTickCount64();

            uint64_t hb = g_CacheHeartbeatMs.load(std::memory_order_acquire);
            if (hb && now > hb && (now - hb) > 30000ULL) {
                if (lastRestartMs > 0 && (now - lastRestartMs) < 60000ULL) {
                    if (!cacheStaleLogged) { LOG("CACHE watchdog: stale (%llums), cooldown active", now - hb); cacheStaleLogged = true; }
                } else {
                    LOG("CACHE watchdog: stale (%llums), restarting", now - hb);
                    lastRestartMs = now;
                    cacheStaleLogged = false;
                    StartCacheWorker("watchdog");
                }
            } else { cacheStaleLogged = false; }

            uint64_t fhb = g_FastRefreshHeartbeatMs.load(std::memory_order_acquire);
            if (fhb && now > fhb && (now - fhb) > 15000ULL) {
                if (lastFastRestartMs > 0 && (now - lastFastRestartMs) < 60000ULL) {
                } else {
                    LOG("POS watchdog: stale (%llums), restarting", now - fhb);
                    lastFastRestartMs = now;
                    StartFastRefreshWorker("watchdog");
                }
            }

            uint64_t shb = g_SkeletonHeartbeatMs.load(std::memory_order_acquire);
            if (shb && now > shb && (now - shb) > 30000ULL) {
                static uint64_t lastSkelRestartMs = 0;
                if (lastSkelRestartMs > 0 && (now - lastSkelRestartMs) < 60000ULL) {
                } else {
                    LOG("SKEL watchdog: stale (%llums), restarting", now - shb);
                    lastSkelRestartMs = now;
                    StartSkeletonWorker("watchdog");
                }
            }
        }
    }).detach();
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
    if (InterlockedCompareExchange(&s_already_running, 1, 0) != 0)
        return 0;

    EXCEPTION_POINTERS* crashEp = nullptr;
    __try
    {
        MainThreadImpl();
    }
    __except (crashEp = GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER)
    {
        DWORD code = GetExceptionCode();
        void* crashAddr = crashEp ? crashEp->ExceptionRecord->ExceptionAddress : nullptr;
        char msg[512];
        wsprintfA(msg, "Cheat crashed!\nException code: 0x%X\nCrash address: 0x%p\n\nSend a screenshot to support.", code, crashAddr);
        LOG("CRASH in MainThread! code=0x%X addr=0x%p", code, crashAddr);
        if (crashEp && crashEp->ExceptionRecord->NumberParameters >= 2) {
            LOG("CRASH: access type=%s at 0x%p",
                crashEp->ExceptionRecord->ExceptionInformation[0] == 0 ? "READ" : "WRITE",
                (void*)crashEp->ExceptionRecord->ExceptionInformation[1]);
        }
        void* frames[32];
        USHORT nFrames = CaptureStackBackTrace(0, 32, frames, NULL);
        LOG("CRASH: stack trace (%u frames):", (unsigned)nFrames);
        for (USHORT i = 0; i < nFrames; i++) {
            LOG("CRASH: [%u] 0x%p", (unsigned)i, frames[i]);
        }
        MessageBoxA(NULL, msg, "Cheat Error", MB_ICONERROR | MB_OK);
    }
    return 0;
}

void MainThreadImpl()
{
    g_vehHandle = AddVectoredExceptionHandler(1, CrashVEH);

<<<<<<< HEAD
    // DPI awareness — SetProcessDpiAwarenessContext fails silently in injected DLLs
    // (host process already set its own DPI awareness). This is intentional — no fallbacks
    // because loading shcore.dll or calling SetProcessDpiAwareness/SetProcessDPIAware
    // triggers EAC to block OpenProcess, breaking get_module().
    // Overlay fixes (swap chain resize, ScreenToClient) still work without DPI awareness.
    {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            typedef BOOL(WINAPI* SetProcessDpiAwarenessContext_t)(HANDLE);
            auto pSetProcessDpiAwarenessContext = (SetProcessDpiAwarenessContext_t)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
            if (pSetProcessDpiAwarenessContext) {
                pSetProcessDpiAwarenessContext((HANDLE)-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
            }
        }
    }

    // Delete ALL temp files at startup (except config + debug log for debug builds)
    {
        std::string dllDir = RuntimePaths::DllDirectory();
        auto delFile = [&](const char* path) { DeleteFileA(path); };
        auto delDll = [&](const char* name) { DeleteFileA((dllDir + name).c_str()); };

        // DLL directory temp files
        delDll("rust_decrypts.dat");
        delDll("rust_mesh.tri");
        delDll("cheat_dll_loaded.txt");
        delDll("rust_debug_enabled.txt");
        delDll("spoofer_debug.log");
        delDll("spoof_seed.dat");

        // C:\ temp files
        delFile("C:\\rust_decrypts.dat");
        delFile("C:\\rust_mesh.tri");
        delFile("C:\\rust_meshes.tri");
        delFile("C:\\rust_debug_enabled.txt");

        // %TEMP% log files (debug build keeps current flavor's log — Logger::Init recreates it)
        char tempPath[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tempPath);
        if (tempPath[0]) {
            std::string tp(tempPath);
            DeleteFileA((tp + "cheat_debug.log").c_str());
            DeleteFileA((tp + "cheat_debug_bomza.log").c_str());
            DeleteFileA((tp + "cheat_debug_bc.log").c_str());
        }
    }

=======
    // Delete ALL temp files at startup (except config + debug log for debug builds)
    {
        std::string dllDir = RuntimePaths::DllDirectory();
        auto delFile = [&](const char* path) { DeleteFileA(path); };
        auto delDll = [&](const char* name) { DeleteFileA((dllDir + name).c_str()); };

        // DLL directory temp files
        delDll("rust_decrypts.dat");
        delDll("rust_mesh.tri");
        delDll("cheat_dll_loaded.txt");
        delDll("rust_debug_enabled.txt");
        delDll("spoofer_debug.log");
        delDll("spoof_seed.dat");

        // C:\ temp files
        delFile("C:\\rust_decrypts.dat");
        delFile("C:\\rust_mesh.tri");
        delFile("C:\\rust_meshes.tri");
        delFile("C:\\rust_debug_enabled.txt");

        // %TEMP% log files (debug build keeps current flavor's log — Logger::Init recreates it)
        char tempPath[MAX_PATH] = {};
        GetTempPathA(MAX_PATH, tempPath);
        if (tempPath[0]) {
            std::string tp(tempPath);
            DeleteFileA((tp + "cheat_debug.log").c_str());
            DeleteFileA((tp + "cheat_debug_bomza.log").c_str());
            DeleteFileA((tp + "cheat_debug_bc.log").c_str());
        }
    }

>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6
    // Diagnostic marker in DLL directory (non-suspicious filename — NOT "cheat" in name)
    {
        HANDLE hDiag = CreateFileA(RuntimePaths::DiagnosticMarkerPath(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hDiag != INVALID_HANDLE_VALUE) {
            const char* msg = "DLL loaded\n";
            DWORD written;
            WriteFile(hDiag, msg, (DWORD)strlen(msg), &written, NULL);
            CloseHandle(hDiag);
        }
    }

    CreateConsole();

    // === STARTUP BANNER ===
    {
        SafeCLS();
        printf("\n");
        printf("  ============================================\n");
#ifdef BOMZA
        printf("         B O M Z A\n");
        printf("    Bomza Cheat  -  External\n");
#elif defined(BETTERCHEATS)
        printf("      B E T T E R   C H E A T S\n");
        printf("    Rust External  -  Better Cheats\n");
#else
        printf("       T H E   B E A Z T   V 3\n");
        printf("    Rust Private  -  External Cheat\n");
#endif
        printf("  ============================================\n\n");
        printf("  Launching cheat...\n\n");
        printf("  ============================================\n\n");
        fflush(stdout);
    }

        // Enable logging if user-created marker exists
        if (GetFileAttributesA("C:\\rust_debug_enabled.txt") != INVALID_FILE_ATTRIBUTES) {
            g_UpdateLoggingEnabled = true;
        }
        // Auto-enable logging if DLL name contains "_debug"
        {
            HMODULE self = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)&g_UpdateLoggingEnabled, &self);
            if (self) {
                char dllPath[MAX_PATH] = {};
                if (GetModuleFileNameA(self, dllPath, MAX_PATH)) {
                    std::string lower(dllPath);
                    for (char& c : lower) c = (char)tolower((unsigned char)c);
                    if (lower.find("_debug") != std::string::npos) {
                        g_UpdateLoggingEnabled = true;
                    }
                }
            }
        }
        Logger::Get().Init(RuntimePaths::DefaultLogPath());
        LOG("DLL injected, MainThread started");
        LOG("Build: %s %s", __DATE__, __TIME__);
        LOG("Logging: %s", g_UpdateLoggingEnabled ? "ENABLED" : "disabled");

        {
            HMODULE hSelf = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&MainThread, &hSelf);
            uintptr_t dllBase = (uintptr_t)hSelf;
            if (!dllBase) {
                MEMORY_BASIC_INFORMATION mbi = {};
                if (VirtualQuery((LPCVOID)&MainThread, &mbi, sizeof(mbi)))
                    dllBase = (uintptr_t)mbi.AllocationBase;
            }
            LOG("DLL base: 0x%IX (GetModuleHandle=%s, VirtualQuery fallback=%s)",
                dllBase, hSelf ? "OK" : "FAIL", !hSelf ? "used" : "not needed");
        }

#if !defined(BETTERCHEATS)
        // === STARTUP PROMPT: Spoof / Play (Bomza) or Spoof / Clean / Play (BeaZt) ===
        {
            SafeCLS();
            printf("\n");
            printf("  ============================================\n");
#if defined(BOMZA)
            printf("         B O M Z A\n");
            printf("    Bomza Cheat  -  External\n");
            printf("  ============================================\n\n");
            printf("  [1] Spoof and Play     (spoof HWID, then launch)\n");
            printf("  [2] Just play           (skip, launch directly)\n");
            printf("  [3] Clean after detection (run deep cleaner)\n\n");
            printf("  Choice: ");
            fflush(stdout);

            int choice = _getch();
            printf("%c\n\n", choice);

            if (choice == '1') {
                LOG("Startup: user chose Spoof and Play");
                RunEmbeddedSpoofer();
                ConsolePrint("\n  Spoof complete. Press any key to continue...\n");
                SafePause();
            }
            else if (choice == '3') {
                LOG("Startup: user chose Clean after detection");
                printf("\n  Launching deep cleaner...\n\n");
                fflush(stdout);
                Sleep(500);
                CleanerMenu();
                SafeCLS();
                printf("\n  ============================================\n");
                printf("  Cleaning complete.\n");
                printf("  Restart your PC, then re-inject to play.\n");
                printf("  ============================================\n\n");
                fflush(stdout);
                Sleep(3000);
                if (g_hSelfModule)
                    FreeLibraryAndExitThread(g_hSelfModule, 0);
                return;
            }
            else {
                LOG("Startup: user chose Just play (skip)");
            }
#else
            printf("       T H E   B E A Z T   V 3\n");
            printf("    Rust Private  -  External Cheat\n");
            printf("  ============================================\n\n");
            printf("  [1] Spoof and Play     (spoof HWID, then launch)\n");
            printf("  [2] Clean after detection (run deep cleaner)\n");
            printf("  [3] Just play           (skip, launch directly)\n\n");
            printf("  Choice: ");
            fflush(stdout);

            int choice = _getch();
            printf("%c\n\n", choice);

            if (choice == '1') {
                LOG("Startup: user chose Spoof and Play");
                RunEmbeddedSpoofer();
                ConsolePrint("\n  Spoof complete. Press any key to continue...\n");
                SafePause();
            }
            else if (choice == '2') {
                LOG("Startup: user chose Clean after detection");
                printf("\n  Launching deep cleaner...\n\n");
                fflush(stdout);
                Sleep(500);
                CleanerMenu();
                SafeCLS();
                printf("\n  ============================================\n");
                printf("  Cleaning complete.\n");
                printf("  Restart your PC, then re-inject to play.\n");
                printf("  ============================================\n\n");
                fflush(stdout);
                Sleep(3000);
                if (g_hSelfModule)
                    FreeLibraryAndExitThread(g_hSelfModule, 0);
                return;
            }
            else {
                LOG("Startup: user chose Just play (skip)");
            }
#endif
            SafeCLS();
        }
#endif

        ConsolePrint("Initializing...\n");

        Drv = new DriverInterface();
        src = new Rust();
        esp = new Visuals();
        hx = new misc();
        aim = new aimbot();
        cac = new cache();

        LOG("All objects constructed");
        Config::Load();
        InitTranslations();
        LOG("Config loaded");

        if (Drv->IsValid()) {
            ConsolePrint("Driver handle opened successfully.\n");
        }
        else {
            ConsolePrint("Failed to open device handle to driver.\n");
            LOG("FATAL: Failed to open driver handle, error=%d", GetLastError());
            SafePause();
            return;
        }

        SafeCLS();
        HWND conWnd = GetConsoleWindow();
        if (conWnd) SetWindowPos(conWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        ConsolePrint("Waiting for RustClient.exe or Rust.exe...\n");
        ConsolePrint("(Make sure EAC has loaded and you're at the main menu)\n");
        LOG("Waiting for RustClient.exe or Rust.exe...");

        static int waitDots = 0;
        while (!Drv->find_process(L"RustClient.exe"))
        {
            if (++waitDots % 8 == 0) ConsolePrint(".");
            Sleep(250);
        }
        ConsolePrint("\nGame found!\n");
        LOG("RustClient.exe found");

        SafeCLS();
        ConsolePrint("When in game 'Main Menu' press 'HOME' key.\n");

        while (!(GetAsyncKeyState(VK_HOME) & 0x8000))
        {
            Sleep(100);
        }
        LOG("HOME key pressed");

        SafeCLS();

        Drv->processid = Drv->find_process(L"RustClient.exe");
        if (!Drv->processid)
        {
            ConsolePrint("Process not found\n");
            LOG("FATAL: Process not found after HOME press");
            SafePause();
            return;
        }
        LOG("PID: %d", Drv->processid);

        if (GetCurrentProcessId() == Drv->processid) {
            g_UseInternalReads = true;
            LOG("Internal reads: ENABLED (running inside RustClient.exe)");
        } else {
            g_UseInternalReads = false;
            LOG("Internal reads: DISABLED (DLL not in Rust process, using driver)");
        }

        bool isbasecorrect = Drv->GetProcessBase(Drv->processid, Drv->process_base);
        if (!isbasecorrect)
        {
            ConsolePrint("Process base not found\n");
            LOG("FATAL: Process base not found");
            SafePause();
            return;
        }
        ConsolePrintHex("process_base", Drv->process_base);
        LOG_HEX("process_base", Drv->process_base);

        // CR3 MUST be set before get_module — PE scan fallback uses ReadMemory_Raw
        // which needs the driver's CR3 to translate virtual addresses.
        Drv->GetCR3_1(Drv->processid, false, Drv->process_cr3);
        ConsolePrintHex("CR3", Drv->process_cr3);
        LOG_HEX("process_cr3", Drv->process_cr3);

        // get_module — OpenProcess (primary) + PE scan (fallback)
        GameAssembly = Drv->get_module(L"GameAssembly.dll");
        if (!GameAssembly)
        {
            ConsolePrint("GameAssembly.dll not found\n");
            LOG("FATAL: GameAssembly.dll not found");
            SafePause();
            return;
        }
        ConsolePrintHex("GameAssembly", GameAssembly);
        LOG_HEX("GameAssembly", GameAssembly);
        Drv->m_game_assembly = GameAssembly;

        // Auto-detect and validate offsets
        OffsetManager::Initialize(GameAssembly);

        UnityPlayer = Drv->get_module(L"UnityPlayer.dll", GameAssembly);
        if (!UnityPlayer)
        {
            ConsolePrint("UnityPlayer.dll not found\n");
            LOG("FATAL: UnityPlayer.dll not found");
            SafePause();
            return;
        }
        ConsolePrintHex("UnityPlayer", UnityPlayer);
        LOG_HEX("UnityPlayer", UnityPlayer);
        ConsolePrintHex("CR3", Drv->process_cr3);
        LOG_HEX("process_cr3", Drv->process_cr3);

            LOG("Init complete, starting cache + overlay...");

            ConsolePrint("Starting... console will hide.\n");
            Sleep(1000);

            ShowWindow(GetConsoleWindow(), SW_HIDE);

        StartCacheWorker("initial");
        StartFastRefreshWorker("initial");
        StartSkeletonWorker("initial");
        StartCacheWatchdog();

        // Centralized process-death watchdog — checks IsProcessAlive every 20ms.
        // This is the FASTEST detection path. When Rust exits, this thread
        // immediately blocks all IOCTLs (g_process_dead + ioctl_blocked) to
        // prevent worker threads from sending IOCTLs with stale CR3 → BSOD.
        std::thread([]() {
            __try {
                while (!MISC::ShutdownRequested) {
                    Sleep(20);  // was 100 — 5x faster detection prevents stale CR3 IOCTLs
                    if (Drv && Drv->processid) {
                        if (!Drv->IsProcessAlive()) {
                            LOG("WATCHDOG: RustClient.exe died — blocking all IOCTLs to prevent BSOD");
                            g_process_dead.store(true, std::memory_order_seq_cst);
                            Drv->ioctl_blocked.store(true, std::memory_order_seq_cst);
                            MISC::ShutdownRequested = true;
                            break;
                        }
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                LOG("CRASH in process-death watchdog! Exception code: 0x%X", GetExceptionCode());
            }
        }).detach();

        // Aimbot + misc background threads (off the render path)
        std::thread([]() {
            __try {
                uint64_t miscCycle = 0;
                while (!MISC::ShutdownRequested) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    if (src && src->LocalPlayer && is_valid((uintptr_t)src->LocalPlayer)
                        && g_BnStableCycles.load(std::memory_order_relaxed) >= 3) {
                        hx->do_misc();
                    }
                    if ((++miscCycle % 20) == 0 && Drv) {
                        static int miscProcMiss = 0;
                        if (!Drv->find_process(L"RustClient.exe")) {
                            if (++miscProcMiss >= 3) {
                                LOG("MISC: RustClient.exe no longer running — shutting down to prevent BSOD");
                                g_process_dead.store(true, std::memory_order_relaxed);
                                MISC::ShutdownRequested = true;
                                break;
                            }
                        } else {
                            miscProcMiss = 0;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                LOG("CRASH in misc thread! Exception code: 0x%X", GetExceptionCode());
            }
        }).detach();

        // Aimbot now runs in the render thread (Do_Cheat) — no separate thread needed
        // This eliminates the race condition between aimbot and cache threads

        // Mark this thread as the render thread — bypasses IOCTL mutex for
        // camera matrix + aimbot reads. Prevents ESP sticking/menu lag on slow PCs.
        g_render_thread = true;

        Overlay* Draw = new Overlay;
        Draw->DrawOverlay();
        delete Draw;

        // Clear render thread flag — worker threads below use mutex
        g_render_thread = false;

        // === FULL CLEANUP SHUTDOWN — restore game, delete traces, unload DLL ===
        LOG("Cheat shutting down — restoring game memory + cleaning traces");

        // 0. Immediately block ALL IOCTLs — stops worker threads from sending driver requests
        g_shutting_down.store(true, std::memory_order_relaxed);
        if (Drv) Drv->ioctl_blocked.store(true, std::memory_order_relaxed);

        // 1. Signal all worker threads to stop BEFORE final do_misc (prevents concurrent do_misc race)
        MISC::ShutdownRequested = true;

        // 2. Force ALL memory-writing features off so do_misc() restores originals
        MISC::RecoilEnabled = false;
        MISC::FovChanger = false;
        MISC::Zoom = false;
        MISC::BrightNight = false;
        MISC::AdminFlags = false;
        MISC::CombatMode = false;
        MISC::DebugCamera = false;
        SETTINGS::BattleMode = false;
        // Call do_misc one final time to flush restoration writes
        if (hx && src && src->LocalPlayer && is_valid((uintptr_t)src->LocalPlayer)) {
            __try { hx->do_misc(); } __except(EXCEPTION_EXECUTE_HANDLER) {}
            Sleep(100); // give restoration writes time to flush
        }

        // 3. Wait for ALL workers to stop (check all heartbeats)
        {
            ULONGLONG startWait = GetTickCount64();
            while (GetTickCount64() - startWait < 10000) {
                ULONGLONG now = GetTickCount64();
                ULONGLONG hb = g_CacheHeartbeatMs.load(std::memory_order_acquire);
                ULONGLONG fhb = g_FastRefreshHeartbeatMs.load(std::memory_order_acquire);
                ULONGLONG shb = g_SkeletonHeartbeatMs.load(std::memory_order_acquire);
                bool cacheIdle = (now - hb > 2000);
                bool fastIdle = (now - fhb > 1000);
                bool skelIdle = (now - shb > 1000);
                if (cacheIdle && fastIdle && skelIdle) break;
                Sleep(100);
            }
        }
        Sleep(500); // extra safety margin for detached threads to fully exit

        // 4. Null the Drv pointer so no new IOCTLs are attempted.
        // DO NOT delete Drv or CloseHandle — if an IOCTL is still in-flight
        // (worker didn't fully stop within the timeout), deleting the object
        // causes use-after-free → BSOD. The process is about to exit anyway,
        // so let Windows clean up the driver handle on process termination.
        Drv = nullptr;

        // 5. Delete ALL temp files — debug keeps config + debug log, production deletes everything
        {
            std::string dllDir = RuntimePaths::DllDirectory();
            auto delDll = [&](const char* name) { DeleteFileA((dllDir + name).c_str()); };
            auto delFile = [&](const char* path) { DeleteFileA(path); };

            // DLL directory temp files — always delete
            delDll("rust_decrypts.dat");
            delDll("rust_mesh.tri");
            delDll("cheat_dll_loaded.txt");
            delDll("rust_debug_enabled.txt");
            delDll("spoofer_debug.log");
            delDll("spoof_seed.dat");

            // C:\ temp files — always delete
            delFile("C:\\rust_decrypts.dat");
            delFile("C:\\rust_mesh.tri");
            delFile("C:\\rust_meshes.tri");
            delFile("C:\\rust_debug_enabled.txt");

            // %TEMP% log files
            char tempPath[MAX_PATH] = {};
            GetTempPathA(MAX_PATH, tempPath);
            if (tempPath[0]) {
                std::string tp(tempPath);
                DeleteFileA((tp + "cheat_debug.log").c_str());
                DeleteFileA((tp + "cheat_debug_bomza.log").c_str());
                DeleteFileA((tp + "cheat_debug_bc.log").c_str());
            }

            if (!g_UpdateLoggingEnabled) {
                // Production: delete config files too
                delDll("rustcfg.dat");
                delDll("rustcfg_bomza.dat");
                delDll("rustcfg_bc.dat");
                if (tempPath[0]) {
                    std::string tp(tempPath);
                    DeleteFileA((tp + "rustcfg.dat").c_str());
                    DeleteFileA((tp + "rustcfg_bomza.dat").c_str());
                    DeleteFileA((tp + "rustcfg_bc.dat").c_str());
                }
                LOG("Production cleanup: ALL temp files deleted");
            } else {
                LOG("DEBUG build: temp files deleted (config + debug log preserved)");
            }
        }

        // 6. Self-delete: spawn hidden batch to delete DLL after unload
        //    Created HERE (during shutdown) — NOT before render loop, to avoid
        //    cmd.exe process running while cheat is active (was causing EAC detection / crash)
        {
            char selfPath[MAX_PATH] = {};
            HMODULE selfModule = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&selfPath, &selfModule);
            GetModuleFileNameA(selfModule, selfPath, sizeof(selfPath));

            std::string dllCheck(selfPath);
            for (char& c : dllCheck) c = (char)tolower((unsigned char)c);
            bool isDebug = dllCheck.find("_debug") != std::string::npos;

            if (!isDebug) {
                std::string dllDir = RuntimePaths::DllDirectory();
                char batPath[MAX_PATH];
                wsprintfA(batPath, "%s\\del.bat", getenv("TEMP") ? getenv("TEMP") : "C:\\Windows\\Temp");

                HANDLE hBat = CreateFileA(batPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hBat != INVALID_HANDLE_VALUE) {
                    std::string bat;
                    bat += "@echo off\r\n";
                    bat += ":retry\r\n";
                    bat += "timeout /t 1 /nobreak >nul\r\n";
                    bat += "del /f /q \"" + std::string(selfPath) + "\" 2>nul\r\n";
                    bat += "if exist \"" + std::string(selfPath) + "\" goto retry\r\n";
                    bat += "del /f /q \"" + dllDir + "rust_mesh.tri\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "rust_decrypts.dat\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "rustcfg_*.dat\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "cheat_debug_*.log\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "cheat_debug.log\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "cheat_dll_loaded.txt\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "rust_debug_enabled.txt\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "spoofer_debug.log\" 2>nul\r\n";
                    bat += "del /f /q \"" + dllDir + "spoof_seed.dat\" 2>nul\r\n";
                    bat += "del /f /q \"%~f0\" 2>nul\r\n";
                    bat += "exit\r\n";
                    DWORD written;
                    WriteFile(hBat, bat.c_str(), (DWORD)bat.size(), &written, NULL);
                    CloseHandle(hBat);

                    STARTUPINFOA si = { sizeof(si) };
                    PROCESS_INFORMATION pi = {};
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    si.wShowWindow = SW_HIDE;
                    char cmdLine[512];
                    wsprintfA(cmdLine, "cmd.exe /c \"%s\"", batPath);
                    CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
                    if (pi.hProcess) CloseHandle(pi.hProcess);
                    if (pi.hThread) CloseHandle(pi.hThread);
                }
            } else {
                LOG("DEBUG build: self-delete batch SKIPPED — all files preserved");
            }
        }

        LOG("Shutdown complete — exiting process");

        if (g_vehHandle) { RemoveVectoredExceptionHandler(g_vehHandle); g_vehHandle = nullptr; }

        // 6. Exit the overlay process completely — closes overlay window, releases all resources
        // Self-delete batch (production) was already launched above and will delete the DLL file
        // after the process exits.
        ExitProcess(0);
}

extern "C" __declspec(dllexport) void Main()       { MainThread(nullptr); }
extern "C" __declspec(dllexport) void Init()       { MainThread(nullptr); }
extern "C" __declspec(dllexport) void Run()        { MainThread(nullptr); }
extern "C" __declspec(dllexport) void Entry()      { MainThread(nullptr); }
extern "C" __declspec(dllexport) void Start()      { MainThread(nullptr); }
extern "C" __declspec(dllexport) void Execute()    { MainThread(nullptr); }
extern "C" __declspec(dllexport) void Initialize() { MainThread(nullptr); }

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        g_hSelfModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
    }
    return TRUE;
}
