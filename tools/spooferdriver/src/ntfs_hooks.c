#include "../include/spoofer.h"

// ============================================================================
// ntfs_hooks.c — NTFS Volume Serial + USN Journal ID spoofing via IRP hooks
//
// Hooks:
//   \Driver\ntfs  IRP_MJ_QUERY_VOLUME_INFORMATION → spoof VolumeSerialNumber
//   \Driver\ntfs  IRP_MJ_FILE_SYSTEM_CONTROL     → spoof USN Journal ID
// ============================================================================

/* USN types from ntifs.h — defined manually to avoid header conflicts */
#ifndef FSCTL_QUERY_USN_JOURNAL
#define FSCTL_QUERY_USN_JOURNAL CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 61, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

typedef LONGLONG USN;

typedef struct {
    DWORDLONG UsnJournalID;
    USN       FirstUsn;
    USN       NextUsn;
    USN       LowestValidUsn;
    USN       MaxUsn;
    DWORDLONG MaximumSize;
    DWORDLONG AllocationDelta;
} USN_JOURNAL_DATA, *PUSN_JOURNAL_DATA;

static PDRIVER_DISPATCH g_OrigQueryVolume  = NULL;
static PDRIVER_DISPATCH g_OrigFsControl    = NULL;
static BOOLEAN          g_NtfsHooksActive  = FALSE;
static ULONG            g_VolumeSeed       = 0;

// ── Deterministic hash ──────────────────────────────────────────────────────
static ULONG NtfsHash(ULONG input)
{
    input = (input ^ (input >> 16)) * 0x45D9F3B;
    input = (input ^ (input >> 16)) * 0x45D9F3B;
    return input ^ (input >> 16);
}

// ── Completion context ──────────────────────────────────────────────────────
typedef struct _NTFS_IOC_CTX {
    IO_COMPLETION_ROUTINE* OldRoutine;
    PVOID                  OldContext;
    ULONG                  SpoofValue;
} NTFS_IOC_CTX, *PNTFS_IOC_CTX;

// ── Volume Serial completion ────────────────────────────────────────────────
static NTSTATUS VolumeQueryCompletion(
    PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    PNTFS_IOC_CTX ctx = (PNTFS_IOC_CTX)Context;
    UNREFERENCED_PARAMETER(DeviceObject);

    if (ctx) {
        if (NT_SUCCESS(Irp->IoStatus.Status) &&
            Irp->IoStatus.Information >= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel))
        {
            PVOID buffer = NULL;
            if (Irp->MdlAddress) {
                if (Irp->MdlAddress->MdlFlags &
                    (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))
                    buffer = Irp->MdlAddress->MappedSystemVa;
                else
                    buffer = ((fn_MmMapLockedPagesSpecifyCache)
                        ApiResolveExport(HASH_MMMAPLOCKEDPAGESSPECIFYCACHE))(
                            Irp->MdlAddress, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
            } else {
                buffer = Irp->UserBuffer;
            }

            if (buffer) {
                PFILE_FS_VOLUME_INFORMATION volInfo = (PFILE_FS_VOLUME_INFORMATION)buffer;
                volInfo->VolumeSerialNumber = ctx->SpoofValue;
            }
        }

        IO_COMPLETION_ROUTINE* oldRoutine = ctx->OldRoutine;
        PVOID oldContext = ctx->OldContext;
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(ctx, 'FsQv');

        if (oldRoutine) {
            PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(Irp);
            ioc->CompletionRoutine = oldRoutine;
            ioc->Context = oldContext;
            ioc->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
            return oldRoutine(DeviceObject, Irp, oldContext);
        }
        if (Irp->PendingReturned) IoMarkIrpPending(Irp);
        return STATUS_CONTINUE_COMPLETION;
    }
    if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    return STATUS_CONTINUE_COMPLETION;
}

// ── Hooked IRP_MJ_QUERY_VOLUME_INFORMATION ──────────────────────────────────
static NTSTATUS HookedQueryVolume(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.QueryVolume.FsInformationClass == FileFsVolumeInformation) {
        PNTFS_IOC_CTX ctx = (PNTFS_IOC_CTX)
            ((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(
                NonPagedPool, sizeof(NTFS_IOC_CTX), 'FsQv');

        if (ctx) {
            /* Per-volume deterministic serial based on device + seed */
            ctx->SpoofValue = NtfsHash(g_VolumeSeed ^
                (ULONG)(ULONG_PTR)DeviceObject ^ 0xABCD);
            ctx->OldRoutine = stack->CompletionRoutine;
            ctx->OldContext = stack->Context;
            stack->Control &= ~SL_INVOKE_ON_ERROR;
            stack->Control |= SL_INVOKE_ON_SUCCESS;
            stack->CompletionRoutine = (PIO_COMPLETION_ROUTINE)VolumeQueryCompletion;
            stack->Context = ctx;
        }
    }
    return g_OrigQueryVolume(DeviceObject, Irp);
}

// ── USN Journal completion ──────────────────────────────────────────────────
static NTSTATUS UsnJournalCompletion(
    PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    PNTFS_IOC_CTX ctx = (PNTFS_IOC_CTX)Context;
    UNREFERENCED_PARAMETER(DeviceObject);

    if (ctx) {
        if (NT_SUCCESS(Irp->IoStatus.Status) &&
            Irp->IoStatus.Information >= sizeof(USN_JOURNAL_DATA))
        {
            PUSN_JOURNAL_DATA usn = (PUSN_JOURNAL_DATA)Irp->AssociatedIrp.SystemBuffer;
            if (usn) usn->UsnJournalID = (USN)ctx->SpoofValue;
        }

        IO_COMPLETION_ROUTINE* oldRoutine = ctx->OldRoutine;
        PVOID oldContext = ctx->OldContext;
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(ctx, 'FsUs');

        if (oldRoutine) {
            PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(Irp);
            ioc->CompletionRoutine = oldRoutine;
            ioc->Context = oldContext;
            ioc->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
            return oldRoutine(DeviceObject, Irp, oldContext);
        }
        if (Irp->PendingReturned) IoMarkIrpPending(Irp);
        return STATUS_CONTINUE_COMPLETION;
    }
    if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    return STATUS_CONTINUE_COMPLETION;
}

// ── Hooked IRP_MJ_FILE_SYSTEM_CONTROL ───────────────────────────────────────
static NTSTATUS HookedFsControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    if (stack->Parameters.FileSystemControl.FsControlCode == FSCTL_QUERY_USN_JOURNAL) {
        PNTFS_IOC_CTX ctx = (PNTFS_IOC_CTX)
            ((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(
                NonPagedPool, sizeof(NTFS_IOC_CTX), 'FsUs');

        if (ctx) {
            ctx->SpoofValue = NtfsHash(g_VolumeSeed ^ 0xBEEF ^
                (ULONG)(ULONG_PTR)DeviceObject);
            ctx->OldRoutine = stack->CompletionRoutine;
            ctx->OldContext = stack->Context;
            stack->Control &= ~SL_INVOKE_ON_ERROR;
            stack->Control |= SL_INVOKE_ON_SUCCESS;
            stack->CompletionRoutine = (PIO_COMPLETION_ROUTINE)UsnJournalCompletion;
            stack->Context = ctx;
        }
    }
    return g_OrigFsControl(DeviceObject, Irp);
}

// ── Install / Uninstall ─────────────────────────────────────────────────────
NTSTATUS InstallNtfsHooks(const UINT8* seed)
{
    if (g_NtfsHooksActive) return STATUS_SUCCESS;

    /* Derive seed */
    g_VolumeSeed = 0x811C9DC5;
    { int i; for (i = 0; i < SPOOFER_SEED_SIZE; i++) {
        g_VolumeSeed ^= (ULONG)seed[i];
        g_VolumeSeed *= 0x01000193;
    }}

    UNICODE_STRING drvName;
    PDRIVER_OBJECT drvObj = NULL;
    NTSTATUS status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
        &drvName, L"\\Driver\\ntfs");

    status = ResolveObRefByName(&drvName, OBJ_CASE_INSENSITIVE, NULL, 0,
        KernelMode, NULL, (PVOID*)&drvObj);

    if (!NT_SUCCESS(status) || !drvObj) {
        SPOOF_DBG("[Spoofer] NtfsHooks: Cannot get \\Driver\\ntfs (0x%08X)\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    g_OrigQueryVolume = (PDRIVER_DISPATCH)InterlockedExchangePointer(
        (PVOID*)&drvObj->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION],
        (PVOID)HookedQueryVolume);

    g_OrigFsControl = (PDRIVER_DISPATCH)InterlockedExchangePointer(
        (PVOID*)&drvObj->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL],
        (PVOID)HookedFsControl);

    ((fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT))(drvObj);

    g_NtfsHooksActive = TRUE;
    SPOOF_DBG("[Spoofer] NtfsHooks: Volume Serial + USN Journal hooks installed\n");
    return STATUS_SUCCESS;
}

void UninstallNtfsHooks(void)
{
    if (!g_NtfsHooksActive) return;

    UNICODE_STRING drvName;
    PDRIVER_OBJECT drvObj = NULL;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
        &drvName, L"\\Driver\\ntfs");

    NTSTATUS status = ResolveObRefByName(&drvName, OBJ_CASE_INSENSITIVE, NULL, 0,
        KernelMode, NULL, (PVOID*)&drvObj);

    if (NT_SUCCESS(status) && drvObj) {
        if (g_OrigQueryVolume) {
            InterlockedExchangePointer(
                (PVOID*)&drvObj->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION],
                (PVOID)g_OrigQueryVolume);
        }
        if (g_OrigFsControl) {
            InterlockedExchangePointer(
                (PVOID*)&drvObj->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL],
                (PVOID)g_OrigFsControl);
        }
        ((fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT))(drvObj);
    }

    g_NtfsHooksActive = FALSE;
    SPOOF_DBG("[Spoofer] NtfsHooks: Uninstalled\n");
}
