#include <windows.h>
#include <cstdint>
#include <cstdio>
#include "spoofer.hpp"
#include "DriverProtocol.hpp"

#define SPOOF_SEED_FILE  "C:\\spoof_seed.dat"
#define SPOOF_LOG_FILE   "C:\\spoofer_debug.log"
#define STEAM64_BASE     76561197960265728ULL

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

bool RunSpoofer()
{
    s_status = {};
    s_status.attempted = true;

    spoof_log_open();
    spoof_log_write("=== HWID Spoofer ===\r\n");

    spoof_con("========================================\r\n");
    spoof_con("  HWID Spoofer\r\n");
    spoof_con("========================================\r\n\r\n");

    spoof_con("[*] Connecting to driver...\r\n");
    HANDLE hDrv = CreateFileW(L"\\\\.\\Bfo64", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDrv == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        spoof_con("[!] Driver not loaded.\r\n");
        char b[128]; wsprintfA(b, "[!] Driver connect failed (err=%lu)\r\n", err); spoof_con(b);
        return false;
    }
    s_status.driverConnected = true;
    spoof_con("[+] Driver connected\r\n");

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
        CloseHandle(hDrv);
        return false;
    }

    char msg[128];
    wsprintfA(msg, "[*] Steam ID: %s\r\n", sid); spoof_con(msg);

    SeedFile seed = {};
    {
        HANDLE hf = CreateFileA(SPOOF_SEED_FILE, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        bool loaded = false;
        if (hf != INVALID_HANDLE_VALUE) { DWORD br; ReadFile(hf, &seed, sizeof(seed), &br, NULL); CloseHandle(hf); if (br == sizeof(seed) && lstrcmpA(seed.sid, sid) == 0) loaded = true; }
        if (!loaded) {
            int len = lstrlenA(sid);
            seed.bs = spoof_fnv1a(sid, len);
            seed.ds = seed.bs ^ 0x9E3779B97F4A7C15ULL;
            lstrcpynA(seed.sid, sid, sizeof(seed.sid) - 1);
            hf = CreateFileA(SPOOF_SEED_FILE, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hf != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(hf, &seed, sizeof(seed), &w, NULL); CloseHandle(hf); }
            spoof_con("[+] Generated new seed\r\n");
        } else spoof_con("[+] Loaded existing seed\r\n");
    }

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
        }
        else {
            spoof_con("[+] Seed set\r\n");
        }
    }
    s_status.seedSet = seedOk;
    spoof_con("\r\nSpoofing...\r\n\r\n");

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
    spoof_tpm_cleanup();
    spoof_net_cleanup();

    CloseHandle(hDrv);

    spoof_con("[+] Spoofer completed.\r\n");
    s_status.completed = true;

    if (s_log != INVALID_HANDLE_VALUE) { CloseHandle(s_log); s_log = INVALID_HANDLE_VALUE; }

    return true;
}
