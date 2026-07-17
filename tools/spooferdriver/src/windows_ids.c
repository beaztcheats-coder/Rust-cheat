#include "../include/spoofer.h"

/* FILE_DIRECTORY_INFORMATION — not always in km headers */
#ifndef _FILE_DIRECTORY_INFORMATION_DEFINED
#define _FILE_DIRECTORY_INFORMATION_DEFINED
typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;
#endif

// ============================================================================
// windows_ids.c — Windows Identity Spoofing
//
// Spoofs system-unique identifiers in the registry:
//   - MachineGuid (Cryptography)
//   - ProductKey + DigitalProductId (Windows NT\CurrentVersion)
//   - ProductId (3 registry paths)
//   - MachineId (SQMClient)
// Pure registry writes via ZwSetValueKey — no patterns, no PDB.
// ============================================================================

// ── Helpers ─────────────────────────────────────────────────────────────────

static ULONG WinIdHash(ULONG input)
{
    input = (input ^ (input >> 16)) * 0x45D9F3B;
    input = (input ^ (input >> 16)) * 0x45D9F3B;
    input = input ^ (input >> 16);
    return input;
}

static ULONG DeriveSeedFromSpoofer(const UINT8* seed)
{
    ULONG s = 0;
    for (int i = 0; i < SPOOFER_SEED_SIZE; i++)
        s ^= ((ULONG)seed[i]) << ((i % 4) * 8);
    if (s == 0) s = 0x4B1E3A7C;
    return s;
}

/* Generate a deterministic spoofed GUID in a WCHAR buffer */
static void SpoofGuidInBuffer(PWCHAR data, ULONG dataLenBytes)
{
    ULONG wcharCount = dataLenBytes / sizeof(WCHAR);
    ULONG seed, i;
    WCHAR hex[] = L"0123456789abcdef";
    WCHAR guid[40];
    ULONG g;

    if (wcharCount < 38) return;

    seed = 0x6a09e667;
    for (i = 0; i < SPOOFER_SEED_SIZE; i++) {
        seed ^= (ULONG)g_Spoofer.Seed[i];
        seed *= 0x01000193;
    }
    for (i = 0; i < wcharCount && data[i] != 0; i++) {
        seed ^= (ULONG)data[i];
        seed *= 0x01000193;
    }

    guid[0] = L'{';
    g = 1;
    for (i = 0; i < 32; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20)
            guid[g++] = L'-';
        seed ^= (i * 0xdeadbeef);
        seed *= 0x01000193;
        guid[g++] = hex[(seed >> 4) & 0xF];
    }
    guid[g++] = L'}';
    guid[g] = 0;

    for (i = 0; i < g && i < wcharCount; i++)
        data[i] = guid[i];
    if (i < wcharCount) data[i] = 0;
}

// ── MachineGuid ─────────────────────────────────────────────────────────────

static void GenerateMachineGuid(ULONG seed, char* out, SIZE_T size)
{
    ULONG p[6];
    for (int i = 0; i < 6; i++)
        p[i] = WinIdHash(seed ^ (ULONG)i);

    RtlStringCbPrintfA(out, size,
        "%08x-%04x-%04x-%04x-%04x%08x",
        p[0], (p[1] >> 16) & 0xFFFF, (p[2] >> 16) & 0xFFFF,
        (p[3] >> 12) & 0xFFFF, (p[4] >> 16) & 0xFFFF, p[5]);
}

// ── ProductId ───────────────────────────────────────────────────────────────

static void GenerateProductId(ULONG seed, char* out, SIZE_T size)
{
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char raw[20];
    ULONG s = seed;

    for (int i = 0; i < 20; i++) {
        s = WinIdHash(s ^ (ULONG)i);
        raw[i] = charset[s % (sizeof(charset) - 1)];
    }

    // Format: XXXXX-XXXXX-XXXXX-XXXXX
    RtlStringCbPrintfA(out, size,
        "%c%c%c%c%c-%c%c%c%c%c-%c%c%c%c%c-%c%c%c%c%c",
        raw[0], raw[1], raw[2], raw[3], raw[4],
        raw[5], raw[6], raw[7], raw[8], raw[9],
        raw[10], raw[11], raw[12], raw[13], raw[14],
        raw[15], raw[16], raw[17], raw[18], raw[19]);
}

// ── ProductKey ──────────────────────────────────────────────────────────────

static void GenerateProductKey(ULONG seed, char* out, SIZE_T size)
{
    static const char charset[] = "BCDFGHJKMPQRTVWXY2346789";
    char raw[25];
    ULONG s = seed ^ 0x7E3A9C2E;

    for (int i = 0; i < 25; i++) {
        s = WinIdHash(s ^ (ULONG)i);
        raw[i] = charset[s % (sizeof(charset) - 1)];
    }

    RtlStringCbPrintfA(out, size,
        "%c%c%c%c%c-%c%c%c%c%c-%c%c%c%c%c-%c%c%c%c%c-%c%c%c%c%c",
        raw[0], raw[1], raw[2], raw[3], raw[4],
        raw[5], raw[6], raw[7], raw[8], raw[9],
        raw[10], raw[11], raw[12], raw[13], raw[14],
        raw[15], raw[16], raw[17], raw[18], raw[19],
        raw[20], raw[21], raw[22], raw[23], raw[24]);
}

// ── SQMClient MachineId ─────────────────────────────────────────────────────

static void GenerateCmId(ULONG seed, char* out, SIZE_T size)
{
    ULONG h = WinIdHash(seed ^ 0xC0FFEE);
    ULONG h2 = WinIdHash(h);

    RtlStringCbPrintfA(out, size,
        "{%08x-%04x-%04x-%04x-%04x%08x}",
        h, (h >> 16) & 0xFFFF, (h >> 24) & 0xFFFF,
        (h >> 8) & 0xFFFF, h2 & 0xFFFF, WinIdHash(h2));
}

// ── DigitalProductId ────────────────────────────────────────────────────────

static void GenerateDigitalProductId(ULONG seed, UCHAR* out, PULONG outSize)
{
    ULONG buffer[64] = { 0 };
    buffer[0] = 0x00040000;  // Version marker
    buffer[1] = seed;

    for (int i = 2; i < 64; i++)
        buffer[i] = WinIdHash(buffer[i - 1] ^ seed);

    ULONG copyLen = 256;
    if (*outSize < copyLen) copyLen = *outSize;
    RtlCopyMemory(out, buffer, copyLen);
    *outSize = 256;
}

// ── Helpers for registry writes ─────────────────────────────────────────────

static void AnsiToWide(const char* src, WCHAR* dst, SIZE_T dstChars)
{
    SIZE_T i = 0;
    while (src[i] && i < dstChars - 1) {
        dst[i] = (WCHAR)(UCHAR)src[i];
        i++;
    }
    dst[i] = L'\0';
}

NTSTATUS SetRegSzValue(const WCHAR* keyPath, const WCHAR* valueName, const char* ansiValue)
{
    UNICODE_STRING uKeyPath, uValueName;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey = NULL;
    NTSTATUS status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uKeyPath, keyPath);
    InitializeObjectAttributes(&oa, &uKeyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_SET_VALUE, &oa);
    if (!NT_SUCCESS(status)) return status;

    WCHAR wideValue[256] = { 0 };
    AnsiToWide(ansiValue, wideValue, 256);

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uValueName, valueName);
    status = ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hKey, &uValueName, 0, REG_SZ,
        wideValue, (ULONG)((((fn_wcslen)ApiResolveExport(HASH_WCSLEN))(wideValue) + 1) * sizeof(WCHAR)));

    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
    return status;
}

NTSTATUS SetRegBinaryValue(const WCHAR* keyPath, const WCHAR* valueName,
                                   PVOID data, ULONG dataLen)
{
    UNICODE_STRING uKeyPath, uValueName;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey = NULL;
    NTSTATUS status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uKeyPath, keyPath);
    InitializeObjectAttributes(&oa, &uKeyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_SET_VALUE, &oa);
    if (!NT_SUCCESS(status)) return status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uValueName, valueName);
    status = ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hKey, &uValueName, 0, REG_BINARY, data, dataLen);

    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
    return status;
}

NTSTATUS SetRegDwordValue(const WCHAR* keyPath, const WCHAR* valueName, ULONG value)
{
    UNICODE_STRING uKeyPath, uValueName;
    OBJECT_ATTRIBUTES oa;
    HANDLE hKey = NULL;
    NTSTATUS status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uKeyPath, keyPath);
    InitializeObjectAttributes(&oa, &uKeyPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_SET_VALUE, &oa);
    if (!NT_SUCCESS(status)) return status;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uValueName, valueName);
    status = ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hKey, &uValueName, 0, REG_DWORD, &value, sizeof(ULONG));

    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
    return status;
}

/* Helper: spoof Identifier + SerialNumber under a SCSI DeviceMap LU key */
static void SpoofScsiLuKey(HANDLE hParent, const WCHAR* subPath, ULONG seed)
{
    UNICODE_STRING uSub;
    OBJECT_ATTRIBUTES oa;
    HANDLE hLu = NULL;
    NTSTATUS status;
    char spoofedId[48] = { 0 };
    char spoofedSn[24] = { 0 };
    ULONG h;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uSub, subPath);
    InitializeObjectAttributes(&oa, &uSub, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hParent, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hLu, KEY_SET_VALUE | KEY_QUERY_VALUE, &oa);
    if (!NT_SUCCESS(status)) return;

    /* Generate spoofed Identifier — preserve original vendor/model, only change serial suffix */
    h = WinIdHash(seed ^ 0x5C51);
    {
        /* Read existing Identifier first to preserve vendor/model */
        UNICODE_STRING vnId;
        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&vnId, L"Identifier");
        UCHAR idBuf[256] = { 0 };
        ULONG idLen = 0;
        KEY_VALUE_PARTIAL_INFORMATION* idInfo = (KEY_VALUE_PARTIAL_INFORMATION*)idBuf;
        NTSTATUS idSt = ((fn_ZwQueryValueKey)ApiResolveExport(HASH_ZWQUERYVALUEKEY))(
            hLu, &vnId, KeyValuePartialInformation, idInfo, sizeof(idBuf), &idLen);

        if (NT_SUCCESS(idSt) && idInfo->Type == REG_SZ && idInfo->DataLength > 8) {
            /* Copy original identifier to spoofedId as ANSI */
            WCHAR* origW = (WCHAR*)idInfo->Data;
            ULONG origChars = idInfo->DataLength / sizeof(WCHAR);
            if (origChars > 47) origChars = 47;
            for (ULONG i = 0; i < origChars && origW[i]; i++)
                spoofedId[i] = (char)origW[i];
            spoofedId[origChars] = 0;

            /* Find the serial suffix (last 4+ alphanumeric chars) and spoof them */
            ULONG len = 0;
            while (spoofedId[len]) len++;
            /* Trim trailing spaces */
            while (len > 0 && spoofedId[len-1] == ' ') len--;
            /* Spoof the last 4 chars (serial portion) */
            ULONG serPart = WinIdHash(h ^ 0xA7B3);
            if (len >= 4) {
                spoofedId[len-4] = 'A' + (char)(serPart % 26);
                spoofedId[len-3] = '0' + (char)((serPart >> 5) % 10);
                spoofedId[len-2] = 'A' + (char)((serPart >> 9) % 26);
                spoofedId[len-1] = '0' + (char)((serPart >> 13) % 10);
            }
        } else {
            /* Fallback: use Samsung model since that's the actual disk */
            ULONG serPart = WinIdHash(h ^ 0xA7B3);
            RtlStringCbPrintfA(spoofedId, sizeof(spoofedId),
                "NVMe    Samsung SSD 990 %c%c%c%c",
                'A' + (char)(serPart % 26), '0' + (char)((serPart >> 5) % 10),
                'A' + (char)((serPart >> 9) % 26), '0' + (char)((serPart >> 13) % 10));
        }
    }

    /* Generate spoofed SerialNumber — read existing and apply same SpoofSerialString
     * as the IOCTL hook (dispatch_hook.c) so WMI and DeviceMap are consistent. */
    {
        UNICODE_STRING vnSn;
        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&vnSn, L"SerialNumber");
        UCHAR snBuf[256] = { 0 };
        ULONG snLen = 0;
        KEY_VALUE_PARTIAL_INFORMATION* snInfo = (KEY_VALUE_PARTIAL_INFORMATION*)snBuf;
        NTSTATUS snSt = ((fn_ZwQueryValueKey)ApiResolveExport(HASH_ZWQUERYVALUEKEY))(
            hLu, &vnSn, KeyValuePartialInformation, snInfo, sizeof(snBuf), &snLen);

        if (NT_SUCCESS(snSt) && snInfo->Type == REG_SZ && snInfo->DataLength > 4) {
            /* Convert wide string to ANSI for SpoofSerialString */
            WCHAR* origSnW = (WCHAR*)snInfo->Data;
            ULONG origSnChars = snInfo->DataLength / sizeof(WCHAR);
            if (origSnChars > 23) origSnChars = 23;
            for (ULONG i = 0; i < origSnChars && origSnW[i]; i++)
                spoofedSn[i] = (char)origSnW[i];

            /* Apply same algorithm as IOCTL hook */
            SpoofSerialString(spoofedSn, sizeof(spoofedSn), g_Spoofer.Seed, "SCSI_DEVMAP_SN");

            /* Store as FirstSpoofedSerial so the IOCTL hook in dispatch_hook.c
             * reuses this EXACT serial instead of generating its own */
            if (!g_Spoofer.FirstSerialCaptured) {
                RtlZeroMemory(g_Spoofer.FirstSpoofedSerial, sizeof(g_Spoofer.FirstSpoofedSerial));
                ULONG sLen = 0;
                while (spoofedSn[sLen] && sLen < 63) {
                    g_Spoofer.FirstSpoofedSerial[sLen] = spoofedSn[sLen];
                    sLen++;
                }
                g_Spoofer.FirstSerialCaptured = TRUE;
                SPOOF_DBG("[Spoofer] WinID: Stored FirstSpoofedSerial='%s' for IOCTL consistency\n", spoofedSn);
            }
        } else {
            /* Fallback: generate deterministic serial */
            h = WinIdHash(seed ^ 0xDEAD);
            ULONG h2 = WinIdHash(h);
            ULONG h3 = WinIdHash(h2);
            ULONG h4 = WinIdHash(h3);
            RtlStringCbPrintfA(spoofedSn, sizeof(spoofedSn),
                "%04X_%04X_%04X_%c%c%c%c.",
                h & 0xFFFF, h2 & 0xFFFF, h3 & 0xFFFF,
                '0' + (char)(h4 % 10), 'A' + (char)((h4 >> 4) % 26),
                'A' + (char)((h4 >> 9) % 26), 'A' + (char)((h4 >> 14) % 26));
        }
    }

    {
        UNICODE_STRING vn;
        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&vn, L"Identifier");
        WCHAR wId[64] = { 0 };
        AnsiToWide(spoofedId, wId, 64);
        ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hLu, &vn, 0, REG_SZ,
            wId, (ULONG)((((fn_wcslen)ApiResolveExport(HASH_WCSLEN))(wId) + 1) * sizeof(WCHAR)));
    }
    {
        UNICODE_STRING vn;
        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&vn, L"SerialNumber");
        WCHAR wSn[32] = { 0 };
        AnsiToWide(spoofedSn, wSn, 32);
        ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hLu, &vn, 0, REG_SZ,
            wSn, (ULONG)((((fn_wcslen)ApiResolveExport(HASH_WCSLEN))(wSn) + 1) * sizeof(WCHAR)));
    }

    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hLu);
    SPOOF_DBG("[Spoofer] WinID: SCSI LU: Id=%s SN=%s\n", spoofedId, spoofedSn);
}

// ── Main entry ──────────────────────────────────────────────────────────────

NTSTATUS SpoofWindowsIdentifiers(const UINT8* seed)
{
    ULONG s = DeriveSeedFromSpoofer(seed);
    int count = 0;

    // 1. MachineGuid
    {
        char guid[64] = { 0 };
        GenerateMachineGuid(s, guid, sizeof(guid));
        {
            STK_REG_CRYPTO(_kp);
            STK_VAL_MACHINEGUID(_vn);
            if (NT_SUCCESS(SetRegSzValue(_kp, _vn, guid))) count++;
            STK_WIPE_W(_kp, 50);
            STK_WIPE_W(_vn, 12);
        }
        SPOOF_DBG("[Spoofer] WinID: MachineGuid = %s\n", guid);
    }

    // 2. ProductId (3 paths)
    {
        char productId[32] = { 0 };
        GenerateProductId(s ^ 0xABCD, productId, sizeof(productId));

        {
            STK_REG_WINNT_CV(_p1);
            STK_VAL_PRODUCTID(_v1);
            if (NT_SUCCESS(SetRegSzValue(_p1, _v1, productId))) count++;
            STK_WIPE_W(_p1, 63);
            STK_WIPE_W(_v1, 10);
        }
        {
            STK_REG_WIN_CV(_p2);
            STK_VAL_PRODUCTID(_v2);
            if (NT_SUCCESS(SetRegSzValue(_p2, _v2, productId))) count++;
            STK_WIPE_W(_p2, 60);
            STK_WIPE_W(_v2, 10);
        }
        {
            STK_REG_WOW64_CV(_p3);
            STK_VAL_PRODUCTID(_v3);
            if (NT_SUCCESS(SetRegSzValue(_p3, _v3, productId))) count++;
            STK_WIPE_W(_p3, 75);
            STK_WIPE_W(_v3, 10);
        }
        SPOOF_DBG("[Spoofer] WinID: ProductId = %s\n", productId);
    }

    // 3. ProductKey + DigitalProductId
    {
        char key[32] = { 0 };
        GenerateProductKey(s ^ 0x1337, key, sizeof(key));

        {
            STK_REG_WINNT_CV(_p4);
            STK_VAL_PRODUCTKEY(_v4);
            if (NT_SUCCESS(SetRegSzValue(_p4, _v4, key))) count++;
            STK_WIPE_W(_p4, 63);
            STK_WIPE_W(_v4, 11);
        }
        SPOOF_DBG("[Spoofer] WinID: ProductKey = %s\n", key);

        UCHAR digitalId[256] = { 0 };
        ULONG digitalIdSize = sizeof(digitalId);
        GenerateDigitalProductId(s, digitalId, &digitalIdSize);

        {
            STK_REG_WINNT_CV(_p5);
            STK_VAL_DIGPRODID(_v5);
            if (NT_SUCCESS(SetRegBinaryValue(_p5, _v5, digitalId, digitalIdSize))) count++;
            STK_WIPE_W(_p5, 63);
            STK_WIPE_W(_v5, 17);
        }
    }

    // 4. SQMClient MachineId
    {
        char cmId[64] = { 0 };
        GenerateCmId(s ^ 0xC0FFEE, cmId, sizeof(cmId));

        {
            STK_REG_SQMCLIENT(_p6);
            STK_VAL_MACHINEID(_v6);
            if (NT_SUCCESS(SetRegSzValue(_p6, _v6, cmId))) count++;
            STK_WIPE_W(_p6, 47);
            STK_WIPE_W(_v6, 10);
        }
        SPOOF_DBG("[Spoofer] WinID: SQMClient MachineId = %s\n", cmId);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // NEW: EAC critical registry keys
    // ═══════════════════════════════════════════════════════════════════════

    // 5. BIOS Registry — BIOSVendor, BIOSReleaseDate
    {
        char vendor[32] = { 0 };
        char date[16] = { 0 };
        ULONG h = WinIdHash(s ^ 0xB105);

        RtlStringCbPrintfA(vendor, sizeof(vendor), "American Megatrends Inc.");
        RtlStringCbPrintfA(date, sizeof(date), "%02u/%02u/%04u",
            (h % 12) + 1, (h % 28) + 1, 2020 + (h % 5));

        if (NT_SUCCESS(SetRegSzValue(
            L"\\Registry\\Machine\\Hardware\\Description\\System\\BIOS",
            L"BIOSVendor", vendor))) count++;
        if (NT_SUCCESS(SetRegSzValue(
            L"\\Registry\\Machine\\Hardware\\Description\\System\\BIOS",
            L"BIOSReleaseDate", date))) count++;

        SPOOF_DBG("[Spoofer] WinID: BIOSVendor=%s Date=%s\n", vendor, date);
    }

    // 6. SystemInformation — ComputerHardwareId, SystemManufacturer, SystemProductName
    {
        char hwid[64] = { 0 };
        ULONG h1 = WinIdHash(s ^ 0x5151);
        ULONG h2 = WinIdHash(h1);

        RtlStringCbPrintfA(hwid, sizeof(hwid),
            "{%08x-%04x-%04x-%04x-%04x%08x}",
            h1, (h1 >> 16) & 0xFFFF, h2 & 0xFFFF,
            (h2 >> 16) & 0xFFFF, (h1 >> 8) & 0xFFFF, WinIdHash(h2));

        if (NT_SUCCESS(SetRegSzValue(
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SystemInformation",
            L"ComputerHardwareId", hwid))) count++;
        if (NT_SUCCESS(SetRegSzValue(
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SystemInformation",
            L"SystemManufacturer", "Micro-Star International Co., Ltd."))) count++;
        if (NT_SUCCESS(SetRegSzValue(
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SystemInformation",
            L"SystemProductName", "MS-7C91"))) count++;

        SPOOF_DBG("[Spoofer] WinID: ComputerHwId=%s\n", hwid);
    }

    // 7. InstallDate (REG_DWORD — Unix epoch seconds)
    {
        ULONG fakeDate = 1640000000 + (WinIdHash(s ^ 0x1DAE) % 63072000);  /* 2021-2023 range */
        if (NT_SUCCESS(SetRegDwordValue(
            L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
            L"InstallDate", fakeDate))) count++;
        SPOOF_DBG("[Spoofer] WinID: InstallDate=%lu\n", fakeDate);
    }

    // 8. SusClientId (WindowsUpdate)
    {
        char susId[64] = { 0 };
        ULONG h = WinIdHash(s ^ 0x505);
        ULONG h2 = WinIdHash(h);
        RtlStringCbPrintfA(susId, sizeof(susId),
            "%08x-%04x-%04x-%04x-%04x%08x",
            h, (h >> 16) & 0xFFFF, (h2 >> 8) & 0xFFFF,
            h2 & 0xFFFF, (h >> 4) & 0xFFFF, WinIdHash(h2));

        if (NT_SUCCESS(SetRegSzValue(
            L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate",
            L"SusClientId", susId))) count++;
        SPOOF_DBG("[Spoofer] WinID: SusClientId=%s\n", susId);
    }

    // 9. SCSI DeviceMap — Identifier + SerialNumber
    {
        UNICODE_STRING scsiBase;
        OBJECT_ATTRIBUTES oa;
        HANDLE hScsi = NULL;

        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(
            &scsiBase, L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi");
        InitializeObjectAttributes(&oa, &scsiBase,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        if (NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hScsi, KEY_READ, &oa)))
        {
            /* Try common paths: Scsi Port 0..3 / Scsi Bus 0 / Target Id 0 / LU 0 */
            int port;
            for (port = 0; port < 4; port++)
            {
                WCHAR luPath[80];
                RtlStringCbPrintfW(luPath, sizeof(luPath),
                    L"Scsi Port %d\\Scsi Bus 0\\Target Id 0\\Logical Unit Id 0", port);
                SpoofScsiLuKey(hScsi, luPath, s ^ (ULONG)port);
            }
            ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hScsi);
            count++;
        }
    }

    /* NetCfgInstanceId: NOT spoofed — it's a Windows-internal random GUID with
     * hundreds of cross-references (Tcpip, Tcpip6, Network, DeviceClasses, NDIS).
     * Modifying it breaks network adapters on restart. The MAC address spoof
     * already covers NIC fingerprinting. */

    // 10. NVIDIA DXCache cleanup — delete GPU-specific shader cache files
    {
        fn_ZwOpenKey pZwOpenKey = (fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY);
        fn_ZwClose pZwClose = (fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE);
        fn_ZwEnumerateKey pZwEnumKey = (fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY);
        fn_ZwQueryValueKey pZwQueryVal = (fn_ZwQueryValueKey)ApiResolveExport(HASH_ZWQUERYVALUEKEY);
        fn_ZwCreateFile pZwCreateFile = (fn_ZwCreateFile)ApiResolveExport(HASH_ZWCREATEFILE);
        fn_ZwQueryDirectoryFile pZwQueryDir = (fn_ZwQueryDirectoryFile)ApiResolveExport(HASH_ZWQUERYDIRECTORYFILE);
        fn_ZwDeleteFile pZwDeleteFile = (fn_ZwDeleteFile)ApiResolveExport(HASH_ZWDELETEFILE);
        fn_ExAllocatePoolWithTag pAlloc = (fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG);
        fn_ExFreePoolWithTag pFree = (fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG);
        fn_RtlInitUnicodeString pRtlInit = (fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING);

        if (pZwOpenKey && pZwClose && pZwEnumKey && pZwQueryVal &&
            pZwCreateFile && pZwQueryDir && pZwDeleteFile && pAlloc && pFree && pRtlInit)
        {
            UNICODE_STRING profListPath;
            OBJECT_ATTRIBUTES profListOa;
            HANDLE hProfList = NULL;
            ULONG profIdx;

            pRtlInit(&profListPath,
                L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList");
            InitializeObjectAttributes(&profListOa, &profListPath,
                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

            if (NT_SUCCESS(pZwOpenKey(&hProfList, KEY_READ, &profListOa)))
            {
                ULONG dxDeleted = 0;
                PVOID keyBuf = pAlloc(0, 512, 'dxpr');

                for (profIdx = 0; keyBuf; profIdx++)
                {
                    ULONG resultLen = 0;
                    NTSTATUS enumSt = pZwEnumKey(hProfList, profIdx,
                        KeyBasicInformation, keyBuf, 512, &resultLen);

                    if (!NT_SUCCESS(enumSt)) break;

                    {
                        PKEY_BASIC_INFORMATION kbi = (PKEY_BASIC_INFORMATION)keyBuf;
                        WCHAR sidName[128];
                        ULONG sidLen, k;
                        HANDLE hSid = NULL;
                        OBJECT_ATTRIBUTES sidOa;
                        UNICODE_STRING sidPath;

                        sidLen = kbi->NameLength / sizeof(WCHAR);
                        if (sidLen >= 126) continue;
                        for (k = 0; k < sidLen; k++) sidName[k] = kbi->Name[k];
                        sidName[sidLen] = 0;

                        /* Skip non-user SIDs (must start with S-1-5-21) */
                        if (sidLen < 8 || sidName[0] != L'S') continue;

                        /* Open the SID subkey to read ProfileImagePath */
                        InitializeObjectAttributes(&sidOa, NULL,
                            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hProfList, NULL);
                        pRtlInit(&sidPath, sidName);
                        sidOa.ObjectName = &sidPath;

                        if (NT_SUCCESS(pZwOpenKey(&hSid, KEY_READ, &sidOa)))
                        {
                            PVOID valBuf = pAlloc(0, 1024, 'dxvl');
                            if (valBuf)
                            {
                                UNICODE_STRING pipName;
                                pRtlInit(&pipName, L"ProfileImagePath");

                                if (NT_SUCCESS(pZwQueryVal(hSid, &pipName,
                                    KeyValuePartialInformation, valBuf, 1024, &resultLen)))
                                {
                                    PKEY_VALUE_PARTIAL_INFORMATION pvpi =
                                        (PKEY_VALUE_PARTIAL_INFORMATION)valBuf;

                                    if (pvpi->Type == REG_EXPAND_SZ || pvpi->Type == REG_SZ)
                                    {
                                        WCHAR dxPath[300];
                                        UNICODE_STRING dxPathU;
                                        OBJECT_ATTRIBUTES dxOa;
                                        IO_STATUS_BLOCK iosb;
                                        HANDLE hDxDir = NULL;

                                        /* Build: \??\<ProfilePath>\AppData\Local\NVIDIA\DXCache */
                                        RtlStringCbPrintfW(dxPath, sizeof(dxPath),
                                            L"\\??\\%s\\AppData\\Local\\NVIDIA\\DXCache",
                                            (PWCHAR)pvpi->Data);

                                        pRtlInit(&dxPathU, dxPath);
                                        InitializeObjectAttributes(&dxOa, &dxPathU,
                                            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

                                        /* Open the DXCache directory */
                                        if (NT_SUCCESS(pZwCreateFile(&hDxDir,
                                            FILE_LIST_DIRECTORY | SYNCHRONIZE, &dxOa, &iosb,
                                            NULL, FILE_ATTRIBUTE_DIRECTORY,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                            FILE_OPEN,
                                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                                            NULL, 0)))
                                        {
                                            /* Enumerate and delete all files */
                                            PVOID dirBuf = pAlloc(0, 0x2000, 'dxen');
                                            if (dirBuf)
                                            {
                                                BOOLEAN restart = TRUE;
                                                while (TRUE)
                                                {
                                                    NTSTATUS dirSt = pZwQueryDir(hDxDir,
                                                        NULL, NULL, NULL, &iosb,
                                                        dirBuf, 0x2000,
                                                        FileDirectoryInformation, FALSE,
                                                        NULL, restart);

                                                    if (!NT_SUCCESS(dirSt)) break;
                                                    restart = FALSE;

                                                    {
                                                        PFILE_DIRECTORY_INFORMATION fdi =
                                                            (PFILE_DIRECTORY_INFORMATION)dirBuf;

                                                        while (TRUE)
                                                        {
                                                            if (!(fdi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                                                            {
                                                                WCHAR filePath[400];
                                                                UNICODE_STRING filePathU;
                                                                OBJECT_ATTRIBUTES fileOa;
                                                                ULONG fnLen = fdi->FileNameLength / sizeof(WCHAR);
                                                                ULONG m;

                                                                /* Build full file path */
                                                                RtlStringCbPrintfW(filePath, sizeof(filePath),
                                                                    L"\\??\\%s\\AppData\\Local\\NVIDIA\\DXCache\\",
                                                                    (PWCHAR)pvpi->Data);

                                                                m = (ULONG)((fn_wcslen)ApiResolveExport(HASH_WCSLEN))(filePath);
                                                                for (k = 0; k < fnLen && (m + k) < 398; k++)
                                                                    filePath[m + k] = fdi->FileName[k];
                                                                filePath[m + k] = 0;

                                                                pRtlInit(&filePathU, filePath);
                                                                InitializeObjectAttributes(&fileOa, &filePathU,
                                                                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                                                    NULL, NULL);

                                                                if (NT_SUCCESS(pZwDeleteFile(&fileOa)))
                                                                    dxDeleted++;
                                                            }

                                                            if (fdi->NextEntryOffset == 0) break;
                                                            fdi = (PFILE_DIRECTORY_INFORMATION)
                                                                ((PUCHAR)fdi + fdi->NextEntryOffset);
                                                        }
                                                    }
                                                }
                                                pFree(dirBuf, 'dxen');
                                            }
                                            pZwClose(hDxDir);
                                        }
                                    }
                                }
                                pFree(valBuf, 'dxvl');
                            }
                            pZwClose(hSid);
                        }
                    }
                }

                if (keyBuf) pFree(keyBuf, 'dxpr');
                pZwClose(hProfList);

                if (dxDeleted > 0) {
                    SPOOF_DBG("[Spoofer] WinID: Cleaned %lu NVIDIA DXCache files\n", dxDeleted);
                    count++;
                }
            }
        }
    }

    SPOOF_DBG("[Spoofer] WinID: %d value(s) spoofed\n", count);
    return (count > 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

