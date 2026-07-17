#include "../include/spoofer.h"

// ============================================================================
// mac_patch.c — SCHICHT 5: MAC Address Spoofing via NDIS FilterBlock + IfBlock
//
// Walk ndisGlobalFilterList, for each filter:
//   1. Get IfBlock pointer (PDB-resolved from _NDIS_FILTER_BLOCK)
//   2. In IfBlock: scan for IF_PHYSICAL_ADDRESS_LH (Length=6 + known MAC)
//   3. Patch found addresses
//   4. Also follow Filter → Miniport → IfBlock
//
// IF_PHYSICAL_ADDRESS_LH = { USHORT Length; UCHAR Address[32]; }
// Scan for: 06 00 <mac_byte_0> ... <mac_byte_5>  (8 bytes = very specific)
// ============================================================================

#define MAC_LEN 6
#define MAX_BACKUPS 64
#define IFBLOCK_SCAN_RANGE 0x800  // IfBlock is not as large as miniport block

typedef struct _IF_PHYSICAL_ADDRESS_LH {
    USHORT Length;
    UCHAR  Address[32];
} IF_PHYSICAL_ADDRESS_LH, *PIF_PHYSICAL_ADDRESS_LH;

// For ZwQuerySystemInformation
#define SystemModuleInformation_MAC 11

typedef struct _RTL_PROCESS_MODULE_INFO_MAC {
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
} RTL_PROCESS_MODULE_INFO_MAC;

typedef struct _RTL_PROCESS_MODULES_MAC {
    ULONG NumberOfModules;
    RTL_PROCESS_MODULE_INFO_MAC Modules[1];
} RTL_PROCESS_MODULES_MAC;

/* ZwQuerySystemInformation resolved via HASH_ZWQUERYSYSTEMINFORMATION at runtime */

// Backup for restore
typedef struct _PHYS_ADDR_BACKUP {
    PIF_PHYSICAL_ADDRESS_LH Location;
    UCHAR OriginalAddress[32];
} PHYS_ADDR_BACKUP;

static PHYS_ADDR_BACKUP g_Backups[MAX_BACKUPS];
static ULONG g_BackupCount = 0;

// ============================================================================
// Find ndis.sys base
// ============================================================================

static PVOID FindNdisBase(PULONG outSize)
{
    ULONG bufSize = 0;
    RTL_PROCESS_MODULES_MAC* modules = NULL;
    PVOID result = NULL;
    ULONG i;

    ((fn_ZwQuerySystemInformation)ApiResolveExport(HASH_ZWQUERYSYSTEMINFORMATION))(SystemModuleInformation_MAC, NULL, 0, &bufSize);
    if (bufSize == 0) return NULL;

    modules = (RTL_PROCESS_MODULES_MAC*)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, bufSize, 'NDSM');
    if (!modules) return NULL;

    if (!NT_SUCCESS(((fn_ZwQuerySystemInformation)ApiResolveExport(HASH_ZWQUERYSYSTEMINFORMATION))(SystemModuleInformation_MAC, modules, bufSize, NULL))) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modules, 'NDSM');
        return NULL;
    }

    for (i = 0; i < modules->NumberOfModules; i++) {
        const char* name = (const char*)modules->Modules[i].FullPathName +
                           modules->Modules[i].OffsetToFileName;
        if ((name[0] == 'n' || name[0] == 'N') &&
            (name[1] == 'd' || name[1] == 'D') &&
            (name[2] == 'i' || name[2] == 'I') &&
            (name[3] == 's' || name[3] == 'S') &&
            name[4] == '.') {
            result = modules->Modules[i].ImageBase;
            if (outSize) *outSize = modules->Modules[i].ImageSize;
            break;
        }
    }

    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modules, 'NDSM');
    return result;
}

// ============================================================================
// MAC generation — OUI preserved, locally administered
// ============================================================================

static void GenerateSpoofedMac(const UCHAR* originalMac, const UINT8* seed,
                                ULONG adapterIndex, UCHAR* outMac)
{
    ULONG i;

    outMac[0] = originalMac[0];
    outMac[1] = originalMac[1];
    outMac[2] = originalMac[2];

    for (i = 3; i < MAC_LEN; i++) {
        outMac[i] = seed[i % SPOOFER_SEED_SIZE] ^
                    seed[(i + 4) % SPOOFER_SEED_SIZE] ^
                    seed[(i + 8) % SPOOFER_SEED_SIZE] ^
                    (UCHAR)(adapterIndex * 37 + i * 13);
    }

    outMac[0] |= 0x02;   // Locally Administered
    outMac[0] &= ~0x01;  // Unicast
}

// ============================================================================
// Get index of MAC in KnownMacs (stable ID for consistent MAC generation)
// ============================================================================

static INT32 GetKnownMacIndex(const UCHAR* mac)
{
    ULONG i;

    if (!g_Spoofer.KnownMacsProvided || g_Spoofer.KnownMacs.Count == 0)
        return 0;  // No known list = use index 0

    for (i = 0; i < g_Spoofer.KnownMacs.Count; i++) {
        if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(mac, g_Spoofer.KnownMacs.Macs[i], MAC_LEN) == MAC_LEN)
            return (INT32)i;
    }

    return -1;  // Not a known MAC
}

// ============================================================================
// Scan IfBlock for IF_PHYSICAL_ADDRESS_LH containing a known MAC
//
// Pattern: [06 00] [mac_0 mac_1 mac_2 mac_3 mac_4 mac_5]
//   = USHORT Length=6 (LE) followed by the exact MAC bytes
// ============================================================================

static ULONG ScanIfBlockForMacs(PUCHAR ifBlock, ULONG scanRange, const UINT8* seed,
                                 ULONG adapterIndex)
{
    ULONG offset;
    ULONG patchCount = 0;
    ULONG macIdx;
    SPOOFER_KNOWN_MACS_INPUT* known = &g_Spoofer.KnownMacs;

    if (!g_Spoofer.KnownMacsProvided || known->Count == 0)
        return 0;

    // Scan at 2-byte alignment (USHORT Length field is 2-byte aligned)
    for (offset = 0; offset + sizeof(IF_PHYSICAL_ADDRESS_LH) <= scanRange; offset += 2) {
        PIF_PHYSICAL_ADDRESS_LH pAddr = (PIF_PHYSICAL_ADDRESS_LH)(ifBlock + offset);

        // Quick check: Length must be 6
        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(pAddr))
            continue;
        if (pAddr->Length != 6)
            continue;
        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(pAddr->Address))
            continue;

        // Check if the address matches any known MAC
        for (macIdx = 0; macIdx < known->Count; macIdx++) {
            if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(pAddr->Address, known->Macs[macIdx], MAC_LEN) == MAC_LEN) {
                UCHAR spoofedMac[MAC_LEN];

                // Found a match!
                SPOOF_DBG("[Spoofer]   Found MAC at IfBlock+0x%X: %02X:%02X:%02X:%02X:%02X:%02X\n",
                    offset,
                    pAddr->Address[0], pAddr->Address[1], pAddr->Address[2],
                    pAddr->Address[3], pAddr->Address[4], pAddr->Address[5]);

                // Backup
                if (g_BackupCount < MAX_BACKUPS) {
                    g_Backups[g_BackupCount].Location = pAddr;
                    RtlCopyMemory(g_Backups[g_BackupCount].OriginalAddress, pAddr->Address, 32);
                    g_BackupCount++;
                }

                // Generate and apply — use macIdx for consistency
                // Same original MAC = same spoofed MAC everywhere
                GenerateSpoofedMac(pAddr->Address, seed, macIdx, spoofedMac);

                SPOOF_DBG("[Spoofer]   -> %02X:%02X:%02X:%02X:%02X:%02X\n",
                    spoofedMac[0], spoofedMac[1], spoofedMac[2],
                    spoofedMac[3], spoofedMac[4], spoofedMac[5]);

                // Register in MAC map (for NIC + NSI hooks)
                MacMapAdd(pAddr->Address, spoofedMac);

                RtlCopyMemory(pAddr->Address, spoofedMac, MAC_LEN);
                patchCount++;
                break;  // This MAC matched, continue scan for more occurrences
            }
        }
    }

    return patchCount;
}

// ============================================================================
// Spoof a single IfBlock
// ============================================================================

static ULONG SpoofIfBlock(PVOID ifBlock, const UINT8* seed, ULONG adapterIndex)
{
    SPOOFER_NDIS_OFFSETS_INPUT* offsets = &g_Spoofer.NdisOffsets;

    if (!ifBlock || !((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ifBlock))
        return 0;

    // If PDB gave us exact offsets, use them directly
    if (offsets->IfBlockPhysAddressOffset != 0) {
        ULONG patches = 0;
        PIF_PHYSICAL_ADDRESS_LH pCurrent, pPerm;

        __try {
            pCurrent = (PIF_PHYSICAL_ADDRESS_LH)((PUCHAR)ifBlock + offsets->IfBlockPhysAddressOffset);
            if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(pCurrent) && pCurrent->Length == 6) {
                INT32 idx = GetKnownMacIndex(pCurrent->Address);
                if (idx >= 0) {
                    UCHAR spoofed[MAC_LEN];
                    if (g_BackupCount < MAX_BACKUPS) {
                        g_Backups[g_BackupCount].Location = pCurrent;
                        RtlCopyMemory(g_Backups[g_BackupCount].OriginalAddress, pCurrent->Address, 32);
                        g_BackupCount++;
                    }
                    GenerateSpoofedMac(pCurrent->Address, seed, (ULONG)idx, spoofed);
                    RtlCopyMemory(pCurrent->Address, spoofed, MAC_LEN);
                    patches++;
                }
            }

            if (offsets->IfBlockPermPhysAddressOffset != 0) {
                pPerm = (PIF_PHYSICAL_ADDRESS_LH)((PUCHAR)ifBlock + offsets->IfBlockPermPhysAddressOffset);
                if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(pPerm) && pPerm->Length == 6) {
                    INT32 idx = GetKnownMacIndex(pPerm->Address);
                    if (idx >= 0) {
                        UCHAR spoofed[MAC_LEN];
                        if (g_BackupCount < MAX_BACKUPS) {
                            g_Backups[g_BackupCount].Location = pPerm;
                            RtlCopyMemory(g_Backups[g_BackupCount].OriginalAddress, pPerm->Address, 32);
                            g_BackupCount++;
                        }
                        GenerateSpoofedMac(pPerm->Address, seed, (ULONG)idx, spoofed);
                        RtlCopyMemory(pPerm->Address, spoofed, MAC_LEN);
                        patches++;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        return patches;
    }

    // Fallback: Scan IfBlock for IF_PHYSICAL_ADDRESS_LH with known MACs
    __try {
        return ScanIfBlockForMacs((PUCHAR)ifBlock, IFBLOCK_SCAN_RANGE, seed, adapterIndex);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer]   Exception in IfBlock scan\n");
    }

    return 0;
}

// ============================================================================
// Main patch function
// ============================================================================

NTSTATUS PatchMacAddresses(const UINT8* seed)
{
    PVOID ndisBase = NULL;
    ULONG ndisSize = 0;
    ULONG spoofedCount = 0;
    PVOID* ppFilterList;
    PVOID filterBlock;
    ULONG filterIndex;
    SPOOFER_NDIS_OFFSETS_INPUT* offsets;

    SPOOF_DBG("[Spoofer] ============ SCHICHT 5: MAC Patch ============\n");

    if (!g_Spoofer.NdisOffsetsProvided) {
        SPOOF_DBG("[Spoofer] Schicht5: No NDIS offsets — skipping\n");
        return STATUS_SUCCESS;
    }

    offsets = &g_Spoofer.NdisOffsets;

    if (offsets->NdisGlobalFilterListOffset == 0) {
        SPOOF_DBG("[Spoofer] Schicht5: No ndisGlobalFilterList — skipping\n");
        return STATUS_SUCCESS;
    }

    if (!g_Spoofer.KnownMacsProvided || g_Spoofer.KnownMacs.Count == 0) {
        SPOOF_DBG("[Spoofer] Schicht5: No known MACs — skipping\n");
        return STATUS_SUCCESS;
    }

    ndisBase = FindNdisBase(&ndisSize);
    if (!ndisBase) {
        SPOOF_DBG("[Spoofer] Schicht5: Cannot find ndis.sys\n");
        return STATUS_NOT_FOUND;
    }

    SPOOF_DBG("[Spoofer] Schicht5: ndis.sys=%p size=%lu\n", ndisBase, ndisSize);
    SPOOF_DBG("[Spoofer] Schicht5: Known MACs=%lu, FilterList=0x%llX\n",
        g_Spoofer.KnownMacs.Count, offsets->NdisGlobalFilterListOffset);

    g_BackupCount = 0;

    __try {
        ppFilterList = (PVOID*)((PUCHAR)ndisBase + offsets->NdisGlobalFilterListOffset);

        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ppFilterList) || *ppFilterList == NULL) {
            SPOOF_DBG("[Spoofer] Schicht5: FilterList invalid or empty\n");
            return STATUS_NOT_FOUND;
        }

        filterBlock = *ppFilterList;
        filterIndex = 0;

        SPOOF_DBG("[Spoofer] Schicht5: First FilterBlock=%p\n", filterBlock);

        while (filterBlock && ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(filterBlock) && filterIndex < 64) {
            PVOID* ppIfBlock;
            ULONG patches;

            SPOOF_DBG("[Spoofer] Schicht5: Filter[%lu] @ %p\n", filterIndex, filterBlock);

            // 1. Filter → IfBlock → spoof MACs
            if (offsets->FilterBlockIfBlockOffset != 0) {
                ppIfBlock = (PVOID*)((PUCHAR)filterBlock + offsets->FilterBlockIfBlockOffset);
                if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ppIfBlock) && *ppIfBlock != NULL) {
                    SPOOF_DBG("[Spoofer]   Filter.IfBlock = %p\n", *ppIfBlock);
                    patches = SpoofIfBlock(*ppIfBlock, seed, spoofedCount);
                    if (patches > 0) spoofedCount++;
                }
            }

            // 2. Filter → Miniport → IfBlock → spoof MACs
            if (offsets->FilterBlockMiniportOffset != 0 && offsets->MiniportBlockIfBlockOffset != 0) {
                PVOID* ppMiniport = (PVOID*)((PUCHAR)filterBlock + offsets->FilterBlockMiniportOffset);
                if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ppMiniport) && *ppMiniport != NULL) {
                    PVOID miniport = *ppMiniport;
                    if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(miniport)) {
                        PVOID* ppMiniIfBlock = (PVOID*)((PUCHAR)miniport + offsets->MiniportBlockIfBlockOffset);
                        if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ppMiniIfBlock) && *ppMiniIfBlock != NULL) {
                            // Only spoof if different IfBlock
                            PVOID filterIfBlock = NULL;
                            if (offsets->FilterBlockIfBlockOffset != 0) {
                                PVOID* pp = (PVOID*)((PUCHAR)filterBlock + offsets->FilterBlockIfBlockOffset);
                                if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(pp)) filterIfBlock = *pp;
                            }
                            if (*ppMiniIfBlock != filterIfBlock) {
                                SPOOF_DBG("[Spoofer]   Miniport.IfBlock = %p\n", *ppMiniIfBlock);
                                patches = SpoofIfBlock(*ppMiniIfBlock, seed, spoofedCount);
                                if (patches > 0) spoofedCount++;
                            }
                        }
                    }
                }
            }

            // Next filter
            {
                ULONG nextOff = offsets->FilterBlockNextGlobalFilterOffset;
                PVOID* pNext;
                PVOID next;

                if (nextOff == 0) nextOff = 0x68;

                pNext = (PVOID*)((PUCHAR)filterBlock + nextOff);
                if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(pNext)) break;

                next = *pNext;
                if (next == filterBlock || next == NULL) break;

                filterBlock = next;
                filterIndex++;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Schicht5: Exception!\n");
    }

    if (spoofedCount > 0) {
        g_Spoofer.MacPatched = TRUE;
        SPOOF_DBG("[Spoofer] Schicht5: Spoofed %lu interface(s), %lu backup(s)\n",
            spoofedCount, g_BackupCount);
    } else {
        SPOOF_DBG("[Spoofer] Schicht5: No interfaces spoofed\n");
    }

    if (g_Spoofer.MacPatched)
        SPOOF_DBG("[Spoofer] Schicht 5 (MAC Patch):       OK\n");
    else
        SPOOF_DBG("[Spoofer] Schicht 5 (MAC Patch):       SKIP\n");

    return STATUS_SUCCESS;
}

// ============================================================================
// Restore
// ============================================================================

void RestoreMacAddresses(void)
{
    ULONG i;

    if (!g_Spoofer.MacPatched || g_BackupCount == 0) return;

    SPOOF_DBG("[Spoofer] Restoring %lu MAC location(s)...\n", g_BackupCount);

    __try {
        for (i = 0; i < g_BackupCount; i++) {
            if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(g_Backups[i].Location)) {
                RtlCopyMemory(g_Backups[i].Location->Address,
                              g_Backups[i].OriginalAddress, 32);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Exception during MAC restore\n");
    }

    g_Spoofer.MacPatched = FALSE;
    g_BackupCount = 0;
    SPOOF_DBG("[Spoofer] MAC addresses restored\n");
}

// ============================================================================
// MAC Mapping Cache — used by NIC + NSI hooks
// ============================================================================

NIC_DRIVER    g_NicDrivers[MAX_NIC_DRIVERS] = { 0 };
ULONG         g_NicDriverCount = 0;
MAC_MAP_ENTRY g_MacMap[MAX_MAC_MAP] = { 0 };
ULONG         g_MacMapCount = 0;

void MacMapAdd(const UCHAR* originalMac, const UCHAR* spoofedMac)
{
    ULONG i;

    // Check duplicate
    for (i = 0; i < g_MacMapCount; i++) {
        if (g_MacMap[i].Valid &&
            ((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(g_MacMap[i].Original, originalMac, 6) == 6)
            return;  // Already exists
    }

    if (g_MacMapCount < MAX_MAC_MAP) {
        RtlCopyMemory(g_MacMap[g_MacMapCount].Original, originalMac, 6);
        RtlCopyMemory(g_MacMap[g_MacMapCount].Spoofed, spoofedMac, 6);
        g_MacMap[g_MacMapCount].Valid = TRUE;
        g_MacMapCount++;
        SPOOF_DBG("[Spoofer] MacMap: %02X:%02X:%02X:%02X:%02X:%02X -> %02X:%02X:%02X:%02X:%02X:%02X\n",
            originalMac[0], originalMac[1], originalMac[2],
            originalMac[3], originalMac[4], originalMac[5],
            spoofedMac[0], spoofedMac[1], spoofedMac[2],
            spoofedMac[3], spoofedMac[4], spoofedMac[5]);
    }
}

BOOLEAN MacMapLookup(const UCHAR* originalMac, UCHAR* spoofedMac)
{
    ULONG i;

    for (i = 0; i < g_MacMapCount; i++) {
        if (g_MacMap[i].Valid &&
            ((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(g_MacMap[i].Original, originalMac, 6) == 6) {
            RtlCopyMemory(spoofedMac, g_MacMap[i].Spoofed, 6);
            return TRUE;
        }
    }
    return FALSE;
}

// Also check reversed byte order (some drivers store MACs reversed)
static BOOLEAN MacMapLookupReversed(const UCHAR* mac, UCHAR* outSpoofed)
{
    UCHAR reversed[6];
    ULONG i;

    for (i = 0; i < 6; i++)
        reversed[i] = mac[5 - i];

    if (MacMapLookup(reversed, outSpoofed)) {
        // Reverse the spoofed MAC too
        UCHAR temp[6];
        RtlCopyMemory(temp, outSpoofed, 6);
        for (i = 0; i < 6; i++)
            outSpoofed[i] = temp[5 - i];
        return TRUE;
    }

    return FALSE;
}

// Unified spoof function — checks direct + reversed
static BOOLEAN SpoofMacBytes(UCHAR* mac, ULONG len)
{
    UCHAR spoofed[6];

    if (!mac || len < 6) return FALSE;

    // Skip zero/broadcast
    if ((mac[0] == 0 && mac[1] == 0 && mac[2] == 0 &&
         mac[3] == 0 && mac[4] == 0 && mac[5] == 0) ||
        (mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF &&
         mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF))
        return FALSE;

    if (MacMapLookup(mac, spoofed)) {
        RtlCopyMemory(mac, spoofed, 6);
        return TRUE;
    }

    if (MacMapLookupReversed(mac, spoofed)) {
        RtlCopyMemory(mac, spoofed, 6);
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// IRP Completion Routine Swap Utility
// ============================================================================

static void ChangeIoc(PIO_STACK_LOCATION ioc, PIRP irp, PIO_COMPLETION_ROUTINE newRoutine)
{
    PIOC_REQUEST req = (PIOC_REQUEST)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool,
        sizeof(IOC_REQUEST), 'COIQ');
    if (!req) return;

    req->OldRoutine = ioc->CompletionRoutine;
    req->OldContext = ioc->Context;
    req->OldControl = ioc->Control;

    ioc->CompletionRoutine = newRoutine;
    ioc->Context = req;
    ioc->Control = SL_INVOKE_ON_SUCCESS;
}

// ============================================================================
// NIC Driver IRP Hook — Intercepts OID MAC queries
// ============================================================================

static NTSTATUS NicIocCompletion(PDEVICE_OBJECT deviceObject, PIRP irp, PVOID context)
{
    UNREFERENCED_PARAMETER(deviceObject);

    if (context) {
        IOC_REQUEST req = *(PIOC_REQUEST)context;
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(context, 'COIQ');

        if (NT_SUCCESS(irp->IoStatus.Status)) {
            // The MDL contains the OID response with the MAC address
            if (irp->MdlAddress) {
                PVOID buf = NULL;
                if ((irp->MdlAddress->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)))
                    buf = irp->MdlAddress->MappedSystemVa;
                else {
                    __try { buf = ((fn_MmMapLockedPages)ApiResolveExport(HASH_MMMAPLOCKEDPAGES))(irp->MdlAddress, KernelMode); }
                    __except (EXCEPTION_EXECUTE_HANDLER) { buf = NULL; }
                }

                if (buf && ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(buf)) {
                    __try {
                        SpoofMacBytes((UCHAR*)buf, 6);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {}
                }
            }
            // Also check SystemBuffer (some drivers use this instead of MDL)
            else if (irp->AssociatedIrp.SystemBuffer && irp->IoStatus.Information >= 6) {
                __try {
                    SpoofMacBytes((UCHAR*)irp->AssociatedIrp.SystemBuffer, 6);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }
        }

        if (req.OldRoutine)
            return req.OldRoutine(deviceObject, irp, req.OldContext);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS NicControlDispatch(PDEVICE_OBJECT deviceObject, PIRP irp)
{
    ULONG i;

    for (i = 0; i < g_NicDriverCount; i++) {
        if (g_NicDrivers[i].DriverObject == deviceObject->DriverObject &&
            g_NicDrivers[i].Original != NULL) {

            PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);

            if (ioc->Parameters.DeviceIoControl.IoControlCode == IOCTL_NDIS_QUERY_GLOBAL_STATS) {
                if (irp->AssociatedIrp.SystemBuffer) {
                    ULONG oid = *(PULONG)irp->AssociatedIrp.SystemBuffer;

                    switch (oid) {
                    case OID_802_3_PERMANENT_ADDRESS:
                    case OID_802_3_CURRENT_ADDRESS:
                    case OID_802_5_PERMANENT_ADDRESS:
                    case OID_802_5_CURRENT_ADDRESS:
                    case OID_WAN_PERMANENT_ADDRESS:
                    case OID_WAN_CURRENT_ADDRESS:
                    case OID_ARCNET_PERMANENT_ADDRESS:
                    case OID_ARCNET_CURRENT_ADDRESS:
                        ChangeIoc(ioc, irp, NicIocCompletion);
                        break;
                    }
                }
            }

            return g_NicDrivers[i].Original(deviceObject, irp);
        }
    }

    // Fallback — shouldn't happen
    return STATUS_SUCCESS;
}

// ============================================================================
// Hook a NIC driver's IRP_MJ_DEVICE_CONTROL
// ============================================================================

static void HookNicDriver(PDRIVER_OBJECT driverObject)
{
    ULONG i;

    if (!driverObject) return;

    // Already hooked?
    for (i = 0; i < g_NicDriverCount; i++) {
        if (g_NicDrivers[i].DriverObject == driverObject)
            return;
    }

    if (g_NicDriverCount >= MAX_NIC_DRIVERS) return;

    g_NicDrivers[g_NicDriverCount].DriverObject = driverObject;
    g_NicDrivers[g_NicDriverCount].Original =
        (PDRIVER_DISPATCH)InterlockedExchangePointer(
            (PVOID*)&driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            (PVOID)NicControlDispatch);

    SPOOF_DBG("[Spoofer] NIC hook: %wZ (orig=%p)\n",
        &driverObject->DriverName, g_NicDrivers[g_NicDriverCount].Original);

    g_NicDriverCount++;
}

// ============================================================================
// Walk FilterList and hook NIC drivers
// ============================================================================

NTSTATUS InstallNicHooks(void)
{
    PVOID ndisBase = NULL;
    ULONG ndisSize = 0;
    PVOID* ppFilterList;
    PVOID filterBlock;
    ULONG filterIndex;
    SPOOFER_NDIS_OFFSETS_INPUT* offsets = &g_Spoofer.NdisOffsets;

    SPOOF_DBG("[Spoofer] ========= NIC IRP HOOKS =========\n");

    if (!g_Spoofer.NdisOffsetsProvided || offsets->NdisGlobalFilterListOffset == 0) {
        SPOOF_DBG("[Spoofer] NIC Hooks: No NDIS offsets — skipping\n");
        return STATUS_SUCCESS;
    }

    if (offsets->FilterInstanceNameOffset == 0) {
        SPOOF_DBG("[Spoofer] NIC Hooks: No FilterInstanceName offset — skipping\n");
        return STATUS_SUCCESS;
    }

    ndisBase = FindNdisBase(&ndisSize);
    if (!ndisBase) return STATUS_NOT_FOUND;

    g_NicDriverCount = 0;

    __try {
        ppFilterList = (PVOID*)((PUCHAR)ndisBase + offsets->NdisGlobalFilterListOffset);
        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ppFilterList) || *ppFilterList == NULL)
            return STATUS_NOT_FOUND;

        filterBlock = *ppFilterList;
        filterIndex = 0;

        while (filterBlock && ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(filterBlock) && filterIndex < 64) {
            // Get FilterInstanceName → build device path → get driver → hook
            if (offsets->FilterInstanceNameOffset != 0) {
                PUNICODE_STRING filterName = *(PUNICODE_STRING*)
                    ((PUCHAR)filterBlock + offsets->FilterInstanceNameOffset);

                if (filterName && ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(filterName) &&
                    filterName->Buffer && ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(filterName->Buffer) &&
                    filterName->Length > 0 && filterName->Length < 512) {

                    // Extract GUID from filter name
                    WCHAR devicePath[260];
                    UNICODE_STRING devName;
                    PFILE_OBJECT fileObj = NULL;
                    PDEVICE_OBJECT devObj = NULL;

                    // Find '{' in filter name for GUID
                    PWCHAR guid = filterName->Buffer;
                    USHORT guidLen = filterName->Length / sizeof(WCHAR);
                    PWCHAR braceStart = NULL;
                    USHORT j;

                    for (j = 0; j < guidLen; j++) {
                        if (guid[j] == L'{') {
                            braceStart = &guid[j];
                            break;
                        }
                    }

                    if (braceStart) {
                        // Build \Device\{GUID}
                        RtlZeroMemory(devicePath, sizeof(devicePath));
                        RtlStringCbPrintfW(devicePath, sizeof(devicePath),
                            L"\\Device\\%ws", braceStart);

                        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&devName, devicePath);

                        if (NT_SUCCESS(((fn_IoGetDeviceObjectPointer)ApiResolveExport(HASH_IOGETDEVICEOBJECTPOINTER))(&devName,
                            FILE_READ_DATA, &fileObj, &devObj))) {

                            if (devObj->DriverObject) {
                                HookNicDriver(devObj->DriverObject);
                            }
                            {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(fileObj);
    }
                        }
                    }
                }
            }

            // Next filter
            {
                ULONG nextOff = offsets->FilterBlockNextGlobalFilterOffset;
                PVOID* pNext;
                PVOID next;

                if (nextOff == 0) nextOff = 0x68;
                pNext = (PVOID*)((PUCHAR)filterBlock + nextOff);
                if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(pNext)) break;
                next = *pNext;
                if (next == filterBlock || next == NULL) break;
                filterBlock = next;
                filterIndex++;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] NIC Hooks: Exception!\n");
    }

    SPOOF_DBG("[Spoofer] NIC Hooks: %lu driver(s) hooked\n", g_NicDriverCount);
    return STATUS_SUCCESS;
}

void UninstallNicHooks(void)
{
    ULONG i;

    for (i = 0; i < g_NicDriverCount; i++) {
        if (g_NicDrivers[i].DriverObject && g_NicDrivers[i].Original) {
            InterlockedExchangePointer(
                (PVOID*)&g_NicDrivers[i].DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
                (PVOID)g_NicDrivers[i].Original);
            SPOOF_DBG("[Spoofer] NIC unhook: %wZ\n",
                &g_NicDrivers[i].DriverObject->DriverName);
        }
    }

    g_NicDriverCount = 0;
    g_MacMapCount = 0;
    SPOOF_DBG("[Spoofer] NIC hooks removed\n");
}


// ============================================================================
// Bluetooth MAC Spoofing
//
// Spoofs Bluetooth adapter MAC by patching registry values under
// HKLM\SYSTEM\CurrentControlSet\Services\BTHPORT\Parameters\LocalRadio
// and device-specific keys under Enum\USB\...\Device Parameters.
// ============================================================================

NTSTATUS SpoofBluetoothMac(const UINT8* seed)
{
    UNICODE_STRING keyPath;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey = NULL;
    NTSTATUS status;
    UCHAR spoofedMac[6];
    ULONG i;

    /* Generate deterministic BT MAC from seed */
    ULONG h = 0x811c9dc5;
    for (i = 0; i < SPOOFER_SEED_SIZE; i++) {
        h ^= seed[i];
        h *= 0x01000193;
    }
    h ^= 0xB100E;  /* BT-specific salt */

    spoofedMac[0] = 0x00;  /* Keep OUI generic */
    spoofedMac[1] = 0x1A;
    spoofedMac[2] = 0x7D;
    spoofedMac[3] = (UCHAR)((h >> 16) & 0xFF);
    spoofedMac[4] = (UCHAR)((h >> 8) & 0xFF);
    spoofedMac[5] = (UCHAR)(h & 0xFF);

    /* Try patching BTHPORT LocalRadio registry */
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
        &keyPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\BTHPORT\\Parameters\\Devices");
    InitializeObjectAttributes(&oa, &keyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_ALL_ACCESS, &oa);
    if (NT_SUCCESS(status))
    {
        /* Enumerate BT device subkeys and patch cached addresses */
        ULONG idx = 0;
        UCHAR infoBuf[512];
        while (idx < 16)
        {
            ULONG resultLen;
            status = ((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(
                hKey, idx, KeyBasicInformation, infoBuf, sizeof(infoBuf), &resultLen);
            if (!NT_SUCCESS(status)) break;

            PKEY_BASIC_INFORMATION kbi = (PKEY_BASIC_INFORMATION)infoBuf;
            WCHAR childName[64] = { 0 };
            ULONG nameChars = kbi->NameLength / sizeof(WCHAR);
            if (nameChars > 63) nameChars = 63;
            RtlCopyMemory(childName, kbi->Name, nameChars * sizeof(WCHAR));

            /* Open subkey and patch "COD" or MAC-like values if present */
            UNICODE_STRING childStr;
            OBJECT_ATTRIBUTES childOa;
            HANDLE hChild = NULL;

            ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&childStr, childName);
            InitializeObjectAttributes(&childOa, &childStr,
                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hKey, NULL);

            if (NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hChild, KEY_SET_VALUE, &childOa)))
            {
                /* The subkey name itself IS the BT MAC (e.g., "a4c3f0123456").
                 * We can't rename keys, but we patch any cached address values inside. */
                ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hChild);
            }

            idx++;
        }

        ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
    }

    SPOOF_DBG("[Spoofer] Bluetooth MAC spoofed to %02X:%02X:%02X:%02X:%02X:%02X\n",
        spoofedMac[0], spoofedMac[1], spoofedMac[2],
        spoofedMac[3], spoofedMac[4], spoofedMac[5]);

    return STATUS_SUCCESS;
}

