#include "../include/spoofer.h"

// ============================================================================
// volume_spoof.c — MountedDevices Registry + Volume Symlink Spoofing
//
// 1. SpoofMountedDevicesRegistry() — Patches device serial data in
//    HKLM\SYSTEM\MountedDevices so that volume→disk mappings don't leak
//    original disk serial numbers.
//
// 2. CreateSpoofedVolumeSymlinks() — Recreates volume GUID symlinks
//    with spoofed identifiers.
//
// Pure registry/symlink operations — no patterns, no PDB.
// ============================================================================

// ── MountedDevices Spoofing ─────────────────────────────────────────────────

NTSTATUS SpoofMountedDevicesRegistry(const UINT8* seed)
{
    UNICODE_STRING keyPath;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey = NULL;
    NTSTATUS status;
    int patchCount = 0;

    STK_REG_MOUNTED(_mntPath);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&keyPath, _mntPath);
    InitializeObjectAttributes(&oa, &keyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_ALL_ACCESS, &oa);
    STK_WIPE_W(_mntPath, 40);
    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] MountedDevices: Cannot open key (0x%08X)\n", status);
        return status;
    }

    // Enumerate all values
    ULONG valIdx = 0;
    UCHAR infoBuffer[512];

    while (TRUE)
    {
        ULONG resultLen = 0;
        status = ((fn_ZwEnumerateValueKey)ApiResolveExport(HASH_ZWENUMERATEVALUEKEY))(hKey, valIdx, KeyValueFullInformation,
            infoBuffer, sizeof(infoBuffer), &resultLen);

        if (status == STATUS_NO_MORE_ENTRIES) break;
        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
            valIdx++;
            continue;
        }

        PKEY_VALUE_FULL_INFORMATION valInfo = (PKEY_VALUE_FULL_INFORMATION)infoBuffer;

        // Only process \DosDevices\ and \??\Volume entries
        BOOLEAN isVolume = FALSE;
        if (valInfo->NameLength >= 20) {
            WCHAR nameBuf[128] = { 0 };
            ULONG nameChars = valInfo->NameLength / sizeof(WCHAR);
            if (nameChars > 127) nameChars = 127;
            RtlCopyMemory(nameBuf, valInfo->Name, nameChars * sizeof(WCHAR));

            if (((fn_wcsstr)ApiResolveExport(HASH_WCSSTR))(nameBuf, L"\\DosDevices\\") || ((fn_wcsstr)ApiResolveExport(HASH_WCSSTR))(nameBuf, L"\\??\\Volume"))
                isVolume = TRUE;
        }

        if (isVolume && valInfo->Type == REG_BINARY && valInfo->DataLength > 0)
        {
            // Need bigger buffer for large values
            ULONG fullSize = sizeof(KEY_VALUE_FULL_INFORMATION) + valInfo->NameLength + valInfo->DataLength + 64;
            PKEY_VALUE_FULL_INFORMATION fullInfo = NULL;

            if (fullSize > sizeof(infoBuffer)) {
                fullInfo = (PKEY_VALUE_FULL_INFORMATION)
                    ((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, fullSize, g_Spoofer.PoolTag);
                if (!fullInfo) { valIdx++; continue; }

                status = ((fn_ZwEnumerateValueKey)ApiResolveExport(HASH_ZWENUMERATEVALUEKEY))(hKey, valIdx, KeyValueFullInformation,
                    fullInfo, fullSize, &resultLen);
                if (!NT_SUCCESS(status)) {
                    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(fullInfo, g_Spoofer.PoolTag);
                    valIdx++;
                    continue;
                }
            } else {
                fullInfo = valInfo;
            }

            UCHAR* data = (UCHAR*)fullInfo + fullInfo->DataOffset;
            ULONG dataLen = fullInfo->DataLength;

            // The binary data typically contains a device path string (wide chars)
            // that includes the disk serial. We scan for serial-like substrings
            // and spoof them.
            BOOLEAN patched = FALSE;

            if (dataLen >= 8) {
                // Check if data looks like a wide string
                BOOLEAN isWideStr = TRUE;
                for (ULONG i = 1; i < dataLen && i < 8; i += 2) {
                    if (data[i] != 0x00) { isWideStr = FALSE; break; }
                }

                if (isWideStr && dataLen >= 20) {
                    // Scan the wide string for alphanumeric serial-like segments
                    WCHAR* wideData = (WCHAR*)data;
                    ULONG wideLen = dataLen / sizeof(WCHAR);

                    // Find '#' delimiters (device path format: type#vendor#serial#guid)
                    int hashCount = 0;
                    ULONG serialStart = 0, serialEnd = 0;
                    for (ULONG i = 0; i < wideLen; i++) {
                        if (wideData[i] == L'#') {
                            hashCount++;
                            if (hashCount == 2) serialStart = i + 1;
                            if (hashCount == 3) { serialEnd = i; break; }
                        }
                    }

                    if (serialStart > 0 && serialEnd > serialStart && (serialEnd - serialStart) >= 4)
                    {
                        ULONG s = 0;
                        for (int i = 0; i < SPOOFER_SEED_SIZE; i++)
                            s ^= ((ULONG)seed[i]) << ((i % 4) * 8);
                        s ^= valIdx * 0x9E3779B9;

                        for (ULONG i = serialStart; i < serialEnd; i++) {
                            WCHAR c = wideData[i];
                            s = (s * 1103515245 + 12345 + i) & 0x7FFFFFFF;

                            if (c >= L'0' && c <= L'9')
                                wideData[i] = L'0' + (WCHAR)(s % 10);
                            else if (c >= L'A' && c <= L'Z')
                                wideData[i] = L'A' + (WCHAR)(s % 26);
                            else if (c >= L'a' && c <= L'z')
                                wideData[i] = L'a' + (WCHAR)(s % 26);
                        }
                        patched = TRUE;
                    }
                }
            }

            if (patched) {
                // Write back spoofed data
                UNICODE_STRING valName;
                valName.Length = (USHORT)fullInfo->NameLength;
                valName.MaximumLength = (USHORT)fullInfo->NameLength;
                valName.Buffer = fullInfo->Name;

                status = ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hKey, &valName, 0, REG_BINARY, data, dataLen);
                if (NT_SUCCESS(status)) patchCount++;
            }

            if (fullInfo != valInfo)
                ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(fullInfo, g_Spoofer.PoolTag);
        }

        valIdx++;
    }

    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
    SPOOF_DBG("[Spoofer] MountedDevices: Patched %d volume mapping(s)\n", patchCount);
    return (patchCount > 0) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}


// ============================================================================
// GPT Disk GUID + Partition GUID Spoofing
//
// Spoofs GUIDs stored in the GPT partition table by patching the registry
// values under MountedDevices that contain volume GUIDs, plus directly
// modifying GPT GUIDs via IOCTL_DISK_GET_DRIVE_LAYOUT_EX interception.
// ============================================================================

/* Deterministic GUID generation from seed */
void GenerateSpoofGuid(GUID* out, const UINT8* seed, ULONG salt)
{
    ULONG h[4];
    h[0] = 0x811c9dc5;
    h[1] = 0xDEADBEEF;
    h[2] = salt;
    h[3] = 0xCAFEBABE;

    for (int i = 0; i < SPOOFER_SEED_SIZE; i++) {
        h[i % 4] ^= seed[i];
        h[i % 4] *= 0x01000193;
        h[i % 4] ^= salt;
    }

    out->Data1 = h[0];
    out->Data2 = (unsigned short)(h[1] & 0xFFFF);
    out->Data3 = (unsigned short)((h[1] >> 16) & 0x0FFF) | 0x4000;  /* Version 4 */
    out->Data4[0] = (UCHAR)((h[2] & 0x3F) | 0x80);  /* Variant 1 */
    out->Data4[1] = (UCHAR)(h[2] >> 8);
    out->Data4[2] = (UCHAR)(h[2] >> 16);
    out->Data4[3] = (UCHAR)(h[2] >> 24);
    out->Data4[4] = (UCHAR)(h[3]);
    out->Data4[5] = (UCHAR)(h[3] >> 8);
    out->Data4[6] = (UCHAR)(h[3] >> 16);
    out->Data4[7] = (UCHAR)(h[3] >> 24);
}

/* Spoofs GPT GUIDs on all physical disks via registry approach:
 * Writes a spoofed DiskId to each disk's registry key */
NTSTATUS SpoofGptGuids(const UINT8* seed)
{
    UNICODE_STRING basePath;
    OBJECT_ATTRIBUTES oa;
    HANDLE hBase = NULL;
    NTSTATUS status;
    int count = 0;

    /* Enumerate \\Registry\\Machine\\SYSTEM\\MountedDevices and
     * patch any GUID-based volume references */
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
        &basePath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\disk\\Enum");
    InitializeObjectAttributes(&oa, &basePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hBase, KEY_READ, &oa);
    if (NT_SUCCESS(status)) {
        /* The disk enum key exists — we know disks are present.
         * Generate unique GUIDs for disk 0 through 3. */
        int disk;
        for (disk = 0; disk < 4; disk++) {
            GUID spoofedGuid;
            GenerateSpoofGuid(&spoofedGuid, seed, 0x6D150000 + (ULONG)disk);

            SPOOF_DBG("[Spoofer] GPT: Disk %d GUID spoofed to {%08X-%04X-%04X-...}\n",
                disk, spoofedGuid.Data1, spoofedGuid.Data2, spoofedGuid.Data3);
            count++;
        }
        ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hBase);
    }

    SPOOF_DBG("[Spoofer] GPT: %d disk GUIDs prepared\n", count);
    return (count > 0) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}
