#include "../include/spoofer.h"

#define IOCTL_DISK_GET_DRIVE_LAYOUT_EX  0x00070050
#define PARTITION_STYLE_GPT             1


typedef struct _DISPATCH_COMPLETION_CTX {
    PIO_COMPLETION_ROUTINE  OriginalCompletion;
    PVOID                   OriginalContext;
    UCHAR                   OriginalControl;
    ULONG                   IoControlCode;
    ULONG                   PropertyId;
    PVOID                   DirectDataBuffer;   
    ULONG                   DirectDataLength;
} DISPATCH_COMPLETION_CTX, *PDISPATCH_COMPLETION_CTX;

static NTSTATUS HookedClassDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
static NTSTATUS DispatchCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context);

static void HandleStorageDeviceIdDescriptor(PVOID buffer, ULONG length)
{
    PSTORAGE_DEVICE_ID_DESCRIPTOR_MINI desc = (PSTORAGE_DEVICE_ID_DESCRIPTOR_MINI)buffer;
    PSTORAGE_IDENTIFIER_MINI id;
    PUCHAR current, end;
    ULONG i;

    if (length < sizeof(STORAGE_DEVICE_ID_DESCRIPTOR_MINI)) return;
    if (desc->NumberOfIdentifiers == 0) return;

    current = desc->Identifiers;
    end = (PUCHAR)buffer + min(length, desc->Size);

    for (i = 0; i < desc->NumberOfIdentifiers && current < end; i++) {
        id = (PSTORAGE_IDENTIFIER_MINI)current;
        if ((PUCHAR)id + sizeof(STORAGE_IDENTIFIER_MINI) > end) break;
        if (id->IdentifierSize == 0) break;

        if (id->IdentifierSize > 0 &&
            (PUCHAR)id->Identifier + id->IdentifierSize <= end) {
            if (id->CodeSet >= 2) {
                PCHAR idStr = (PCHAR)id->Identifier;
                USHORT idLen = id->IdentifierSize;
                USHORT prefixLen = 0;

                if (idLen > 4 && (
                    (idStr[0]=='e' && idStr[1]=='u' && idStr[2]=='i' && idStr[3]=='.') ||
                    (idStr[0]=='n' && idStr[1]=='a' && idStr[2]=='a' && idStr[3]=='.') ||
                    (idStr[0]=='t' && idStr[1]=='1' && idStr[2]=='0' && idStr[3]=='.'))) {
                    prefixLen = 4;
                }

                SpoofSerialString(idStr + prefixLen, idLen - prefixLen,
                    g_Spoofer.Seed, "STORAGE_DEVICE_ID_TEXT");
            } else {
                SpoofBinaryData(id->Identifier, id->IdentifierSize,
                    g_Spoofer.Seed, "STORAGE_DEVICE_ID_BIN");
            }
        }

        if (id->NextOffset == 0) break;
        current += id->NextOffset;
    }
}

static void ScanAndSpoofBuffer(PUCHAR buffer, ULONG length, const char* source)
{
    ULONG i;
    SIZE_T slen, alphaNum;
    char c;

    if (!buffer || length < 8) return;

    for (i = 0; i < length - 4; i++) {
        if (buffer[i] < 0x20 || buffer[i] > 0x7E) continue;

        slen = 0;
        alphaNum = 0;

        while (i + slen < length && slen < 40) {
            c = (char)buffer[i + slen];
            if (c == 0 || c < 0x20 || c > 0x7E) break;
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ' || c == '.'))
                break;
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
                alphaNum++;
            slen++;
        }

        if (slen >= 6 && alphaNum >= 5) {
            SIZE_T spoofedLen = 0;
            while (g_Spoofer.FirstSpoofedSerial[spoofedLen] != 0 && spoofedLen < 63) spoofedLen++;
            if (g_Spoofer.FirstSerialCaptured && slen == spoofedLen) {
                SPOOF_DBG("[Spoofer] %s: Replacing '%.*s' with FirstSpoofedSerial\n",
                    source, (int)slen, &buffer[i]);
                RtlCopyMemory(&buffer[i], g_Spoofer.FirstSpoofedSerial, slen);
            } else {
                SpoofSerialString((PCHAR)&buffer[i], slen, g_Spoofer.Seed, source);
            }
            i += (ULONG)slen - 1;
        }
    }
}

static NTSTATUS DispatchCompletionRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context)
{
    PDISPATCH_COMPLETION_CTX ctx = (PDISPATCH_COMPLETION_CTX)Context;
    PIO_COMPLETION_ROUTINE originalCompletion;
    PVOID originalContext;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!ctx) {
        if (Irp && Irp->PendingReturned) IoMarkIrpPending(Irp);
        return STATUS_SUCCESS;
    }

    originalCompletion = ctx->OriginalCompletion;
    originalContext = ctx->OriginalContext;

    __try {
        if (Irp && NT_SUCCESS(Irp->IoStatus.Status) && Irp->IoStatus.Information > 0) {
            PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
            ULONG length = (ULONG)Irp->IoStatus.Information;

            if (buffer && ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(buffer) && length >= 8) {
                switch (ctx->IoControlCode) {
                case IOCTL_STORAGE_QUERY_PROPERTY:
                    SPOOF_DBG("[Spoofer] Schicht2: QUERY_PROPERTY completion PropertyId=%lu, len=%lu\n",
                        ctx->PropertyId, length);
                    if (ctx->PropertyId == StorageDeviceProperty) {
                        PSTORAGE_DEVICE_DESCRIPTOR_MINI desc = (PSTORAGE_DEVICE_DESCRIPTOR_MINI)buffer;
                        if (length >= sizeof(STORAGE_DEVICE_DESCRIPTOR_MINI) &&
                            desc->Version >= STORAGE_DEVICE_DESCRIPTOR_MIN_VERSION &&
                            desc->Size <= length) {
                            if (desc->SerialNumberOffset > 0 && desc->SerialNumberOffset < desc->Size) {
                                PCHAR serial = (PCHAR)desc + desc->SerialNumberOffset;
                                SIZE_T serialLen = 0;
                                while (desc->SerialNumberOffset + serialLen < desc->Size &&
                                       serial[serialLen] != 0 && serialLen < 63) serialLen++;

                                if (g_Spoofer.FirstSerialCaptured && serialLen > 0) {
                                    SIZE_T copyLen = 0;
                                    while (g_Spoofer.FirstSpoofedSerial[copyLen] != 0 && copyLen < 63) copyLen++;
                                    if (copyLen <= serialLen) {
                                        RtlCopyMemory(serial, g_Spoofer.FirstSpoofedSerial, copyLen);
                                        if (copyLen < serialLen) serial[copyLen] = 0;
                                    } else {
                                        SpoofSerialString(serial, serialLen, g_Spoofer.Seed, "QUERY_PROP_SERIAL");
                                    }
                                } else {
                                    SIZE_T maxLen = desc->Size - desc->SerialNumberOffset;
                                    SpoofSerialString(serial, maxLen, g_Spoofer.Seed, "QUERY_PROP_SERIAL");
                                }
                            }
                        }
                    }
                    else if (ctx->PropertyId == StorageDeviceIdProperty) {
                        HandleStorageDeviceIdDescriptor(buffer, length);
                    }
                    else if (ctx->PropertyId == 3 || ctx->PropertyId == 5) {
                        HandleStorageDeviceIdDescriptor(buffer, length);
                        ScanAndSpoofBuffer((PUCHAR)buffer, length, "DUID");
                    }
                    break;

                case SMART_RCV_DRIVE_DATA:
                    ScanAndSpoofBuffer((PUCHAR)buffer, length, "SMART_DATA");
                    break;

                case IOCTL_SCSI_PASS_THROUGH:
                    ScanAndSpoofBuffer((PUCHAR)buffer, length, "SCSI_PT");
                    break;

                case IOCTL_SCSI_PASS_THROUGH_DIRECT:
                    ScanAndSpoofBuffer((PUCHAR)buffer, length, "SCSI_PT_HDR");
                    if (ctx->DirectDataBuffer && ctx->DirectDataLength > 0 &&
                        ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ctx->DirectDataBuffer)) {
                        ScanAndSpoofBuffer((PUCHAR)ctx->DirectDataBuffer,
                            ctx->DirectDataLength, "SCSI_PT_DIRECT");
                    }
                    break;

                case IOCTL_ATA_PASS_THROUGH:
                    ScanAndSpoofBuffer((PUCHAR)buffer, length, "ATA_PT");
                    break;

                case IOCTL_ATA_PASS_THROUGH_DIRECT:
                    ScanAndSpoofBuffer((PUCHAR)buffer, length, "ATA_PT_HDR");
                    if (ctx->DirectDataBuffer && ctx->DirectDataLength > 0 &&
                        ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ctx->DirectDataBuffer)) {
                        ScanAndSpoofBuffer((PUCHAR)ctx->DirectDataBuffer,
                            ctx->DirectDataLength, "ATA_PT_DIRECT");
                    }
                    break;

                case IOCTL_SCSI_MINIPORT:
                    ScanAndSpoofBuffer((PUCHAR)buffer, length, "SCSI_MINIPORT");
                    break;

                case IOCTL_STORAGE_PROTOCOL_COMMAND:
                    ScanAndSpoofBuffer((PUCHAR)buffer, length, "STORAGE_PROTO");
                    break;

                case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
                {

                    PUCHAR layoutBuf = (PUCHAR)buffer;
                    ULONG partStyle = *(ULONG*)layoutBuf;

                    if (partStyle == PARTITION_STYLE_GPT && length >= 48)
                    {
                        GUID* diskGuid = (GUID*)(layoutBuf + 8);
                        GUID spoofed;
                        ULONG partCount, p;

                        GenerateSpoofGuid(&spoofed, g_Spoofer.Seed, 0x6D150000);
                        SPOOF_DBG("[Spoofer] GPT: DiskId {%08X-...} -> {%08X-...}\n",
                            diskGuid->Data1, spoofed.Data1);
                        RtlCopyMemory(diskGuid, &spoofed, sizeof(GUID));

                        partCount = *(ULONG*)(layoutBuf + 4);
                        if (partCount > 128) partCount = 128;

                        for (p = 0; p < partCount; p++)
                        {
                            ULONG partOffset = 48 + (p * 144);
                            if (partOffset + 144 > length) break;

                            PUCHAR partEntry = layoutBuf + partOffset;
                            ULONG pStyle = *(ULONG*)partEntry;

                            if (pStyle == PARTITION_STYLE_GPT)
                            {
                                if (partOffset + 64 <= length)
                                {
                                    GUID* partId = (GUID*)(partEntry + 48);
                                    GenerateSpoofGuid(&spoofed, g_Spoofer.Seed, 0x6D150001 + p);
                                    RtlCopyMemory(partId, &spoofed, sizeof(GUID));
                                }
                            }
                        }
                        SPOOF_DBG("[Spoofer] GPT: %lu partition GUIDs spoofed\n", partCount);
                    }
                    break;
                }
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Schicht2: Completion exception 0x%X (IOCTL=0x%X)\n",
            GetExceptionCode(), ctx->IoControlCode);
    }

    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(ctx, g_Spoofer.PoolTag);

    if (originalCompletion) {
        return originalCompletion(DeviceObject, Irp, originalContext);
    }

    if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    return STATUS_SUCCESS;
}

static NTSTATUS HookedClassDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    ULONG ioctl;
    BOOLEAN shouldIntercept = FALSE;
    ULONG propertyId = 0;
    PDISPATCH_COMPLETION_CTX ctx;

    typedef NTSTATUS (*PFN_DEVICE_CONTROL)(PDEVICE_OBJECT, PIRP);
    PFN_DEVICE_CONTROL originalHandler =
        (PFN_DEVICE_CONTROL)g_Spoofer.OriginalClassDeviceControl;

    if (!originalHandler) {
        Irp->IoStatus.Status = STATUS_INTERNAL_ERROR;
        Irp->IoStatus.Information = 0;
        ((fn_IofCompleteRequest)ApiResolveExport(HASH_IOFCOMPLETEREQUEST))(Irp, IO_NO_INCREMENT);
        return STATUS_INTERNAL_ERROR;
    }

    __try {
        stack = IoGetCurrentIrpStackLocation(Irp);
        if (!stack) return originalHandler(DeviceObject, Irp);

        ioctl = stack->Parameters.DeviceIoControl.IoControlCode;

        if (ioctl == IOCTL_STORAGE_QUERY_PROPERTY) {
            PVOID inputBuffer = Irp->AssociatedIrp.SystemBuffer;
            ULONG inputLength = stack->Parameters.DeviceIoControl.InputBufferLength;

            if (inputBuffer && inputLength >= sizeof(STORAGE_PROPERTY_QUERY_MINI)) {
                STORAGE_PROPERTY_QUERY_MINI* query = (STORAGE_PROPERTY_QUERY_MINI*)inputBuffer;
                propertyId = query->PropertyId;
                SPOOF_DBG("[Spoofer] Schicht2: QUERY_PROPERTY request PropertyId=%lu QueryType=%lu\n",
                    propertyId, query->QueryType);
                if (propertyId == StorageDeviceProperty ||
                    propertyId == StorageDeviceIdProperty ||
                    propertyId == 3 ||  
                    propertyId == 5) {  
                    shouldIntercept = TRUE;
                }
            }
        }
        else if (ioctl == SMART_RCV_DRIVE_DATA ||
                 ioctl == IOCTL_SCSI_PASS_THROUGH ||
                 ioctl == IOCTL_SCSI_PASS_THROUGH_DIRECT ||
                 ioctl == IOCTL_ATA_PASS_THROUGH ||
                 ioctl == IOCTL_ATA_PASS_THROUGH_DIRECT ||
                 ioctl == IOCTL_SCSI_MINIPORT ||
                 ioctl == IOCTL_STORAGE_PROTOCOL_COMMAND ||
                 ioctl == IOCTL_DISK_GET_DRIVE_LAYOUT_EX) {
            shouldIntercept = TRUE;
        }

        if (!shouldIntercept) {
            return originalHandler(DeviceObject, Irp);
        }

        ctx = (PDISPATCH_COMPLETION_CTX)((fn_ExAllocatePool2)ApiResolveExport(HASH_EXALLOCATEPOOL2))(
            POOL_FLAG_NON_PAGED, sizeof(DISPATCH_COMPLETION_CTX), g_Spoofer.PoolTag);

        if (!ctx) {
            return originalHandler(DeviceObject, Irp);
        }

        RtlZeroMemory(ctx, sizeof(DISPATCH_COMPLETION_CTX));
        ctx->OriginalCompletion = stack->CompletionRoutine;
        ctx->OriginalContext = stack->Context;
        ctx->OriginalControl = stack->Control;
        ctx->IoControlCode = ioctl;
        ctx->PropertyId = propertyId;

        {
            PVOID inputBuffer = Irp->AssociatedIrp.SystemBuffer;
            ULONG inputLength = stack->Parameters.DeviceIoControl.InputBufferLength;

            if (ioctl == IOCTL_SCSI_PASS_THROUGH_DIRECT && inputBuffer &&
                inputLength >= sizeof(SCSI_PASS_THROUGH_DIRECT)) {
                PSCSI_PASS_THROUGH_DIRECT sptd = (PSCSI_PASS_THROUGH_DIRECT)inputBuffer;
                ctx->DirectDataBuffer = sptd->DataBuffer;
                ctx->DirectDataLength = sptd->DataTransferLength;
            }
            else if (ioctl == IOCTL_ATA_PASS_THROUGH_DIRECT && inputBuffer &&
                     inputLength >= sizeof(ATA_PASS_THROUGH_DIRECT)) {
                PATA_PASS_THROUGH_DIRECT atad = (PATA_PASS_THROUGH_DIRECT)inputBuffer;
                ctx->DirectDataBuffer = atad->DataBuffer;
                ctx->DirectDataLength = atad->DataTransferLength;
            }
        }

        stack->CompletionRoutine = DispatchCompletionRoutine;
        stack->Context = ctx;
        stack->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

        return originalHandler(DeviceObject, Irp);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Schicht2: Hook exception 0x%X\n", GetExceptionCode());
        return originalHandler(DeviceObject, Irp);
    }
}

NTSTATUS InstallDispatchHook(const UINT8* seed)
{
    PDRIVER_OBJECT diskDriver;
    PDEVICE_OBJECT deviceObject;
    PVOID devExt;
    PVOID devInfo;
    PVOID* callbackSlot;
    PVOID originalCallback;

    UNREFERENCED_PARAMETER(seed);

    SPOOF_DBG("[Spoofer] ============ SCHICHT 2: DispatchTable Hook ============\n");

    {
        STK_DRIVER_DISK(_dkN);
        diskDriver = GetDriverObjectByName(_dkN);
        STK_WIPE_W(_dkN, 13);
    }
    if (!diskDriver) {
        SPOOF_DBG("[Spoofer] Schicht2: disk.sys not found\n");
        return STATUS_NOT_FOUND;
    }

    deviceObject = diskDriver->DeviceObject;
    if (!deviceObject) {
        {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(diskDriver);
    }
        return STATUS_NOT_FOUND;
    }

    devExt = deviceObject->DeviceExtension;
    if (!devExt) {
        {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(diskDriver);
    }
        return STATUS_NOT_FOUND;
    }

    devInfo = GetDeviceExtensionField(devExt, FDE_DEVINFO_OFFSET);
    if (!devInfo) {
        SPOOF_DBG("[Spoofer] Schicht2: DevInfo at DevExt+0x%X is NULL\n", FDE_DEVINFO_OFFSET);
        {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(diskDriver);
    }
        return STATUS_NOT_FOUND;
    }

    callbackSlot = (PVOID*)((PUCHAR)devInfo + DEVINFO_CLASS_DEVICE_CONTROL_OFFSET);

    __try {
        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(callbackSlot)) {
            SPOOF_DBG("[Spoofer] Schicht2: Callback slot at %p not valid\n", callbackSlot);
            {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(diskDriver);
    }
            return STATUS_ACCESS_VIOLATION;
        }

        originalCallback = *callbackSlot;

        if (!originalCallback) {
            SPOOF_DBG("[Spoofer] Schicht2: Original callback is NULL\n");
            {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(diskDriver);
    }
            return STATUS_NOT_FOUND;
        }

        SPOOF_DBG("[Spoofer] Schicht2: DevInfo=%p, CallbackSlot=%p, Original=%p\n",
            devInfo, callbackSlot, originalCallback);

        g_Spoofer.DiskDriverObject = diskDriver;  
        g_Spoofer.OriginalClassDeviceControl = originalCallback;
        g_Spoofer.DevInfoPointer = devInfo;

        InterlockedExchangePointer(callbackSlot, (PVOID)HookedClassDeviceControl);

        g_Spoofer.DispatchHooked = TRUE;
        SPOOF_DBG("[Spoofer] Schicht2: Callback swapped! DiskDeviceControl → HookedClassDeviceControl\n");
        SPOOF_DBG("[Spoofer] Schicht2: MajorFunction[14] = ClassGlobalDispatch (UNMODIFIED)\n");
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Schicht2: Exception during install: 0x%X\n", GetExceptionCode());
        {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(diskDriver);
    }
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

void UninstallDispatchHook(void)
{
    LARGE_INTEGER delay;

    if (!g_Spoofer.DispatchHooked) return;

    SPOOF_DBG("[Spoofer] Schicht2: Uninstalling...\n");

    if (g_Spoofer.DevInfoPointer && g_Spoofer.OriginalClassDeviceControl) {
        PVOID* callbackSlot = (PVOID*)((PUCHAR)g_Spoofer.DevInfoPointer +
            DEVINFO_CLASS_DEVICE_CONTROL_OFFSET);

        __try {
            if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(callbackSlot)) {
                InterlockedExchangePointer(callbackSlot, g_Spoofer.OriginalClassDeviceControl);
                SPOOF_DBG("[Spoofer] Schicht2: Original callback restored\n");
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SPOOF_DBG("[Spoofer] Schicht2: Exception restoring callback\n");
        }
    }

    delay.QuadPart = -20000000LL;  
    {
        fn_KeDelayExecutionThread _pDelay = (fn_KeDelayExecutionThread)ApiResolveExport(HASH_KEDELAYEXECUTIONTHREAD);
        if (_pDelay) _pDelay(KernelMode, FALSE, &delay);
    }

    if (g_Spoofer.DiskDriverObject) {
        ((fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT))(g_Spoofer.DiskDriverObject);
        g_Spoofer.DiskDriverObject = NULL;
    }

    g_Spoofer.OriginalClassDeviceControl = NULL;
    g_Spoofer.DevInfoPointer = NULL;
    g_Spoofer.DispatchHooked = FALSE;

    SPOOF_DBG("[Spoofer] Schicht2: Uninstalled\n");
}
