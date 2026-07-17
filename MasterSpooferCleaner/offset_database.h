#pragma once
#include <windows.h>
#include <iphlpapi.h>
#include <cstdio>

struct OffsetData {
    DWORD  buildNumber;
    DWORD  major;
    DWORD  minor;

    // SMBIOS offsets (ntoskrnl.exe)
    UINT64 smbiosPhysAddr;
    UINT64 smbiosTableLen;

    // NDIS offsets (ndis.sys)
    UINT64 ndisFilterList;
    UINT32 filterBlockIfBlock;
    UINT32 filterBlockMiniport;
    UINT32 filterBlockNextGlobal;
    UINT32 filterInstanceName;
    UINT32 ifBlockPhysAddr;
    UINT32 ifBlockPermPhysAddr;
    UINT32 miniportIfBlock;

    // StorPort offsets (stornvme.sys / storport.sys)
    UINT32 raidSerialString;
    UINT32 raidRichDescPtr;
    UINT32 raidInlineSerial;
    UINT32 raidUnitAdapter;
    UINT32 raidAdapterMiniport;
    UINT32 miniportPrivateExt;
    UINT32 nvmeController;
    UINT32 identifyDataPtr;
    UINT32 identifySerial;
    UINT32 identifySerialLen;
    UINT32 nvmeAdapterSerialCache;
    UINT32 raidAdapterSerialCache;
    UINT64 raidUnitRegIfRva;
    UINT32 spDeviceIdPtr;
    UINT32 spDeviceIdData;
    UINT32 spDeviceIdDataLen;

    // TPM offsets
    UINT64 tpmDispatchRva;
};

// PDB resolver — auto-downloads and resolves offsets from Microsoft Symbol Server
bool ResolveOffsetsViaPDB(OffsetData& out);

inline bool ResolveOffsetsForCurrentBuild(OffsetData& out) {
    memset(&out, 0, sizeof(out));

    // Get Windows version info
    OSVERSIONINFOEXW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    typedef LONG(WINAPI* RtlGetVersion_t)(PRTL_OSVERSIONINFOEXW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;
    auto pRtlGetVersion = (RtlGetVersion_t)GetProcAddress(ntdll, "RtlGetVersion");
    if (!pRtlGetVersion || pRtlGetVersion(&osvi) != 0) return false;

    out.major = osvi.dwMajorVersion;
    out.minor = osvi.dwMinorVersion;
    out.buildNumber = osvi.dwBuildNumber;

    printf("  [*] Windows build %lu.%lu.%lu\n", out.major, out.minor, out.buildNumber);
    printf("  [*] Resolving kernel offsets via PDB...\n");

    // Use PDB resolver — auto-downloads symbols from Microsoft
    if (ResolveOffsetsViaPDB(out)) {
        return true;
    }

    printf("  [!] PDB resolution failed. Trying fallback offsets...\n");

    // Fallback: NDIS structure offsets (relatively stable)
    out.filterBlockIfBlock       = 0x028;
    out.filterBlockMiniport      = 0x010;
    out.filterBlockNextGlobal    = 0x068;
    out.filterInstanceName       = 0x038;
    out.ifBlockPhysAddr          = 0x020;
    out.ifBlockPermPhysAddr      = 0x048;
    out.miniportIfBlock          = 0x110;

    // Can't continue without SMBIOS offsets
    return false;
}
