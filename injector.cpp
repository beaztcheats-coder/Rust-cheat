#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

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

int main() {
    SetConsoleTitleA("Rust Injector");
    HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
    HWND conWnd = GetConsoleWindow();
    if (conWnd) SetWindowPos(conWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    printf("========================================\n");
    printf("         RUST DLL INJECTOR\n");
    printf("========================================\n\n");

    char baseDir[MAX_PATH];
    GetModuleFileNameA(nullptr, baseDir, sizeof(baseDir));
    char* lastSlash = strrchr(baseDir, '\\');
    if (lastSlash) *lastSlash = '\0';

    // Scan for DLL files in the injector's directory
    char searchPattern[MAX_PATH];
    snprintf(searchPattern, sizeof(searchPattern), "%s\\*.dll", baseDir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("No DLL files found in:\n  %s\n", baseDir);
        printf("Place injector.exe next to your cheat DLLs.\n");
        system("pause");
        return 1;
    }

    std::vector<std::string> dlls;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            dlls.push_back(fd.cFileName);
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);

    if (dlls.empty()) {
        printf("No DLL files found in:\n  %s\n", baseDir);
        system("pause");
        return 1;
    }

    // List DLLs
    printf("Available DLLs in %s:\n\n", baseDir);
    for (size_t i = 0; i < dlls.size(); i++) {
        // Add description for known DLLs
        const char* desc = "";
        if (_stricmp(dlls[i].c_str(), "Rust Prv Ext_debug.dll") == 0)
            desc = "  (BeaZt DEBUG - logging enabled)";
        else if (_stricmp(dlls[i].c_str(), "Rust Prv Ext.dll") == 0)
            desc = "  (BeaZt PRODUCTION)";
        else if (_stricmp(dlls[i].c_str(), "bomzarust.dll") == 0)
            desc = "  (Bomza PRODUCTION)";
        else if (_stricmp(dlls[i].c_str(), "bettercheats.dll") == 0)
            desc = "  (Better Cheats PRODUCTION)";
        printf("  [%d] %s%s\n", (int)(i + 1), dlls[i].c_str(), desc);
    }
    printf("\n");

    // Ask user to pick
    int choice = 0;
    printf("Select DLL to inject [1-%d]: ", (int)dlls.size());
    char input[16];
    if (!fgets(input, sizeof(input), stdin)) { system("pause"); return 1; }
    choice = atoi(input);
    if (choice < 1 || choice > (int)dlls.size()) {
        printf("Invalid selection.\n");
        system("pause");
        return 1;
    }

    std::string dllName = dlls[choice - 1];
    char dllPath[MAX_PATH];
    snprintf(dllPath, sizeof(dllPath), "%s\\%s", baseDir, dllName.c_str());

    printf("\nSelected: %s\n", dllName.c_str());

    // Find Rust
    printf("\nLooking for RustClient.exe...\n");
    DWORD pid = find_process(L"RustClient.exe");
    if (!pid) {
        // Try Rust.exe as fallback
        pid = find_process(L"Rust.exe");
    }
    if (!pid) {
        printf("RustClient.exe not found!\n");
        printf("Launch Rust first, get to main menu, then run this injector.\n");
        system("pause");
        return 1;
    }
    printf("Found RustClient.exe (PID: %u)\n\n", pid);

    // Inject
    printf("Injecting %s...\n", dllName.c_str());

    HANDLE hProc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!hProc) {
        printf("OpenProcess failed (error %lu).\n", GetLastError());
        printf("Make sure you're running as Administrator.\n");
        system("pause");
        return 1;
    }

    size_t pathLen = strlen(dllPath) + 1;
    void* remoteMem = VirtualAllocEx(hProc, nullptr, pathLen, MEM_COMMIT, PAGE_READWRITE);
    if (!remoteMem) {
        printf("VirtualAllocEx failed (error %lu).\n", GetLastError());
        CloseHandle(hProc);
        system("pause");
        return 1;
    }

    if (!WriteProcessMemory(hProc, remoteMem, dllPath, pathLen, nullptr)) {
        printf("WriteProcessMemory failed (error %lu).\n", GetLastError());
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        system("pause");
        return 1;
    }

    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        remoteMem, 0, nullptr);
    if (!hThread) {
        printf("CreateRemoteThread failed (error %lu).\n", GetLastError());
        VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        system("pause");
        return 1;
    }

    WaitForSingleObject(hThread, 5000);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hProc);

    printf("\n========================================\n");
    printf("  INJECTED SUCCESSFULLY!\n");
    printf("========================================\n");
    printf("\n");
    printf("  DLL: %s\n", dllName.c_str());
    printf("  PID: %u\n", pid);
    printf("\n");
    printf("  Press HOME at main menu to attach.\n");
    printf("  Press INSERT for cheat menu.\n");
    printf("\n");
    printf("  This window will close in 5 seconds...\n");

    Sleep(5000);
    return 0;
}
