#include "../include/spoofer.h"


#define NTSCAN_SYSTEM_MODULE_INFO 11

typedef struct _NTSCAN_MODULE {
    HANDLE Section;
    PVOID  MappedBase;
    PVOID  ImageBase;
    ULONG  ImageSize;
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[256];
} NTSCAN_MODULE;

typedef struct _NTSCAN_MODULES {
    ULONG NumberOfModules;
    NTSCAN_MODULE Modules[1];
} NTSCAN_MODULES;

extern "C" NTSTATUS ZwQuerySystemInformation(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);


static PVOID FindKernelModule(const char* partialName, PULONG outSize)
{
    ULONG bufSize = 0;
    NTSCAN_MODULES* modules = NULL;
    PVOID result = NULL;
    ULONG i;

    if (outSize) *outSize = 0;

    ZwQuerySystemInformation(NTSCAN_SYSTEM_MODULE_INFO, NULL, 0, &bufSize);
    if (bufSize == 0) return NULL;

    modules = (NTSCAN_MODULES*)((fn_ExAllocatePoolWithTag)
        ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, bufSize, 'nScM');
    if (!modules) return NULL;

    if (!NT_SUCCESS(ZwQuerySystemInformation(NTSCAN_SYSTEM_MODULE_INFO, modules, bufSize, NULL))) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modules, 'nScM');
        return NULL;
    }

    for (i = 0; i < modules->NumberOfModules; i++) {
        const char* name = (const char*)modules->Modules[i].FullPathName +
                           modules->Modules[i].OffsetToFileName;

        const char* p = partialName;
        const char* n = name;
        BOOLEAN match = TRUE;
        while (*p) {
            char c1 = (*n >= 'A' && *n <= 'Z') ? *n + 32 : *n;
            char c2 = (*p >= 'A' && *p <= 'Z') ? *p + 32 : *p;
            if (c1 != c2) { match = FALSE; break; }
            p++; n++;
        }

        if (match) {
            result = modules->Modules[i].ImageBase;
            if (outSize) *outSize = modules->Modules[i].ImageSize;
            break;
        }
    }

    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modules, 'nScM');
    return result;
}


static LONG ScanBufferForMac(const UCHAR* buf, ULONG bufLen,
                              const UCHAR* mac, ULONG startOffset)
{
    ULONG i;
    if (!buf || bufLen < 6 || !mac) return -1;

    for (i = startOffset; i <= bufLen - 6; i++) {
        if (buf[i]     == mac[0] && buf[i + 1] == mac[1] &&
            buf[i + 2] == mac[2] && buf[i + 3] == mac[3] &&
            buf[i + 4] == mac[4] && buf[i + 5] == mac[5])
        {
            return (LONG)i;
        }
    }
    return -1;
}


static BOOLEAN SafeCopyPage(PUCHAR src, PUCHAR dst, ULONG len)
{
    ULONG i;

    for (i = 0; i < len; i += 0x100) {
        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(&src[i]))
            return FALSE;
    }
    if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(&src[len - 1]))
        return FALSE;

    __try {
        RtlCopyMemory(dst, src, len);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }
    return TRUE;
}


static NTSTATUS PatchBytesAtAddress(PUCHAR addr, const UCHAR* data, ULONG len)
{
    PMDL mdl;
    PVOID mapped;

    if (!addr || !data || len == 0) return STATUS_INVALID_PARAMETER;

    if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(addr))
        return STATUS_ACCESS_VIOLATION;

    mdl = ((fn_IoAllocateMdl)ApiResolveExport(HASH_IOALLOCATEMDL))(
        addr, len, FALSE, FALSE, NULL);
    if (!mdl) return STATUS_INSUFFICIENT_RESOURCES;

    __try {
        ((fn_MmBuildMdlForNonPagedPool)ApiResolveExport(HASH_MMBUILDMDLFORNONPAGEDPOOL))(mdl);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ((fn_IoFreeMdl)ApiResolveExport(HASH_IOFREEMDL))(mdl);
        return STATUS_ACCESS_VIOLATION;
    }

    mapped = ((fn_MmMapLockedPagesSpecifyCache)ApiResolveExport(HASH_MMMAPLOCKEDPAGESSPECIFYCACHE))(
        mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
    if (!mapped) {
        ((fn_IoFreeMdl)ApiResolveExport(HASH_IOFREEMDL))(mdl);
        return STATUS_UNSUCCESSFUL;
    }

    RtlCopyMemory(mapped, data, len);

    ((fn_MmUnmapLockedPages)ApiResolveExport(HASH_MMUNMAPLOCKEDPAGES))(mapped, mdl);
    ((fn_IoFreeMdl)ApiResolveExport(HASH_IOFREEMDL))(mdl);

    return STATUS_SUCCESS;
}


#define SCAN_PAGE_SIZE 0x1000

static ULONG ScanModuleForMacs(PUCHAR moduleBase, ULONG moduleSize)
{
    ULONG patchCount = 0;
    ULONG macIdx;
    PUCHAR pageBuf = NULL;

    if (!moduleBase || moduleSize == 0) return 0;
    if (!g_Spoofer.KnownMacsProvided || g_Spoofer.KnownMacs.Count == 0) return 0;

    pageBuf = (PUCHAR)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(
        NonPagedPool, SCAN_PAGE_SIZE + 6, 'nScP');
    if (!pageBuf) return 0;

    for (macIdx = 0; macIdx < g_Spoofer.KnownMacs.Count && macIdx < MAX_KNOWN_MACS; macIdx++) {
        UCHAR* originalMac = g_Spoofer.KnownMacs.Macs[macIdx];
        UCHAR spoofedMac[6];
        UCHAR reversedOriginal[6], reversedSpoofed[6];
        ULONG pageOff, r;

        if ((originalMac[0] == 0 && originalMac[1] == 0 && originalMac[2] == 0 &&
             originalMac[3] == 0 && originalMac[4] == 0 && originalMac[5] == 0) ||
            (originalMac[0] == 0xFF && originalMac[1] == 0xFF && originalMac[2] == 0xFF &&
             originalMac[3] == 0xFF && originalMac[4] == 0xFF && originalMac[5] == 0xFF))
            continue;

        if (!MacMapLookup(originalMac, spoofedMac))
            continue;

        for (r = 0; r < 6; r++) {
            reversedOriginal[r] = originalMac[5 - r];
            reversedSpoofed[r] = spoofedMac[5 - r];
        }

        for (pageOff = 0; pageOff < moduleSize; pageOff += SCAN_PAGE_SIZE) {
            ULONG copyLen = SCAN_PAGE_SIZE;
            LONG found;

            if (pageOff + copyLen > moduleSize)
                copyLen = moduleSize - pageOff;

            if (copyLen < 6) break;

            if (!SafeCopyPage(moduleBase + pageOff, pageBuf, copyLen))
                continue;  

            found = 0;
            while ((found = ScanBufferForMac(pageBuf, copyLen, originalMac, (ULONG)found)) >= 0) {
                PUCHAR realAddr = moduleBase + pageOff + found;

                __try {
                    NTSTATUS st = PatchBytesAtAddress(realAddr, spoofedMac, 6);
                    if (NT_SUCCESS(st)) {
                        patchCount++;
                        SPOOF_DBG("[Spoofer] 5b: Patched MAC at %p (page+0x%X)\n",
                            realAddr, (ULONG)found);
                        RtlCopyMemory(&pageBuf[found], spoofedMac, 6);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}

                found += 6;
                if ((ULONG)found + 6 > copyLen) break;
            }

            found = 0;
            while ((found = ScanBufferForMac(pageBuf, copyLen, reversedOriginal, (ULONG)found)) >= 0) {
                PUCHAR realAddr = moduleBase + pageOff + found;

                __try {
                    NTSTATUS st = PatchBytesAtAddress(realAddr, reversedSpoofed, 6);
                    if (NT_SUCCESS(st)) {
                        patchCount++;
                        SPOOF_DBG("[Spoofer] 5b: Patched reversed MAC at %p\n", realAddr);
                        RtlCopyMemory(&pageBuf[found], reversedSpoofed, 6);
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}

                found += 6;
                if ((ULONG)found + 6 > copyLen) break;
            }
        }
    }

    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(pageBuf, 'nScP');
    return patchCount;
}

NTSTATUS ScanKernelModulesForMacs(const UINT8* seed)
{
    ULONG totalPatched = 0;
    ULONG moduleSize;
    PVOID moduleBase;

    UNREFERENCED_PARAMETER(seed);

    SPOOF_DBG("[Spoofer] ========= SCHICHT 5b: ntoskrnl MAC Scan =========\n");

    if (!g_Spoofer.KnownMacsProvided || g_Spoofer.KnownMacs.Count == 0) {
        SPOOF_DBG("[Spoofer] 5b: No known MACs — skipping\n");
        return STATUS_SUCCESS;
    }

    if (g_MacMapCount == 0) {
        SPOOF_DBG("[Spoofer] 5b: MacMap empty (Schicht 5 didn't run?) — skipping\n");
        return STATUS_SUCCESS;
    }

    moduleBase = FindKernelModule("ntoskrnl", &moduleSize);
    if (!moduleBase) moduleBase = FindKernelModule("ntkrnlmp", &moduleSize);
    if (moduleBase) {
        SPOOF_DBG("[Spoofer] 5b: Scanning ntoskrnl @ %p (size=0x%X)\n", moduleBase, moduleSize);
        __try {
            totalPatched += ScanModuleForMacs((PUCHAR)moduleBase, moduleSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SPOOF_DBG("[Spoofer] 5b: Exception scanning ntoskrnl\n");
        }
    }

    moduleBase = FindKernelModule("ndis.sys", &moduleSize);
    if (moduleBase) {
        SPOOF_DBG("[Spoofer] 5b: Scanning ndis.sys @ %p (size=0x%X)\n", moduleBase, moduleSize);
        __try {
            totalPatched += ScanModuleForMacs((PUCHAR)moduleBase, moduleSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SPOOF_DBG("[Spoofer] 5b: Exception scanning ndis.sys\n");
        }
    }

    moduleBase = FindKernelModule("tcpip.sys", &moduleSize);
    if (moduleBase) {
        SPOOF_DBG("[Spoofer] 5b: Scanning tcpip.sys @ %p (size=0x%X)\n", moduleBase, moduleSize);
        __try {
            totalPatched += ScanModuleForMacs((PUCHAR)moduleBase, moduleSize);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SPOOF_DBG("[Spoofer] 5b: Exception scanning tcpip.sys\n");
        }
    }

    SPOOF_DBG("[Spoofer] 5b: Total patched: %lu\n", totalPatched);

    return totalPatched > 0 ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}
