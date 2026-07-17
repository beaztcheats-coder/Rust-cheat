#include "../include/spoofer.h"


static TPM_SPOOF_DATA g_TpmSpoofData = { 0 };
static PDRIVER_OBJECT g_TpmDriverObject = NULL;
static PDRIVER_DISPATCH g_OriginalTpmDispatch = NULL;
static UINT8 g_TpmSeed[16] = { 0 };


typedef struct _TPM_IOC_CTX {
    PVOID                   Buffer;
    ULONG                   Size;
    PVOID                   OriginalContext;
    PIO_COMPLETION_ROUTINE  Original;
    UINT32                  CommandCode;
} TPM_IOC_CTX, *PTPM_IOC_CTX;


static void GenerateFakeKey(UINT8* seed, TPM2B_PUBLIC_KEY_RSA* key)
{
    UINT32 hash;
    SIZE_T i;

    if (!seed || !key)
        return;

    key->size = 256;
    hash = 0x811c9dc5;

    for (i = 0; i < 256; i++)
    {
        hash ^= seed[i % 16];
        hash *= 0x01000193;
        hash ^= (UINT32)(i * 0x9e3779b9);
        hash *= 0x85ebca6b;
        hash ^= hash >> 16;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 13;

        key->buffer[i] = (UINT8)(hash & 0xFF);
    }

    key->buffer[0] |= 0x80;
    key->buffer[255] |= 0x01;
}



static NTSTATUS TpmRequestCompletion(PDEVICE_OBJECT device, PIRP irp, PVOID context)
{
    PTPM_IOC_CTX request;
    PUCHAR buffer;
    ULONG responseSize;
    SIZE_T i;
    BOOLEAN found = FALSE;

    UNREFERENCED_PARAMETER(device);

    if (!context)
    {
        if (irp->PendingReturned)
            IoMarkIrpPending(irp);
        return STATUS_SUCCESS;
    }

    request = (PTPM_IOC_CTX)context;


    if (NT_SUCCESS(irp->IoStatus.Status) && request->Buffer)
    {
        buffer = (PUCHAR)request->Buffer;
        responseSize = request->Size;  

        if (responseSize > 0x2000) responseSize = 0x2000;
        if (responseSize < 10) goto done;


        {
            PUCHAR page;
            if (!MmIsAddressValid(buffer))
                goto done;
            for (page = (PUCHAR)((ULONG_PTR)(buffer) & ~0xFFF) + 0x1000;
                 page < buffer + responseSize;
                 page += 0x1000)
            {
                if (!MmIsAddressValid(page))
                    goto done;
            }
            if (!MmIsAddressValid(buffer + responseSize - 1))
                goto done;
        }

        {
            UINT32 respCode = ((UINT32)buffer[6] << 24) | ((UINT32)buffer[7] << 16) |
                              ((UINT32)buffer[8] << 8) | buffer[9];
            if (respCode != 0)
                goto done;
        }


        if (responseSize >= 22)
        {

            UINT16 algType = ((UINT16)buffer[12] << 8) | buffer[13];

            if (algType == 0x0001) 
            {
                for (i = 20; i + 258 <= responseSize; i++)
                {
                    UINT16 keySize = ((UINT16)buffer[i] << 8) | buffer[i + 1];
                    if (keySize == 256 &&
                        (buffer[i + 2] != 0x00 || buffer[i + 3] != 0x00))
                    {
                        RtlCopyMemory(&buffer[i + 2],
                            g_TpmSpoofData.GeneratedKey.buffer, 256);
                        SPOOF_DBG("[TPM] RSA key replaced at offset %llu, respSize=%u\n",
                            i, responseSize);
                        found = TRUE;
                        break;
                    }
                }
            }
        }
    }

done:

    if (request->Original)
    {
        fn_ExFreePoolWithTag pFree;
        NTSTATUS status = request->Original(device, irp, request->OriginalContext);
        pFree = (fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG);
        if (pFree) pFree(request, 'TpmS');
        return status;
    }

    {
        fn_ExFreePoolWithTag pFree;
        pFree = (fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG);
        if (pFree) pFree(request, 'TpmS');
    }

    if (irp->PendingReturned)
        IoMarkIrpPending(irp);

    return STATUS_SUCCESS;
}



static void InstallCompletionRoutine(PIO_STACK_LOCATION ioc, PIRP irp, UINT32 commandCode)
{
    fn_ExAllocatePoolWithTag pAlloc;
    PTPM_IOC_CTX request;

    pAlloc = (fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG);
    if (!pAlloc) return;

    request = (PTPM_IOC_CTX)pAlloc(0, sizeof(TPM_IOC_CTX), 'TpmS');
    if (!request) return;

    request->Buffer = irp->AssociatedIrp.SystemBuffer;
    request->Size = ioc->Parameters.DeviceIoControl.OutputBufferLength;
    request->OriginalContext = ioc->Context;
    request->Original = ioc->CompletionRoutine;
    request->CommandCode = commandCode;

    ioc->Control = SL_INVOKE_ON_SUCCESS;
    ioc->Context = request;
    ioc->CompletionRoutine = TpmRequestCompletion;
}



static inline UINT32 SwapU32(UINT32 v)
{
    return ((v >> 24) & 0xFF) | ((v >> 8) & 0xFF00) |
           ((v << 8) & 0xFF0000) | ((v << 24) & 0xFF000000u);
}

static NTSTATUS HookedTpmDispatch(PDEVICE_OBJECT device, PIRP irp)
{
    PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(irp);


    if (ioc->MajorFunction == IRP_MJ_DEVICE_CONTROL ||
        ioc->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
    {
        ULONG ioctl = ioc->Parameters.DeviceIoControl.IoControlCode;


        if (ioctl == 0x173 || ioctl == IOCTL_TPM_SUBMIT_COMMAND)
        {
            InstallCompletionRoutine(ioc, irp, 0);
        }
    }

    return g_OriginalTpmDispatch(device, irp);
}



static NTSTATUS DeleteKeyRecursive(HANDLE hParentKey, PUNICODE_STRING subKeyName)
{
    NTSTATUS status;
    HANDLE hKey = NULL;
    OBJECT_ATTRIBUTES oa;
    UCHAR keyInfoBuf[512];
    ULONG resultLen;
    PKEY_BASIC_INFORMATION keyInfo;
    UNICODE_STRING childName;
    WCHAR childBuf[128];
    ULONG index;
    fn_ZwOpenKey pOpen;
    fn_ZwEnumerateKey pEnum;
    fn_ZwDeleteKey pDelete;
    fn_ZwClose pClose;
    fn_RtlInitUnicodeString pInit;

    pOpen   = (fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY);
    pEnum   = (fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY);
    pDelete = (fn_ZwDeleteKey)ApiResolveExport(HASH_ZWDELETEKEY);
    pClose  = (fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE);
    pInit   = (fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING);

    if (!pOpen || !pEnum || !pDelete || !pClose || !pInit)
        return STATUS_NOT_FOUND;

    InitializeObjectAttributes(&oa, subKeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hParentKey, NULL);

    status = pOpen(&hKey, KEY_ALL_ACCESS, &oa);
    if (!NT_SUCCESS(status)) return status;

    index = 0;
    while (TRUE)
    {
        status = pEnum(hKey, 0, KeyBasicInformation,
            keyInfoBuf, sizeof(keyInfoBuf), &resultLen);
        if (!NT_SUCCESS(status)) break;

        keyInfo = (PKEY_BASIC_INFORMATION)keyInfoBuf;
        RtlZeroMemory(childBuf, sizeof(childBuf));
        RtlCopyMemory(childBuf, keyInfo->Name,
            min(keyInfo->NameLength, sizeof(childBuf) - sizeof(WCHAR)));
        pInit(&childName, childBuf);
        DeleteKeyRecursive(hKey, &childName);
        if (++index > 100) break;
    }

    status = pDelete(hKey);
    pClose(hKey);
    return status;
}

static NTSTATUS DeleteTpmEndorsementRegistry(void)
{
    NTSTATUS status;
    HANDLE hWmiKey = NULL;
    UNICODE_STRING wmiPath, endorsementName;
    OBJECT_ATTRIBUTES oa;
    fn_RtlInitUnicodeString pInit;
    fn_ZwOpenKey pOpen;
    fn_ZwClose pClose;

    pInit  = (fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING);
    pOpen  = (fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY);
    pClose = (fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE);
    if (!pInit || !pOpen || !pClose) return STATUS_NOT_FOUND;

    pInit(&wmiPath, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\TPM\\WMI");
    InitializeObjectAttributes(&oa, &wmiPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = pOpen(&hWmiKey, KEY_ALL_ACCESS, &oa);
    if (!NT_SUCCESS(status)) return status;

    pInit(&endorsementName, L"Endorsement");
    status = DeleteKeyRecursive(hWmiKey, &endorsementName);
    SPOOF_DBG("[TPM] Registry: Endorsement delete 0x%X\n", status);
    pClose(hWmiKey);
    return status;
}



NTSTATUS InitializeTpmSpoofer(UINT8* seed)
{
    if (g_TpmSpoofData.Initialized)
        return STATUS_SUCCESS;
    if (!seed)
        return STATUS_INVALID_PARAMETER;


    /* DeleteTpmEndorsementRegistry(); */

    RtlCopyMemory(g_TpmSeed, seed, 16);
    GenerateFakeKey(seed, &g_TpmSpoofData.GeneratedKey);

    g_TpmSpoofData.Initialized = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS InstallTpmHook(void)
{
    NTSTATUS status;
    UNICODE_STRING driverName;
    fn_RtlInitUnicodeString pRtlInit;

    if (!g_TpmSpoofData.Initialized)
        return STATUS_UNSUCCESSFUL;
    if (g_TpmSpoofData.HookInstalled)
        return STATUS_SUCCESS;

    pRtlInit = (fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING);
    if (!pRtlInit) return STATUS_NOT_FOUND;

    pRtlInit(&driverName, L"\\Driver\\TPM");

    status = ResolveObRefByName(
        &driverName, OBJ_CASE_INSENSITIVE, NULL, 0,
        KernelMode, NULL, (PVOID*)&g_TpmDriverObject);

    if (!NT_SUCCESS(status) || !g_TpmDriverObject)
        return status;

    g_OriginalTpmDispatch = g_TpmDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];

    InterlockedExchangePointer(
        (PVOID*)&g_TpmDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
        (PVOID)HookedTpmDispatch);

    g_TpmSpoofData.HookInstalled = TRUE;

    SPOOF_DBG("[TPM] Dispatch swap installed: orig=%p, hook=%p\n",
        g_OriginalTpmDispatch, HookedTpmDispatch);

    return STATUS_SUCCESS;
}

void UninstallTpmHook(void)
{
    fn_KeDelayExecutionThread pDelay;
    fn_ObfDereferenceObject pDeref;
    LARGE_INTEGER delay;

    if (!g_TpmSpoofData.HookInstalled)
        return;

    if (g_TpmDriverObject && g_OriginalTpmDispatch)
    {
        InterlockedExchangePointer(
            (PVOID*)&g_TpmDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL],
            (PVOID)g_OriginalTpmDispatch);

        pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (pDeref) pDeref(g_TpmDriverObject);
    }

    pDelay = (fn_KeDelayExecutionThread)ApiResolveExport(HASH_KEDELAYEXECUTIONTHREAD);
    if (pDelay)
    {
        delay.QuadPart = -30000000LL;
        pDelay(KernelMode, FALSE, &delay);
    }

    g_TpmDriverObject = NULL;
    g_OriginalTpmDispatch = NULL;
    g_TpmSpoofData.HookInstalled = FALSE;
}

PTPM_SPOOF_DATA GetTpmSpoofData(void)
{
    return &g_TpmSpoofData;
}

BOOLEAN IsTpmSpooferInitialized(void)
{
    return g_TpmSpoofData.Initialized;
}

void CleanupTpmSpoofer(void)
{
    UninstallTpmHook();
    RtlZeroMemory(&g_TpmSpoofData, sizeof(g_TpmSpoofData));
    RtlZeroMemory(g_TpmSeed, sizeof(g_TpmSeed));
}
