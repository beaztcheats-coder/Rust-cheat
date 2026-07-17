#include "../include/spoofer.h"

// ============================================================================
// efi_spoof.c — EFI Firmware Variable Spoofing
//
// Spoofs EFI NVRAM variables that contain hardware identifiers:
//   - UnlockIDCopy, OfflineUniqueIDEKPub/CRC, PlatformModuleData
//   - OfflineUniqueIDRandomSeed/CRC
// Uses ExGet/SetFirmwareEnvironmentVariable — no patterns, no PDB.
// ============================================================================

// Format-preserving spoof: alphanumeric → random alphanumeric, 
// zero/0xFF → keep, other binary → random byte
static void SpoofBufferFormatPreserving(UINT8* out, const UINT8* in, ULONG len, const UINT8* seed)
{
    if (!out || !in || len == 0) return;

    // Derive a seed from the spoofer seed + buffer content hash
    ULONG s = 0;
    for (int i = 0; i < SPOOFER_SEED_SIZE; i++)
        s ^= ((ULONG)seed[i]) << ((i % 4) * 8);
    for (ULONG i = 0; i < len && i < 16; i++)
        s ^= ((ULONG)in[i]) << ((i % 4) * 8);

    for (ULONG i = 0; i < len; i++)
    {
        UINT8 orig = in[i];
        s = (s * 1103515245 + 12345 + i) & 0x7FFFFFFF;

        if ((orig >= 'A' && orig <= 'Z') || (orig >= 'a' && orig <= 'z') || (orig >= '0' && orig <= '9'))
        {
            static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
            out[i] = charset[s % (sizeof(charset) - 1)];
        }
        else if (orig == 0 || orig == 0xFF)
        {
            out[i] = orig;  // Preserve structure markers
        }
        else
        {
            out[i] = (UINT8)(s & 0xFF);
        }
    }
}

static BOOLEAN HandleFirmwareVariable(const WCHAR* entryName, const WCHAR* guidStr, const UINT8* seed)
{
    UNICODE_STRING guidUnicode;
    UNICODE_STRING entryUnicode;
    GUID guid;
    UCHAR buffer[256] = { 0 };
    UCHAR original[256] = { 0 };
    ULONG length = sizeof(buffer);
    ULONG attributes = 0;
    NTSTATUS status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&guidUnicode, guidStr);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&entryUnicode, entryName);
    
    status = ((fn_RtlGUIDFromString)ApiResolveExport(HASH_RTLGUIDFROMSTRING))(&guidUnicode, &guid);
    if (!NT_SUCCESS(status)) return FALSE;

    status = ((fn_ExGetFirmwareEnvironmentVariable)ApiResolveExport(HASH_EXGETFIRMWAREENVIRONMENTVARIABLE))(&entryUnicode, &guid, buffer, &length, &attributes);
    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] EFI: Variable '%ws' not found (0x%08X)\n", entryName, status);
        return FALSE;
    }

    // Save original
    RtlCopyMemory(original, buffer, length);

    // Spoof
    SpoofBufferFormatPreserving(buffer, original, length, seed);

    // Verify it actually changed
    if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(buffer, original, length) == length) {
        // Retry
        for (int retry = 0; retry < 5; retry++) {
            UINT8 extSeed[SPOOFER_SEED_SIZE];
            RtlCopyMemory(extSeed, seed, SPOOFER_SEED_SIZE);
            extSeed[0] ^= (UINT8)(retry + 1);
            SpoofBufferFormatPreserving(buffer, original, length, extSeed);
            if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(buffer, original, length) != length)
                break;
        }
        if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(buffer, original, length) == length)
            return FALSE;
    }

    // Write back
    attributes |= 1;  // EFI_VARIABLE_NON_VOLATILE
    status = ((fn_ExSetFirmwareEnvironmentVariable)ApiResolveExport(HASH_EXSETFIRMWAREENVIRONMENTVARIABLE))(&entryUnicode, &guid, buffer, length, attributes);
    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] EFI: Write '%ws' failed (0x%08X)\n", entryName, status);
        return FALSE;
    }

    SPOOF_DBG("[Spoofer] EFI: Spoofed '%ws' (%lu bytes)\n", entryName, length);
    return TRUE;
}

NTSTATUS SpoofEfiVariables(const UINT8* seed)
{
    int count = 0;

    // GUID: {eaec226f-c9a3-477a-a826-ddc716cdc0e3}
    static const WCHAR* efiGuid = L"{eaec226f-c9a3-477a-a826-ddc716cdc0e3}";
    // GUID: {1b463f9c-803c-49e4-b468-2868908479b2}
    static const WCHAR* tpmGuid = L"{1b463f9c-803c-49e4-b468-2868908479b2}";

    if (HandleFirmwareVariable(L"UnlockIDCopy", efiGuid, seed)) count++;
    if (HandleFirmwareVariable(L"OfflineUniqueIDEKPubCRC", efiGuid, seed)) count++;
    if (HandleFirmwareVariable(L"OfflineUniqueIDEKPub", efiGuid, seed)) count++;
    if (HandleFirmwareVariable(L"PlatformModuleData", tpmGuid, seed)) count++;
    if (HandleFirmwareVariable(L"OfflineUniqueIDRandomSeed", efiGuid, seed)) count++;
    if (HandleFirmwareVariable(L"OfflineUniqueIDRandomSeedCRC", efiGuid, seed)) count++;

    SPOOF_DBG("[Spoofer] EFI: %d variable(s) spoofed\n", count);
    return (count > 0) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}
