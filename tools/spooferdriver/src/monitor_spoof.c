#include "../include/spoofer.h"

#define WNODE_FLAG_FIXED_INSTANCE_SIZE 0x00000010

typedef struct _WNODE_HEADER_MINI {
    ULONG   BufferSize;
    ULONG   ProviderId;
    ULONG64 HistoricalContext;
    LARGE_INTEGER TimeStamp;
    GUID    Guid;
    ULONG   ClientContext;
    ULONG   Flags;
} WNODE_HEADER_MINI;

typedef struct _OFFSETINSTDATALEN {
    ULONG OffsetInstanceData;
    ULONG LengthInstanceData;
} OFFSETINSTDATALEN;

typedef struct _WNODE_ALL_DATA_MINI {
    WNODE_HEADER_MINI WnodeHeader;
    ULONG        DataBlockOffset;
    ULONG        InstanceCount;
    ULONG        OffsetInstanceNameOffsets;
    union {
        ULONG               FixedInstanceSize;
        OFFSETINSTDATALEN   OffsetInstanceDataAndLength[1];
    };
} WNODE_ALL_DATA_MINI;

typedef struct _WmiMonitorID_MINI {
    USHORT ProductCodeID[16];
    USHORT SerialNumberID[16];
    USHORT ManufacturerName[16];
    UCHAR  WeekOfManufacture;
    USHORT YearOfManufacture;
    USHORT UserFriendlyNameLength;
    USHORT UserFriendlyName[1];
} WmiMonitorID_MINI;


static PDRIVER_DISPATCH g_OrigMonitorDispatch = NULL;
static PDRIVER_OBJECT   g_MonitorDriverObj = NULL;
static UINT8            g_MonSeed[SPOOFER_SEED_SIZE] = { 0 };

static char  g_MonSpoofedSerial[14] = { 0 };
static char  g_MonSpoofedName[14] = { 0 };
static BOOLEAN g_MonStringsInit = FALSE;

#define MON_CACHE_SIZE 16
static PDEVICE_OBJECT g_MonCacheDevice[MON_CACHE_SIZE] = { 0 };
static char           g_MonCacheSerial[MON_CACHE_SIZE][14] = { 0 };
static BOOLEAN        g_MonCacheInit[MON_CACHE_SIZE] = { 0 };


static ULONG MonXorShift(ULONG* s)
{
    ULONG x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static void GenerateMonitorSerial(char* out, SIZE_T outSize, ULONG seed)
{
    static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const char digits[] = "0123456789";
    int pos = 0;

    for (int i = 0; i < 4 && pos < (int)outSize - 1; i++)
        out[pos++] = alphanum[(MonXorShift(&seed) >> 16) % 36];
    for (int i = 0; i < 2 && pos < (int)outSize - 1; i++)
        out[pos++] = digits[(MonXorShift(&seed) >> 16) % 10];
    for (int i = 0; i < 4 && pos < (int)outSize - 1; i++)
        out[pos++] = alphanum[(MonXorShift(&seed) >> 16) % 36];
    out[pos] = '\0';
}

static void AnsiToWmiStr(const char* src, USHORT* dst, ULONG maxChars)
{
    for (ULONG i = 0; i < maxChars; i++)
        dst[i] = src[i] ? (USHORT)(UCHAR)src[i] : 0;
}

static void InitMonitorStrings(void)
{
    if (g_MonStringsInit) return;

    static const char* models[] = {
        "VG279QM", "VG27AQ", "S24D330", "S27F350", "P2419H",
        "U2722D", "27GL850", "27GP850", "AG274QZ", "HP24mh",
        "27UK850", "LC34G55T", "32GN650", "C27F390", "E24d"
    };

    ULONG seed = 0;
    for (int i = 0; i < SPOOFER_SEED_SIZE; i++)
        seed ^= ((ULONG)g_MonSeed[i]) << ((i % 4) * 8);
    seed ^= 0xED1D5EED;

    ULONG mi = (MonXorShift(&seed) >> 16) % (sizeof(models) / sizeof(models[0]));
    SIZE_T nameLen = ((fn_strlen)ApiResolveExport(HASH_STRLEN))(models[mi]);
    if (nameLen > 13) nameLen = 13;
    RtlCopyMemory(g_MonSpoofedName, models[mi], nameLen);
    g_MonSpoofedName[nameLen] = '\0';

    GenerateMonitorSerial(g_MonSpoofedSerial, sizeof(g_MonSpoofedSerial), seed);

    g_MonStringsInit = TRUE;
    SPOOF_DBG("[Spoofer] Monitor: Serial='%s' Name='%s'\n", g_MonSpoofedSerial, g_MonSpoofedName);
}


static void PatchEdidDescriptor(UCHAR* edid, ULONG edidLen, UCHAR type, const char* newStr)
{
    if (edidLen < 128) return;

    for (ULONG off = 54; off <= 108; off += 18)
    {
        if (edid[off] != 0x00 || edid[off + 1] != 0x00) continue;
        if (edid[off + 2] != 0x00 || edid[off + 3] != type) continue;

        UCHAR* data = edid + off + 5;
        RtlZeroMemory(data, 13);
        SIZE_T len = ((fn_strlen)ApiResolveExport(HASH_STRLEN))(newStr);
        if (len > 12) len = 12;
        RtlCopyMemory(data, newStr, len);
        data[len] = 0x0A; 
        break;
    }

    UCHAR sum = 0;
    for (ULONG i = 0; i < 127; i++) sum += edid[i];
    edid[127] = (UCHAR)(0x100 - sum);
}

static void PatchRegistryEdid(void)
{
    UNICODE_STRING displayPath;
    OBJECT_ATTRIBUTES oa;
    HANDLE hDisplay = NULL;

        STK_REG_DISPLAY(_dp);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&displayPath, _dp);
    InitializeObjectAttributes(&oa, &displayPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    if (!NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hDisplay, KEY_READ, &oa)))
        return;

    ULONG modelIdx = 0;
    int patchCount = 0;

    while (TRUE)
    {
        ULONG modelLen = 0;
        NTSTATUS s = ((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(hDisplay, modelIdx, KeyBasicInformation, NULL, 0, &modelLen);
        if (s == STATUS_NO_MORE_ENTRIES) break;
        if (!modelLen) { modelIdx++; continue; }

        PKEY_BASIC_INFORMATION modelInfo = (PKEY_BASIC_INFORMATION)
            ((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, modelLen, g_Spoofer.PoolTag);
        if (!modelInfo) { modelIdx++; continue; }

        s = ((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(hDisplay, modelIdx++, KeyBasicInformation, modelInfo, modelLen, &modelLen);
        if (!NT_SUCCESS(s)) { ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modelInfo, g_Spoofer.PoolTag); continue; }

        UNICODE_STRING modelName;
        modelName.Length = (USHORT)modelInfo->NameLength;
        modelName.MaximumLength = (USHORT)modelInfo->NameLength;
        modelName.Buffer = modelInfo->Name;

        HANDLE hModel = NULL;
        InitializeObjectAttributes(&oa, &modelName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hDisplay, NULL);
        if (!NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hModel, KEY_READ, &oa))) {
            ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modelInfo, g_Spoofer.PoolTag);
            continue;
        }
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(modelInfo, g_Spoofer.PoolTag);

        ULONG instIdx = 0;
        while (TRUE)
        {
            ULONG instLen = 0;
            s = ((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(hModel, instIdx, KeyBasicInformation, NULL, 0, &instLen);
            if (s == STATUS_NO_MORE_ENTRIES) break;
            if (!instLen) { instIdx++; continue; }

            PKEY_BASIC_INFORMATION instInfo = (PKEY_BASIC_INFORMATION)
                ((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, instLen, g_Spoofer.PoolTag);
            if (!instInfo) { instIdx++; continue; }

            s = ((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(hModel, instIdx++, KeyBasicInformation, instInfo, instLen, &instLen);
            if (!NT_SUCCESS(s)) { ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(instInfo, g_Spoofer.PoolTag); continue; }

            UNICODE_STRING instName;
            instName.Length = (USHORT)instInfo->NameLength;
            instName.MaximumLength = (USHORT)instInfo->NameLength;
            instName.Buffer = instInfo->Name;

            HANDLE hInst = NULL;
            InitializeObjectAttributes(&oa, &instName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hModel, NULL);
            if (!NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hInst, KEY_READ, &oa))) {
                ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(instInfo, g_Spoofer.PoolTag);
                continue;
            }
            ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(instInfo, g_Spoofer.PoolTag);

            UNICODE_STRING devParams;
            STK_VAL_DEVPARAMS(_dvp);
            ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&devParams, _dvp);
            HANDLE hDevParams = NULL;
            InitializeObjectAttributes(&oa, &devParams, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hInst, NULL);

            if (!NT_SUCCESS(((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hDevParams, KEY_ALL_ACCESS, &oa))) {
                STK_WIPE_W(_dvp, 18);
                ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hInst);
                continue;
            }

            UNICODE_STRING edidVal;
            STK_VAL_EDID(_ev);
            ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&edidVal, _ev);

            ULONG edidInfoSize = 0;
            ((fn_ZwQueryValueKey)ApiResolveExport(HASH_ZWQUERYVALUEKEY))(hDevParams, &edidVal, KeyValueFullInformation, NULL, 0, &edidInfoSize);

            if (edidInfoSize > 0)
            {
                PKEY_VALUE_FULL_INFORMATION edidInfo = (PKEY_VALUE_FULL_INFORMATION)
                    ((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, edidInfoSize, g_Spoofer.PoolTag);

                if (edidInfo)
                {
                    ULONG needed = 0;
                    if (NT_SUCCESS(((fn_ZwQueryValueKey)ApiResolveExport(HASH_ZWQUERYVALUEKEY))(hDevParams, &edidVal, KeyValueFullInformation,
                        edidInfo, edidInfoSize, &needed)))
                    {
                        UCHAR* edidData = (UCHAR*)edidInfo + edidInfo->DataOffset;
                        ULONG edidLen = edidInfo->DataLength;

                        if (edidLen >= 128)
                        {
                            ULONG seed = 0;
                            for (int i = 0; i < SPOOFER_SEED_SIZE; i++)
                                seed ^= ((ULONG)g_MonSeed[i]) << ((i % 4) * 8);
                            seed ^= (modelIdx * 0x1337) ^ (instIdx * 0x7331);

                            char devSerial[14] = { 0 };
                            GenerateMonitorSerial(devSerial, sizeof(devSerial), seed);

                            ULONG pcSeed = seed ^ 0x0C0D;
                            MonXorShift(&pcSeed);
                            edidData[10] = (UCHAR)(pcSeed & 0xFF);
                            edidData[11] = (UCHAR)((pcSeed >> 8) & 0xFF);

                            ULONG snSeed = seed ^ 0x5E41A1;
                            MonXorShift(&snSeed);
                            edidData[12] = (UCHAR)(snSeed & 0xFF);
                            edidData[13] = (UCHAR)((snSeed >> 8) & 0xFF);
                            edidData[14] = (UCHAR)((snSeed >> 16) & 0xFF);
                            edidData[15] = (UCHAR)((snSeed >> 24) & 0xFF);

                            PatchEdidDescriptor(edidData, edidLen, 0xFF, devSerial);

                            ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hDevParams, &edidVal, 0, REG_BINARY, edidData, edidLen);

                            UNICODE_STRING overrideVal;
                            STK_VAL_EDID_OVER(_eo);
                            ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&overrideVal, _eo);
                            ((fn_ZwSetValueKey)ApiResolveExport(HASH_ZWSETVALUEKEY))(hDevParams, &overrideVal, 0, REG_BINARY, edidData, edidLen);
                            STK_WIPE_W(_eo, 14);

                            patchCount++;
                        }
                    }
                    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(edidInfo, g_Spoofer.PoolTag);
                }
            }

            ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hDevParams);
            ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hInst);
        }
        ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hModel);
    }
    ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hDisplay);

    SPOOF_DBG("[Spoofer] Monitor: %d EDID(s) patched in registry\n", patchCount);
}


static NTSTATUS MonitorDispatchIo(PDEVICE_OBJECT device, PIRP Irp)
{
    if (!g_OrigMonitorDispatch)
        return STATUS_INVALID_DEVICE_REQUEST;

    PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(Irp);
    if (ioc->MinorFunction != IRP_MN_QUERY_ALL_DATA)
        return g_OrigMonitorDispatch(device, Irp);

    PVOID wmiBuffer = ioc->Parameters.WMI.Buffer;
    ULONG wmiSize = ioc->Parameters.WMI.BufferSize;
    NTSTATUS status = g_OrigMonitorDispatch(device, Irp);

    if (!NT_SUCCESS(status) || !wmiBuffer || wmiSize < sizeof(WNODE_ALL_DATA_MINI))
        return status;

    WNODE_ALL_DATA_MINI* allData = (WNODE_ALL_DATA_MINI*)wmiBuffer;
    if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(allData) || !allData->InstanceCount)
        return status;

    BOOLEAN fixed = (allData->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE) != 0;
    UCHAR* base = (UCHAR*)allData + allData->DataBlockOffset;

    for (ULONG i = 0; i < allData->InstanceCount; i++)
    {
        WmiMonitorID_MINI* mon = NULL;

        if (fixed) {
            mon = (WmiMonitorID_MINI*)(base + i * allData->FixedInstanceSize);
        } else {
            OFFSETINSTDATALEN* inst = &allData->OffsetInstanceDataAndLength[i];
            if (!inst->OffsetInstanceData) continue;
            mon = (WmiMonitorID_MINI*)((UCHAR*)allData + inst->OffsetInstanceData);
        }

        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(mon)) continue;

        ULONG cacheIdx = 0xFF;
        for (ULONG k = 0; k < MON_CACHE_SIZE; k++) {
            if (g_MonCacheInit[k] && g_MonCacheDevice[k] == device) { cacheIdx = k; break; }
            if (!g_MonCacheInit[k] && cacheIdx == 0xFF) cacheIdx = k;
        }
        if (cacheIdx >= MON_CACHE_SIZE) cacheIdx = 0;

        if (!g_MonCacheInit[cacheIdx]) {
            ULONG seed = 0;
            for (int s = 0; s < SPOOFER_SEED_SIZE; s++)
                seed ^= ((ULONG)g_MonSeed[s]) << ((s % 4) * 8);
            seed ^= ((ULONG)(ULONG_PTR)device) * 0x9E3779B9;

            GenerateMonitorSerial(g_MonCacheSerial[cacheIdx], sizeof(g_MonCacheSerial[cacheIdx]), seed);
            g_MonCacheDevice[cacheIdx] = device;
            g_MonCacheInit[cacheIdx] = TRUE;
        }

        AnsiToWmiStr(g_MonCacheSerial[cacheIdx], mon->SerialNumberID, 16);

        {
            ULONG pcSeed = 0;
            for (int s = 0; s < SPOOFER_SEED_SIZE; s++)
                pcSeed ^= ((ULONG)g_MonSeed[s]) << ((s % 4) * 8);
            pcSeed ^= ((ULONG)(ULONG_PTR)device) * 0x0C0D;
            MonXorShift(&pcSeed);

            char pcStr[5];
            pcStr[0] = "0123456789ABCDEF"[(pcSeed >> 0) & 0xF];
            pcStr[1] = "0123456789ABCDEF"[(pcSeed >> 4) & 0xF];
            pcStr[2] = "0123456789ABCDEF"[(pcSeed >> 8) & 0xF];
            pcStr[3] = "0123456789ABCDEF"[(pcSeed >> 12) & 0xF];
            pcStr[4] = '\0';
            AnsiToWmiStr(pcStr, mon->ProductCodeID, 4);

            for (int j = 4; j < 16; j++) mon->ProductCodeID[j] = 0;
        }

        if (mon->UserFriendlyNameLength > 0) {
            ULONG nameLen = (ULONG)((fn_strlen)ApiResolveExport(HASH_STRLEN))(g_MonSpoofedName);
            if (nameLen > 13) nameLen = 13;
            AnsiToWmiStr(g_MonSpoofedName, mon->UserFriendlyName, nameLen);
            mon->UserFriendlyNameLength = (USHORT)nameLen;
        }
    }

    return status;
}


NTSTATUS InstallMonitorHook(const UINT8* seed)
{
    RtlCopyMemory(g_MonSeed, seed, SPOOFER_SEED_SIZE);

    RtlZeroMemory(g_MonCacheDevice, sizeof(g_MonCacheDevice));
    RtlZeroMemory(g_MonCacheSerial, sizeof(g_MonCacheSerial));
    RtlZeroMemory(g_MonCacheInit, sizeof(g_MonCacheInit));
    g_MonStringsInit = FALSE;

    InitMonitorStrings();

    PatchRegistryEdid();

    {
        UNICODE_STRING monName;
        PDRIVER_OBJECT drv = NULL;
        NTSTATUS status;
        STK_DRIVER_MONITOR(_monN);
        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&monName, _monN);
        status = ResolveObRefByName(&monName, OBJ_CASE_INSENSITIVE, NULL, 0, KernelMode, NULL, (PVOID*)&drv);
        STK_WIPE_W(_monN, 16);

        if (!NT_SUCCESS(status) || !drv) {
            SPOOF_DBG("[Spoofer] Monitor: driver not found (0x%08X)\n", status);
            return STATUS_SUCCESS;
        }

        g_OrigMonitorDispatch = drv->MajorFunction[IRP_MJ_SYSTEM_CONTROL];
        drv->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = MonitorDispatchIo;
        g_MonitorDriverObj = drv;

        {
            fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
            if (_pDeref) _pDeref(drv);
        }
    }
    SPOOF_DBG("[Spoofer] Monitor: WMI hook installed\n");
    return STATUS_SUCCESS;
}

void UninstallMonitorHook(void)
{
    if (g_MonitorDriverObj && g_OrigMonitorDispatch) {
        g_MonitorDriverObj->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = g_OrigMonitorDispatch;
        SPOOF_DBG("[Spoofer] Monitor: WMI hook removed\n");
    }

    g_OrigMonitorDispatch = NULL;
    g_MonitorDriverObj = NULL;
    g_MonStringsInit = FALSE;
}
