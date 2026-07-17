#include "../include/spoofer.h"

// ============================================================================
// smbios_patch.c — Schicht 4: SMBIOS Table Spoof
//
// Two-layer approach:
//   1. Reads raw SMBIOS from registry, creates patched copy for reg callback
//   2. Uses PDB-resolved offsets to find kernel's physical SMBIOS buffer,
//      maps it with MmMapIoSpace and patches in-place
//
// Layer 1 covers: direct registry readers
// Layer 2 covers: GetSystemFirmwareTable, WMI (Win32_BIOS etc.)
// ============================================================================

#define SMBIOS_TYPE_BIOS      0
#define SMBIOS_TYPE_SYSTEM    1
#define SMBIOS_TYPE_BASEBOARD 2
#define SMBIOS_TYPE_CHASSIS   3
#define SMBIOS_TYPE_END       127

#pragma pack(push, 1)
typedef struct _SMBIOS_HEADER {
    UCHAR  Type;
    UCHAR  Length;
    USHORT Handle;
} SMBIOS_HEADER, *PSMBIOS_HEADER;

typedef struct _RAW_SMBIOS_DATA {
    UCHAR  Used20CallingMethod;
    UCHAR  SMBIOSMajorVersion;
    UCHAR  SMBIOSMinorVersion;
    UCHAR  DmiRevision;
    ULONG  Length;
    UCHAR  SMBIOSTableData[1];
} RAW_SMBIOS_DATA, *PRAW_SMBIOS_DATA;
#pragma pack(pop)

// For ZwQuerySystemInformation(SystemModuleInformation)
#define SystemModuleInformation 11

typedef struct _RTL_PROCESS_MODULE_INFORMATION {
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
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

/* ZwQuerySystemInformation resolved via HASH_ZWQUERYSYSTEMINFORMATION at runtime */

// ============================================================================
// Global storage
// ============================================================================

PUCHAR  g_SpoofedSmbiosRaw = NULL;
ULONG   g_SpoofedSmbiosLen = 0;

char    g_SmbiosSystemSerial[128]    = {0};
char    g_SmbiosBaseBoardSerial[128] = {0};
char    g_SmbiosChassisSerial[128]   = {0};
char    g_SmbiosSystemUUID[64]       = {0};
UCHAR   g_SmbiosUUIDBytes[16]       = {0};

BOOLEAN g_SmbiosPatched = FALSE;

// Original kernel SMBIOS data for restore
static PUCHAR  g_OriginalTableData = NULL;
static ULONG   g_OriginalTableLen  = 0;

// ============================================================================
// SMBIOS string table helpers
// ============================================================================

static PCHAR GetSmbiosString(PUCHAR structStart, UCHAR structLength, UCHAR stringIndex)
{
    PCHAR str = (PCHAR)(structStart + structLength);
    UCHAR idx = 1;

    if (stringIndex == 0) return NULL;

    while (*str) {
        if (idx == stringIndex) return str;
        while (*str) str++;
        str++;
        idx++;
    }
    return NULL;
}

static PUCHAR NextSmbiosStruct(PUCHAR current)
{
    PSMBIOS_HEADER hdr = (PSMBIOS_HEADER)current;
    PUCHAR p = current + hdr->Length;

    while (!(p[0] == 0 && p[1] == 0))
        p++;

    return p + 2;
}

static SIZE_T SafeStrLen(const char* s, SIZE_T maxLen)
{
    SIZE_T i;
    for (i = 0; i < maxLen; i++) {
        if (s[i] == 0) return i;
    }
    return maxLen;
}

// ============================================================================
// UUID spoofing
// ============================================================================

static void GenerateSpoofedUUID(UCHAR* outUUID, const UINT8* seed)
{
    ULONG i;
    for (i = 0; i < 16; i++) {
        UINT32 hash = 0x811c9dc5;
        hash ^= seed[i % 16];        hash *= 0x01000193;
        hash ^= seed[(i + 5) % 16];  hash *= 0x01000193;
        hash ^= (UINT8)(i + 0xAA);   hash *= 0x01000193;
        hash ^= seed[(i + 11) % 16]; hash *= 0x01000193;
        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        outUUID[i] = (UINT8)(hash & 0xFF);
    }
    // UUID v4 bits
    outUUID[6] = (outUUID[6] & 0x0F) | 0x40;
    outUUID[8] = (outUUID[8] & 0x3F) | 0x80;
}

static void FormatUUIDString(const UCHAR* uuid, char* out, SIZE_T outLen)
{
    RtlStringCbPrintfA(out, outLen,
        "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        uuid[3], uuid[2], uuid[1], uuid[0],
        uuid[5], uuid[4],
        uuid[7], uuid[6],
        uuid[8], uuid[9],
        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}

// ============================================================================
// Read raw SMBIOS data from registry
// ============================================================================

static NTSTATUS ReadSmbiosFromRegistry(PUCHAR* outBuffer, PULONG outLength)
{
    NTSTATUS status;
    HANDLE keyHandle = NULL;
    UNICODE_STRING keyPath;
    UNICODE_STRING valueName;
    OBJECT_ATTRIBUTES objAttr;
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo = NULL;
    ULONG resultLength = 0;

    *outBuffer = NULL;
    *outLength = 0;

    STK_REG_SMBIOS(_smbPath);
    STK_VAL_SMBIOSDATA(_smbVal);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&keyPath, _smbPath);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&valueName, _smbVal);

    InitializeObjectAttributes(&objAttr, &keyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&keyHandle, KEY_READ, &objAttr);
    STK_WIPE_W(_smbPath, 66);  /* wipe path after ZwOpenKey is done with it */
    if (!NT_SUCCESS(status)) {
        STK_WIPE_W(_smbVal, 11);
        SPOOF_DBG("[Spoofer] Schicht4: Cannot open mssmbios key (0x%X)\n", status);
        return status;
    }

    status = ((fn_ZwQueryValueKey)ApiResolveExport(HASH_ZWQUERYVALUEKEY))(keyHandle, &valueName, KeyValuePartialInformation,
        NULL, 0, &resultLength);
    if (status != STATUS_BUFFER_TOO_SMALL && status != STATUS_BUFFER_OVERFLOW) {
        STK_WIPE_W(_smbVal, 11);
        ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(keyHandle);
        return STATUS_NOT_FOUND;
    }

    valueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(
        NonPagedPool, resultLength, g_Spoofer.PoolTag);
    if (!valueInfo) {
        STK_WIPE_W(_smbVal, 11);
        ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(keyHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ((fn_ZwQueryValueKey)ApiResolveExport(HASH_ZWQUERYVALUEKEY))(keyHandle, &valueName, KeyValuePartialInformation,
        valueInfo, resultLength, &resultLength);
    STK_WIPE_W(_smbVal, 11);  /* wipe value name after last ZwQueryValueKey */
    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(keyHandle);

    if (!NT_SUCCESS(status)) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(valueInfo, g_Spoofer.PoolTag);
        return status;
    }

    *outLength = valueInfo->DataLength;
    *outBuffer = (PUCHAR)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, *outLength, g_Spoofer.PoolTag);
    if (!*outBuffer) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(valueInfo, g_Spoofer.PoolTag);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(*outBuffer, valueInfo->Data, *outLength);
    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(valueInfo, g_Spoofer.PoolTag);

    SPOOF_DBG("[Spoofer] Schicht4: Read SMBiosData (%lu bytes)\n", *outLength);
    return STATUS_SUCCESS;
}

// ============================================================================
// Get ntoskrnl base address via ZwQuerySystemInformation
// ============================================================================

static PVOID GetNtoskrnlBase(PULONG outSize)
{
    NTSTATUS status;
    ULONG bufSize = 0;
    PRTL_PROCESS_MODULES modules = NULL;
    PVOID base = NULL;

    *outSize = 0;

    ((fn_ZwQuerySystemInformation)ApiResolveExport(HASH_ZWQUERYSYSTEMINFORMATION))(SystemModuleInformation, NULL, 0, &bufSize);
    if (bufSize == 0) return NULL;

    modules = (PRTL_PROCESS_MODULES)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, bufSize, g_Spoofer.PoolTag);
    if (!modules) return NULL;

    status = ((fn_ZwQuerySystemInformation)ApiResolveExport(HASH_ZWQUERYSYSTEMINFORMATION))(SystemModuleInformation, modules, bufSize, &bufSize);
    if (NT_SUCCESS(status) && modules->NumberOfModules > 0) {
        base = modules->Modules[0].ImageBase;
        *outSize = modules->Modules[0].ImageSize;
        SPOOF_DBG("[Spoofer] Schicht4: ntoskrnl at %p, size=%lu\n", base, *outSize);
    }

    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modules, g_Spoofer.PoolTag);
    return base;
}

// ============================================================================
// Kernel SMBIOS buffer — PDB-offset based approach
//
// Client resolves WmipSMBiosTablePhysicalAddress + WmipSMBiosTableLength
// offsets from ntoskrnl PDB. Driver uses offset + ntoskrnl base to read
// the physical address, then maps it with MmMapIoSpace to patch in-place.
//
// Both the kernel's mapping and ours point to the SAME physical pages,
// so patching through our mapping is visible to NtQuerySystemInformation.
// ============================================================================

static PVOID g_MappedSmbiosVA = NULL;
static ULONG g_MappedSmbiosLen = 0;

static PVOID FindKernelSmbiosBuffer(PULONG outTableLen)
{
    PVOID ntBase = NULL;
    ULONG ntSize = 0;
    PHYSICAL_ADDRESS physAddr;
    ULONG tableLen;
    PVOID mapped;

    *outTableLen = 0;

    if (!g_Spoofer.OffsetsProvided) {
        SPOOF_DBG("[Spoofer] Schicht4: No PDB offsets — kernel patch skipped\n");
        return NULL;
    }

    ntBase = GetNtoskrnlBase(&ntSize);
    if (!ntBase) {
        SPOOF_DBG("[Spoofer] Schicht4: Cannot find ntoskrnl\n");
        return NULL;
    }

    // Validate offsets are within ntoskrnl image
    if (g_Spoofer.SmbiosPhysAddrOffset >= ntSize ||
        g_Spoofer.SmbiosTableLenOffset >= ntSize) {
        SPOOF_DBG("[Spoofer] Schicht4: Offsets out of range\n");
        return NULL;
    }

    __try {
        // Read the PHYSICAL_ADDRESS from ntoskrnl + offset
        physAddr = *(PHYSICAL_ADDRESS*)((PUCHAR)ntBase + g_Spoofer.SmbiosPhysAddrOffset);
        tableLen = *(ULONG*)((PUCHAR)ntBase + g_Spoofer.SmbiosTableLenOffset);

        SPOOF_DBG("[Spoofer] Schicht4: PhysAddr=%llX, TableLen=%lu\n",
            physAddr.QuadPart, tableLen);

        if (physAddr.QuadPart == 0 || tableLen == 0 || tableLen > 0x100000) {
            SPOOF_DBG("[Spoofer] Schicht4: Invalid phys addr or length\n");
            return NULL;
        }

        // Map the physical SMBIOS table into our virtual address space
        mapped = ((fn_MmMapIoSpace)ApiResolveExport(HASH_MMMAPIOSPACE))(physAddr, tableLen, MmCached);
        if (!mapped) {
            SPOOF_DBG("[Spoofer] Schicht4: MmMapIoSpace failed\n");
            return NULL;
        }

        SPOOF_DBG("[Spoofer] Schicht4: Mapped SMBIOS phys %llX -> virt %p (%lu bytes)\n",
            physAddr.QuadPart, mapped, tableLen);

        g_MappedSmbiosVA = mapped;
        g_MappedSmbiosLen = tableLen;
        *outTableLen = tableLen;
        return mapped;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Schicht4: Exception reading offsets\n");
        return NULL;
    }
}

// ============================================================================
// Patch SMBIOS structures in a buffer
// ============================================================================

static void PatchSmbiosStructures(PUCHAR tableData, ULONG tableLength, const UINT8* seed)
{
    PUCHAR current = tableData;
    PUCHAR end = tableData + tableLength;
    ULONG structCount = 0;

    while (current + sizeof(SMBIOS_HEADER) < end) {
        PSMBIOS_HEADER hdr = (PSMBIOS_HEADER)current;
        PUCHAR nextStruct;

        if (hdr->Type == SMBIOS_TYPE_END) break;
        if (hdr->Length < sizeof(SMBIOS_HEADER)) break;
        if (current + hdr->Length >= end) break;

        nextStruct = NextSmbiosStruct(current);
        if (nextStruct <= current || nextStruct > end) break;

        switch (hdr->Type) {
        case SMBIOS_TYPE_SYSTEM:
            if (hdr->Length >= 0x19) {
                UCHAR serialIdx = current[7];
                PCHAR serial = GetSmbiosString(current, hdr->Length, serialIdx);
                if (serial) {
                    SIZE_T slen = SafeStrLen(serial, 127);
                    if (!IsDefaultString(serial, slen)) {
                        SPOOF_DBG("[Spoofer] Schicht4: System Serial='%.*s'\n", (int)slen, serial);
                        SpoofSerialString(serial, slen + 1, seed, E("SMBIOS_SYS"));
                        RtlCopyMemory(g_SmbiosSystemSerial, serial, slen < 127 ? slen : 127);
                    }
                }
                {
                    PUCHAR uuid = current + 8;
                    BOOLEAN allZero = TRUE, allFF = TRUE;
                    ULONG u;
                    for (u = 0; u < 16; u++) {
                        if (uuid[u] != 0x00) allZero = FALSE;
                        if (uuid[u] != 0xFF) allFF = FALSE;
                    }
                    if (!allZero && !allFF) {
                        SPOOF_DBG("[Spoofer] Schicht4: Spoofing UUID\n");
                        GenerateSpoofedUUID(uuid, seed);
                        RtlCopyMemory(g_SmbiosUUIDBytes, uuid, 16);
                        FormatUUIDString(uuid, g_SmbiosSystemUUID, sizeof(g_SmbiosSystemUUID));
                        SPOOF_DBG("[Spoofer] Schicht4: UUID=%s\n", g_SmbiosSystemUUID);
                    }
                }
            }
            break;

        case SMBIOS_TYPE_BASEBOARD:
            if (hdr->Length >= 0x08) {
                UCHAR serialIdx = current[7];
                PCHAR serial = GetSmbiosString(current, hdr->Length, serialIdx);
                if (serial) {
                    SIZE_T slen = SafeStrLen(serial, 127);
                    if (!IsDefaultString(serial, slen)) {
                        SPOOF_DBG("[Spoofer] Schicht4: BB Serial='%.*s'\n", (int)slen, serial);
                        SpoofSerialString(serial, slen + 1, seed, E("SMBIOS_BB"));
                        RtlCopyMemory(g_SmbiosBaseBoardSerial, serial, slen < 127 ? slen : 127);
                    }
                }
            }
            break;

        case SMBIOS_TYPE_CHASSIS:
            if (hdr->Length >= 0x09) {
                UCHAR serialIdx = current[7];
                PCHAR serial = GetSmbiosString(current, hdr->Length, serialIdx);
                if (serial) {
                    SIZE_T slen = SafeStrLen(serial, 127);
                    if (!IsDefaultString(serial, slen)) {
                        SPOOF_DBG("[Spoofer] Schicht4: Chassis Serial='%.*s'\n", (int)slen, serial);
                        SpoofSerialString(serial, slen + 1, seed, E("SMBIOS_CH"));
                        RtlCopyMemory(g_SmbiosChassisSerial, serial, slen < 127 ? slen : 127);
                    }
                }
            }
            break;

        /* Type 4 — Processor Information: serial at string index [0x20] */
        case 4:
            if (hdr->Length >= 0x21) {
                UCHAR serialIdx = current[0x20];
                if (serialIdx != 0) {
                    PCHAR serial = GetSmbiosString(current, hdr->Length, serialIdx);
                    if (serial) {
                        SIZE_T slen = SafeStrLen(serial, 127);
                        if (!IsDefaultString(serial, slen)) {
                            SPOOF_DBG("[Spoofer] Schicht4: Processor Serial='%.*s'\n", (int)slen, serial);
                            SpoofSerialString(serial, slen + 1, seed, E("SMBIOS_CPU"));
                        }
                    }
                }
            }
            break;

        /* Type 17 — Memory Device: serial at [0x18], part number at [0x1A] */
        case 17:
            if (hdr->Length >= 0x1B) {
                /* Serial Number (string index at offset 0x18) */
                UCHAR serialIdx = current[0x18];
                if (serialIdx != 0) {
                    PCHAR serial = GetSmbiosString(current, hdr->Length, serialIdx);
                    if (serial) {
                        SIZE_T slen = SafeStrLen(serial, 127);
                        if (!IsDefaultString(serial, slen)) {
                            SPOOF_DBG("[Spoofer] Schicht4: RAM Serial='%.*s'\n", (int)slen, serial);
                            SpoofSerialString(serial, slen + 1, seed, E("SMBIOS_RAM"));
                        }
                    }
                }
                /* Part Number (string index at offset 0x1A) */
                {
                    UCHAR partIdx = current[0x1A];
                    if (partIdx != 0) {
                        PCHAR part = GetSmbiosString(current, hdr->Length, partIdx);
                        if (part) {
                            SIZE_T plen = SafeStrLen(part, 127);
                            if (!IsDefaultString(part, plen)) {
                                SPOOF_DBG("[Spoofer] Schicht4: RAM Part='%.*s'\n", (int)plen, part);
                                SpoofSerialString(part, plen + 1, seed, E("SMBIOS_RAMP"));
                            }
                        }
                    }
                }
            }
            break;
        }

        current = nextStruct;
        structCount++;
        if (structCount > 256) break;
    }
}

// ============================================================================
// Public API
// ============================================================================

NTSTATUS PatchSmbiosTable(const UINT8* seed)
{
    NTSTATUS status;
    PUCHAR rawData = NULL;
    ULONG rawLength = 0;

    SPOOF_DBG("[Spoofer] ============ SCHICHT 4: SMBIOS Patch ============\n");

    status = ReadSmbiosFromRegistry(&rawData, &rawLength);
    if (!NT_SUCCESS(status)) return status;

    if (rawLength < sizeof(RAW_SMBIOS_DATA)) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(rawData, g_Spoofer.PoolTag);
        return STATUS_DATA_ERROR;
    }

    {
        PRAW_SMBIOS_DATA smbios = (PRAW_SMBIOS_DATA)rawData;
        ULONG tableLength = smbios->Length;
        PUCHAR tableData = smbios->SMBIOSTableData;

        SPOOF_DBG("[Spoofer] Schicht4: SMBIOS v%u.%u, table=%lu bytes\n",
            smbios->SMBIOSMajorVersion, smbios->SMBIOSMinorVersion, tableLength);

        if (tableLength > rawLength - 8)
            tableLength = rawLength - 8;

        // === Layer 2: Find kernel's physical SMBIOS buffer via PDB offsets ===
        {
            ULONG kernelTableLen = 0;
            PVOID kernelBuf = FindKernelSmbiosBuffer(&kernelTableLen);

            if (kernelBuf && kernelTableLen > 0) {
                g_OriginalTableLen = kernelTableLen;
                g_OriginalTableData = (PUCHAR)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(
                    NonPagedPool, kernelTableLen, g_Spoofer.PoolTag);
                if (g_OriginalTableData) {
                    RtlCopyMemory(g_OriginalTableData, kernelBuf, kernelTableLen);
                }
                SPOOF_DBG("[Spoofer] Schicht4: Patching kernel SMBIOS at %p\n", kernelBuf);
                __try {
                    PatchSmbiosStructures((PUCHAR)kernelBuf, kernelTableLen, seed);
                    SPOOF_DBG("[Spoofer] Schicht4: Kernel SMBIOS patched OK\n");
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    SPOOF_DBG("[Spoofer] Schicht4: Kernel SMBIOS patch EXCEPTION\n");
                }
            }
        }

        // === Layer 1: Patch registry copy (for registry callback) ===
        PatchSmbiosStructures(tableData, tableLength, seed);
    }

    g_SpoofedSmbiosRaw = rawData;
    g_SpoofedSmbiosLen = rawLength;
    g_SmbiosPatched = TRUE;

    SPOOF_DBG("[Spoofer] Schicht4: SMBIOS patch complete\n");
    return STATUS_SUCCESS;
}

void RestoreSmbiosTable(void)
{
    if (g_MappedSmbiosVA && g_OriginalTableData && g_OriginalTableLen > 0) {
        __try {
            RtlCopyMemory(g_MappedSmbiosVA, g_OriginalTableData, g_OriginalTableLen);
            SPOOF_DBG("[Spoofer] Schicht4: Kernel SMBIOS restored\n");
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SPOOF_DBG("[Spoofer] Schicht4: Kernel restore failed\n");
        }
    }

    if (g_MappedSmbiosVA && g_MappedSmbiosLen > 0) {
        ((fn_MmUnmapIoSpace)ApiResolveExport(HASH_MMUNMAPIOSPACE))(g_MappedSmbiosVA, g_MappedSmbiosLen);
        g_MappedSmbiosVA = NULL;
        g_MappedSmbiosLen = 0;
    }

    if (g_OriginalTableData) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(g_OriginalTableData, g_Spoofer.PoolTag);
        g_OriginalTableData = NULL;
        g_OriginalTableLen = 0;
    }

    if (g_SpoofedSmbiosRaw) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(g_SpoofedSmbiosRaw, g_Spoofer.PoolTag);
        g_SpoofedSmbiosRaw = NULL;
        g_SpoofedSmbiosLen = 0;
    }

    g_SmbiosPatched = FALSE;
    SPOOF_DBG("[Spoofer] Schicht4: SMBIOS data freed\n");
}

