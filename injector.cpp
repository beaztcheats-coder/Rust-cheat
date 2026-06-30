#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstring>

// Returns: 0=cancel, 1=bomza, 2=beazt, 3=debug
static int pick_variant() {
    int pick = MessageBoxA(nullptr,
        "Rust Launcher\n\nChoose DLL variant to inject:\n\n"
        "  Yes    = bomzarust.dll (Bomza production)\n"
        "  No     = Rust Prv Ext.dll (BeaZt production)\n"
        "  Cancel = Abort",
        "Rust", MB_ICONQUESTION | MB_YESNOCANCEL | MB_TOPMOST);
    if (pick == IDYES) return 1;
    if (pick == IDNO) return 2;
    return 0;
}

static bool ask_spoof() {
    int pick = MessageBoxA(nullptr,
        "Rust Launcher\n\nRun HWID Spoofer before injecting?\n\n  Yes = Spoof hardware IDs first (recommended)\n  No  = Skip spoofing, inject only",
        "Rust", MB_ICONQUESTION | MB_YESNO | MB_TOPMOST);
    return pick == IDYES;
}

static DWORD find_process(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do { if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; } }
        while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static bool run_spoofer(const char* exeDir) {
    char spooferPath[MAX_PATH];
    lstrcpynA(spooferPath, exeDir, MAX_PATH);
    strcat_s(spooferPath, "\\spoofer.exe");

    if (GetFileAttributesA(spooferPath) == INVALID_FILE_ATTRIBUTES) {
        printf("Spoofer not found: %s\n", spooferPath);
        return false;
    }

    printf("Running HWID Spoofer...\n");

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessA(spooferPath, nullptr, nullptr, nullptr, FALSE,
        CREATE_NEW_CONSOLE, nullptr, exeDir, &si, &pi);

    if (!ok) {
        printf("Failed to launch spoofer (err=%lu)\n", GetLastError());
        return false;
    }

    printf("Waiting for spoofer to complete...\n");
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("Spoofer finished (code=%lu)\n", exitCode);
    return exitCode == 0;
}

int main() {
    printf("========================================\n");
    printf("         RUST INJECTOR\n");
    printf("========================================\n\n");

    // Get base directory
    char baseDir[MAX_PATH];
    GetModuleFileNameA(nullptr, baseDir, sizeof(baseDir));
    char* lastSlash = strrchr(baseDir, '\\');
    if (lastSlash) *lastSlash = '\0';

    // Step 1: Spoof prompt
    bool doSpoof = true;
    if (__argc > 1 && _stricmp(__argv[1], "nospoof") == 0) doSpoof = false;
    if (doSpoof && __argc <= 1) doSpoof = ask_spoof();

    if (doSpoof) {
        if (!run_spoofer(baseDir)) {
            printf("WARNING: Spoofer failed or not found.\n");
            printf("Continue with injection anyway? Press any key...\n");
            system("pause >nul");
        }
    }

    // Step 2: Variant selection
    // Arguments: "debug" = debug DLL (logging on), "bomza" = bomza DLL, "private" = production BeaZt
    bool useDebug = false;
    bool useBomza = false;
    if (__argc > 1) {
        useDebug = (_stricmp(__argv[1], "debug") == 0);
        useBomza = (_stricmp(__argv[1], "bomza") == 0);
    } else {
        int choice = pick_variant();
        if (choice == 0) { printf("Canceled by user.\n"); return 0; }
        useBomza = (choice == 1);
    }

    // Step 3: Find Rust
    printf("\nLooking for RustClient.exe...\n");
    DWORD pid = find_process(L"RustClient.exe");
    if (!pid) {
        printf("RustClient.exe not found! Is Rust running?\n");
        printf("Launch Rust first, then run this injector again.\n");
        system("pause");
        return 1;
    }
    printf("Found RustClient.exe (PID: %u)\n", pid);

    // Step 4: Find DLL — try primary first, then fallbacks
    const char* candidates[3];
    if (useDebug) {
        candidates[0] = "Rust Prv Ext_debug.dll";
        candidates[1] = "Rust Prv Ext.dll";
        candidates[2] = "bomzarust.dll";
    } else if (useBomza) {
        candidates[0] = "bomzarust.dll";
        candidates[1] = "Rust Prv Ext.dll";
        candidates[2] = "Rust Prv Ext_debug.dll";
    } else {
        candidates[0] = "Rust Prv Ext.dll";
        candidates[1] = "bomzarust.dll";
        candidates[2] = "Rust Prv Ext_debug.dll";
    }

    char dllPath[MAX_PATH];
    bool found = false;
    for (int i = 0; i < 3; ++i) {
        lstrcpynA(dllPath, baseDir, MAX_PATH);
        strcat_s(dllPath, "\\");
        strcat_s(dllPath, candidates[i]);
        if (GetFileAttributesA(dllPath) != INVALID_FILE_ATTRIBUTES) {
            found = true;
            break;
        }
    }

    if (!found) {
        printf("DLL not found. Tried:\n");
        printf("  %s\n", candidates[0]);
        printf("  %s\n", candidates[1]);
        printf("  %s\n", candidates[2]);
        system("pause");
        return 1;
    }

    printf("Selected DLL: %s\n\n", dllPath);

    // Step 5: Inject
    HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
    if (!hProc) { printf("OpenProcess failed: %lu\n", GetLastError()); system("pause"); return 1; }

    size_t pathLen = strlen(dllPath) + 1;
    void* remoteMem = VirtualAllocEx(hProc, nullptr, pathLen, MEM_COMMIT, PAGE_READWRITE);
    if (!remoteMem) { printf("VirtualAllocEx failed: %lu\n", GetLastError()); CloseHandle(hProc); system("pause"); return 1; }

    WriteProcessMemory(hProc, remoteMem, dllPath, pathLen, nullptr);
    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        remoteMem, 0, nullptr);
    if (!hThread) { printf("CreateRemoteThread failed: %lu\n", GetLastError()); CloseHandle(hProc); system("pause"); return 1; }

    printf("Injected! Press HOME at main menu, INSERT for menu.\n");
    printf("Close this window to keep playing.\n");
    system("pause >nul");
    return 0;
}
