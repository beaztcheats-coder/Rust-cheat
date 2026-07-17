#include "../include/spoofer.h"

#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#endif

#define SystemModuleInformation 11


#define GPU_MAX_COUNT       32
#define GPU_UUID_SIZE       16      


typedef struct _RTL_MODULE_ENTRY {
    LIST_ENTRY  InLoadOrderLinks;
    PVOID       Reserved[2];
    PVOID       DllBase;
    PVOID       EntryPoint;
    ULONG       SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} RTL_MODULE_ENTRY, *PRTL_MODULE_ENTRY;

static PVOID GetKernelModuleBase(const WCHAR* moduleName, PULONG outSize)
{
    PVOID base = NULL;
    ULONG size = 0;
    ULONG bufSize = 0;
    PVOID buffer = NULL;
    NTSTATUS status;

    status = ((fn_ZwQuerySystemInformation)ApiResolveExport(HASH_ZWQUERYSYSTEMINFORMATION))(11, NULL, 0, &bufSize);
    if (status != STATUS_INFO_LENGTH_MISMATCH || bufSize == 0)
        return NULL;

    buffer = ((fn_ExAllocatePoolWithTag)ApiResolveExport(HASH_EXALLOCATEPOOLWITHTAG))(NonPagedPool, bufSize, g_Spoofer.PoolTag);
    if (!buffer) return NULL;

    status = ((fn_ZwQuerySystemInformation)ApiResolveExport(HASH_ZWQUERYSYSTEMINFORMATION))(11, buffer, bufSize, &bufSize);
    if (!NT_SUCCESS(status)) {
        ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(buffer, g_Spoofer.PoolTag);
        return NULL;
    }

    {
        typedef struct _RTL_PROCESS_MODULE_INFORMATION {
            HANDLE Section;
            PVOID  MappedBase;
            PVOID  ImageBase;
            ULONG  ImageSize;
            ULONG  Flags;
            USHORT LoadOrderIndex;
            USHORT InitOrderIndex;
            USHORT LoadCount;
            USHORT OffsetToFileName;
            UCHAR  FullPathName[256];
        } RTL_PROCESS_MODULE_INFORMATION;

        typedef struct _RTL_PROCESS_MODULES {
            ULONG NumberOfModules;
            RTL_PROCESS_MODULE_INFORMATION Modules[1];
        } RTL_PROCESS_MODULES;

        RTL_PROCESS_MODULES* mods = (RTL_PROCESS_MODULES*)buffer;
        ULONG i;
        ANSI_STRING ansiTarget;
        char narrowName[64];
        UNICODE_STRING uniName;

        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uniName, moduleName);
        ((fn_RtlUnicodeStringToAnsiString)ApiResolveExport(HASH_RTLUNICODESTRINGTOANSISTRING))(&ansiTarget, &uniName, TRUE);

        for (i = 0; i < mods->NumberOfModules; i++) {
            const char* fileName = (const char*)(mods->Modules[i].FullPathName +
                                    mods->Modules[i].OffsetToFileName);
            SIZE_T fnLen = 0;
            const char* p = fileName;
            while (*p) { fnLen++; p++; }

            if (fnLen == (SIZE_T)ansiTarget.Length) {
                BOOLEAN match = TRUE;
                SIZE_T j;
                for (j = 0; j < fnLen; j++) {
                    char a = fileName[j];
                    char b = ansiTarget.Buffer[j];
                    if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
                    if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
                    if (a != b) { match = FALSE; break; }
                }
                if (match) {
                    base = mods->Modules[i].ImageBase;
                    size = mods->Modules[i].ImageSize;
                    break;
                }
            }
        }

        ((fn_RtlFreeAnsiString)ApiResolveExport(HASH_RTLFREEANSISTRING))(&ansiTarget);
    }

    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(buffer, g_Spoofer.PoolTag);

    if (outSize) *outSize = size;
    return base;
}

static PVOID FindPattern(PUCHAR base, ULONG size, const UCHAR* pattern, const char* mask)
{
    SIZE_T maskLen = 0;
    const char* m = mask;
    SIZE_T i, j;

    while (*m) { maskLen++; m++; }

    if (maskLen == 0 || size < maskLen)
        return NULL;

    for (i = 0; i <= size - maskLen; i++) {
        BOOLEAN found = TRUE;

        __try {
            for (j = 0; j < maskLen; j++) {
                if (mask[j] == 'x' && base[i + j] != pattern[j]) {
                    found = FALSE;
                    break;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            continue;
        }

        if (found)
            return (PVOID)(base + i);
    }

    return NULL;
}


static PVOID ResolveRelativeCall(PVOID callInstr)
{
    PUCHAR addr = (PUCHAR)callInstr;
    INT32 offset;

    __try {
        if (*addr != 0xE8)
            return NULL;
        offset = *(INT32*)(addr + 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

    return (PVOID)(addr + 5 + offset);
}



static BOOLEAN IsNvidiaGpuPresent(void)
{
    NTSTATUS status;
    UNICODE_STRING regPath;
    OBJECT_ATTRIBUTES objAttr;
    HANDLE hKey = NULL;
    BOOLEAN found = FALSE;

    STK_REG_PCI_ENUM(_pciPath);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&regPath, _pciPath);
    InitializeObjectAttributes(&objAttr, &regPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ((fn_ZwOpenKey)ApiResolveExport(HASH_ZWOPENKEY))(&hKey, KEY_READ, &objAttr);
    STK_WIPE_W(_pciPath, 52);
    if (NT_SUCCESS(status)) {
        ULONG index = 0;
        UCHAR buffer[512];
        ULONG resultLen;

        while (NT_SUCCESS(((fn_ZwEnumerateKey)ApiResolveExport(HASH_ZWENUMERATEKEY))(hKey, index, KeyBasicInformation, buffer, sizeof(buffer), &resultLen))) {
            KEY_BASIC_INFORMATION* keyInfo = (KEY_BASIC_INFORMATION*)buffer;
            WCHAR nameBuffer[128];
            ULONG copyLen = keyInfo->NameLength / sizeof(WCHAR);

            if (copyLen > 127) copyLen = 127;
            RtlCopyMemory(nameBuffer, keyInfo->Name, copyLen * sizeof(WCHAR));
            nameBuffer[copyLen] = L'\0';

            {
                ULONG k;
                for (k = 0; k + 7 < copyLen; k++) {
                    if ((nameBuffer[k]     == L'V' || nameBuffer[k]     == L'v') &&
                        (nameBuffer[k + 1] == L'E' || nameBuffer[k + 1] == L'e') &&
                        (nameBuffer[k + 2] == L'N' || nameBuffer[k + 2] == L'n') &&
                        nameBuffer[k + 3] == L'_' &&
                        nameBuffer[k + 4] == L'1' &&
                        nameBuffer[k + 5] == L'0' &&
                        (nameBuffer[k + 6] == L'D' || nameBuffer[k + 6] == L'd') &&
                        (nameBuffer[k + 7] == L'E' || nameBuffer[k + 7] == L'e'))
                    {
                        found = TRUE;
                        break;
                    }
                }
            }

            if (found) break;
            index++;
        }

        ((fn_ZwClose)ApiResolveExport(HASH_ZWCLOSE))(hKey);
    }

    if (!found) {
        ULONG dummy;
        PVOID nvBase = GetKernelModuleBase(L"nvlddmkm.sys", &dummy);
        if (nvBase) {
            found = TRUE;
            SPOOF_DBG("[Spoofer] GPU: NVIDIA detected via module presence\n");
        }
    }

    return found;
}


static void GenerateGpuUuid(const UINT8* seed, UINT8* outUuid)
{
    ULONG i;

    for (i = 0; i < GPU_UUID_SIZE; i++) {
        UINT32 hash = 0x811c9dc5;
        hash ^= seed[i % SPOOFER_SEED_SIZE];    hash *= 0x01000193;
        hash ^= seed[(i + 5) % SPOOFER_SEED_SIZE];  hash *= 0x01000193;
        hash ^= seed[(i + 11) % SPOOFER_SEED_SIZE]; hash *= 0x01000193;
        hash ^= (UINT8)(i & 0xFF);              hash *= 0x01000193;
        hash ^= 0x47;
        hash *= 0x01000193;
        hash ^= seed[(i * 3 + 7) % SPOOFER_SEED_SIZE];  hash *= 0x01000193;

        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;

        outUuid[i] = (UINT8)(hash & 0xFF);
    }

    {
        BOOLEAN allZero = TRUE;
        for (i = 0; i < GPU_UUID_SIZE; i++) {
            if (outUuid[i] != 0) { allZero = FALSE; break; }
        }
        if (allZero) outUuid[0] = 0x01;
    }
}


NTSTATUS SpoofGpuUuid(const UINT8* seed)
{
    PVOID nvBase = NULL;
    ULONG nvSize = 0;
    PVOID patternAddr = NULL;
    PVOID gpuMgrFunc = NULL;
    UINT32 uuidValidOffset = 0;
    UINT32 uuidDataOffset = 0;
    UINT8 spoofedUuid[GPU_UUID_SIZE];
    ULONG spoofedCount = 0;
    int gpuIdx;

    if (!IsNvidiaGpuPresent()) {
        SPOOF_DBG("[Spoofer] GPU: No NVIDIA GPU detected — skipping GPU spoof (AMD/Intel safe)\n");
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    SPOOF_DBG("[Spoofer] GPU: NVIDIA GPU confirmed — starting UUID spoof\n");

    nvBase = GetKernelModuleBase(L"nvlddmkm.sys", &nvSize);
    if (!nvBase || nvSize == 0) {
        SPOOF_DBG("[Spoofer] GPU: nvlddmkm.sys not found\n");
        return STATUS_NOT_FOUND;
    }

    SPOOF_DBG("[Spoofer] GPU: nvlddmkm.sys base=%p size=0x%X\n", nvBase, nvSize);


    {
        // Matches: 561.xx, 566.xx, 610.xx (aktuelle)
        static const UCHAR pat1[] = {
            0xE8, 0x00, 0x00, 0x00, 0x00,   // call GpuMgrGetGpuFromId
            0x48, 0x8B, 0xD8,                // mov rbx, rax
            0x48, 0x85, 0xC0,                // test rax, rax
            0x0F, 0x84, 0x00, 0x00, 0x00, 0x00, // jz <skip>
            0x44, 0x8B, 0x80, 0x00, 0x00, 0x00, 0x00, // mov r8d, [rax+??]
            0x48, 0x8D, 0x15                 // lea rdx, [rip+??]
        };
        static const char mask1[] = "x????xxxxxxxx????xxx????xxx";

        // Matches: 537.xx, 546.xx
        static const UCHAR pat2[] = {
            0xE8, 0x00, 0x00, 0x00, 0x00,   // call GpuMgrGetGpuFromId
            0x48, 0x8B, 0xD8,                // mov rbx, rax
            0x48, 0x85, 0xC0,                // test rax, rax
            0x74, 0x00,                      // jz <skip> (short)
            0x44, 0x8B, 0x80, 0x00, 0x00, 0x00, 0x00, // mov r8d, [rax+??]
            0x48, 0x8D, 0x15                 // lea rdx, [rip+??]
        };
        static const char mask2[] = "x????xxxxxxx?xxx????xxx";

        // Matches: ältere 5xx.xx versionen
        static const UCHAR pat3[] = {
            0xE8, 0x00, 0x00, 0x00, 0x00,   // call GpuMgrGetGpuFromId
            0x48, 0x8B, 0xD8,                // mov rbx, rax
            0x48, 0x85, 0xC0,                // test rax, rax
            0x0F, 0x84, 0x00, 0x00, 0x00, 0x00, // jz <skip>
            0x8B, 0x88, 0x00, 0x00, 0x00, 0x00, // mov ecx, [rax+??]
            0x48, 0x8D, 0x15                 // lea rdx, [rip+??]
        };
        static const char mask3[] = "x????xxxxxxxx????xx????xxx";

        struct {
            const UCHAR* pattern;
            const char*  mask;
            const char*  name;
        } patterns[] = {
            { pat1, mask1, "Pattern1 (610.xx)" },
            { pat2, mask2, "Pattern2 (537-546.xx)" },
            { pat3, mask3, "Pattern3 (older 5xx)" },
        };

        ULONG p;
        for (p = 0; p < 3; p++) {
            patternAddr = FindPattern((PUCHAR)nvBase, nvSize,
                patterns[p].pattern, patterns[p].mask);
            if (patternAddr) {
                SPOOF_DBG("[Spoofer] GPU: Found %s at %p\n", patterns[p].name, patternAddr);
                break;
            }
        }
    }

    if (!patternAddr) {
        SPOOF_DBG("[Spoofer] GPU: No valid pattern found — driver version unsupported\n");
        return STATUS_NOT_FOUND;
    }

    gpuMgrFunc = ResolveRelativeCall(patternAddr);
    if (!gpuMgrFunc) {
        SPOOF_DBG("[Spoofer] GPU: Failed to resolve GpuMgrGetGpuFromId call\n");
        return STATUS_NOT_FOUND;
    }

    if ((ULONG_PTR)gpuMgrFunc < (ULONG_PTR)nvBase ||
        (ULONG_PTR)gpuMgrFunc >= (ULONG_PTR)nvBase + nvSize) {
        SPOOF_DBG("[Spoofer] GPU: GpuMgrGetGpuFromId at %p outside module bounds\n", gpuMgrFunc);
        return STATUS_INVALID_ADDRESS;
    }

    SPOOF_DBG("[Spoofer] GPU: GpuMgrGetGpuFromId = %p\n", gpuMgrFunc);

    {
        PUCHAR scanBase = (PUCHAR)patternAddr;
        ULONG scanRange = 0x100;  
        ULONG i;
        BOOLEAN found = FALSE;

        for (i = 0; i < scanRange - 7; i++) {
            __try {
                if (scanBase[i] == 0x80 && scanBase[i + 1] == 0xBB) {
                    UINT32 candidateOffset = *(UINT32*)(scanBase + i + 2);
                    UINT8 cmpValue = scanBase[i + 6];

                    if (cmpValue == 0x00 && candidateOffset > 0x100 && candidateOffset < 0x10000) {
                        uuidValidOffset = candidateOffset;
                        uuidDataOffset = candidateOffset + 1;
                        found = TRUE;
                        SPOOF_DBG("[Spoofer] GPU: bIsUuidValid offset = 0x%X\n", uuidValidOffset);
                        SPOOF_DBG("[Spoofer] GPU: UUID data offset     = 0x%X\n", uuidDataOffset);
                        break;
                    }
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                continue;
            }
        }

        if (!found) {
            uuidValidOffset = 0x0CD4;
            uuidDataOffset  = 0x0CD5;
            SPOOF_DBG("[Spoofer] GPU: Using hardcoded offsets (0xCD4/0xCD5)\n");
        }
    }

    GenerateGpuUuid(seed, spoofedUuid);

    SPOOF_DBG("[Spoofer] GPU: Spoofed UUID = %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
        spoofedUuid[0], spoofedUuid[1], spoofedUuid[2], spoofedUuid[3],
        spoofedUuid[4], spoofedUuid[5], spoofedUuid[6], spoofedUuid[7],
        spoofedUuid[8], spoofedUuid[9], spoofedUuid[10], spoofedUuid[11],
        spoofedUuid[12], spoofedUuid[13], spoofedUuid[14], spoofedUuid[15]);

    {
        typedef UINT64 (__fastcall *PFN_GpuMgrGetGpuFromId)(int);
        PFN_GpuMgrGetGpuFromId pfnGetGpu = (PFN_GpuMgrGetGpuFromId)gpuMgrFunc;

        for (gpuIdx = 0; gpuIdx < GPU_MAX_COUNT; gpuIdx++) {
            UINT64 gpuObj = 0;

            __try {
                gpuObj = pfnGetGpu(gpuIdx);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                SPOOF_DBG("[Spoofer] GPU: Exception calling GetGpuFromId(%d)\n", gpuIdx);
                continue;
            }

            if (!gpuObj) continue;

            __try {
                if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))((PVOID)gpuObj)) {
                    SPOOF_DBG("[Spoofer] GPU: GPU %d object at %p not valid\n", gpuIdx, (PVOID)gpuObj);
                    continue;
                }

                UINT8 isValid = *(UINT8*)(gpuObj + uuidValidOffset);
                if (!isValid) {
                    SPOOF_DBG("[Spoofer] GPU: GPU %d UUID not initialized — skipping\n", gpuIdx);
                    continue;
                }

                PUCHAR uuidDest = (PUCHAR)(gpuObj + uuidDataOffset);
                if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(uuidDest) || !((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(uuidDest + GPU_UUID_SIZE - 1)) {
                    SPOOF_DBG("[Spoofer] GPU: GPU %d UUID address %p not valid\n", gpuIdx, uuidDest);
                    continue;
                }

                SPOOF_DBG("[Spoofer] GPU: GPU %d original UUID = %02X%02X%02X%02X-%02X%02X...\n",
                    gpuIdx,
                    uuidDest[0], uuidDest[1], uuidDest[2], uuidDest[3],
                    uuidDest[4], uuidDest[5]);

                RtlCopyMemory(uuidDest, spoofedUuid, GPU_UUID_SIZE);

                spoofedCount++;
                SPOOF_DBG("[Spoofer] GPU: GPU %d UUID spoofed successfully\n", gpuIdx);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                SPOOF_DBG("[Spoofer] GPU: Exception spoofing GPU %d: 0x%X\n",
                    gpuIdx, GetExceptionCode());
                continue;
            }
        }
    }

    if (spoofedCount > 0) {
        g_Spoofer.GpuSpoofed = TRUE;
        SPOOF_DBG("[Spoofer] GPU: Successfully spoofed %lu GPU(s)\n", spoofedCount);
        return STATUS_SUCCESS;
    }

    SPOOF_DBG("[Spoofer] GPU: No GPUs were spoofed (found %lu total)\n", spoofedCount);
    return STATUS_UNSUCCESSFUL;
}
