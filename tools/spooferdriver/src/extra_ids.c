#include "../include/spoofer.h"

// ============================================================================
// extra_ids.c — Additional registry/file spoofs adopted from ApocWork
//
// Covers: NVIDIA IDs, ProcessorId, GraphicsDrivers timestamps,
//         Office MotherboardUUID, Windows Activation MachineId,
//         IE Migration date, TPM ODUID, setupapi.dev.log cleanup,
//         SoftwareProtectionPlatform, MachineGuid.txt file.
//
// Uses the same helpers as windows_ids.c (SetRegSzValue, etc.)
// ============================================================================

/* Forward-declare helpers from windows_ids.c */
extern NTSTATUS SetRegSzValue(const WCHAR* keyPath, const WCHAR* valueName, const char* ansiValue);
extern NTSTATUS SetRegBinaryValue(const WCHAR* keyPath, const WCHAR* valueName, PVOID data, ULONG dataLen);
extern NTSTATUS SetRegDwordValue(const WCHAR* keyPath, const WCHAR* valueName, ULONG value);

static ULONG ExHash(ULONG input)
{
    input = (input ^ (input >> 16)) * 0x45D9F3B;
    input = (input ^ (input >> 16)) * 0x45D9F3B;
    input = input ^ (input >> 16);
    return input;
}

static void GenGuidNoBraces(ULONG seed, WCHAR* out, ULONG maxChars)
{
    ULONG p[6];
    int i;
    for (i = 0; i < 6; i++)
        p[i] = ExHash(seed ^ (ULONG)i);
    RtlStringCbPrintfW(out, maxChars * sizeof(WCHAR),
        L"%08X-%04X-%04X-%04X-%04X%08X",
        p[0], (p[1] >> 16) & 0xFFFF, (p[2] >> 16) & 0xFFFF,
        (p[3] >> 12) & 0xFFFF, (p[4] >> 16) & 0xFFFF, p[5]);
}

static ULONG DeriveSeed(const UINT8* seed)
{
    ULONG s = 0x811C9DC5;
    int i;
    for (i = 0; i < SPOOFER_SEED_SIZE; i++) {
        s ^= (ULONG)seed[i];
        s *= 0x01000193;
    }
    return s;
}

// ── Wide-string registry write helper (no ANSI conversion needed) ──────────
static NTSTATUS SetRegSzValueW(const WCHAR* keyPath, const WCHAR* valueName, const WCHAR* data)
{
    UNICODE_STRING kp, vn;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey = NULL;
    NTSTATUS status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&kp, (PWSTR)keyPath);
    InitializeObjectAttributes(&oa, &kp, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_SET_VALUE, &oa);
    if (!NT_SUCCESS(status)) {
        /* Try create */
        status = ((fn_ZwCreateKey)ApiResolveExport(HASH_ZWCREATEKEY))(
            &hKey, KEY_SET_VALUE, &oa, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
        if (!NT_SUCCESS(status)) return status;
    }

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&vn, (PWSTR)valueName);

    ULONG len = 0;
    const WCHAR* p = data;
    while (*p++) len++;
    len = (len + 1) * sizeof(WCHAR);

    status = ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(
        hKey, &vn, 0, REG_SZ, (PVOID)data, len);

    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
    return status;
}

// ============================================================================
// Main entry point
// ============================================================================
NTSTATUS SpoofExtraIdentifiers(const UINT8* seed)
{
    ULONG s = DeriveSeed(seed);
    int count = 0;

    // ── 1. NVIDIA Corporation Registry IDs ──────────────────────────────────
    {
        WCHAR buf[128];

        // ClientUUID
        GenGuidNoBraces(s ^ 0xC0C, buf, 128);
        if (NT_SUCCESS(SetRegSzValueW(
            L"\\Registry\\Machine\\SOFTWARE\\NVIDIA Corporation\\Global",
            L"ClientUUID", buf))) count++;

        // PersistenceIdentifier
        ULONG persist = ExHash(s ^ 0xD0D);
        RtlStringCbPrintfW(buf, sizeof(buf), L"%08X", persist);
        if (NT_SUCCESS(SetRegSzValueW(
            L"\\Registry\\Machine\\SOFTWARE\\NVIDIA Corporation\\Global",
            L"PersistenceIdentifier", buf))) count++;

        // ChipsetMatchID
        GenGuidNoBraces(s ^ 0xE0E, buf, 128);
        if (NT_SUCCESS(SetRegSzValueW(
            L"\\Registry\\Machine\\SOFTWARE\\NVIDIA Corporation\\Global\\CoProcManager",
            L"ChipsetMatchID", buf))) count++;

        SPOOF_DBG("[Spoofer] ExtraID: NVIDIA 3 keys spoofed\n");
    }

    // ── 2. ProcessorId ──────────────────────────────────────────────────────
    {
        ULONG eax = ExHash(s ^ 0xA0A);
        ULONG edx = eax ^ 0x178BFBFF;
        WCHAR procId[32];
        RtlStringCbPrintfW(procId, sizeof(procId), L"%08X%08X", edx, eax);
        if (NT_SUCCESS(SetRegSzValueW(
            L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
            L"ProcessorId", procId))) count++;
        SPOOF_DBG("[Spoofer] ExtraID: ProcessorId spoofed\n");
    }

    // ── 3. GraphicsDrivers Configuration Timestamps ─────────────────────────
    {
        UNICODE_STRING kp;
        OBJECT_ATTRIBUTES oa;
        HANDLE hKey = NULL;

        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
            &kp, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Configuration");
        InitializeObjectAttributes(&oa, &kp, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        if (NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_ENUMERATE_SUB_KEYS, &oa)))
        {
            ULONG subIdx = 0;
            UCHAR subBuf[512];

            while (TRUE)
            {
                ULONG resultLen = 0;
                NTSTATUS st = ((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(
                    hKey, subIdx, KeyBasicInformation, subBuf, sizeof(subBuf), &resultLen);
                if (!NT_SUCCESS(st)) break;
                subIdx++;

                PKEY_BASIC_INFORMATION kbi = (PKEY_BASIC_INFORMATION)subBuf;
                ULONG nameChars = kbi->NameLength / sizeof(WCHAR);
                if (nameChars >= 250) continue;

                /* Build full path */
                WCHAR fullPath[512];
                WCHAR subName[256];
                ULONG j;
                for (j = 0; j < nameChars; j++) subName[j] = kbi->Name[j];
                subName[nameChars] = L'\0';

                RtlStringCbPrintfW(fullPath, sizeof(fullPath),
                    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers\\Configuration\\%s",
                    subName);

                /* Generate spoofed timestamp binary */
                UCHAR ts[8];
                ULONG tsSeed = ExHash(s ^ 0xAAA ^ subIdx);
                for (j = 0; j < 8; j++) {
                    tsSeed = ExHash(tsSeed);
                    ts[j] = (UCHAR)(tsSeed & 0xFF);
                }
                SetRegBinaryValue(fullPath, L"Timestamp", ts, 8);
                count++;
            }
            ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
            SPOOF_DBG("[Spoofer] ExtraID: %d GfxConfig timestamps spoofed\n", subIdx);
        }
    }

    // ── 4. Office MotherboardUUID (all user SIDs) ───────────────────────────
    {
        UNICODE_STRING kp;
        OBJECT_ATTRIBUTES oa;
        HANDLE hKey = NULL;

        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
            &kp, L"\\Registry\\User");
        InitializeObjectAttributes(&oa, &kp, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        if (NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_ENUMERATE_SUB_KEYS, &oa)))
        {
            ULONG userIdx = 0;
            UCHAR userBuf[512];

            while (TRUE)
            {
                ULONG resultLen = 0;
                NTSTATUS st = ((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(
                    hKey, userIdx, KeyBasicInformation, userBuf, sizeof(userBuf), &resultLen);
                if (!NT_SUCCESS(st)) break;
                userIdx++;

                PKEY_BASIC_INFORMATION kbi = (PKEY_BASIC_INFORMATION)userBuf;
                ULONG nameChars = kbi->NameLength / sizeof(WCHAR);
                if (nameChars >= 200 || nameChars < 9) continue;

                WCHAR sidName[200];
                ULONG j;
                for (j = 0; j < nameChars; j++) sidName[j] = kbi->Name[j];
                sidName[nameChars] = L'\0';

                /* Only process S-1-5-21-* and .DEFAULT */
                BOOLEAN isSid = (sidName[0] == L'S' && sidName[2] == L'1');
                BOOLEAN isDef = (sidName[0] == L'.' && sidName[1] == L'D');
                if (!isSid && !isDef) continue;

                WCHAR officePath[256];
                RtlStringCbPrintfW(officePath, sizeof(officePath),
                    L"\\Registry\\User\\%s\\Software\\Microsoft\\Office\\Common\\ClientTelemetry",
                    sidName);

                WCHAR guidBuf[64];
                GenGuidNoBraces(s ^ userIdx, guidBuf, 60);
                WCHAR mbUuid[64];
                RtlStringCbPrintfW(mbUuid, sizeof(mbUuid), L"{%s}", guidBuf);
                SetRegSzValueW(officePath, L"MotherboardUUID", mbUuid);
                count++;
            }
            ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
            SPOOF_DBG("[Spoofer] ExtraID: Office MotherboardUUID spoofed\n");
        }
    }

    // ── 5. Windows Activation MachineId ─────────────────────────────────────
    {
        UCHAR actId[64];
        ULONG actSeed = ExHash(s ^ 0x808);
        ULONG j;
        for (j = 0; j < 64; j++) {
            actSeed = ExHash(actSeed);
            actId[j] = (UCHAR)(actSeed & 0xFF);
        }
        if (NT_SUCCESS(SetRegBinaryValue(
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows Activation Technologies\\AdminObject\\Store",
            L"MachineId", actId, 64))) count++;
        SPOOF_DBG("[Spoofer] ExtraID: Activation MachineId spoofed\n");
    }

    // ── 6. IE Migration Installed Date ──────────────────────────────────────
    {
        UCHAR ieDate[96];
        ULONG ieSeed = ExHash(s ^ 0x505);
        ULONG j;
        for (j = 0; j < 96; j++) {
            ieSeed = ExHash(ieSeed);
            ieDate[j] = (UCHAR)(ieSeed & 0xFF);
        }
        if (NT_SUCCESS(SetRegBinaryValue(
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Internet Explorer\\Migration",
            L"IE Installed Date", ieDate, 96))) count++;
    }

    // ── 7. TPM ODUID RandomSeed ─────────────────────────────────────────────
    {
        ULONG oduid = ExHash(s ^ 0x606);
        if (NT_SUCCESS(SetRegDwordValue(
            L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\TPM\\ODUID",
            L"RandomSeed", oduid))) count++;
    }

    // ── 8. SoftwareProtectionPlatform BackupProductKeyDefault ────────────────
    {
        static const char charset[] = "BCDFGHJKMPQRTVWXY2346789";
        char key[30];
        ULONG keySeed = ExHash(s ^ 0x909);
        int i, pos = 0;
        for (i = 0; i < 25; i++) {
            if (i > 0 && (i % 5) == 0) key[pos++] = '-';
            keySeed = ExHash(keySeed ^ (ULONG)i);
            key[pos++] = charset[keySeed % 24];
        }
        key[pos] = '\0';
        SetRegSzValue(
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SoftwareProtectionPlatform",
            L"BackupProductKeyDefault", key);
        count++;
    }

    // ── 9. OneSettings deviceId (Windows Update telemetry) ──────────────────
    {
        WCHAR devId[128];
        GenGuidNoBraces(s ^ 0x707, devId, 128);
        SetRegSzValueW(
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OneSettings\\WSD\\UpdateAgent\\QueryParameters",
            L"deviceId", devId);
        SetRegSzValueW(
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OneSettings\\WSD\\Setup360\\QueryParameters",
            L"deviceId", devId);
        count += 2;
    }

    // ── 10. Win32kWPP TraceGuid ─────────────────────────────────────────────
    {
        WCHAR traceGuid[128];
        GenGuidNoBraces(s ^ 0x404, traceGuid + 1, 126);
        traceGuid[0] = L'{';
        ULONG len = 0;
        while (traceGuid[len + 1]) len++;
        traceGuid[len + 1] = L'}';
        traceGuid[len + 2] = L'\0';
        SetRegSzValueW(
            L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows\\Win32kWPP\\Parameters",
            L"WppRecorder_TraceGuid", traceGuid);
        count++;
    }

    // ── 11. MachineGuid.txt file ────────────────────────────────────────────
    {
        UNICODE_STRING filePath;
        OBJECT_ATTRIBUTES foa;
        HANDLE hFile = NULL;
        IO_STATUS_BLOCK iosb;

        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
            &filePath, L"\\??\\C:\\Windows\\System32\\restore\\MachineGuid.txt");
        InitializeObjectAttributes(&foa, &filePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        NTSTATUS fst = ((fn_ZwCreateFile)ApiResolveExport(HASH_ZWCREATEFILE))(
            &hFile, GENERIC_WRITE, &foa, &iosb, NULL,
            FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
            FILE_SUPERSEDE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

        if (NT_SUCCESS(fst)) {
            char guid[64] = { 0 };
            ULONG gh = ExHash(s ^ 0xBEEF);
            ULONG gh2 = ExHash(gh);
            RtlStringCbPrintfA(guid, sizeof(guid),
                "%08x-%04x-%04x-%04x-%04x%08x\r\n",
                gh, (gh >> 16) & 0xFFFF, (gh2 >> 8) & 0xFFFF,
                gh2 & 0xFFFF, (gh >> 4) & 0xFFFF, ExHash(gh2));

            ((fn_ZwWriteFile)ApiResolveExport(HASH_ZWWRITEFILE))(
                hFile, NULL, NULL, NULL, &iosb, guid,
                (ULONG)((fn_strlen)ApiResolveExport(HASH_STRLEN))(guid), NULL, NULL);
            ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hFile);
            count++;
        }
    }

    // ── 12. Cleanup setupapi.dev.log ────────────────────────────────────────
    {
        UNICODE_STRING filePath;
        OBJECT_ATTRIBUTES foa;
        HANDLE hFile = NULL;
        IO_STATUS_BLOCK iosb;

        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
            &filePath, L"\\??\\C:\\Windows\\INF\\setupapi.dev.log");
        InitializeObjectAttributes(&foa, &filePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        NTSTATUS fst = ((fn_ZwCreateFile)ApiResolveExport(HASH_ZWCREATEFILE))(
            &hFile, GENERIC_WRITE, &foa, &iosb, NULL,
            FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
            FILE_OVERWRITE_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

        if (NT_SUCCESS(fst)) {
            ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hFile);
            count++;
            SPOOF_DBG("[Spoofer] ExtraID: setupapi.dev.log cleared\n");
        }
    }

    SPOOF_DBG("[Spoofer] ExtraID: %d total extra value(s) spoofed\n", count);
    return (count > 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}
