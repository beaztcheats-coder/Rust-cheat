#include "../include/spoofer.h"

// ============================================================================
// nsi_hook.c — NSI Proxy ARP Hook
//
// Hooks \Driver\nsiproxy IRP_MJ_DEVICE_CONTROL to intercept
// IOCTL_NSI_PROXY_ARP (0x0012001B) responses. Spoofs ALL MAC addresses
// in the ARP/neighbor table so `arp -a` shows spoofed MACs.
//
// Auto-spoof: MACs not in MacMap get deterministic random replacements
// cached in a local ARP spoof cache.
// ============================================================================

#define IOCTL_NSI_PROXY_ARP     0x0012001B
#define NSI_GET_IP_NET_TABLE    11

// ARP MAC auto-spoof cache (for remote MACs not in MacMap)
#define MAX_ARP_SPOOF 128

typedef struct _ARP_SPOOF_ENTRY {
    UCHAR Original[6];
    UCHAR Spoofed[6];
    BOOLEAN Valid;
} ARP_SPOOF_ENTRY;

static ARP_SPOOF_ENTRY g_ArpCache[MAX_ARP_SPOOF] = { 0 };
static ULONG g_ArpCacheCount = 0;

static PDRIVER_DISPATCH g_NsiOriginal = NULL;
static PDRIVER_OBJECT   g_NsiDriverObject = NULL;

// ============================================================================
// Auto-spoof: generate deterministic MAC for unknown remote MACs
// ============================================================================

static void AutoSpoofMac(UCHAR* mac)
{
    UCHAR spoofed[6];
    ULONG i;

    if (!mac) return;

    // Skip zero
    if ((mac[0] | mac[1] | mac[2] | mac[3] | mac[4] | mac[5]) == 0)
        return;

    // Skip broadcast
    if (mac[0] == 0xFF && mac[1] == 0xFF && mac[2] == 0xFF &&
        mac[3] == 0xFF && mac[4] == 0xFF && mac[5] == 0xFF)
        return;

    // 1. Check MacMap first (local adapter MACs)
    if (MacMapLookup(mac, spoofed)) {
        RtlCopyMemory(mac, spoofed, 6);
        return;
    }

    // 2. Check ARP auto-spoof cache
    for (i = 0; i < g_ArpCacheCount; i++) {
        if (g_ArpCache[i].Valid &&
            ((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(g_ArpCache[i].Original, mac, 6) == 6) {
            RtlCopyMemory(mac, g_ArpCache[i].Spoofed, 6);
            return;
        }
    }

    // 3. Generate new deterministic spoofed MAC
    // Preserve OUI, randomize last 3 bytes using seed + original MAC
    {
        ULONG seed = 0;
        for (i = 0; i < 6; i++)
            seed ^= ((ULONG)mac[i]) << ((i % 4) * 8);

        // Mix with driver seed
        for (i = 0; i < SPOOFER_SEED_SIZE; i++)
            seed ^= ((ULONG)g_Spoofer.Seed[i]) << ((i % 4) * 8);

        spoofed[0] = mac[0];
        spoofed[1] = mac[1];
        spoofed[2] = mac[2];

        for (i = 3; i < 6; i++) {
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
            spoofed[i] = (UCHAR)((seed >> 16) & 0xFF);
        }

        // Set locally administered bit
        spoofed[0] = (spoofed[0] & 0xFE) | 0x02;
    }

    // Cache it
    if (g_ArpCacheCount < MAX_ARP_SPOOF) {
        RtlCopyMemory(g_ArpCache[g_ArpCacheCount].Original, mac, 6);
        RtlCopyMemory(g_ArpCache[g_ArpCacheCount].Spoofed, spoofed, 6);
        g_ArpCache[g_ArpCacheCount].Valid = TRUE;
        g_ArpCacheCount++;
    }

    RtlCopyMemory(mac, spoofed, 6);
}

// ============================================================================
// NSI Completion Routine — spoofs ALL MACs in ARP response
// ============================================================================

static NTSTATUS NsiCompletion(PDEVICE_OBJECT deviceObject, PIRP irp, PVOID context)
{
    UNREFERENCED_PARAMETER(deviceObject);

    if (context) {
        IOC_REQUEST req = *(PIOC_REQUEST)context;
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(context, 'NSIQ');

        if (NT_SUCCESS(irp->IoStatus.Status)) {
            __try {
                char* buf = (char*)irp->UserBuffer;
                if (!buf || !((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(buf))
                    goto _exit;

                // Check NSI request type
                int type = *(int*)(buf + 0x18);
                if (type != NSI_GET_IP_NET_TABLE)
                    goto _exit;

                // Parse table pointers from NSI response structure
                UINT64 tblPtrs[4] = { 0 };
                int tblSizes[4] = { 0 };
                int tblOffs[4] = { 0 };
                int tblCount = 0;
                int off;

                for (off = 0x20; off <= 0x80 && tblCount < 4; off += 8) {
                    UINT64 val = *(UINT64*)(buf + off);
                    if (val > 0x10000 && val < 0x7FFFFFFFFFFF) {
                        int sz = *(int*)(buf + off + 8);
                        if (sz >= 0x10 && sz <= 0x200) {
                            tblPtrs[tblCount] = val;
                            tblSizes[tblCount] = sz;
                            tblOffs[tblCount] = off;
                            tblCount++;
                            off += 8;
                        }
                    }
                }

                if (tblCount < 2)
                    goto _exit;

                // Get entry count
                {
                    int count = 0;
                    int start = tblOffs[tblCount - 1] + 16;
                    int c;

                    for (c = start; c <= start + 16; c += 4) {
                        int v = *(int*)(buf + c);
                        if (v > 0 && v <= 256) {
                            count = v;
                            break;
                        }
                    }

                    if (count <= 0)
                        goto _exit;

                    // Spoof ALL MACs in neighbor entries
                    {
                        char* neighbor = (char*)tblPtrs[1];
                        int entrySize = tblSizes[1];
                        int i;

                        for (i = 0; i < count; i++) {
                            UCHAR* mac = (UCHAR*)(neighbor + (i * entrySize));

                            if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(mac))
                                continue;

                            AutoSpoofMac(mac);
                        }
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                SPOOF_DBG("[Spoofer] NSI: Exception in completion\n");
            }
        }

    _exit:
        if (req.OldRoutine)
            return req.OldRoutine(deviceObject, irp, req.OldContext);
    }

    return STATUS_SUCCESS;
}

// ============================================================================
// NSI Dispatch Hook
// ============================================================================

static NTSTATUS NsiControlDispatch(PDEVICE_OBJECT deviceObject, PIRP irp)
{
    PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);

    if (ioc->Parameters.DeviceIoControl.IoControlCode == IOCTL_NSI_PROXY_ARP) {
        // Swap completion routine
        PIOC_REQUEST req = (PIOC_REQUEST)((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool,
            sizeof(IOC_REQUEST), 'NSIQ');
        if (req) {
            req->OldRoutine = ioc->CompletionRoutine;
            req->OldContext = ioc->Context;
            req->OldControl = ioc->Control;

            ioc->CompletionRoutine = NsiCompletion;
            ioc->Context = req;
            ioc->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
        }
    }

    return g_NsiOriginal(deviceObject, irp);
}

// ============================================================================
// Install / Uninstall
// ============================================================================

NTSTATUS InstallNsiHook(void)
{
    UNICODE_STRING driverName;
    PDRIVER_OBJECT nsiDriver = NULL;
    NTSTATUS status;
    fn_RtlInitUnicodeString pRtlInit;

    SPOOF_DBG("[Spoofer] ========= NSI ARP HOOK =========\n");

    pRtlInit = (fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING);
    if (!pRtlInit) return STATUS_NOT_FOUND;

    /* Stack-encrypted \Driver\nsiproxy */
    {
        STK_DRIVER_NSIPROXY(_nsiName);
        pRtlInit(&driverName, _nsiName);

        status = ResolveObRefByName(&driverName, OBJ_CASE_INSENSITIVE, NULL,
            0, KernelMode, NULL, (PVOID*)&nsiDriver);

        STK_WIPE_W(_nsiName, 17);
    }

    if (!NT_SUCCESS(status) || !nsiDriver) {
        SPOOF_DBG("[Spoofer] NSI: Cannot find nsiproxy (0x%X)\n", status);
        return status;
    }

    g_NsiDriverObject = nsiDriver;
    g_ArpCacheCount = 0;

    g_NsiOriginal = (PDRIVER_DISPATCH)InterlockedExchangePointer(
        (PVOID*)&nsiDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL],
        (PVOID)NsiControlDispatch);

    {
        fn_ObfDereferenceObject pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (pDeref) pDeref(nsiDriver);
    }

    SPOOF_DBG("[Spoofer] NSI: Hooked nsiproxy (orig=%p)\n", g_NsiOriginal);
    return STATUS_SUCCESS;
}

void UninstallNsiHook(void)
{
    if (g_NsiDriverObject && g_NsiOriginal) {
        InterlockedExchangePointer(
            (PVOID*)&g_NsiDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            (PVOID)g_NsiOriginal);
        SPOOF_DBG("[Spoofer] NSI: Hook removed\n");
        g_NsiOriginal = NULL;
        g_NsiDriverObject = NULL;
        g_ArpCacheCount = 0;
    }
}
