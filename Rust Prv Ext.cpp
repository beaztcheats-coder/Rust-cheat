#include <windows.h>
#include <mutex>
#include <string>
#include <vector>
#include <TlHelp32.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <conio.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "Logger.hpp"
#include "Config.hpp"
#include "Overlay.hpp"
#include "OffsetManager.hpp"
#include "RuntimePaths.hpp"

#if !defined(BOMZA) && !defined(BETTERCHEATS)
#include "Hacks/Misc/spoofer.hpp"
#include "Hacks/Misc/cleaner.hpp"
#endif

extern std::atomic<uint64_t> g_CacheHeartbeatMs;
extern std::atomic<uint32_t> g_CacheThreadEpoch;
extern std::atomic<int> g_BnStableCycles;
extern std::atomic<uint64_t> g_FastRefreshHeartbeatMs;
extern std::atomic<uint32_t> g_FastRefreshEpoch;
extern std::atomic<uint64_t> g_SkeletonHeartbeatMs;
extern std::atomic<uint32_t> g_SkeletonEpoch;

#pragma comment(lib, "ntdll.lib")

HANDLE driver_handle = nullptr;
HMODULE g_hSelfModule = nullptr;

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
            LOG("CRASH in cache worker thread! Exception code: 0x%X (last entity=%d)", GetExceptionCode(), g_CacheLastEntity.load(std::memory_order_relaxed));
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
        while (!MISC::ShutdownRequested) {
            Sleep(1000);
            uint64_t now = GetTickCount64();

            // Monitor slow cache thread
            uint64_t hb = g_CacheHeartbeatMs.load(std::memory_order_acquire);
            if (hb && now > hb && (now - hb) > 30000ULL) {
                if (lastRestartMs > 0 && (now - lastRestartMs) < 60000ULL) {
                    LOG("CACHE watchdog: stale (%llums) but cooldown active, skipping restart", now - hb);
                } else {
                    LOG("CACHE watchdog: stale heartbeat (%llums), restarting worker", now - hb);
                    lastRestartMs = now;
                    StartCacheWorker("watchdog");
                }
            }

            // Monitor fast refresh (position) thread
            uint64_t fhb = g_FastRefreshHeartbeatMs.load(std::memory_order_acquire);
            if (fhb && now > fhb && (now - fhb) > 15000ULL) {
                if (lastFastRestartMs > 0 && (now - lastFastRestartMs) < 60000ULL) {
                    // cooldown active, skip
                } else {
                    LOG("POS watchdog: stale heartbeat (%llums), restarting worker", now - fhb);
                    lastFastRestartMs = now;
                    StartFastRefreshWorker("watchdog");
                }
            }

            // Monitor skeleton thread
            uint64_t shb = g_SkeletonHeartbeatMs.load(std::memory_order_acquire);
            if (shb && now > shb && (now - shb) > 30000ULL) {
                static uint64_t lastSkelRestartMs = 0;
                if (lastSkelRestartMs > 0 && (now - lastSkelRestartMs) < 60000ULL) {
                    // cooldown active, skip
                } else {
                    LOG("SKEL watchdog: stale heartbeat (%llums), restarting worker", now - shb);
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

    __try
    {
        MainThreadImpl();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        LOG("CRASH in MainThread! Exception code: 0x%X", GetExceptionCode());
        ConsolePrint("CRASH detected - check debug log\n");
        SafePause();
    }
    return 0;
}

void MainThreadImpl()
{
    timeBeginPeriod(1);

    // RAW DIAGNOSTIC: write to C:\cheat_debug.log IMMEDIATELY — fixed path always findable
    {
        const char* diagPath = "C:\\cheat_debug.log";
        HANDLE hDiag = CreateFileA(diagPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hDiag != INVALID_HANDLE_VALUE) {
            std::string dllDir = RuntimePaths::DllDirectory();
            std::string msg = "[BOOT] DLL loaded. DllDir=" + dllDir + " LogPath=" + diagPath + "\r\n";
            DWORD written;
            WriteFile(hDiag, msg.c_str(), (DWORD)msg.size(), &written, NULL);
            CloseHandle(hDiag);
        }
    }

    // Diagnostic marker in DLL directory
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
        fflush(stdout);
    }

        // Enable logging if user-created marker exists (NOT the auto-created diagnostic marker)
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
                    printf("[LOG] DLL: %s\n", dllPath);
                    if (lower.find("_debug") != std::string::npos) {
                        g_UpdateLoggingEnabled = true;
                        printf("[LOG] Debug DLL detected - logging ENABLED\n");
                    } else {
                        printf("[LOG] Production DLL - logging disabled\n");
                    }
                }
            }
        }
        // For debug DLLs: force log path to C:\cheat_debug.log
        if (g_UpdateLoggingEnabled) {
            const char* debugLogPath = "C:\\cheat_debug.log";
            printf("[LOG] Log path: %s\n", debugLogPath);
            Logger::Get().Init(debugLogPath);
        } else {
            Logger::Get().Init(RuntimePaths::DefaultLogPath());
        }
        LOG("DLL injected, MainThread started");
        LOG("Build: %s %s", __DATE__, __TIME__);
        LOG("Logging: %s", g_UpdateLoggingEnabled ? "ENABLED" : "disabled");

#if !defined(BOMZA) && !defined(BETTERCHEATS)
        // === BEAZT STARTUP PROMPT: Spoof / Clean / Play ===
        {
            SafeCLS();
            printf("\n");
            printf("  ============================================\n");
            printf("       T H E   B E A Z T   V 3\n");
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
                // After cleaner exits, unload DLL — user restarts and re-injects
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

        UnityPlayer = Drv->get_module(L"UnityPlayer.dll");
        if (!UnityPlayer)
        {
            ConsolePrint("UnityPlayer.dll not found\n");
            LOG("FATAL: UnityPlayer.dll not found");
            SafePause();
            return;
        }
        ConsolePrintHex("UnityPlayer", UnityPlayer);
        LOG_HEX("UnityPlayer", UnityPlayer);

                Drv->GetCR3_1(Drv->processid, false, Drv->process_cr3);
            ConsolePrintHex("CR3", Drv->process_cr3);
            LOG_HEX("process_cr3", Drv->process_cr3);

            // Log decrypt constants for diagnostics
            LOG("DecryptCfg: nk_rol=0x%X nk_sub=0x%X nk_xor=0x%X nk_add=0x%X", OffsetManager::DecryptCfg.nk_rol, OffsetManager::DecryptCfg.nk_sub, OffsetManager::DecryptCfg.nk_xor, OffsetManager::DecryptCfg.nk_add);
            LOG("DecryptCfg: nk2_rol=0x%X nk2_xor=0x%X nk2_rol_2=0x%X nk2_xor_2=0x%X", OffsetManager::DecryptCfg.nk2_rol, OffsetManager::DecryptCfg.nk2_xor, OffsetManager::DecryptCfg.nk2_rol_2, OffsetManager::DecryptCfg.nk2_xor_2);
            LOG("DecryptCfg: cla_sub=0x%X cla_rol=0x%X cla_xor=0x%X cla_add=0x%X", OffsetManager::DecryptCfg.cla_sub, OffsetManager::DecryptCfg.cla_rol, OffsetManager::DecryptCfg.cla_xor, OffsetManager::DecryptCfg.cla_add);
            LOG("DecryptCfg: inv_rol=0x%X inv_add=0x%X inv_rol_2=0x%X", OffsetManager::DecryptCfg.inv_rol, OffsetManager::DecryptCfg.inv_add, OffsetManager::DecryptCfg.inv_rol_2);
            LOG("DecryptCfg: ey_sub=0x%X ey_xor=0x%X ey_rol=0x%X ey_add=0x%X", OffsetManager::DecryptCfg.ey_sub, OffsetManager::DecryptCfg.ey_xor, OffsetManager::DecryptCfg.ey_rol, OffsetManager::DecryptCfg.ey_add);
            LOG("DecryptCfg: fov_xor=0x%X fov_add=0x%X fov_rol=0x%X fov_sub=0x%X", OffsetManager::DecryptCfg.fov_xor, OffsetManager::DecryptCfg.fov_add, OffsetManager::DecryptCfg.fov_rol, OffsetManager::DecryptCfg.fov_sub);
            LOG("Offsets: bn_ptr=0x%I64X cam_ptr=0x%I64X wrapper=0x%I64X entity=0x%I64X",
                offsets::basenetworkable_pointer, offsets::camera_pointer,
                offsets::BaseNetworkable::wrapper_class, offsets::BaseNetworkable::entity);

            LOG("Init complete, starting cache + overlay...");
            ConsolePrint("Starting... console will hide.\n");
            Sleep(1000);

            ShowWindow(GetConsoleWindow(), SW_HIDE);

        // Self-delete: ONLY for production builds — debug keeps all files for analysis
        {
            char selfPath[MAX_PATH] = {};
            HMODULE selfModule = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&selfPath, &selfModule);
            GetModuleFileNameA(selfModule, selfPath, sizeof(selfPath));

            // Check if this is a debug DLL
            std::string dllCheck(selfPath);
            for (char& c : dllCheck) c = (char)tolower((unsigned char)c);
            bool isDebug = dllCheck.find("_debug") != std::string::npos;

            if (!isDebug) {
                // Production: spawn hidden batch that removes DLL + trace files after unloads
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

        StartCacheWorker("initial");
        StartFastRefreshWorker("initial");
        StartSkeletonWorker("initial");
        StartCacheWatchdog();

        // Centralized process-death watchdog — checks IsProcessAlive every 100ms.
        // This is the FASTEST detection path. When Rust exits, this thread
        // immediately blocks all IOCTLs (g_process_dead + ioctl_blocked) to
        // prevent worker threads from sending IOCTLs with stale CR3 → BSOD.
        std::thread([]() {
            __try {
                while (!MISC::ShutdownRequested) {
                    Sleep(100);
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

        Overlay* Draw = new Overlay;
        Draw->DrawOverlay();
        delete Draw;

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

        // 5. Delete trace files — ONLY for production builds. Debug keeps everything.
        if (!g_UpdateLoggingEnabled) {
            std::string dllDir = RuntimePaths::DllDirectory();
            auto delFile = [&](const char* filename) {
                std::string p = dllDir + filename;
                DeleteFileA(p.c_str());
            };
            delFile("rust_decrypts.dat");
            delFile("cheat_dll_loaded.txt");
            delFile("rust_debug_enabled.txt");
            delFile("spoofer_debug.log");
            delFile("spoof_seed.dat");
            // NOTE: Do NOT delete config files (rustcfg*.dat) — user settings must persist
            delFile("rust_mesh.tri");
            delFile("cheat_debug.log");
            delFile("cheat_debug_bomza.log");
            delFile("cheat_debug_bc.log");
            LOG("Production cleanup: trace files deleted (configs preserved)");
        } else {
            LOG("DEBUG build: no trace files deleted — all preserved for analysis");
        }

        LOG("Shutdown complete — DLL unloading");
        timeEndPeriod(1);

        // 6. Unload the DLL from the host process
        // The self-delete batch file (already running) will delete the DLL file
        // once it's no longer loaded
        HMODULE hSelf = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&MainThread, &hSelf);
        if (hSelf) {
            FreeLibraryAndExitThread(hSelf, 0);
        }
        return; // fallback if FreeLibraryAndExitThread fails
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
