#include "../include/spoofer.h"

static BOOLEAN ContainsPattern(PCUNICODE_STRING str, const WCHAR* pattern)
{
    UNICODE_STRING pat;
    USHORT i, patLen;

    if (!str || !str->Buffer || !pattern) return FALSE;

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&pat, pattern);
    patLen = pat.Length / sizeof(WCHAR);

    if (str->Length / sizeof(WCHAR) < patLen) return FALSE;

    for (i = 0; i <= (str->Length / sizeof(WCHAR)) - patLen; i++) {
        BOOLEAN match = TRUE;
        USHORT j;
        for (j = 0; j < patLen; j++) {
            WCHAR a = str->Buffer[i + j];
            WCHAR b = pattern[j];
            if (a >= L'A' && a <= L'Z') a += 32;
            if (b >= L'A' && b <= L'Z') b += 32;
            if (a != b) { match = FALSE; break; }
        }
        if (match) return TRUE;
    }
    return FALSE;
}

static BOOLEAN IsTargetRegistryKey(PCUNICODE_STRING keyPath)
{
    if (!keyPath || !keyPath->Buffer) return FALSE;

    if (ContainsPattern(keyPath, L"DEVICEMAP\\Scsi")) return TRUE;

    if (ContainsPattern(keyPath, L"Enum\\SCSI\\Disk")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\NVMe\\Disk")) return TRUE;

    if (ContainsPattern(keyPath, L"Services\\disk\\Enum")) return TRUE;

    /* TPM WMI Endorsement registry paths */
    if (ContainsPattern(keyPath, L"Services\\TPM\\WMI")) return TRUE;
    if (ContainsPattern(keyPath, L"Endorsement")) return TRUE;

    /* HardwareConfig — contains machine GUID that EAC reads for fingerprinting */
    if (ContainsPattern(keyPath, L"HardwareConfig")) return TRUE;

    return FALSE;
}

static BOOLEAN IsSerialValueName(PCUNICODE_STRING valueName)
{
    if (!valueName || !valueName->Buffer) return FALSE;

    if (ContainsPattern(valueName, L"SerialNumber")) return TRUE;
    if (ContainsPattern(valueName, L"DeviceIdentifierPage")) return TRUE;

    if (ContainsPattern(valueName, L"SystemSerialNumber")) return TRUE;
    if (ContainsPattern(valueName, L"BaseBoardSerialNumber")) return TRUE;
    if (ContainsPattern(valueName, L"ChassisSerialNumber")) return TRUE;

    return FALSE;
}

static BOOLEAN IsEkPubValueName(PCUNICODE_STRING valueName)
{
    if (!valueName || !valueName->Buffer) return FALSE;
    if (ContainsPattern(valueName, L"EKPub")) return TRUE;
    if (ContainsPattern(valueName, L"EKCert")) return TRUE;
    return FALSE;
}

static BOOLEAN IsNetCfgInstanceId(PCUNICODE_STRING valueName)
{
    if (!valueName || !valueName->Buffer) return FALSE;
    return ContainsPattern(valueName, L"NetCfgInstanceId");
}

static BOOLEAN IsHardwareConfigValue(PCUNICODE_STRING valueName)
{
    if (!valueName || !valueName->Buffer) return FALSE;
    if (ContainsPattern(valueName, L"LastConfig")) return TRUE;
    return FALSE;
}

/* Generate a deterministic spoofed GUID from seed + extra context */
static void SpoofGuidInBuffer(PWCHAR data, ULONG dataLenBytes)
{
    /* GUID format: {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}  = 38 chars + null */
    ULONG wcharCount = dataLenBytes / sizeof(WCHAR);
    ULONG seed, i;
    WCHAR hex[] = L"0123456789abcdef";
    WCHAR guid[40];
    ULONG g;

    if (wcharCount < 38) return;

    /* Build seed from spoofer seed + original GUID content */
    seed = 0x6a09e667;
    for (i = 0; i < SPOOFER_SEED_SIZE; i++) {
        seed ^= (ULONG)g_Spoofer.Seed[i];
        seed *= 0x01000193;
    }
    /* Mix in the original GUID as extra entropy */
    for (i = 0; i < wcharCount && data[i] != 0; i++) {
        seed ^= (ULONG)data[i];
        seed *= 0x01000193;
    }

    /* Build GUID string */
    guid[0] = L'{';
    g = 1;
    for (i = 0; i < 32; i++) {
        if (i == 8 || i == 12 || i == 16 || i == 20) {
            guid[g++] = L'-';
        }
        seed ^= (i * 0xdeadbeef);
        seed *= 0x01000193;
        guid[g++] = hex[(seed >> 4) & 0xF];
    }
    guid[g++] = L'}';
    guid[g] = 0;

    /* Copy to output */
    for (i = 0; i < g && i < wcharCount; i++) {
        data[i] = guid[i];
    }
    if (i < wcharCount) data[i] = 0;

    SPOOF_DBG("[Spoofer] Schicht3: Spoofed NetCfgInstanceId GUID\n");
}

/*
 * Spoof the EKPub registry blob by finding and replacing the RSA modulus.
 * The EKPub blob stored by tpm.sys contains an encoded TPMT_PUBLIC with
 * the RSA public key embedded. We scan for the 256-byte modulus and replace.
 */
static void SpoofEkPubData(PUCHAR data, ULONG dataLength)
{
    ULONG i;
    PTPM_SPOOF_DATA tpmData;

    if (!data || dataLength < 260) return;

    tpmData = GetTpmSpoofData();
    if (!tpmData || !tpmData->Initialized) return;

    /* Scan for 256-byte RSA modulus in the blob */
    for (i = 0; i < dataLength - 256; i++)
    {
        if ((data[i] & 0x80) != 0 && data[i] != 0xFF &&
            (data[i + 255] & 0x01) != 0)
        {
            ULONG nonZero = 0, unique = 0, j;
            UINT8 prev = 0;
            for (j = 0; j < 32; j++)
            {
                if (data[i + j] != 0) nonZero++;
                if (data[i + j] != prev) unique++;
                prev = data[i + j];
            }
            if (nonZero >= 28 && unique >= 8)
            {
                /* Skip if already spoofed */
                if (RtlCompareMemory(data + i,
                    tpmData->GeneratedKey.buffer, 32) == 32)
                {
                    i += 255;
                    continue;
                }

                RtlCopyMemory(data + i,
                    tpmData->GeneratedKey.buffer,
                    min(256, tpmData->GeneratedKey.size));
                SPOOF_DBG("[Spoofer] Schicht3: EKPub RSA modulus replaced at offset %lu\n", i);
                return;
            }
        }
    }
}

static BOOLEAN IsSmbiosDataValue(PCUNICODE_STRING valueName)
{
    if (!valueName || !valueName->Buffer) return FALSE;
    return ContainsPattern(valueName, L"SMBiosData");
}

static void SpoofVpd83Page(PUCHAR data, ULONG dataLength)
{
    ULONG idOffset, idLen, prefixLen;
    PCHAR idStr;

    if (dataLength < 12) return;

    if (data[1] != 0x83) return;  


    if ((data[4] & 0x0F) < 2) {
        idOffset = 8;
        idLen = dataLength - idOffset;
        SpoofBinaryData(data + idOffset, idLen, g_Spoofer.Seed, "VPD83_BIN");
        return;
    }

    idOffset = 8;
    idLen = dataLength - idOffset;
    if (idLen < 4) return;

    idStr = (PCHAR)(data + idOffset);
    prefixLen = 0;

    if (idLen > 4 && (
        (idStr[0]=='e' && idStr[1]=='u' && idStr[2]=='i' && idStr[3]=='.') ||
        (idStr[0]=='n' && idStr[1]=='a' && idStr[2]=='a' && idStr[3]=='.') ||
        (idStr[0]=='t' && idStr[1]=='1' && idStr[2]=='0' && idStr[3]=='.'))) {
        prefixLen = 4;
    }

    SpoofSerialString(idStr + prefixLen, idLen - prefixLen,
        g_Spoofer.Seed, "VPD83_TEXT");
}

static void SpoofRegistryString(PVOID data, ULONG dataLength, ULONG type, PCUNICODE_STRING valueName)
{
    if (!data || dataLength < 4) return;

    if (type == REG_SZ || type == REG_EXPAND_SZ) {
        PWCHAR wstr = (PWCHAR)data;
        ULONG wlen = dataLength / sizeof(WCHAR);
        char narrow[256];
        ULONG i, nLen = 0;

        for (i = 0; i < wlen && i < 255 && wstr[i] != 0; i++) {
            narrow[i] = (char)(wstr[i] & 0xFF);
            nLen++;
        }
        narrow[nLen] = 0;

        if (SpoofSerialString(narrow, nLen + 1, g_Spoofer.Seed, "REG_SZ")) {
            for (i = 0; i < nLen; i++) {
                wstr[i] = (WCHAR)(UCHAR)narrow[i];
            }
        }
    }
    else if (type == REG_BINARY) {
        if (valueName && ContainsPattern(valueName, L"DeviceIdentifierPage")) {
            SpoofVpd83Page((PUCHAR)data, dataLength);
        } else {
            SpoofBinaryData((PUCHAR)data, dataLength, g_Spoofer.Seed, "REG_BINARY");
        }
    }
}


static BOOLEAN LooksLikeInstanceId(PWCHAR name, ULONG charCount)
{
    ULONG i, ampCount = 0, hexChars = 0;
    BOOLEAN inHex = FALSE;

    if (charCount < 7) return FALSE; 

    for (i = 0; i < charCount && name[i] != 0; i++) {
        if (name[i] == L'&') {
            ampCount++;
            if (ampCount == 1) inHex = TRUE;
            else if (ampCount == 2) inHex = FALSE;
        }
        else if (inHex) {
            WCHAR c = name[i];
            if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F'))
                hexChars++;
            else
                return FALSE;  
        }
    }

    return (ampCount >= 2 && hexChars >= 2);
}

static BOOLEAN LooksLikeUsbSerial(PWCHAR name, ULONG charCount)
{
    ULONG i, alnumCount = 0;
    BOOLEAN hitAmp = FALSE;

    if (charCount < 6) return FALSE;

    for (i = 0; i < charCount && name[i] != 0; i++) {
        WCHAR c = name[i];
        if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z')) {
            if (hitAmp) {
                if (i == charCount - 1 || name[i+1] == 0)
                    continue; 
                else
                    return FALSE; 
            }
            alnumCount++;
        }
        else if (c == L'&' && !hitAmp && alnumCount >= 6) {
            hitAmp = TRUE;  
        }
        else {
            return FALSE; 
        }
    }

    return (alnumCount >= 6);
}

static BOOLEAN IsUsbEnumParent(PCUNICODE_STRING keyPath)
{
    if (!keyPath || !keyPath->Buffer) return FALSE;
    if (ContainsPattern(keyPath, L"Enum\\USB\\VID_") && ContainsPattern(keyPath, L"PID_")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\USBSTOR\\") && ContainsPattern(keyPath, L"Prod_")) return TRUE;
    return FALSE;
}

static void SpoofUsbSerialName(PWCHAR name, ULONG charCount)
{
    ULONG seed = 0x811c9dc5;
    ULONG i;

    for (i = 0; i < charCount && name[i] != 0; i++) {
        seed ^= (ULONG)name[i];
        seed *= 0x01000193;
    }
    for (i = 0; i < SPOOFER_SEED_SIZE && i < 16; i++) {
        seed ^= (ULONG)g_Spoofer.Seed[i] << ((i % 4) * 8);
        seed *= 0x01000193;
    }

    for (i = 0; i < charCount && name[i] != 0; i++) {
        WCHAR c = name[i];
        seed = (seed * 1103515245 + 12345 + i) & 0x7FFFFFFF;
        if (c >= L'0' && c <= L'9') {
            name[i] = L'0' + (WCHAR)(seed % 10);
        }
        else if (c >= L'A' && c <= L'Z') {
            name[i] = L'A' + (WCHAR)(seed % 26);
        }
        else if (c >= L'a' && c <= L'z') {
            name[i] = L'a' + (WCHAR)(seed % 26);
        }
    }
}

static BOOLEAN IsVideoDeviceValue(PCUNICODE_STRING valueName)
{
    if (!valueName || !valueName->Buffer) return FALSE;
    return ContainsPattern(valueName, L"\\Device\\Video");
}

static void SpoofInstanceIdInName(PWCHAR name, ULONG charCount)
{
    ULONG i, ampCount = 0, hexStart = 0, hexEnd = 0;

    for (i = 0; i < charCount && name[i] != 0; i++) {
        if (name[i] == L'&') {
            ampCount++;
            if (ampCount == 1) hexStart = i + 1;
            if (ampCount == 2) { hexEnd = i; break; }
        }
    }

    if (hexStart > 0 && hexEnd > hexStart && (hexEnd - hexStart) >= 4) {
        ULONG seed = 0x811c9dc5;
        ULONG j;
        for (j = hexStart; j < hexEnd; j++) {
            seed ^= (ULONG)name[j];
            seed *= 0x01000193;
        }
        for (j = 0; j < SPOOFER_SEED_SIZE && j < 16; j++) {
            seed ^= (ULONG)g_Spoofer.Seed[j] << ((j % 4) * 8);
            seed *= 0x01000193;
        }

        for (j = hexStart; j < hexEnd; j++) {
            WCHAR c = name[j];
            seed = (seed * 1103515245 + 12345 + j) & 0x7FFFFFFF;
            if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')) {
                ULONG v = seed % 16;
                if (v < 10) name[j] = L'0' + (WCHAR)v;
                else name[j] = L'a' + (WCHAR)(v - 10);
            }
        }
    }
}

static void SpoofVideoPathGuid(PWCHAR wstr, ULONG wlen)
{
    ULONG i;
    for (i = 0; i < wlen - 38; i++) {
        if (wstr[i] == L'{') {
            ULONG end = i + 37;
            if (end >= wlen || wstr[end] != L'}') continue;
            if (wstr[i+9] != L'-' || wstr[i+14] != L'-' ||
                wstr[i+19] != L'-' || wstr[i+24] != L'-') continue;

            ULONG seed = 0xDEADBEEF;
            ULONG j;
            for (j = i + 1; j < end; j++) {
                seed ^= (ULONG)wstr[j];
                seed *= 0x01000193;
            }
            for (j = 0; j < SPOOFER_SEED_SIZE && j < 16; j++) {
                seed ^= (ULONG)g_Spoofer.Seed[j] << ((j % 4) * 8);
            }

            for (j = i + 1; j < end; j++) {
                WCHAR c = wstr[j];
                if (c == L'-') continue;
                seed = (seed * 1103515245 + 12345 + j) & 0x7FFFFFFF;
                if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')) {
                    ULONG v = seed % 16;
                    if (v < 10) wstr[j] = L'0' + (WCHAR)v;
                    else wstr[j] = L'a' + (WCHAR)(v - 10);
                }
            }
            SPOOF_DBG("[Spoofer] Schicht3: VIDEO GUID spoofed\n");
            return;
        }
    }
}
static PCUNICODE_STRING GetKeyPath(PREG_POST_OPERATION_INFORMATION postInfo)
{
    PCUNICODE_STRING keyName = NULL;
    fn_CmCallbackGetKeyObjectIDEx pGetId;

    if (!postInfo || !postInfo->Object) return NULL;

    pGetId = (fn_CmCallbackGetKeyObjectIDEx)ApiResolveExport(HASH_CMCALLBACKGETKEYOBJECTIDEX);
    if (!pGetId) return NULL;

    if (NT_SUCCESS(pGetId(&g_Spoofer.RegistryCallbackCookie,
                          postInfo->Object, NULL, &keyName, 0)))
    {
        return keyName;
    }
    return NULL;
}

static BOOLEAN IsHwEnumPath(PCUNICODE_STRING keyPath)
{
    if (!keyPath || !keyPath->Buffer) return FALSE;
    if (ContainsPattern(keyPath, L"Enum\\HDAUDIO\\")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\DISPLAY\\")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\PCI\\")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\USB\\")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\HID\\")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\USBSTOR\\")) return TRUE;
    if (ContainsPattern(keyPath, L"Enum\\SWD\\")) return TRUE;
    return FALSE;
}

static BOOLEAN IsMmDevicesPath(PCUNICODE_STRING keyPath)
{
    if (!keyPath || !keyPath->Buffer) return FALSE;
    return ContainsPattern(keyPath, L"MMDevices\\Audio\\");
}


static void SpoofInstanceIdsInPath(PWCHAR path, ULONG charCount)
{
    ULONG i = 0;
    while (i < charCount && path[i] != 0) {
        if (i + 4 < charCount && path[i + 1] == L'&' &&
            ((path[i] >= L'0' && path[i] <= L'9') || (path[i] >= L'a' && path[i] <= L'f'))) {
            ULONG hexStart = i + 2;
            ULONG hexEnd = hexStart;
            BOOLEAN isPureHex = TRUE;
            while (hexEnd < charCount && path[hexEnd] != L'&' && path[hexEnd] != L'\\' && path[hexEnd] != 0) {
                WCHAR c = path[hexEnd];
                if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f'))) {
                    isPureHex = FALSE;
                }
                hexEnd++;
            }
            if (isPureHex && hexEnd > hexStart + 3 && hexEnd < charCount && path[hexEnd] == L'&') {
                ULONG seed = 0x811c9dc5;
                ULONG j;
                for (j = hexStart; j < hexEnd; j++) {
                    seed ^= (ULONG)path[j];
                    seed *= 0x01000193;
                }
                for (j = 0; j < SPOOFER_SEED_SIZE && j < 16; j++) {
                    seed ^= (ULONG)g_Spoofer.Seed[j] << ((j % 4) * 8);
                    seed *= 0x01000193;
                }
                for (j = hexStart; j < hexEnd; j++) {
                    seed = (seed * 1103515245 + 12345 + j) & 0x7FFFFFFF;
                    ULONG v = seed % 16;
                    if (v < 10) path[j] = L'0' + (WCHAR)v;
                    else path[j] = L'a' + (WCHAR)(v - 10);
                }
            }
            i = hexEnd;
        } else {
            i++;
        }
    }
}

static void SpoofMmDeviceGuids(PWCHAR path, ULONG charCount)
{
    ULONG i;
    for (i = 0; i + 38 < charCount; i++) {
        if (path[i] == L'{') {
            ULONG end = i + 37;
            if (end >= charCount || path[end] != L'}') continue;
            if (path[i+9] != L'-' || path[i+14] != L'-' ||
                path[i+19] != L'-' || path[i+24] != L'-') continue;

            ULONG seed = 0xABADCAFE;  
            ULONG j;
            for (j = i + 1; j < end; j++) {
                seed ^= (ULONG)path[j];
                seed *= 0x01000193;
            }
            for (j = 0; j < SPOOFER_SEED_SIZE && j < 16; j++) {
                seed ^= (ULONG)g_Spoofer.Seed[j] << ((j % 4) * 8);
            }
            for (j = i + 1; j < end; j++) {
                WCHAR c = path[j];
                if (c == L'-') continue;
                seed = (seed * 1103515245 + 12345 + j) & 0x7FFFFFFF;
                if ((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F')) {
                    ULONG v = seed % 16;
                    if (v < 10) path[j] = L'0' + (WCHAR)v;
                    else path[j] = L'a' + (WCHAR)(v - 10);
                }
            }
            SPOOF_DBG("[Spoofer] Schicht3: MMDevice GUID spoofed\n");
            i = end;
        }
    }
}

static NTSTATUS RegistryCallback(
    PVOID CallbackContext,
    PVOID Argument1,
    PVOID Argument2)
{
    REG_NOTIFY_CLASS notifyClass;
    PREG_POST_OPERATION_INFORMATION postInfo;

    UNREFERENCED_PARAMETER(CallbackContext);

    if (!Argument1 || !Argument2) return STATUS_SUCCESS;

    notifyClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

    if (notifyClass == RegNtPostEnumerateKey)
    {
        postInfo = (PREG_POST_OPERATION_INFORMATION)Argument2;
        if (!postInfo || !NT_SUCCESS(postInfo->Status)) return STATUS_SUCCESS;

        __try {
            PREG_ENUMERATE_KEY_INFORMATION enumInfo =
                (PREG_ENUMERATE_KEY_INFORMATION)postInfo->PreInformation;
            if (!enumInfo || !enumInfo->KeyInformation) return STATUS_SUCCESS;

            if (enumInfo->KeyInformationClass == KeyBasicInformation) {
                PKEY_BASIC_INFORMATION kbi = (PKEY_BASIC_INFORMATION)enumInfo->KeyInformation;
                ULONG nameChars = kbi->NameLength / sizeof(WCHAR);
                if (nameChars > 4 && LooksLikeInstanceId(kbi->Name, nameChars)) {
                    SpoofInstanceIdInName(kbi->Name, nameChars);
                }
                else if (nameChars >= 6 && LooksLikeUsbSerial(kbi->Name, nameChars)) {
                    PCUNICODE_STRING kp = GetKeyPath(postInfo);
                    if (kp && IsUsbEnumParent(kp)) {
                        SpoofUsbSerialName(kbi->Name, nameChars);
                    }
                }
                if (nameChars >= 38 && kbi->Name[0] == L'{') {
                    PCUNICODE_STRING kp = GetKeyPath(postInfo);
                    if (kp && IsMmDevicesPath(kp)) {
                        SpoofMmDeviceGuids(kbi->Name, nameChars);
                    }
                }
            }
            else if (enumInfo->KeyInformationClass == KeyNodeInformation) {
                PKEY_NODE_INFORMATION kni = (PKEY_NODE_INFORMATION)enumInfo->KeyInformation;
                ULONG nameChars = kni->NameLength / sizeof(WCHAR);
                if (nameChars > 4 && LooksLikeInstanceId(kni->Name, nameChars)) {
                    SpoofInstanceIdInName(kni->Name, nameChars);
                }
                else if (nameChars >= 6 && LooksLikeUsbSerial(kni->Name, nameChars)) {
                    PCUNICODE_STRING kp = GetKeyPath(postInfo);
                    if (kp && IsUsbEnumParent(kp)) {
                        SpoofUsbSerialName(kni->Name, nameChars);
                    }
                }
                if (nameChars >= 38 && kni->Name[0] == L'{') {
                    PCUNICODE_STRING kp = GetKeyPath(postInfo);
                    if (kp && IsMmDevicesPath(kp)) {
                        SpoofMmDeviceGuids(kni->Name, nameChars);
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        return STATUS_SUCCESS;
    }

    if (notifyClass != RegNtPostQueryValueKey) return STATUS_SUCCESS;

    postInfo = (PREG_POST_OPERATION_INFORMATION)Argument2;
    if (!postInfo || !NT_SUCCESS(postInfo->Status)) return STATUS_SUCCESS;

    __try {
        PREG_QUERY_VALUE_KEY_INFORMATION queryInfo =
            (PREG_QUERY_VALUE_KEY_INFORMATION)postInfo->PreInformation;

        if (!queryInfo || !queryInfo->ValueName) return STATUS_SUCCESS;

        {
            PCUNICODE_STRING keyPath = GetKeyPath(postInfo);

            if (keyPath && IsHwEnumPath(keyPath)) {

                if (queryInfo->KeyValueInformationClass == KeyValuePartialInformation &&
                    queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                    PKEY_VALUE_PARTIAL_INFORMATION partial =
                        (PKEY_VALUE_PARTIAL_INFORMATION)queryInfo->KeyValueInformation;

                    if ((partial->Type == REG_SZ || partial->Type == REG_EXPAND_SZ) &&
                        partial->DataLength >= 8) {
                        SpoofInstanceIdsInPath((PWCHAR)partial->Data,
                            partial->DataLength / sizeof(WCHAR));
                        SPOOF_DBG("[Spoofer] Schicht3: HwEnum value spoofed (partial)\n");
                    }
                    else if (partial->Type == REG_MULTI_SZ && partial->DataLength >= 8) {
     
                        SpoofInstanceIdsInPath((PWCHAR)partial->Data,
                            partial->DataLength / sizeof(WCHAR));
                        SPOOF_DBG("[Spoofer] Schicht3: HwEnum MULTI_SZ spoofed\n");
                    }
                }
                else if (queryInfo->KeyValueInformationClass == KeyValueFullInformation &&
                         queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                    PKEY_VALUE_FULL_INFORMATION full =
                        (PKEY_VALUE_FULL_INFORMATION)queryInfo->KeyValueInformation;

                    if ((full->Type == REG_SZ || full->Type == REG_EXPAND_SZ ||
                         full->Type == REG_MULTI_SZ) &&
                        full->DataLength >= 8 && full->DataOffset > 0) {
                        PVOID dataPtr = (PUCHAR)full + full->DataOffset;
                        SpoofInstanceIdsInPath((PWCHAR)dataPtr,
                            full->DataLength / sizeof(WCHAR));
                    }
                }
            }

            if (keyPath && IsMmDevicesPath(keyPath)) {
                if (queryInfo->KeyValueInformationClass == KeyValuePartialInformation &&
                    queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                    PKEY_VALUE_PARTIAL_INFORMATION partial =
                        (PKEY_VALUE_PARTIAL_INFORMATION)queryInfo->KeyValueInformation;

                    if ((partial->Type == REG_SZ || partial->Type == REG_EXPAND_SZ) &&
                        partial->DataLength >= 8) {
                        SpoofMmDeviceGuids((PWCHAR)partial->Data,
                            partial->DataLength / sizeof(WCHAR));
                        SpoofInstanceIdsInPath((PWCHAR)partial->Data,
                            partial->DataLength / sizeof(WCHAR));
                    }
                }
                else if (queryInfo->KeyValueInformationClass == KeyValueFullInformation &&
                         queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                    PKEY_VALUE_FULL_INFORMATION full =
                        (PKEY_VALUE_FULL_INFORMATION)queryInfo->KeyValueInformation;

                    if ((full->Type == REG_SZ || full->Type == REG_EXPAND_SZ) &&
                        full->DataLength >= 8 && full->DataOffset > 0) {
                        PVOID dataPtr = (PUCHAR)full + full->DataOffset;
                        SpoofMmDeviceGuids((PWCHAR)dataPtr,
                            full->DataLength / sizeof(WCHAR));
                        SpoofInstanceIdsInPath((PWCHAR)dataPtr,
                            full->DataLength / sizeof(WCHAR));
                    }
                }
            }
        }

        if (IsVideoDeviceValue(queryInfo->ValueName)) {
            if (queryInfo->KeyValueInformationClass == KeyValuePartialInformation &&
                queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_PARTIAL_INFORMATION partial =
                    (PKEY_VALUE_PARTIAL_INFORMATION)queryInfo->KeyValueInformation;

                if ((partial->Type == REG_SZ || partial->Type == REG_EXPAND_SZ) &&
                    partial->DataLength >= 20) {
                    SpoofVideoPathGuid((PWCHAR)partial->Data, partial->DataLength / sizeof(WCHAR));
                }
            }
            else if (queryInfo->KeyValueInformationClass == KeyValueFullInformation &&
                     queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_FULL_INFORMATION full =
                    (PKEY_VALUE_FULL_INFORMATION)queryInfo->KeyValueInformation;

                if ((full->Type == REG_SZ || full->Type == REG_EXPAND_SZ) &&
                    full->DataLength >= 20 && full->DataOffset > 0) {
                    PVOID dataPtr = (PUCHAR)full + full->DataOffset;
                    SpoofVideoPathGuid((PWCHAR)dataPtr, full->DataLength / sizeof(WCHAR));
                }
            }
            return STATUS_SUCCESS;
        }

        if (g_SmbiosPatched && IsSmbiosDataValue(queryInfo->ValueName)) {
            if (queryInfo->KeyValueInformationClass == KeyValuePartialInformation &&
                queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_PARTIAL_INFORMATION partial =
                    (PKEY_VALUE_PARTIAL_INFORMATION)queryInfo->KeyValueInformation;

                if (partial->DataLength >= g_SpoofedSmbiosLen && g_SpoofedSmbiosRaw) {
                    RtlCopyMemory(partial->Data, g_SpoofedSmbiosRaw, g_SpoofedSmbiosLen);
                    SPOOF_DBG("[Spoofer] Schicht3: Intercepted SMBiosData read (%lu bytes)\n",
                        g_SpoofedSmbiosLen);
                }
            }
            else if (queryInfo->KeyValueInformationClass == KeyValueFullInformation &&
                     queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_FULL_INFORMATION full =
                    (PKEY_VALUE_FULL_INFORMATION)queryInfo->KeyValueInformation;

                if (full->DataLength >= g_SpoofedSmbiosLen && full->DataOffset > 0 && g_SpoofedSmbiosRaw) {
                    PVOID dataPtr = (PUCHAR)full + full->DataOffset;
                    RtlCopyMemory(dataPtr, g_SpoofedSmbiosRaw, g_SpoofedSmbiosLen);
                }
            }
            return STATUS_SUCCESS;
        }

        if (IsHardwareConfigValue(queryInfo->ValueName)) {
            if (queryInfo->KeyValueInformationClass == KeyValuePartialInformation &&
                queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_PARTIAL_INFORMATION partial =
                    (PKEY_VALUE_PARTIAL_INFORMATION)queryInfo->KeyValueInformation;

                if ((partial->Type == REG_SZ || partial->Type == REG_EXPAND_SZ) &&
                    partial->DataLength >= 38 * sizeof(WCHAR)) {
                    SpoofGuidInBuffer((PWCHAR)partial->Data, partial->DataLength);
                    SPOOF_DBG("[Spoofer] Schicht3: Spoofed HardwareConfig LastConfig GUID\n");
                }
            }
            else if (queryInfo->KeyValueInformationClass == KeyValueFullInformation &&
                     queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_FULL_INFORMATION full =
                    (PKEY_VALUE_FULL_INFORMATION)queryInfo->KeyValueInformation;

                if ((full->Type == REG_SZ || full->Type == REG_EXPAND_SZ) &&
                    full->DataLength >= 38 * sizeof(WCHAR) && full->DataOffset > 0) {
                    PVOID dataPtr = (PUCHAR)full + full->DataOffset;
                    SpoofGuidInBuffer((PWCHAR)dataPtr, full->DataLength);
                }
            }
            return STATUS_SUCCESS;
        }

        if (IsEkPubValueName(queryInfo->ValueName)) {
            if (queryInfo->KeyValueInformationClass == KeyValuePartialInformation &&
                queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_PARTIAL_INFORMATION partial =
                    (PKEY_VALUE_PARTIAL_INFORMATION)queryInfo->KeyValueInformation;

                if (partial->Type == REG_BINARY && partial->DataLength >= 260) {
                    SpoofEkPubData(partial->Data, partial->DataLength);
                }
            }
            else if (queryInfo->KeyValueInformationClass == KeyValueFullInformation &&
                     queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_FULL_INFORMATION full =
                    (PKEY_VALUE_FULL_INFORMATION)queryInfo->KeyValueInformation;

                if (full->Type == REG_BINARY && full->DataLength >= 260 && full->DataOffset > 0) {
                    PVOID dataPtr = (PUCHAR)full + full->DataOffset;
                    SpoofEkPubData((PUCHAR)dataPtr, full->DataLength);
                }
            }
            return STATUS_SUCCESS;
        }

        if (!IsSerialValueName(queryInfo->ValueName)) return STATUS_SUCCESS;

        {
            if (queryInfo->KeyValueInformationClass == KeyValuePartialInformation &&
                queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_PARTIAL_INFORMATION partial =
                    (PKEY_VALUE_PARTIAL_INFORMATION)queryInfo->KeyValueInformation;

                if (partial->DataLength >= 4) {
                    SpoofRegistryString(partial->Data, partial->DataLength, partial->Type, queryInfo->ValueName);
                }
            }
            else if (queryInfo->KeyValueInformationClass == KeyValueFullInformation &&
                     queryInfo->ResultLength && queryInfo->KeyValueInformation) {

                PKEY_VALUE_FULL_INFORMATION full =
                    (PKEY_VALUE_FULL_INFORMATION)queryInfo->KeyValueInformation;

                if (full->DataLength >= 4 && full->DataOffset > 0) {
                    PVOID dataPtr = (PUCHAR)full + full->DataOffset;
                    SpoofRegistryString(dataPtr, full->DataLength, full->Type, queryInfo->ValueName);
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    return STATUS_SUCCESS;
}

NTSTATUS InstallRegistryCallback(const UINT8* seed)
{
    NTSTATUS status;
    UNICODE_STRING altitude;

    UNREFERENCED_PARAMETER(seed);

    SPOOF_DBG("[Spoofer] ============ SCHICHT 3: Registry Callback ============\n");

    STK_VAL_ALTITUDE(_alt);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&altitude, _alt);

    status = ((fn_CmRegisterCallbackEx)ApiResolveExport(HASH_CMREGISTERCALLBACKEX))(
        RegistryCallback,
        &altitude,
        ((fn_IoGetCurrentProcess)ApiResolveExport(HASH_IOGETCURRENTPROCESS))(),  
        NULL,                   
        &g_Spoofer.RegistryCallbackCookie,
        NULL                  
    );
    STK_WIPE_W(_alt, 7);

    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] Schicht3: CmRegisterCallbackEx failed: 0x%X\n", status);
        return status;
    }

    g_Spoofer.RegistryCallbackActive = TRUE;
    SPOOF_DBG("[Spoofer] Schicht3: Registry callback installed (cookie=%lld)\n",
        g_Spoofer.RegistryCallbackCookie.QuadPart);

    return STATUS_SUCCESS;
}

void UninstallRegistryCallback(void)
{
    if (!g_Spoofer.RegistryCallbackActive) return;

    ((fn_CmUnRegisterCallback)ApiResolveExport(HASH_CMUNREGISTERCALLBACK))(g_Spoofer.RegistryCallbackCookie);
    g_Spoofer.RegistryCallbackActive = FALSE;
    g_Spoofer.RegistryCallbackCookie.QuadPart = 0;

    SPOOF_DBG("[Spoofer] Schicht3: Registry callback removed\n");
}
