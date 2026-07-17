#include "../include/spoofer.h"

#define MAX_PATCHED_DEVICES 16

typedef struct _PATCHED_DEVICE {
    PDEVICE_OBJECT  DeviceObject;
    PVOID           DescriptorPtr;      
    char            OriginalSerial[64];
    ULONG           OriginalSerialOffset;
    ULONG           OriginalSerialLen;
    BOOLEAN         Used;
} PATCHED_DEVICE, *PPATCHED_DEVICE;

static PATCHED_DEVICE g_PatchedDevices[MAX_PATCHED_DEVICES] = { 0 };

static void PatchNvmeIdentity(const UINT8* seed);

static BOOLEAN PatchSingleDescriptor(
    PDEVICE_OBJECT deviceObject,
    PVOID deviceExtension,
    const UINT8* seed)
{
    PSTORAGE_DEVICE_DESCRIPTOR_MINI desc;
    PCHAR serial;
    SIZE_T serialLen;
    ULONG i;
    PPATCHED_DEVICE slot = NULL;

    desc = (PSTORAGE_DEVICE_DESCRIPTOR_MINI)GetDeviceExtensionField(
        deviceExtension, FDE_DEVICE_DESCRIPTOR_OFFSET);

    if (!desc) {
        SPOOF_DBG("[Spoofer] Schicht1: DevExt+0x%X is NULL for DevObj %p\n",
            FDE_DEVICE_DESCRIPTOR_OFFSET, deviceObject);
        return FALSE;
    }

    if (!ValidateDeviceDescriptor(desc)) {
        SPOOF_DBG("[Spoofer] Schicht1: Invalid descriptor at %p (Version=%u Size=%u)\n",
            desc, desc->Version, desc->Size);
        return FALSE;
    }

    if (desc->SerialNumberOffset == 0 || desc->SerialNumberOffset >= desc->Size) {
        SPOOF_DBG("[Spoofer] Schicht1: No serial in descriptor (offset=%u)\n",
            desc->SerialNumberOffset);
        return FALSE;
    }

    serial = (PCHAR)desc + desc->SerialNumberOffset;
    serialLen = 0;

    __try {
        while (desc->SerialNumberOffset + serialLen < desc->Size &&
               serial[serialLen] != 0 && serialLen < 63) {
            serialLen++;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        SPOOF_DBG("[Spoofer] Schicht1: Exception reading serial\n");
        return FALSE;
    }

    if (serialLen < 4) {
        SPOOF_DBG("[Spoofer] Schicht1: Serial too short (%llu)\n", (UINT64)serialLen);
        return FALSE;
    }

    for (i = 0; i < MAX_PATCHED_DEVICES; i++) {
        if (!g_PatchedDevices[i].Used) {
            slot = &g_PatchedDevices[i];
            break;
        }
    }

    if (!slot) {
        SPOOF_DBG("[Spoofer] Schicht1: No free slots!\n");
        return FALSE;
    }

    RtlZeroMemory(slot->OriginalSerial, sizeof(slot->OriginalSerial));
    RtlCopyMemory(slot->OriginalSerial, serial, serialLen);
    slot->DeviceObject = deviceObject;
    slot->DescriptorPtr = desc;
    slot->OriginalSerialOffset = desc->SerialNumberOffset;
    slot->OriginalSerialLen = (ULONG)serialLen;
    slot->Used = TRUE;

    SPOOF_DBG("[Spoofer] Schicht1: Device %p — Descriptor at %p, Serial at +%u\n",
        deviceObject, desc, desc->SerialNumberOffset);

    SpoofSerialString(serial, serialLen, seed, "DESCRIPTOR_PATCH");

    if (desc->VendorIdOffset > 0 && desc->VendorIdOffset < desc->Size) {
        PCHAR vendor = (PCHAR)desc + desc->VendorIdOffset;
        SIZE_T vendorLen = 0;
        while (desc->VendorIdOffset + vendorLen < desc->Size &&
               vendor[vendorLen] != 0 && vendorLen < 40) vendorLen++;
        SPOOF_DBG("[Spoofer] Schicht1: VendorId='%.*s' (not spoofed)\n", (int)vendorLen, vendor);
    }

    return TRUE;
}

NTSTATUS PatchDeviceDescriptors(const UINT8* seed)
{
    PDRIVER_OBJECT diskDriver = NULL;
    PDEVICE_OBJECT deviceObject;
    ULONG patchedCount = 0;

    SPOOF_DBG("[Spoofer] ============ SCHICHT 1: DeviceDescriptor Patch ============\n");

    {
        STK_DRIVER_DISK(_dkN);
        diskDriver = GetDriverObjectByName(_dkN);
        STK_WIPE_W(_dkN, 13);
    }
    if (!diskDriver) {
        SPOOF_DBG("[Spoofer] Schicht1: disk.sys not found\n");
        return STATUS_NOT_FOUND;
    }

    RtlZeroMemory(g_PatchedDevices, sizeof(g_PatchedDevices));

    deviceObject = diskDriver->DeviceObject;
    while (deviceObject) {
        PVOID devExt = deviceObject->DeviceExtension;

        if (devExt) {
            __try {
                if (PatchSingleDescriptor(deviceObject, devExt, seed)) {
                    patchedCount++;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                SPOOF_DBG("[Spoofer] Schicht1: Exception patching DevObj %p: 0x%X\n",
                    deviceObject, GetExceptionCode());
            }
        }

        deviceObject = deviceObject->NextDevice;
    }

    {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(diskDriver);
    }

    g_Spoofer.DescriptorsPatched = (patchedCount > 0);
    g_Spoofer.DevicesPatchedCount = patchedCount;

    SPOOF_DBG("[Spoofer] Schicht1: Patched %lu device descriptors\n", patchedCount);

    return (patchedCount > 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

static UCHAR HexToNibble(char c)
{
    if (c >= '0' && c <= '9') return (UCHAR)(c - '0');
    if (c >= 'A' && c <= 'F') return (UCHAR)(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (UCHAR)(c - 'a' + 10);
    return 0xFF;
}

static ULONG SerialToRawEui(const char* serial, PUCHAR outBytes, ULONG maxBytes)
{
    ULONG byteCount = 0;
    ULONG nibbleCount = 0;
    UCHAR currentByte = 0;
    SIZE_T i;

    for (i = 0; serial[i] != 0 && byteCount < maxBytes; i++) {
        UCHAR nib = HexToNibble(serial[i]);
        if (nib == 0xFF) continue; 

        if (nibbleCount % 2 == 0) {
            currentByte = (UCHAR)(nib << 4);
        } else {
            currentByte |= nib;
            outBytes[byteCount++] = currentByte;
        }
        nibbleCount++;
    }
    return byteCount;
}

static ULONG ScanAndPatchBytes(
    PUCHAR memory,
    ULONG memorySize,
    const UCHAR* pattern,
    ULONG patternSize,
    const UINT8* seed,
    const char* source)
{
    ULONG matches = 0;
    ULONG i;

    if (!memory || memorySize < patternSize || patternSize < 4) return 0;

    for (i = 0; i <= memorySize - patternSize; i++) {
        if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(memory + i, pattern, patternSize) == patternSize) {
            SPOOF_DBG("[Spoofer] %s: Found EUI match at offset +0x%X\n", source, i);
            SpoofBinaryData(memory + i, patternSize, seed, source);
            matches++;
            i += patternSize - 1;
        }
    }
    return matches;
}

static ULONG ScanAndPatchText(
    PUCHAR memory,
    ULONG memorySize,
    const char* searchText,
    const char* replaceText,
    ULONG textLen,
    const char* source)
{
    ULONG matches = 0;
    ULONG i;

    if (!memory || !searchText || !replaceText || memorySize < textLen || textLen < 4) return 0;

    for (i = 0; i <= memorySize - textLen; i++) {
        if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(memory + i, searchText, textLen) == textLen) {
            SPOOF_DBG("[Spoofer] %s: Found TEXT EUI at offset +0x%X: '%.*s'\n",
                source, i, textLen, memory + i);
            RtlCopyMemory(memory + i, replaceText, textLen);
            matches++;
            i += textLen - 1;
        }
    }
    return matches;
}

static ULONG ScanAndPatchTextWide(
    PUCHAR memory,
    ULONG memorySize,
    const char* searchText,
    const char* replaceText,
    ULONG textLen,
    const char* source)
{
    ULONG matches = 0;
    ULONG wideLen = textLen * 2;  
    ULONG i, j;
    BOOLEAN found;

    if (!memory || !searchText || !replaceText || memorySize < wideLen || textLen < 4) return 0;

    for (i = 0; i <= memorySize - wideLen; i++) {
        found = TRUE;
        for (j = 0; j < textLen; j++) {
            if (memory[i + j * 2] != (UCHAR)searchText[j] ||
                memory[i + j * 2 + 1] != 0x00) {
                found = FALSE;
                break;
            }
        }

        if (found) {
            SPOOF_DBG("[Spoofer] %s: Found WIDE TEXT EUI at offset +0x%X\n", source, i);
            for (j = 0; j < textLen; j++) {
                memory[i + j * 2] = (UCHAR)replaceText[j];
            }
            matches++;
            i += wideLen - 1;
        }
    }
    return matches;
}

static void PatchNvmeIdentity(const UINT8* seed)
{
    UCHAR originalEui[16] = { 0 };
    ULONG euiLen = 0;
    ULONG totalMatches = 0;
    ULONG i;

    SPOOF_DBG("[Spoofer] ============ SCHICHT 1b: stornvme EUI Patch ============\n");

    if (!g_PatchedDevices[0].Used || g_PatchedDevices[0].OriginalSerialLen < 4) {
        SPOOF_DBG("[Spoofer] Schicht1b: No original serial captured, skipping\n");
        return;
    }

    euiLen = SerialToRawEui(g_PatchedDevices[0].OriginalSerial, originalEui, 16);
    if (euiLen < 4) {
        SPOOF_DBG("[Spoofer] Schicht1b: Could not derive EUI (%lu bytes)\n", euiLen);
        return;
    }

    SPOOF_DBG("[Spoofer] Schicht1b: EUI pattern (%lu bytes): %02X %02X %02X %02X %02X %02X %02X %02X\n",
        euiLen,
        euiLen > 0 ? originalEui[0] : 0, euiLen > 1 ? originalEui[1] : 0,
        euiLen > 2 ? originalEui[2] : 0, euiLen > 3 ? originalEui[3] : 0,
        euiLen > 4 ? originalEui[4] : 0, euiLen > 5 ? originalEui[5] : 0,
        euiLen > 6 ? originalEui[6] : 0, euiLen > 7 ? originalEui[7] : 0);

    for (i = 0; i < MAX_PATCHED_DEVICES; i++) {
        PDEVICE_OBJECT diskDevObj;
        PDEVICE_OBJECT lowerDevice;
        PDEVICE_OBJECT currentDevice;

        if (!g_PatchedDevices[i].Used) continue;
        diskDevObj = g_PatchedDevices[i].DeviceObject;
        if (!diskDevObj) continue;

        SPOOF_DBG("[Spoofer] Schicht1b: Walking stack from disk DevObj %p\n", diskDevObj);

        currentDevice = ((fn_IoGetLowerDeviceObject)ApiResolveExport(HASH_IOGETLOWERDEVICEOBJECT))(diskDevObj);
        __try {
            while (currentDevice) {
                PVOID devExt = currentDevice->DeviceExtension;

                SPOOF_DBG("[Spoofer] Schicht1b:   -> DevObj %p Driver=%wZ\n",
                    currentDevice, &currentDevice->DriverObject->DriverName);

                if (devExt && ((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(devExt)) {
                    ULONG scanSize = 32768;
                    ULONG matches;
                    ULONG ptrIdx;

                    while (scanSize > 1024) {
                        if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))((PUCHAR)devExt + scanSize - 1)) break;
                        scanSize /= 2;
                    }

                    if (scanSize >= 1024) {
                        matches = ScanAndPatchBytes(
                            (PUCHAR)devExt, scanSize,
                            originalEui, euiLen, seed, "NVME_STACK");
                        totalMatches += matches;

                        {
                            PVOID* ptrArray = (PVOID*)devExt;
                            ULONG maxPtrs = scanSize / sizeof(PVOID);
                            if (maxPtrs > 4096) maxPtrs = 4096;

                            for (ptrIdx = 0; ptrIdx < maxPtrs; ptrIdx++) {
                                PVOID ptr;
                                __try { ptr = ptrArray[ptrIdx]; } __except(1) { continue; }

                                if ((ULONG_PTR)ptr < 0xFFFF800000000000ULL) continue;
                                if ((ULONG_PTR)ptr > 0xFFFFFFFFFFFFF000ULL) continue;
                                if (ptr == devExt) continue;

                                __try {
                                    ULONG pSize = 2048;
                                    if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(ptr)) continue;
                                    while (pSize > 128 && !((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))((PUCHAR)ptr + pSize - 1))
                                        pSize /= 2;
                                    if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))((PUCHAR)ptr + pSize - 1)) continue;

                                    matches = ScanAndPatchBytes(
                                        (PUCHAR)ptr, pSize,
                                        originalEui, euiLen, seed, "NVME_STACK_PTR");
                                    totalMatches += matches;
                                }
                                __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
                            }
                        }
                    }
                }

                lowerDevice = ((fn_IoGetLowerDeviceObject)ApiResolveExport(HASH_IOGETLOWERDEVICEOBJECT))(currentDevice);
                {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(currentDevice);
    }
                currentDevice = lowerDevice;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SPOOF_DBG("[Spoofer] Schicht1b: Exception walking stack: 0x%X\n",
                GetExceptionCode());
        }
    }


    SPOOF_DBG("[Spoofer] Schicht1b: Patched %lu EUI matches total\n", totalMatches);
}

void RestoreDeviceDescriptors(void)
{
    ULONG i;

    for (i = 0; i < MAX_PATCHED_DEVICES; i++) {
        PPATCHED_DEVICE slot = &g_PatchedDevices[i];

        if (!slot->Used || !slot->DescriptorPtr) continue;

        __try {
            PSTORAGE_DEVICE_DESCRIPTOR_MINI desc =
                (PSTORAGE_DEVICE_DESCRIPTOR_MINI)slot->DescriptorPtr;

            if (((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(desc) &&
                desc->SerialNumberOffset == slot->OriginalSerialOffset) {
                PCHAR serial = (PCHAR)desc + desc->SerialNumberOffset;
                RtlCopyMemory(serial, slot->OriginalSerial, slot->OriginalSerialLen);
                SPOOF_DBG("[Spoofer] Schicht1: Restored serial for DevObj %p\n",
                    slot->DeviceObject);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            SPOOF_DBG("[Spoofer] Schicht1: Exception restoring DevObj %p\n",
                slot->DeviceObject);
        }

        slot->Used = FALSE;
    }

    g_Spoofer.DescriptorsPatched = FALSE;
    g_Spoofer.DevicesPatchedCount = 0;

    SPOOF_DBG("[Spoofer] Schicht1: All descriptors restored\n");
}
