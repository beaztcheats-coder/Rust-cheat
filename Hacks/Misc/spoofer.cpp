#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <iphlpapi.h>
#include "spoofer.hpp"
#include "spoofer_driver_client.h"
#include "offset_database.h"
#include "../../Logger.hpp"
#include "../../Driver.hpp"
#include "../../RuntimePaths.hpp"

#pragma comment(lib, "iphlpapi.lib")

#define SPOOF_LOG_FILE   RuntimePaths::SpooferLogPath()
#define STEAM64_BASE     76561197960265728ULL

static SpooferStatus s_status = {};
static HANDLE s_hDrv = INVALID_HANDLE_VALUE;

const SpooferStatus& GetLastSpooferStatus() {
    return s_status;
}

#pragma pack(push, 1)
struct SeedFile { ULONG64 bs, ds; char sid[64]; };
#pragma pack(pop)

static HANDLE s_log = INVALID_HANDLE_VALUE;
static char s_sid[64] = {};

// ---- Console + log helpers ----

static void s_log_open() {
    if (s_log == INVALID_HANDLE_VALUE)
        s_log = CreateFileA(SPOOF_LOG_FILE, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}
static void s_log_write(const char* t) { s_log_open(); if (s_log != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(s_log, t, (DWORD)lstrlenA(t), &w, NULL); } }
static void s_con(const char* t) { HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE); if (h && h != INVALID_HANDLE_VALUE) { DWORD w; WriteConsoleA(h, t, (DWORD)lstrlenA(t), &w, NULL); } }
static void s_con_line(const char* n, bool ok) { char b[256]; wsprintfA(b, "  [%s] %s\r\n", ok ? "+" : "!", n); s_con(b); }

// ---- Hash helpers ----

static uint64_t s_fnv1a(const char* d, int len) { uint64_t h = 0xcbf29ce484222325ULL; for (int i = 0; i < len; i++) { h ^= (uint8_t)d[i]; h *= 0x100000001b3ULL; } return h; }
static void s_u64str(uint64_t v, char* o) { char t[32]; int i = 0; if (v == 0) { o[0] = '0'; o[1] = 0; return; } while (v > 0) { t[i++] = '0' + (char)(v % 10); v /= 10; } for (int j = 0; j < i; j++) o[j] = t[i - 1 - j]; o[i] = 0; }

// ---- Direct executable runner (no cmd.exe wrapper) ----

static void s_run_exe(const char* cmd) {
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};
    if (CreateProcessA(NULL, const_cast<char*>(cmd), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 15000);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

// ---- Win32 Registry helpers (NO cmd.exe) ----

static void s_reg_delete_value(HKEY root, const char* path, const char* val) {
    HKEY hKey;
    if (RegOpenKeyExA(root, path, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, val);
        RegCloseKey(hKey);
    }
}

static void s_reg_delete_tree(HKEY root, const char* path) {
    RegDeleteTreeA(root, path);
}

// ---- IOCTL sender (wraps DeviceIoControl via the shared driver handle) ----

static bool SendSpoofIoctl(PREQUEST_DATA req) {
    if (s_hDrv == INVALID_HANDLE_VALUE) return false;
    DWORD br = 0;
    SetLastError(0);
    BOOL ok = DeviceIoControl(s_hDrv, IOCTL_DISPATCH, req, sizeof(REQUEST_DATA), req, sizeof(REQUEST_DATA), &br, NULL);
    bool result = req->Result ? true : false;
    return ok && br == sizeof(REQUEST_DATA) && result;
}

// ---- Steam ID ----

static bool s_read_steam_id() {
    HKEY hk;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam\\ActiveProcess", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        DWORD au = 0, sz = sizeof(DWORD), ty = REG_DWORD;
        RegQueryValueExA(hk, "ActiveUser", NULL, &ty, (LPBYTE)&au, &sz);
        RegCloseKey(hk);
        if (au != 0) { uint64_t s64 = (uint64_t)au + STEAM64_BASE; s_u64str(s64, s_sid); }
    }
    if (lstrlenA(s_sid) < 10) {
        s_con("[!] Steam ID not detected. Aborting spoof.\r\n");
        LOG("Spoofer: Steam ID not found");
        return false;
    }
    char msg[128]; wsprintfA(msg, "[*] Steam ID: %s\r\n", s_sid); s_con(msg);
    return true;
}

// ---- Seed file ----

static SeedFile s_seed = {};

static bool s_load_or_generate_seed() {
    HANDLE hf = CreateFileA(RuntimePaths::SpooferSeedPath(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    bool loaded = false;
    if (hf != INVALID_HANDLE_VALUE) { DWORD br; ReadFile(hf, &s_seed, sizeof(s_seed), &br, NULL); CloseHandle(hf); if (br == sizeof(s_seed) && lstrcmpA(s_seed.sid, s_sid) == 0) loaded = true; }
    if (!loaded) {
        int len = lstrlenA(s_sid);
        s_seed.bs = s_fnv1a(s_sid, len);
        s_seed.ds = s_seed.bs ^ 0x9E3779B97F4A7C15ULL;
        lstrcpynA(s_seed.sid, s_sid, sizeof(s_seed.sid) - 1);
        hf = CreateFileA(RuntimePaths::SpooferSeedPath(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hf != INVALID_HANDLE_VALUE) { DWORD w; WriteFile(hf, &s_seed, sizeof(s_seed), &w, NULL); CloseHandle(hf); }
        s_con("[+] Generated new seed\r\n");
    } else s_con("[+] Loaded existing seed\r\n");
    return true;
}

// ---- Seed to driver ----

static bool updateseed_todriver() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Set_Spoof_Seed;
    req.SpoofData.BIOS_SEED = s_seed.bs;
    req.SpoofData.DISK_SEED = s_seed.ds;
    if (SendSpoofIoctl(&req)) {
        s_con("[+] Seed set\r\n");
        LOG("Spoofer: SetSeed OK");
        return true;
    }
    s_con("[!] Failed to set seed (continuing with default state)\r\n");
    LOG("Spoofer: SetSeed FAILED, continuing");
    return false;
}

// ---- Individual spoof functions ----

static bool SpoofDisk() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_DISK;
    return SendSpoofIoctl(&req);
}

static bool SpoofMAC() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_MAC;
    return SendSpoofIoctl(&req);
}

static bool SpoofGPU() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_GPU;
    return SendSpoofIoctl(&req);
}

static bool SpoofSMBIOS() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_SMBIOS;
    return SendSpoofIoctl(&req);
}

static bool SpoofARP() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_ARP;
    return SendSpoofIoctl(&req);
}

static bool SpoofSMBIOSNULL() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_SMBIOSNULL;
    return SendSpoofIoctl(&req);
}

static bool SpoofTPM() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_TPMNULL;
    return SendSpoofIoctl(&req);
}

static bool SpoofMonitor() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_MONITOR;
    return SendSpoofIoctl(&req);
}

static bool SpoofRegistry() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_REGISTRY;
    return SendSpoofIoctl(&req);
}

static bool SpoofBootInfo() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_BOOTINFO;
    return SendSpoofIoctl(&req);
}

static bool SpoofFirmwareEntry() {
    REQUEST_DATA req{};
    req.SecurityCode1 = SECURITY_CODE1;
    req.SecurityCode2 = SECURITY_CODE2;
    req.Command = RequestId::Spoof_FIRMWAREENTRY;
    return SendSpoofIoctl(&req);
}

// ---- BitLocker check (Win32 API, no cmd.exe) ----

static bool s_is_bitlocker_on() {
    SECURITY_ATTRIBUTES sa = {};
    sa.bInheritHandle = TRUE;
    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return false;
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    char cmd[] = "manage-bde.exe -status C:";
    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe);
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    char buf[4096] = {};
    DWORD rd = 0;
    ReadFile(hReadPipe, buf, sizeof(buf) - 1, &rd, NULL);
    CloseHandle(hReadPipe);

    std::string s(buf);
    for (auto& c : s) c = (char)tolower(c);
    return s.find("protection on") != std::string::npos;
}

// ---- HVCI detection ----

static bool s_is_hvci_on() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD enabled = 0, sz = sizeof(DWORD);
        RegQueryValueExA(hKey, "Enabled", NULL, NULL, (LPBYTE)&enabled, &sz);
        RegCloseKey(hKey);
        if (enabled != 0) return true;
    }
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\DeviceGuard",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD vbs = 0, sz = sizeof(DWORD);
        RegQueryValueExA(hKey, "EnableVirtualizationBasedSecurity", NULL, NULL, (LPBYTE)&vbs, &sz);
        RegCloseKey(hKey);
        if (vbs != 0) return true;
    }
    return false;
}

// ---- Windows version detection ----

static bool s_is_win10() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buildStr[64] = {};
        DWORD sz = sizeof(buildStr) - 1;
        RegQueryValueExA(hKey, "CurrentBuild", NULL, NULL, (LPBYTE)buildStr, &sz);
        RegCloseKey(hKey);
        int build = atoi(buildStr);
        return build > 0 && build < 22000;
    }
    return false;
}

// ---- MAC registry fallback (Win32 API, physical adapters only) ----

static void spoof_mac_registry_fallback() {
    s_con("  MAC registry fallback (physical adapters only)...\r\n");
    const char* netClassPath = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}";
    HKEY hClass;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, netClassPath, 0, KEY_READ, &hClass) != ERROR_SUCCESS) {
        s_con("  [!] Could not open network class registry\r\n");
        return;
    }
    int spoofed = 0;
    for (int idx = 0; idx < 500; idx++) {
        char subKeyName[256];
        DWORD subNameLen = sizeof(subKeyName);
        if (RegEnumKeyA(hClass, idx, subKeyName, subNameLen) != ERROR_SUCCESS) break;

        HKEY hAdapter;
        if (RegOpenKeyExA(hClass, subKeyName, 0, KEY_READ | KEY_SET_VALUE, &hAdapter) != ERROR_SUCCESS) continue;

        char driverDesc[256] = {};
        DWORD descSz = sizeof(driverDesc) - 1;
        RegQueryValueExA(hAdapter, "DriverDesc", NULL, NULL, (LPBYTE)driverDesc, &descSz);

        if (!driverDesc[0]) { RegCloseKey(hAdapter); continue; }

        std::string desc(driverDesc);
        for (auto& c : desc) c = (char)tolower(c);

        if (desc.find("bluetooth") != std::string::npos ||
            desc.find("virtual") != std::string::npos ||
            desc.find("wan") != std::string::npos ||
            desc.find("tap") != std::string::npos ||
            desc.find("tunnel") != std::string::npos ||
            desc.find("ras") != std::string::npos ||
            desc.find("vpn") != std::string::npos ||
            desc.find("hyper-v") != std::string::npos ||
            desc.find("loopback") != std::string::npos ||
            desc.find("ndis") != std::string::npos ||
            desc.find("miniport") != std::string::npos) {
            RegCloseKey(hAdapter);
            continue;
        }

        char newMac[16];
        wsprintfA(newMac, "%02X%02X%02X%02X%02X%02X",
            0x02, (rand() % 256), (rand() % 256),
            (rand() % 256), (rand() % 256), (rand() % 256));

        if (RegSetValueExA(hAdapter, "NetworkAddress", 0, REG_SZ, (const BYTE*)newMac, (DWORD)(lstrlenA(newMac) + 1)) == ERROR_SUCCESS) {
            s_con("  [+] ");
            s_con(driverDesc);
            s_con(" -> ");
            s_con(newMac);
            s_con("\r\n");
            spoofed++;
        }
        RegCloseKey(hAdapter);
    }
    RegCloseKey(hClass);
    if (spoofed == 0) s_con("  [!] No physical adapters found\r\n");
    else {
        char msg[64];
        wsprintfA(msg, "  %d adapter(s) spoofed OK\r\n", spoofed);
        s_con(msg);
    }
}

// ---- Minimal temp cleanup (Win32 API only, auto-regenerating keys) ----

static void spoof_temp_cleanup() {
    s_con("  Temp cleanup (auto-regenerating keys only)...\r\n");

    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\MountedDevices");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\mssmbios\\Data");

    s_reg_delete_tree(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2");
    s_reg_delete_tree(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\BitBucket\\Volume");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Dfrg\\Statistics");

    s_con("  Temp cleanup OK\r\n");
}

static void spoof_tpm_cleanup() {
    s_con("  Wiping TPM keys...\r\n");

    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI", "WindowsAIKHash");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI", "TaskManufacturerId");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI", "TaskInformationFlags");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EkRetryLast");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EKPub");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EkNoFetch");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement", "EkTries");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Admin", "SRKPub");
    s_reg_delete_value(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\Parameters\\Wdf", "TimeOfLastTelemetry");

    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\TaskStates");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\PlatformQuoteKeys");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\Endorsement\\EKCertStoreECC\\Certificates");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI\\User");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\KeyAttestationKeys");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\Enum");
    s_reg_delete_tree(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\TPM\\ODUID");

    s_con("  TPM keys wiped OK\r\n");
}

// ---- Registry HWID spoofing (MachineGuid, ProductId, SusClientId) ----

static void spoof_registry_hwid() {
    s_con("  Spoofing registry HWID identifiers...\r\n");

    // 1. MachineGuid — EAC's #1 registry identifier
    //    HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid
    {
        // Generate random GUID manually (no ole32 dependency)
        char guidStr[40];
        wsprintfA(guidStr, "{%08X-%04X-%04X-%04X-%012X}",
            (unsigned)(rand() << 16 | rand()),
            (unsigned)(rand() & 0xFFFF),
            (unsigned)((rand() & 0x0FFF) | 0x4000),  // version 4
            (unsigned)((rand() & 0x3FFF) | 0x8000),  // variant
            (unsigned long long)(rand() << 16 | rand()) << 32 | (unsigned long long)(rand() << 16 | rand()));
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (RegSetValueExA(hKey, "MachineGuid", 0, REG_SZ, (const BYTE*)guidStr, (DWORD)(lstrlenA(guidStr) + 1)) == ERROR_SUCCESS) {
                s_con("    MachineGuid -> spoofed OK\r\n");
            } else {
                s_con("    MachineGuid -> write FAILED\r\n");
            }
            RegCloseKey(hKey);
        } else {
            s_con("    MachineGuid -> open key FAILED (need admin?)\r\n");
        }
    }

    // 2. ProductId — Windows Product ID
    //    HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProductId
    {
        char newProductId[24];
        wsprintfA(newProductId, "%05d-%03d-%05d-%05d",
            10000 + (rand() % 89999),
            100 + (rand() % 899),
            10000 + (rand() % 89999),
            10000 + (rand() % 89999));
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (RegSetValueExA(hKey, "ProductId", 0, REG_SZ, (const BYTE*)newProductId, (DWORD)(lstrlenA(newProductId) + 1)) == ERROR_SUCCESS) {
                s_con("    ProductId -> spoofed OK\r\n");
            } else {
                s_con("    ProductId -> write FAILED\r\n");
            }
            RegCloseKey(hKey);
        } else {
            s_con("    ProductId -> open key FAILED\r\n");
        }
    }

    // 3. SusClientId — Windows Update Client ID
    //    HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\WindowsUpdate\SusClientId
    {
        char newSusId[40];
        wsprintfA(newSusId, "%08x%08x%08x%08x",
            rand(), rand(), rand(), rand());
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            if (RegSetValueExA(hKey, "SusClientId", 0, REG_SZ, (const BYTE*)newSusId, (DWORD)(lstrlenA(newSusId) + 1)) == ERROR_SUCCESS) {
                s_con("    SusClientId -> spoofed OK\r\n");
            } else {
                s_con("    SusClientId -> write FAILED\r\n");
            }
            // Also clean SusClientIdValidation if present
            RegDeleteValueA(hKey, "SusClientIdValidation");
            RegCloseKey(hKey);
        } else {
            s_con("    SusClientId -> open key FAILED\r\n");
        }
    }

    s_con("  Registry HWID spoofing OK\r\n");
}

// ---- Extended registry spoofing (additional hardware identifiers) ----

static void spoof_registry_hwid_extended() {
    s_con("  Spoofing extended HWID identifiers...\r\n");

    char guidStr[40];
    wsprintfA(guidStr, "{%08X-%04X-%04X-%04X-%012X}",
        (unsigned)(rand() << 16 | rand()),
        (unsigned)(rand() & 0xFFFF),
        (unsigned)((rand() & 0x0FFF) | 0x4000),
        (unsigned)((rand() & 0x3FFF) | 0x8000),
        (unsigned long long)(rand() << 16 | rand()) << 32 | (unsigned long long)(rand() << 16 | rand()));

    // ComputerHardwareId
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\SystemInformation", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            char hwid[40];
            wsprintfA(hwid, "{%08X-%04X-%04X-%04X-%012X}",
                (unsigned)(rand() << 16 | rand()), (unsigned)(rand() & 0xFFFF),
                (unsigned)((rand() & 0x0FFF) | 0x4000), (unsigned)((rand() & 0x3FFF) | 0x8000),
                (unsigned long long)(rand() << 16 | rand()) << 32 | (unsigned long long)(rand() << 16 | rand()));
            RegSetValueExA(hKey, "ComputerHardwareId", 0, REG_SZ, (const BYTE*)hwid, (DWORD)(lstrlenA(hwid) + 1));
            RegCloseKey(hKey);
        }
    }

    // SystemManufacturer + SystemProductName
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\SystemInformation", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "SystemManufacturer", 0, REG_SZ, (const BYTE*)"Default", 8);
            RegSetValueExA(hKey, "SystemProductName", 0, REG_SZ, (const BYTE*)"Default", 8);
            RegCloseKey(hKey);
        }
    }

    // InstallDate
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            DWORD newDate = (DWORD)(GetTickCount64() / 1000) - (86400 * (365 * (3 + rand() % 5)));
            RegSetValueExA(hKey, "InstallDate", 0, REG_DWORD, (const BYTE*)&newDate, sizeof(newDate));
            RegCloseKey(hKey);
        }
    }

    // SQMClient MachineId
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\SQMClient", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "MachineId", 0, REG_SZ, (const BYTE*)guidStr, (DWORD)(lstrlenA(guidStr) + 1));
            RegCloseKey(hKey);
        }
    }

    // WindowsUpdate MachineId
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "MachineId", 0, REG_SZ, (const BYTE*)guidStr, (DWORD)(lstrlenA(guidStr) + 1));
            RegCloseKey(hKey);
        }
    }

    // SCSI DeviceMap — Identifier + SerialNumber (walk all Scsi Ports)
    {
        for (int port = 0; port < 8; port++) {
            char scsiPath[256];
            wsprintfA(scsiPath, "HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port %d\\Scsi Bus 0\\Target Id 0\\Logical Unit Id 0", port);
            HKEY hKey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, scsiPath, 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                char newId[40];
                wsprintfA(newId, "VBOX%-8d", rand());
                RegSetValueExA(hKey, "Identifier", 0, REG_SZ, (const BYTE*)newId, (DWORD)(lstrlenA(newId) + 1));
                char newSerial[24];
                wsprintfA(newSerial, "%08X%08X", rand(), rand());
                RegSetValueExA(hKey, "SerialNumber", 0, REG_SZ, (const BYTE*)newSerial, (DWORD)(lstrlenA(newSerial) + 1));
                RegCloseKey(hKey);
            }
        }
    }

    // Processor Identifier
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            char newProcId[40];
            wsprintfA(newProcId, "%08X%08X%08X%08X", rand(), rand(), rand(), rand());
            RegSetValueExA(hKey, "Identifier", 0, REG_SZ, (const BYTE*)newProcId, (DWORD)(lstrlenA(newProcId) + 1));
            RegCloseKey(hKey);
        }
    }

    // GraphicsDrivers Timestamp
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GraphicsDrivers", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            DWORD ts = (DWORD)(GetTickCount64() / 1000) - (86400 * (rand() % 30));
            RegSetValueExA(hKey, "Timestamp", 0, REG_BINARY, (const BYTE*)&ts, sizeof(ts));
            RegCloseKey(hKey);
        }
    }

    // BackupProductKeyDefault
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            char keyStr[30];
            wsprintfA(keyStr, "%05d-%05d-%05d-%05d", 10000 + rand() % 89999, 10000 + rand() % 89999, 10000 + rand() % 89999, 10000 + rand() % 89999);
            RegSetValueExA(hKey, "BackupProductKeyDefault", 0, REG_SZ, (const BYTE*)keyStr, (DWORD)(lstrlenA(keyStr) + 1));
            RegCloseKey(hKey);
        }
    }

    // NVIDIA ClientUUID + PersistenceIdentifier (if NVIDIA present)
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Enum\\PCI", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD subIndex = 0;
            char subName[256];
            DWORD subLen = sizeof(subName);
            while (RegEnumKeyExA(hKey, subIndex, subName, &subLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                if (strstr(subName, "VEN_10DE")) {
                    // Found NVIDIA device — walk to device instance
                    char nvidiaKey[512];
                    wsprintfA(nvidiaKey, "SYSTEM\\CurrentControlSet\\Enum\\PCI\\%s", subName);
                    HKEY hNv;
                    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, nvidiaKey, 0, KEY_READ, &hNv) == ERROR_SUCCESS) {
                        DWORD devIndex = 0;
                        char devName[256];
                        DWORD devLen = sizeof(devName);
                        while (RegEnumKeyExA(hNv, devIndex, devName, &devLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                            char devKey[768];
                            wsprintfA(devKey, "%s\\%s", nvidiaKey, devName);
                            HKEY hDev;
                            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, devKey, 0, KEY_SET_VALUE, &hDev) == ERROR_SUCCESS) {
                                RegSetValueExA(hDev, "ClientUUID", 0, REG_SZ, (const BYTE*)guidStr, (DWORD)(lstrlenA(guidStr) + 1));
                                char persistGuid[40];
                                wsprintfA(persistGuid, "{%08X-%04X-%04X-%04X-%012X}",
                                    (unsigned)(rand() << 16 | rand()), (unsigned)(rand() & 0xFFFF),
                                    (unsigned)((rand() & 0x0FFF) | 0x4000), (unsigned)((rand() & 0x3FFF) | 0x8000),
                                    (unsigned long long)(rand() << 16 | rand()) << 32 | (unsigned long long)(rand() << 16 | rand()));
                                RegSetValueExA(hDev, "PersistenceIdentifier", 0, REG_SZ, (const BYTE*)persistGuid, (DWORD)(lstrlenA(persistGuid) + 1));
                                RegCloseKey(hDev);
                            }
                            devIndex++;
                            devLen = sizeof(devName);
                        }
                        RegCloseKey(hNv);
                    }
                }
                subIndex++;
                subLen = sizeof(subName);
            }
            RegCloseKey(hKey);
        }
    }

    // File cleanup — overwrite MachineGuid.txt backup
    {
        const char* mgPath = "C:\\Windows\\System32\\restore\\MachineGuid.txt";
        HANDLE hFile = CreateFileA(mgPath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(hFile, guidStr, (DWORD)lstrlenA(guidStr), &written, NULL);
            CloseHandle(hFile);
        }
    }

    // Clear setupapi.dev.log
    {
        const char* logPath = "C:\\Windows\\INF\\setupapi.dev.log";
        DeleteFileA(logPath);
    }

    // Clear NVIDIA DXCache shader files
    {
        char dxPath[MAX_PATH];
        GetEnvironmentVariableA("LOCALAPPDATA", dxPath, MAX_PATH);
        lstrcatA(dxPath, "\\NVIDIA\\DXCache");
        WIN32_FIND_DATAA fd;
        char searchStr[MAX_PATH];
        wsprintfA(searchStr, "%s\\*", dxPath);
        HANDLE hFind = FindFirstFileA(searchStr, &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    char filePath[MAX_PATH];
                    wsprintfA(filePath, "%s\\%s", dxPath, fd.cFileName);
                    DeleteFileA(filePath);
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }
    }

    s_con("  Extended HWID spoofing OK\r\n");
}

// ---- Network recovery (needed after Spoof_MAC / Spoof_ARP IOCTLs) ----

static void spoof_net_recover() {
    s_con("  Flushing DNS cache...\r\n");
    s_run_exe("ipconfig.exe /flushdns");
    s_con("  DNS flush OK\r\n");
}

// ---- Minimal user-mode cleanup ----

static void runafterspoof(bool win10Mode, bool macIoctlFailed) {
    s_con("[*] Running user-mode cleanup...\r\n");

    spoof_temp_cleanup();
    spoof_registry_hwid();
    spoof_registry_hwid_extended();

    if (s_is_bitlocker_on()) {
        s_con("  [!] BitLocker ON - skipping TPM wipe to avoid recovery lockout\r\n");
        LOG("Spoofer: BitLocker detected, skipping TPM cleanup");
    } else {
        spoof_tpm_cleanup();
    }

    // MAC registry fallback: on Win10 (always) or when MAC IOCTL failed on Win11
    if (win10Mode || macIoctlFailed) {
        spoof_mac_registry_fallback();
    }

    spoof_net_recover();

    s_con("[+] Cleanup complete (all changes revert on reboot)\r\n");
}

// ---- Main entry point ----

bool RunEmbeddedSpoofer(HANDLE hDriver)
{
    s_status = {};
    s_status.attempted = true;
    s_hDrv = hDriver;

    s_con("========================================\r\n");
    s_con("  HWID Spoofer (embedded)\r\n");
    s_con("========================================\r\n\r\n");

    s_log_open();
    s_log_write("=== HWID Spoofer ===\r\n");

    // Windows version check
    bool win10 = s_is_win10();
    if (win10) {
        s_con("[!] Windows 10 detected.\r\n");
        s_con("    Kernel IOCTLs may cause BSOD on this OS.\r\n");
        s_con("    Using safe mode: registry-level spoofing only.\r\n");
        s_con("    Disk serials and SMBIOS cannot be spoofed in safe mode.\r\n\r\n");
        LOG("Spoofer: Windows 10 detected, safe mode");
    } else {
        s_con("[+] Windows 11 detected.\r\n\r\n");
    }

    // HVCI check
    if (s_is_hvci_on()) {
        s_con("[!] WARNING: HVCI / VBS is ENABLED.\r\n");
        s_con("    Kernel driver may not function properly.\r\n\r\n");
        LOG("Spoofer: HVCI/VBS detected");
    }

    // ---- Phase 0: Try full kernel hardware spoof (17 layers) ----
    {
        s_con("[*] Attempting full kernel hardware spoof...\r\n");
        OffsetData kernelOffsets;
        if (ResolveOffsetsForCurrentBuild(kernelOffsets)) {
            if (DriverSpoofer_Connect()) {
                SPOOFER_OFFSETS_INPUT smbios = {};
                smbios.SmbiosPhysAddrOffset = kernelOffsets.smbiosPhysAddr;
                smbios.SmbiosTableLenOffset = kernelOffsets.smbiosTableLen;

                SPOOFER_NDIS_OFFSETS_INPUT ndis = {};
                ndis.NdisGlobalFilterListOffset = kernelOffsets.ndisFilterList;
                ndis.FilterBlockIfBlockOffset = kernelOffsets.filterBlockIfBlock;
                ndis.FilterBlockMiniportOffset = kernelOffsets.filterBlockMiniport;
                ndis.FilterBlockNextGlobalFilterOffset = kernelOffsets.filterBlockNextGlobal;
                ndis.FilterInstanceNameOffset = kernelOffsets.filterInstanceName;
                ndis.IfBlockPhysAddressOffset = kernelOffsets.ifBlockPhysAddr;
                ndis.IfBlockPermPhysAddressOffset = kernelOffsets.ifBlockPermPhysAddr;
                ndis.MiniportBlockIfBlockOffset = kernelOffsets.miniportIfBlock;

                SPOOFER_KNOWN_MACS_INPUT macs = {};
                // Collect current MACs
                {
                    ULONG outBufLen = sizeof(IP_ADAPTER_INFO) * 16;
                    PIP_ADAPTER_INFO pAdapt = (PIP_ADAPTER_INFO)malloc(outBufLen);
                    if (pAdapt && GetAdaptersInfo(pAdapt, &outBufLen) == NO_ERROR) {
                        PIP_ADAPTER_INFO p = pAdapt;
                        while (p && macs.Count < MAX_KNOWN_MACS) {
                            if (p->AddressLength >= 6) {
                                memcpy(macs.Macs[macs.Count], p->Address, 6);
                                macs.Count++;
                            }
                            p = p->Next;
                        }
                    }
                    if (pAdapt) free(pAdapt);
                }

                SPOOFER_STORPORT_OFFSETS_INPUT storport = {};
                SPOOFER_TPM_OFFSETS_INPUT tpm = {};
                tpm.DispatchCommandRva = kernelOffsets.tpmDispatchRva;

                if (DriverSpoofer_SendOffsets(smbios, ndis, macs, storport, tpm)) {
                    UINT8 seed[16] = {};
                    {
                        LARGE_INTEGER pc;
                        QueryPerformanceCounter(&pc);
                        UINT64 val = (UINT64)GetTickCount64() ^ pc.QuadPart;
                        for (int i = 0; i < 16; i++) {
                            val = val * 6364136223846793005ULL + 1442695040888963407ULL;
                            seed[i] = (UINT8)(val >> 24);
                        }
                    }
                    if (DriverSpoofer_Install(seed)) {
                        DriverSpooferStatus st = DriverSpoofer_GetStatus();
                        char buf[128];
                        wsprintfA(buf, "[+] Full kernel spoof: %d layers active\r\n", st.layersActive);
                        s_con(buf);
                        if (st.firstSerial[0]) {
                            wsprintfA(buf, "    Disk serial: %s\r\n", st.firstSerial);
                            s_con(buf);
                        }
                        if (st.spoofedUUID[0]) {
                            wsprintfA(buf, "    SMBIOS UUID: %s\r\n", st.spoofedUUID);
                            s_con(buf);
                        }
                        LOG("Spoofer: Full kernel spoof active (%d layers)", st.layersActive);
                    } else {
                        s_con("[!] Full kernel spoof: install failed\r\n");
                    }
                } else {
                    s_con("[!] Full kernel spoof: offsets rejected\r\n");
                }
                DriverSpoofer_Disconnect();
            } else {
                s_con("[*] Full kernel spoof not available — using fallback\r\n");
            }
        } else {
            s_con("[*] Kernel offsets not resolved — using fallback\r\n");
        }
        s_con("\r\n");
    }

    bool macIoctlFailed = false;

    if (win10) {
        // Win10 safe mode: skip all kernel IOCTLs, use registry alternatives only
        s_con("[*] Safe mode: skipping kernel IOCTLs to prevent BSOD.\r\n");
        s_con("[*] Using registry-level alternatives.\r\n\r\n");

        s_status.driverConnected = (s_hDrv != INVALID_HANDLE_VALUE);
        s_status.total = 0;
        s_status.okCount = 0;

        // Steam ID (for seed)
        s_read_steam_id();

        // Registry cleanup + MAC fallback + TPM + DNS
        runafterspoof(true, false);

        s_con("\r\n[+] Safe mode spoof complete.\r\n");
        s_con("[!] NOT SPOOFED (requires kernel driver):\r\n");
        s_con("    - Disk Serials (EAC reads via SMART)\r\n");
        s_con("    - SMBIOS Data (EAC reads via firmware table)\r\n");
        s_con("    - GPU HWID (EAC reads via DXGI)\r\n");
        s_con("    - Monitor Serials (EAC reads via EDID)\r\n");
        s_con("[+] SPOOFED (registry-level):\r\n");
        s_con("    - MAC Addresses (NetworkAddress registry)\r\n");
        s_con("    - TPM keys (registry wipe)\r\n");
        s_con("    - MountedDevices, mssmbios (auto-regen)\r\n");

        LOG("Spoofer: Win10 safe mode complete");
        s_status.completed = true;
        if (s_log != INVALID_HANDLE_VALUE) { CloseHandle(s_log); s_log = INVALID_HANDLE_VALUE; }
        s_hDrv = INVALID_HANDLE_VALUE;
        return true;
    }

    // ---- Win11: Run kernel IOCTLs ----
    if (s_hDrv == INVALID_HANDLE_VALUE) {
        s_con("[!] Invalid driver handle. Running user-mode cleanup only.\r\n");
        LOG("Spoofer: driver handle invalid, user-mode cleanup only");
        runafterspoof(false, false);
        s_status.completed = true;
        return false;
    }
    s_status.driverConnected = true;
    s_con("[+] Driver ready\r\n");

    // Steam ID
    if (!s_read_steam_id()) {
        runafterspoof(false, false);
        s_status.completed = true;
        return false;
    }

    // Seed
    s_load_or_generate_seed();
    s_status.seedSet = updateseed_todriver();

    // ---- Run all spoof IOCTLs ----
    s_con("\r\nSpoofing hardware IDs...\r\n\r\n");

    struct { bool (*fn)(); const char* n; } cmds[] = {
        { SpoofDisk,          "Disk Serials" },
        { SpoofMAC,           "MAC Addresses" },
        { SpoofGPU,           "GPU HWID" },
        { SpoofSMBIOS,        "SMBIOS Data" },
        { SpoofARP,           "ARP Cache" },
        { SpoofSMBIOSNULL,    "SMBIOS Nulls" },
        { SpoofTPM,           "TPM Identifiers" },
        { SpoofMonitor,       "Monitor Serials" },
        { SpoofRegistry,      "Registry HWID" },
    };

    int total = sizeof(cmds) / sizeof(cmds[0]);
    int okCount = 0;
    bool earlyMismatchAbort = false;
    for (int i = 0; i < total; i++) {
        bool ok = cmds[i].fn();
        s_con_line(cmds[i].n, ok);
        if (ok) okCount++;

        // Track MAC failure for registry fallback
        if (i == 1 && !ok) macIoctlFailed = true;

        if (i == 0 && !s_status.seedSet && !ok) {
            s_con("[!] Spoof opcodes rejected by driver. Driver mismatch on this PC.\r\n");
            s_con("[!] Aborting remaining spoof commands early.\r\n");
            earlyMismatchAbort = true;
            break;
        }
        Sleep(100);
    }

    char msg[128]; wsprintfA(msg, "\r\n  %d/%d kernel spoofs successful\r\n\r\n", okCount, total);
    s_con(msg);
    s_status.okCount = okCount;
    s_status.total = total;
    s_status.attemptedCount = total;
    s_status.driverMismatch = earlyMismatchAbort;

    // Show what was spoofed vs what failed
    if (macIoctlFailed) {
        s_con("[!] MAC IOCTL failed — will use registry fallback\r\n");
        LOG("Spoofer: MAC IOCTL failed, using registry fallback");
    }

    // User-mode cleanup + MAC fallback if needed
    runafterspoof(false, macIoctlFailed);

    s_con("[+] Spoofer completed.\r\n");
    LOG("Spoofer: completed (%d/%d ok)", okCount, total);
    s_status.completed = true;

    if (s_log != INVALID_HANDLE_VALUE) { CloseHandle(s_log); s_log = INVALID_HANDLE_VALUE; }
    s_hDrv = INVALID_HANDLE_VALUE;
    return true;
}
