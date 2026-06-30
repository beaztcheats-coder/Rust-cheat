#include <windows.h>
#include <cstdio>
#include <conio.h>
#include "spoofer.hpp"
#include "cleaner.hpp"

static void SafeCLS() {
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

static void PrintMenu() {
    SafeCLS();
    printf("\n");
    printf("  ============================================\n");
    printf("         HWID Spoofer & Cleaner\n");
    printf("  ============================================\n\n");
    printf("  Select option:\n\n");
    printf("    [1]  Spoof HWID + Run Cheat\n");
    printf("         Spoof disk, GPU, MAC, SMBIOS, monitor,\n");
    printf("         TPM, and registry HWID keys\n\n");
    printf("    [2]  Clean Traces + Run Cheat\n");
    printf("         Deep clean traces, anti-cheat remnants,\n");
    printf("         registry, network reset, MAC spoof\n\n");
    printf("    [3]  Run Cheat (No Spoof/Clean)\n");
    printf("         Skip directly to cheat injection\n\n");
    printf("  ============================================\n");
    printf("  Choice [1-3]: ");
    fflush(stdout);
}

static DWORD WINAPI WorkerThread(LPVOID) {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    SetConsoleTitleA("Spoofer & Cleaner");
    HWND conWnd = GetConsoleWindow();
    if (conWnd) {
        SetWindowPos(conWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    }

    PrintMenu();

    int choice = _getch();
    printf("%c\n\n", choice);
    fflush(stdout);

    switch (choice) {
        case '1': {
            printf("[*] Starting HWID Spoof...\n\n");
            fflush(stdout);
            bool ok = RunSpoofer();
            const SpooferStatus& st = GetLastSpooferStatus();
            if (st.driverConnected && st.total > 0) {
                char sb[180];
                wsprintfA(sb, "\nSpoof result: %d/%d successful (%d attempted, seed: %s)\n",
                    st.okCount, st.total, st.attemptedCount, st.seedSet ? "set" : "failed");
                printf("%s", sb);
                fflush(stdout);
                if (st.driverMismatch) {
                    printf("Note: driver mismatch suspected on this machine.\n");
                    fflush(stdout);
                }
            }
            if (!ok) {
                printf("\n[!] Spoof failed. Check driver.\n");
            }
            printf("\n[+] Done. Loader can now inject the cheat.\n");
            printf("Press any key to exit...\n");
            fflush(stdout);
            _getch();
            break;
        }
        case '2': {
            printf("[*] Starting Deep Cleaner...\n\n");
            fflush(stdout);
            CleanerMenu();
            printf("\n[+] Done. Loader can now inject the cheat.\n");
            printf("Press any key to exit...\n");
            fflush(stdout);
            _getch();
            break;
        }
        case '3': {
            printf("[*] Skipping spoof/clean.\n");
            printf("[+] Loader can now inject the cheat.\n");
            printf("Press any key to exit...\n");
            fflush(stdout);
            _getch();
            break;
        }
        default: {
            printf("  Invalid choice. Exiting.\n");
            fflush(stdout);
            Sleep(1000);
            break;
        }
    }

    FreeConsole();
    FreeLibraryAndExitThread(GetModuleHandleA(NULL), 0);
    return 0;
}

extern "C" __declspec(dllexport) void RunSpooferCleaner() {
    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    if (hThread) CloseHandle(hThread);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}
