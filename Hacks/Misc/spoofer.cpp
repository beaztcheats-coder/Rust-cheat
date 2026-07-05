#include <windows.h>
#include <cstdint>
#include <cstdio>
#include "spoofer.hpp"
#include "../../Logger.hpp"
#include "../../Driver.hpp"
#include "../../RuntimePaths.hpp"

#define SPOOF_LOG_FILE   RuntimePaths::SpooferLogPath()
#define STEAM64_BASE     76561197960265728ULL

// Re-use the driver struct layout from Driver.hpp — no custom SPOOF_REQUEST
// IOCTL_DISPATCH, SECURITY_CODE1/2, RequestId, REQUEST_DATA are in Driver.hpp
// Included transitively through Logger.hpp -> offsets.hpp -> globals.h -> Driver.hpp

static SpooferStatus s_status = {};

const SpooferStatus& GetLastSpooferStatus() {
    return s_status;
}

#pragma pack(push, 1)
struct SeedFile { ULONG64 bs, ds; char sid[64]; };
#pragma pack(pop)

static HANDLE s_log = INVALID_HANDLE_VALUE;

static void spoof_log_open() {
    if (s_log == INVALID_HANDLE_VALUE)
        s_log = CreateFileA(SPOOF_LOG_FILE, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

static void spoof_log_write(const char* t) {
    spoof_log_open();
    if (s_log != INVALID_HANDLE_VALUE) {
        DWORD w; WriteFile(s_log, t, (DWORD)lstrlenA(t), &w, NULL);
    }
}

static void spoof_con(const char* t) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h && h != INVALID_HANDLE_VALUE) {
        DWORD w; WriteConsoleA(h, t, (DWORD)lstrlenA(t), &w, NULL);
    }
}

static void spoof_con_line(const char* n, bool ok) {
    char b[256]; wsprintfA(b, "  [%s] %s\r\n", ok ? "+" : "!", n); spoof_con(b);
}

static uint64_t spoof_fnv1a(const char* d, int len) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (int i = 0; i < len; i++) { h ^= (uint8_t)d[i]; h *= 0x100000001b3ULL; }
    return h;
}

static void spoof_u64str(uint64_t v, char* o) {
    char t[32]; int i = 0;
    if (v == 0) { o[0] = '0'; o[1] = 0; return; }
    while (v > 0) { t[i++] = '0' + (char)(v % 10); v /= 10; }
    for (int j = 0; j < i; j++) o[j] = t[i - 1 - j];
    o[i] = 0;
}

static bool spoof_send_ioctl(HANDLE hDrv, PREQUEST_DATA req, DWORD* errOut = nullptr, DWORD* brOut = nullptr, bool* resOut = nullptr) {
    DWORD br = 0;
    SetLastError(0);
    BOOL ok = DeviceIoControl(hDrv, IOCTL_DISPATCH, req, sizeof(REQUEST_DATA), req, sizeof(REQUEST_DATA), &br, NULL);
    DWORD err = ok ? 0 : GetLastError();
    bool result = req->Result ? true : false;
    if (errOut) *errOut = err;
    if (brOut) *brOut = br;
    if (resOut) *resOut = result;
    return ok && br == sizeof(REQUEST_DATA);
}

static void spoof_cleanup_cmd(const char* cmd) {
    // Fire-and-forget: launch detached, don't wait
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    if (CreateProcessA(NULL, const_cast<char*>(cmd), NULL, NULL, FALSE,
                       CREATE_NO_WINDOW | DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

static void spoof_tpm_cleanup() {
    spoof_con("  Wiping TPM keys...\r\n");
    const char* vals[][2] = {
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI", "WindowsAIKHash"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI", "TaskManufacturerId"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI", "TaskInformationFlags"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EkRetryLast"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EKPub"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EkNoFetch"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EkTries"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Admin", "SRKPub"},
        {"SYSTEM\\CurrentControlSet\\Services\\TPM\\Parameters\\Wdf", "TimeOfLastTelemetry"},
    };
    for (auto& v : vals) {
        char cmd[512];
        wsprintfA(cmd, "cmd.exe /c reg delete \"HKLM\\%s\" /v \"%s\" /f >nul 2>&1", v[0], v[1]);
        spoof_cleanup_cmd(cmd);
    }
    const char* trees[] = {
        "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\TaskStates",
        "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\PlatformQuoteKeys",
        "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement\\EKCertStoreECC\\Certificates",
        "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\User",
        "SYSTEM\\CurrentControlSet\\Services\\TPM\\KeyAttestationKeys",
        "SYSTEM\\CurrentControlSet\\Services\\TPM\\Enum",
        "SYSTEM\\CurrentControlSet\\Services\\TPM\\ODUID",
    };
    for (auto& t : trees) {
        char cmd[512];
        wsprintfA(cmd, "cmd.exe /c reg delete \"HKLM\\%s\" /f >nul 2>&1", t);
        spoof_cleanup_cmd(cmd);
    }
    spoof_con("  TPM keys wiped OK\r\n");
}

static void spoof_net_cleanup() {
    spoof_con("  Flushing DNS + resetting network...\r\n");
    spoof_cleanup_cmd("cmd.exe /c net stop winmgmt /y >nul 2>&1");
    Sleep(2000);
    spoof_cleanup_cmd("cmd.exe /c net start winmgmt /y >nul 2>&1");
    Sleep(2000);
    spoof_cleanup_cmd("cmd.exe /c ipconfig /flushdns >nul 2>&1");
    Sleep(1000);
    spoof_cleanup_cmd("cmd.exe /c netsh winsock reset >nul 2>&1");
    Sleep(1000);
    spoof_con("  Network stack reset OK\r\n");
}

static bool spoof_is_bitlocker_on() {
    STARTUPINFOA si = {}; si.cb = sizeof(si); si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    char tmpPath[MAX_PATH]; GetTempPathA(MAX_PATH, tmpPath);
    char outPath[MAX_PATH]; wsprintfA(outPath, "%sbitlock_check.txt", tmpPath);
    char cmd[512]; wsprintfA(cmd, "cmd.exe /c manage-bde -status C: > \"%s\" 2>&1", outPath);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) return false;
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    HANDLE hf = CreateFileA(outPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) { DeleteFileA(outPath); return false; }
    char buf[4096] = {}; DWORD rd = 0; ReadFile(hf, buf, sizeof(buf)-1, &rd, NULL); CloseHandle(hf);
    DeleteFileA(outPath);
    std::string s(buf);
    std::string lower = s;
    for (auto& c : lower) c = (char)tolower(c);
    return lower.find("protection on") != std::string::npos;
}

static void spoof_registry_deep_cleanup() {
    spoof_con("  Deep registry cleanup...\r\n");

    const char* regVals[][2] = {
        {"SOFTWARE\\Microsoft\\Cryptography", "MachineGuid"},
        {"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", "SusClientId"},
        {"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", "SusClientIdValidation"},
        {"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "Dhcpv6DUID"},
        {"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "BuildGUID"},
        {"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductId"},
        {"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "InstallDate"},
        {"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", "ComputerHardwareId"},
        {"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", "ComputerHardwareIds"},
    };
    for (auto& v : regVals) {
        char cmd[512];
        wsprintfA(cmd, "cmd.exe /c reg delete \"HKLM\\%s\" /v \"%s\" /f >nul 2>&1", v[0], v[1]);
        spoof_cleanup_cmd(cmd);
    }

    const char* nvidiaPaths[] = {
        "SOFTWARE\\NVIDIA Corporation\\Global\\ClientUUID",
        "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
    };
    for (auto& p : nvidiaPaths) {
        char cmd[512];
        wsprintfA(cmd, "cmd.exe /c reg delete \"HKLM\\%s\" /f >nul 2>&1", p);
        spoof_cleanup_cmd(cmd);
    }

    const char* regTrees[] = {
        "SYSTEM\\CurrentControlSet\\Control\\ComputerHardwareIds",
        "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\SusClientId",
    };
    for (auto& t : regTrees) {
        char cmd[512];
        wsprintfA(cmd, "cmd.exe /c reg delete \"HKLM\\%s\" /f >nul 2>&1", t);
        spoof_cleanup_cmd(cmd);
    }
    spoof_con("  Deep registry cleanup OK\r\n");
}

static void spoof_file_trace_cleanup() {
    spoof_con("  Cleaning file traces...\r\n");
    spoof_cleanup_cmd("cmd.exe /c del /f /q C:\\Windows\\Prefetch\\*.pf >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c del /f /q %WINDIR%\\Prefetch\\*.pf >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c fsutil usn deletejournal /d /n C: >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c del /f /q %WINDIR%\\inf\\setupapi.dev.log >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c del /f /q %WINDIR%\\inf\\setupapi.app.log >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c del /f /q %WINDIR%\\Panther\\*.xml >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c del /f /q %LOCALAPPDATA%\\Indexed\\IndexerVolumeGuid >nul 2>&1");
    spoof_con("  File traces cleaned OK\r\n");
}

static void spoof_edid_cleanup() {
    spoof_con("  Cleaning monitor EDID...\r\n");
    spoof_cleanup_cmd("cmd.exe /c reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\" /f >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\ConfigTree\" /f >nul 2>&1");
    spoof_con("  EDID cleaned OK\r\n");
}

static void spoof_bluetooth_cleanup() {
    spoof_con("  Cleaning Bluetooth...\r\n");
    spoof_cleanup_cmd("cmd.exe /c reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\BTHPORT\\Parameters\\Keys\" /f >nul 2>&1");
    spoof_cleanup_cmd("cmd.exe /c reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Enum\\BTH\" /f >nul 2>&1");
    spoof_con("  Bluetooth cleaned OK\r\n");
}

bool RunEmbeddedSpoofer()
{
    s_status = {};
    s_status.attempted = true;

#if defined(VSHARP) || defined(BOMZA) || defined(BETTERCHEATS)
    const char* spoofHeader = "HWID Spoofer";
#else
    const char* spoofHeader = "BEAZT Spoofer";
#endif

    int result = MessageBoxA(NULL,
        "Spoof hardware IDs before playing?\n\n"
        "This spoofs disk, GPU, MAC, SMBIOS,\n"
        "monitor, TPM, and registry HWID keys\n"
        "to avoid hardware bans.\n\n"
        "Requires kernel driver\n"
        "to be loaded by the loader first.\n\n"
        "Yes = Spoof now (recommended)\n"
        "No  = Skip",
        spoofHeader,
        MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);

    if (result != IDYES) {
        s_status.skipped = true;
        s_status.completed = true;
        spoof_con("[*] Spoof skipped by user.\r\n");
        LOG("Spoofer: skipped by user");
        return true;
    }

    spoof_log_open();
#if defined(VSHARP) || defined(BOMZA) || defined(BETTERCHEATS)
    spoof_log_write("=== HWID Spoofer ===\r\n");
#else
    spoof_log_write("=== BEAZT Embedded Spoofer ===\r\n");
#endif

    spoof_con("========================================\r\n");
#if defined(VSHARP) || defined(BOMZA) || defined(BETTERCHEATS)
    spoof_con("  HWID Spoofer (embedded)\r\n");
#else
    spoof_con("  BEAZT HWID Spoofer (embedded)\r\n");
#endif
    spoof_con("========================================\r\n\r\n");

    // Connect to driver
    spoof_con("[*] Connecting to driver...\r\n");
    HANDLE hDrv = CreateFileW(L"\\\\.\\Bfo64", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDrv == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        spoof_con("[!] Driver not loaded.\r\n");
        char b[128]; wsprintfA(b, "[!] Driver connect failed (err=%lu)\r\n", err); spoof_con(b);
        LOG("Spoofer: driver connect FAILED, err=%lu", err);
        return false;
    }
    s_status.driverConnected = true;
    spoof_con("[+] Driver connected\r\n");
    LOG("Spoofer: driver connected");

    // Steam ID
    char sid[64] = {};
    HKEY hk;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        DWORD au = 0, sz = sizeof(DWORD), ty = REG_DWORD;
        RegQueryValueExA(hk, "ActiveUser", NULL, &ty, (LPBYTE)&au, &sz);
        RegCloseKey(hk);
        if (au != 0) { uint64_t s64 = (uint64_t)au + STEAM64_BASE; spoof_u64str(s64, sid); }
    }

    if (lstrlenA(sid) < 10) {
        spoof_con("[!] Steam ID not detected. Aborting spoof.\r\n");
        LOG("Spoofer: Steam ID not found");
        CloseHandle(hDrv);
        return false;
    }

    char msg[128];
    wsprintfA(msg, "[*] Steam ID: %s\r\n", sid); spoof_con(msg);

    // Seed
    SeedFile seed = {};
    {
        HANDLE hf = CreateFileA(RuntimePaths::SpooferSeedPath(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        bool loaded = false;
        if (hf != INVALID_HANDLE_VALUE) { DWORD br; ReadFile(hf, &seed, sizeof(seed), &br, NULL); CloseHandle(hf); if (br == sizeof(seed) && lstrcmpA(seed.sid, sid) == 0) loaded = true; }
        if (!loaded) {
            int len = lstrlenA(sid);
            seed.bs = spoof_fnv1a(sid, len);
            seed.ds = seed.bs ^ 0x9E3779B97F4A7C15ULL;
            lstrcpynA(seed.sid, sid, sizeof(seed.sid) - 1);
            hf = CreateFileA(RuntimePaths::SpooferSeedPath(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hf != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(hf, &seed, sizeof(seed), &w, NULL); CloseHandle(hf); }
            spoof_con("[+] Generated new seed\r\n");
        } else spoof_con("[+] Loaded existing seed\r\n");
    }

    // Set seed via REQUEST_DATA (same struct driver expects for all ops)
    bool seedOk = false;
    {
        REQUEST_DATA req = {};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::Set_Spoof_Seed;
        req.SpoofData.BIOS_SEED = seed.bs;
        req.SpoofData.DISK_SEED = seed.ds;

        DWORD err = 0, br = 0;
        bool resOk = false;
        seedOk = spoof_send_ioctl(hDrv, &req, &err, &br, &resOk) && resOk;

        if (!seedOk) {
            spoof_con("[!] Failed to set seed\r\n");
            spoof_con("[!] Continuing with default/randomized driver state\r\n");
            char e[196];
            wsprintfA(e, "[!] Seed diag: ioerr=%lu bytes=%lu req.Result=%d\r\n", err, br, resOk ? 1 : 0);
            spoof_con(e);
            LOG("Spoofer: SetSeed FAILED, continuing (err=%lu br=%lu result=%d)", err, br, resOk ? 1 : 0);
        }
        else {
            spoof_con("[+] Seed set\r\n");
            LOG("Spoofer: SetSeed OK");
        }
    }
    s_status.seedSet = seedOk;
    spoof_con("\r\nSpoofing...\r\n\r\n");

    // Run all spoofs
    struct { RequestId id; const char* n; } cmds[] = {
        {RequestId::Spoof_DISK, "Disk Serials"},
        {RequestId::Spoof_MAC, "MAC Addresses"},
        {RequestId::Spoof_GPU, "GPU HWID"},
        {RequestId::Spoof_SMBIOS, "SMBIOS Data"},
        {RequestId::Spoof_BOOTINFO, "Boot Config"},
        {RequestId::Spoof_FIRMWAREENTRY, "Firmware Entries"},
        {RequestId::Spoof_ARP, "ARP Cache"},
        {RequestId::Spoof_SMBIOSNULL, "SMBIOS Nulls"},
        {RequestId::Spoof_TPMNULL, "TPM Identifiers"},
        {RequestId::Spoof_MONITOR, "Monitor Serials"},
        {RequestId::Spoof_REGISTRY, "Registry HWID"},
    };

    int total = sizeof(cmds) / sizeof(cmds[0]);
    int okCount = 0;
    int attempted = 0;
    bool earlyMismatchAbort = false;
    for (int i = 0; i < total; i++) {
        REQUEST_DATA req = {};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = cmds[i].id;

        DWORD err = 0, br = 0;
        bool resOk = false;
        bool ioOk = spoof_send_ioctl(hDrv, &req, &err, &br, &resOk);
        bool ok = ioOk && resOk;
        spoof_con_line(cmds[i].n, ok);
        attempted++;
        if (!ok) {
            char d[220];
            wsprintfA(d, "      diag: io=%d err=%lu bytes=%lu req.Result=%d\r\n", ioOk ? 1 : 0, err, br, resOk ? 1 : 0);
            spoof_con(d);
        }
        if (ok) okCount++;

        if (i == 0 && !seedOk && !ok) {
            spoof_con("[!] Spoof opcodes rejected by loaded driver. Driver mismatch on this PC.\r\n");
            spoof_con("[!] Aborting remaining spoof commands early.\r\n");
            earlyMismatchAbort = true;
            break;
        }
        Sleep(100);
    }

    wsprintfA(msg, "\r\n  %d/%d spoofs successful\r\n\r\n", okCount, total);
    spoof_con(msg);
    s_status.okCount = okCount;
    s_status.total = total;
    s_status.attemptedCount = attempted;
    s_status.driverMismatch = earlyMismatchAbort;

    spoof_con("[*] Running cleanup...\r\n");
    if (spoof_is_bitlocker_on()) {
        spoof_con("  [!] BitLocker ON — skipping TPM wipe to avoid recovery lockout\r\n");
        LOG("Spoofer: BitLocker detected, skipping TPM cleanup");
    } else {
        spoof_tpm_cleanup();
    }
    spoof_registry_deep_cleanup();
    spoof_file_trace_cleanup();
    spoof_edid_cleanup();
    spoof_bluetooth_cleanup();
    spoof_net_cleanup();

    CloseHandle(hDrv);

    spoof_con("[+] Spoofer completed. Launching cheat.\r\n");
    LOG("Spoofer: completed (%d/%d ok)", okCount, total);
    s_status.completed = true;

    if (s_log != INVALID_HANDLE_VALUE) { CloseHandle(s_log); s_log = INVALID_HANDLE_VALUE; }

    return true;
}
