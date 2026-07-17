#include "../include/spoofer.h"


PDRIVER_OBJECT GetDriverObjectByName(const WCHAR* name)
{
    UNICODE_STRING driverName;
    PDRIVER_OBJECT driverObject = NULL;
    NTSTATUS status;
    fn_RtlInitUnicodeString pRtlInit;

    pRtlInit = (fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING);
    if (!pRtlInit) return NULL;

    pRtlInit(&driverName, name);

    status = ResolveObRefByName(
        &driverName,
        OBJ_CASE_INSENSITIVE,
        NULL, 0,
        KernelMode,
        NULL,
        (PVOID*)&driverObject
    );

    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] GetDriverObject failed: 0x%X\n", status);
        return NULL;
    }

    return driverObject;  
}

BOOLEAN ValidateDeviceDescriptor(PVOID descriptor)
{
    PSTORAGE_DEVICE_DESCRIPTOR_MINI desc;
    fn_MmIsAddressValid pMmValid;

    if (!descriptor) return FALSE;

    pMmValid = (fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID);
    if (!pMmValid) return FALSE;

    __try {
        if (!pMmValid(descriptor)) return FALSE;

        desc = (PSTORAGE_DEVICE_DESCRIPTOR_MINI)descriptor;

        if (desc->Version < STORAGE_DEVICE_DESCRIPTOR_MIN_VERSION) return FALSE;

        if (desc->Size == 0 || desc->Size > STORAGE_DEVICE_DESCRIPTOR_MAX_SIZE) return FALSE;

        if (desc->BusType > BUSTYPE_MAX_VALID) return FALSE;

        if (desc->SerialNumberOffset > 0 && desc->SerialNumberOffset >= desc->Size) return FALSE;

        return TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Exception validating descriptor: 0x%X\n", GetExceptionCode());
        return FALSE;
    }
}

PVOID GetDeviceExtensionField(PVOID deviceExtension, ULONG offset)
{
    PVOID value = NULL;
    fn_MmIsAddressValid pMmValid;

    pMmValid = (fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID);

    __try {
        if (!deviceExtension || (pMmValid && !pMmValid(deviceExtension))) return NULL;

        PUCHAR base = (PUCHAR)deviceExtension;
        if (pMmValid && !pMmValid(base + offset)) return NULL;

        value = *(PVOID*)(base + offset);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Exception reading DevExt+0x%X: 0x%X\n", offset, GetExceptionCode());
        return NULL;
    }

    return value;
}

void InitPoolTag(const UINT8* seed, PULONG outTag)
{
    ULONG tag;
    UINT8* b;
    int i;

    tag = ((ULONG)seed[0] << 24) | ((ULONG)seed[3] << 16)
        | ((ULONG)seed[7] << 8)  | ((ULONG)seed[11]);

    b = (UINT8*)&tag;
    for (i = 0; i < 4; i++) {
        if (b[i] < 0x41 || b[i] > 0x7A)
            b[i] = 0x41 + (b[i] % 26);
    }

    *outTag = tag;
}
