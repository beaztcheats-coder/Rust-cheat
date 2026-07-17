#include "../include/spoofer.h"
#include "../include/spoof_engine.h"

// ============================================================================
// spaceport_patch.c — Schicht 10: SpacePort Binary EUI Patching
//
// ONLY patches the raw binary EUI-64 bytes in SpacePort's cache.
// Text-based EUI (eui.XXXX) is handled by Schicht 2 (dispatch_hook)
// via IOCTL interception — we must NOT touch text here to avoid
// double-spoofing.
//
// The binary EUI lives in a pool allocation pointed to by
// SP_DEVICE+0x890. We find it by scanning the DeviceExtension
// and following pointers 1 level deep, looking for the EXACT
// original serial bytes.
// ============================================================================

// ── Helpers ─────────────────────────────────────────────────────────────────

static int HexCharVal(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static BOOLEAN IsHexChar(char c)
{
    return HexCharVal(c) >= 0;
}

// Extract hex chars from serial, strip underscores/dots/spaces
static int ExtractHexChars(const char* serial, char* outHex, int maxOut)
{
    int i, count = 0;
    for (i = 0; serial[i] && count < maxOut - 1; i++) {
        if (IsHexChar(serial[i]))
            outHex[count++] = serial[i];
    }
    outHex[count] = 0;
    return count;
}

// Convert hex string to binary bytes
static int HexToBytes(const char* hex, int hexLen, UINT8* outBytes, int maxBytes)
{
    int i, byteIdx = 0;
    for (i = 0; i + 1 < hexLen && byteIdx < maxBytes; i += 2) {
        int hi = HexCharVal(hex[i]);
        int lo = HexCharVal(hex[i + 1]);
        if (hi < 0 || lo < 0) return byteIdx;
        outBytes[byteIdx++] = (UINT8)((hi << 4) | lo);
    }
    return byteIdx;
}

// Generate deterministic spoofed binary bytes from seed
static void GenerateSpoofedBytes(const UINT8* origBytes, int len, UINT8* outBytes, const UINT8* seed)
{
    int i;
    ULONG s = 0xE0100;
    for (i = 0; i < SPOOFER_SEED_SIZE; i++)
        s ^= ((ULONG)seed[i]) << ((i % 4) * 8);

    for (i = 0; i < len; i++) {
        s = (s * 1103515245 + 12345 + i) & 0x7FFFFFFF;
        outBytes[i] = (UINT8)((origBytes[i] ^ (s & 0xFF)) | 0x01);
    }
}

// ── Cached values ───────────────────────────────────────────────────────────

static UINT8 g_OrigBytes[32];
static UINT8 g_SpoofBytes[32];
static int   g_ByteLen = 0;
static BOOLEAN g_CacheReady = FALSE;

static BOOLEAN PrepareSpoof(const UINT8* seed)
{
    char origHex[64];
    int hexLen;

    if (g_CacheReady) return TRUE;

    if (!g_Spoofer.FirstSerialCaptured || g_Spoofer.FirstOriginalSerial[0] == 0)
        return FALSE;

    hexLen = ExtractHexChars(g_Spoofer.FirstOriginalSerial, origHex, 64);
    if (hexLen < 4 || (hexLen % 2) != 0) return FALSE;

    g_ByteLen = HexToBytes(origHex, hexLen, g_OrigBytes, 32);
    if (g_ByteLen < 2) return FALSE;

    GenerateSpoofedBytes(g_OrigBytes, g_ByteLen, g_SpoofBytes, seed);

    SPOOF_DBG("[Spoofer] Schicht10: Original serial='%s' -> %d binary bytes\n",
        g_Spoofer.FirstOriginalSerial, g_ByteLen);
    {
        int i;
        SPOOF_DBG("[Spoofer] Schicht10: OrigBytes=");
        for (i = 0; i < g_ByteLen; i++)
            SPOOF_DBG("%02X", g_OrigBytes[i]);
        SPOOF_DBG("\n");
        SPOOF_DBG("[Spoofer] Schicht10: SpoofBytes=");
        for (i = 0; i < g_ByteLen; i++)
            SPOOF_DBG("%02X", g_SpoofBytes[i]);
        SPOOF_DBG("\n");
    }

    g_CacheReady = TRUE;
    return TRUE;
}

// ── Binary byte scan ────────────────────────────────────────────────────────

static ULONG ScanMemoryForBytes(PUCHAR mem, ULONG memSize)
{
    ULONG off;
    ULONG patched = 0;

    for (off = 0; off + g_ByteLen <= memSize; off++)
    {
        if (!((fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID))(mem + off + g_ByteLen - 1)) break;

        __try {
            if (((fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY))(mem + off, g_OrigBytes, g_ByteLen) == (SIZE_T)g_ByteLen)
            {
                SPOOF_DBG("[Spoofer] BinaryEUI: match at offset 0x%X\n", off);
                RtlCopyMemory(mem + off, g_SpoofBytes, g_ByteLen);
                patched++;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            break;
        }
    }

    return patched;
}

// ── Main entry point ────────────────────────────────────────────────────────

NTSTATUS PatchSpacePortCache(const UINT8* seed)
{
    UNICODE_STRING driverName;
    PDRIVER_OBJECT drvObj = NULL;
    PDEVICE_OBJECT devObj;
    ULONG patchCount = 0;
    int devCount = 0;
    NTSTATUS status;

    /* Cache all API pointers ONCE — these were being resolved thousands of
       times inside tight loops, causing the hang at "Finalizing 95%". */
    fn_MmIsAddressValid pMmValid = (fn_MmIsAddressValid)ApiResolveExport(HASH_MMISADDRESSVALID);
    fn_RtlCompareMemory pCmpMem = (fn_RtlCompareMemory)ApiResolveExport(HASH_RTLCOMPAREMEMORY);
    fn_RtlInitUnicodeString pRtlInit = (fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING);
    fn_ObfDereferenceObject pObDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);

    if (!pMmValid || !pCmpMem || !pRtlInit) {
        SPOOF_DBG("[Spoofer] Schicht10: API resolve failed\n");
        return STATUS_NOT_FOUND;
    }

    if (!PrepareSpoof(seed)) {
        SPOOF_DBG("[Spoofer] Schicht10: No original serial captured yet, skipping\n");
        return STATUS_NOT_FOUND;
    }

    /* Stack-encrypted \\Driver\\spaceport */
    {
        STK_DRIVER_SPACEPORT(_spName);
        pRtlInit(&driverName, _spName);
        status = ResolveObRefByName(&driverName, OBJ_CASE_INSENSITIVE, NULL, 0, KernelMode, NULL, (PVOID*)&drvObj);
        STK_WIPE_W(_spName, 18);
    }
    if (!NT_SUCCESS(status) || !drvObj)
    {
        SPOOF_DBG("[Spoofer] Schicht10: spaceport not found\n");
        return STATUS_NOT_FOUND;
    }

    devObj = drvObj->DeviceObject;
    while (devObj)
    {
        devCount++;
        if (devObj->DeviceExtension)
        {
            PUCHAR ext = (PUCHAR)devObj->DeviceExtension;
            ULONG extSize = 0;
            ULONG page;

            for (page = 0; page < 0x10000; page += PAGE_SIZE) {
                if (!pMmValid(ext + page)) break;
                extSize = page + PAGE_SIZE;
            }
            if (extSize < 64) { devObj = devObj->NextDevice; continue; }
            if (extSize > 0x10000) extSize = 0x10000;

            /* Inline scan with cached pointers */
            {
                ULONG off;
                for (off = 0; off + g_ByteLen <= extSize; off++)
                {
                    if (!pMmValid(ext + off + g_ByteLen - 1)) break;
                    __try {
                        if (pCmpMem(ext + off, g_OrigBytes, g_ByteLen) == (SIZE_T)g_ByteLen)
                        {
                            SPOOF_DBG("[Spoofer] BinaryEUI: match at offset 0x%X\n", off);
                            RtlCopyMemory(ext + off, g_SpoofBytes, g_ByteLen);
                            patchCount++;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) { break; }
                }
            }

            {
                ULONG ptrOff;
                for (ptrOff = 0; ptrOff + sizeof(PVOID) <= extSize; ptrOff += sizeof(PVOID))
                {
                    PUCHAR ptr;
                    ULONG ptrRegionSize;
                    ULONG ptrPage;

                    if (!pMmValid(ext + ptrOff)) break;

                    __try {
                        ptr = *(PUCHAR*)(ext + ptrOff);
                        if (!ptr) continue;
                        if ((ULONG_PTR)ptr < 0xFFFF800000000000ULL) continue;
                        if (ptr == ext) continue;
                        if (!pMmValid(ptr)) continue;

                        ptrRegionSize = 0;
                        for (ptrPage = 0; ptrPage < 0x2000; ptrPage += PAGE_SIZE) {
                            if (!pMmValid(ptr + ptrPage)) break;
                            ptrRegionSize = ptrPage + PAGE_SIZE;
                        }
                        if (ptrRegionSize < (ULONG)g_ByteLen) continue;
                        if (ptrRegionSize > 0x2000) ptrRegionSize = 0x2000;

                        /* Inline scan with cached pointers */
                        {
                            ULONG off;
                            for (off = 0; off + g_ByteLen <= ptrRegionSize; off++)
                            {
                                if (!pMmValid(ptr + off + g_ByteLen - 1)) break;
                                __try {
                                    if (pCmpMem(ptr + off, g_OrigBytes, g_ByteLen) == (SIZE_T)g_ByteLen)
                                    {
                                        SPOOF_DBG("[Spoofer] BinaryEUI: match at offset 0x%X (ptr)\n", off);
                                        RtlCopyMemory(ptr + off, g_SpoofBytes, g_ByteLen);
                                        patchCount++;
                                    }
                                }
                                __except (EXCEPTION_EXECUTE_HANDLER) { break; }
                            }
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        continue;
                    }
                }
            }
        }
        devObj = devObj->NextDevice;
    }

    if (pObDeref) pObDeref(drvObj);

    SPOOF_DBG("[Spoofer] Schicht10: %d device(s), %lu binary patches\n",
        devCount, patchCount);

    return (patchCount > 0) ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}

