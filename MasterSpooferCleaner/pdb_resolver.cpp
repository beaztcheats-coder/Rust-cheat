#include "pdb_resolver.h"
#include "offset_database.h"
#include <dbghelp.h>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "dbghelp.lib")

#pragma pack(push, 8)
struct SYM_INFO_EX {
    SYMBOL_INFO sym;
    WCHAR name[256];
};
#pragma pack(pop)

static UINT64 ResolveSymbol(HANDLE hProcess, DWORD64 modBase, const char* symName) {
    SYM_INFO_EX symInfo = {};
    symInfo.sym.SizeOfStruct = sizeof(SYMBOL_INFO);
    symInfo.sym.MaxNameLen = sizeof(symInfo.name) / sizeof(WCHAR);

    if (!SymFromName(hProcess, symName, &symInfo.sym)) {
        return 0;
    }

    if (symInfo.sym.ModBase == modBase) {
        return symInfo.sym.Address - modBase;
    }
    return symInfo.sym.Address - modBase;
}

static DWORD64 LoadModuleSymbols(HANDLE hProcess, const wchar_t* dllPath, const char* dllName) {
    // Copy to temp with generic name to avoid path detection
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    char tempFile[MAX_PATH];
    snprintf(tempFile, sizeof(tempFile), "%s_sys_%d.dll", tempPath, GetTickCount() & 0xFFFF);

    CopyFileW(dllPath, L"", FALSE); // placeholder

    // Just use the original path directly
    DWORD64 modBase = SymLoadModuleExW(hProcess, NULL, dllPath, NULL, 0, 0, NULL, 0);
    if (!modBase) {
        // Try with just filename
        wchar_t fullPath[MAX_PATH];
        GetSystemDirectoryW(fullPath, MAX_PATH);
        wcscat_s(fullPath, L"\\");
        wcscat_s(fullPath, dllPath);
        modBase = SymLoadModuleExW(hProcess, NULL, fullPath, NULL, 0, 0, NULL, 0);
    }
    return modBase;
}

bool ResolveOffsetsViaPDB(OffsetData& out) {
    // Initialize symbol resolver with Microsoft Symbol Server
    HANDLE hProcess = GetCurrentProcess();

    // Set symbol path: local cache + Microsoft Symbol Server
    char symPath[MAX_PATH * 2];
    char tempPath[MAX_PATH];
    GetTempPathA(MAX_PATH, tempPath);
    snprintf(symPath, sizeof(symPath), "srv*%ssymbols*https://msdl.microsoft.com/download/symbols", tempPath);

    SymSetOptions(SYMOPT_DEBUG | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (!SymInitialize(hProcess, symPath, FALSE)) {
        printf("  [!] SymInitialize failed (error %lu)\n", GetLastError());
        return false;
    }

    // --- Load ntoskrnl.exe ---
    wchar_t ntoskrnlPath[MAX_PATH] = {};
    GetSystemDirectoryW(ntoskrnlPath, MAX_PATH);
    wcscat_s(ntoskrnlPath, L"\\ntoskrnl.exe");

    printf("  [*] Loading ntoskrnl symbols...\n");
    DWORD64 ntBase = SymLoadModuleExW(hProcess, NULL, ntoskrnlPath, NULL, 0, 0, NULL, 0);
    if (!ntBase) {
        printf("  [!] Failed to load ntoskrnl.exe symbols\n");
        SymCleanup(hProcess);
        return false;
    }

    // Resolve SMBIOS offsets
    UINT64 smbiosPhys = ResolveSymbol(hProcess, ntBase, "WmipSMBiosTablePhysicalAddress");
    UINT64 smbiosLen = ResolveSymbol(hProcess, ntBase, "WmipSMBiosTableLength");

    printf("  [+] WmipSMBiosTablePhysicalAddress: 0x%llx\n", (unsigned long long)smbiosPhys);
    printf("  [+] WmipSMBiosTableLength:          0x%llx\n", (unsigned long long)smbiosLen);

    out.smbiosPhysAddr = smbiosPhys;
    out.smbiosTableLen = smbiosLen;

    SymUnloadModule64(hProcess, ntBase);

    // --- Load ndis.sys ---
    wchar_t ndisPath[MAX_PATH] = {};
    GetSystemDirectoryW(ndisPath, MAX_PATH);
    wcscat_s(ndisPath, L"\\drivers\\ndis.sys");

    printf("  [*] Loading ndis symbols...\n");
    DWORD64 ndisBase = SymLoadModuleExW(hProcess, NULL, ndisPath, NULL, 0, 0, NULL, 0);
    if (ndisBase) {
        UINT64 filterList = ResolveSymbol(hProcess, ndisBase, "ndisGlobalFilterList");
        printf("  [+] ndisGlobalFilterList: 0x%llx\n", (unsigned long long)filterList);
        out.ndisFilterList = filterList;

        // Try to resolve NDIS structure offsets from type info
        // These are harder to get from PDB (need type enumeration)
        // For now use known stable values that work across most builds
        SymUnloadModule64(hProcess, ndisBase);
    } else {
        printf("  [!] Could not load ndis.sys symbols (using default offsets)\n");
    }

    // NDIS structure offsets — these are relatively stable across builds
    // Can be refined with dt ndis!_NDIS_FILTER_BLOCK in WinDbg per build
    out.filterBlockIfBlock       = 0x028;
    out.filterBlockMiniport      = 0x010;
    out.filterBlockNextGlobal    = 0x068;
    out.filterInstanceName       = 0x038;
    out.ifBlockPhysAddr          = 0x020;
    out.ifBlockPermPhysAddr      = 0x048;
    out.miniportIfBlock          = 0x110;

    // --- Load stornvme.sys for NVMe offsets (optional) ---
    wchar_t nvmePath[MAX_PATH] = {};
    GetSystemDirectoryW(nvmePath, MAX_PATH);
    wcscat_s(nvmePath, L"\\drivers\\stornvme.sys");

    DWORD64 nvmeBase = SymLoadModuleExW(hProcess, NULL, nvmePath, NULL, 0, 0, NULL, 0);
    if (nvmeBase) {
        printf("  [+] stornvme symbols loaded\n");
        // NVMe-specific offsets would be resolved here
        // These require type structure enumeration which is more complex
        SymUnloadModule64(hProcess, nvmeBase);
    }

    // TPM dispatch RVA — needs tpm.sys symbols
    // This varies significantly per build, would need:
    //   x tpm!TpmSubmitCommand  (to find dispatch function)
    //   Calculate RVA from module base
    out.tpmDispatchRva = 0;

    // StorPort offsets — need storport.sys or stornvme.sys type info
    // These are structure member offsets, not symbol addresses
    // Would need: dt stornvme!_NVME_DEVICE_EXTENSION
    out.raidSerialString = 0;
    out.raidRichDescPtr = 0;
    out.raidInlineSerial = 0;
    out.raidUnitAdapter = 0;
    out.raidAdapterMiniport = 0;
    out.miniportPrivateExt = 0;
    out.nvmeController = 0;
    out.identifyDataPtr = 0;
    out.identifySerial = 0;
    out.identifySerialLen = 0;
    out.nvmeAdapterSerialCache = 0;
    out.raidAdapterSerialCache = 0;
    out.raidUnitRegIfRva = 0;
    out.spDeviceIdPtr = 0;
    out.spDeviceIdData = 0;
    out.spDeviceIdDataLen = 0;

    SymCleanup(hProcess);

    // Verify we got the critical offsets
    if (out.smbiosPhysAddr == 0) {
        printf("  [!] Failed to resolve SMBIOS offsets\n");
        return false;
    }

    printf("  [+] PDB resolution complete.\n");

    // Cleanup downloaded PDB files
    char pdbPath[MAX_PATH * 2];
    WIN32_FIND_DATAA fd;
    char searchStr[MAX_PATH * 2];
    snprintf(searchStr, sizeof(searchStr), "%ssymbols\\*.pdb", tempPath);
    // Don't delete — might be needed again. Leave for caching.

    return true;
}
