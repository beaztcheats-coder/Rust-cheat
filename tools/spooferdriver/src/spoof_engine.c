#include "../include/spoofer.h"

static BOOLEAN CharsEqualInsensitive(char a, char b)
{
    if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
    if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
    return a == b;
}

static BOOLEAN StrMatchInsensitive(const char* str, SIZE_T strLen, const char* pattern)
{
    SIZE_T i;
    SIZE_T patLen = 0;
    const char* p = pattern;
    while (*p) { patLen++; p++; }

    if (strLen != patLen) return FALSE;

    for (i = 0; i < strLen; i++) {
        if (!CharsEqualInsensitive(str[i], pattern[i]))
            return FALSE;
    }
    return TRUE;
}

BOOLEAN IsDefaultString(const char* str, SIZE_T len)
{
    if (!str || len < 1) return TRUE;

    while (len > 0 && str[len - 1] == ' ')
        len--;

    if (len == 0) return TRUE;

    if (StrMatchInsensitive(str, len, "none")) return TRUE;
    if (StrMatchInsensitive(str, len, "n/a")) return TRUE;
    if (StrMatchInsensitive(str, len, "na")) return TRUE;
    if (StrMatchInsensitive(str, len, "not specified")) return TRUE;
    if (StrMatchInsensitive(str, len, "not available")) return TRUE;
    if (StrMatchInsensitive(str, len, "default string")) return TRUE;
    if (StrMatchInsensitive(str, len, "default")) return TRUE;

    if (StrMatchInsensitive(str, len, "to be filled by o.e.m.")) return TRUE;
    if (StrMatchInsensitive(str, len, "to be filled by o.e.m")) return TRUE;
    if (StrMatchInsensitive(str, len, "to be filled by oem")) return TRUE;
    if (StrMatchInsensitive(str, len, "to be filled by oem.")) return TRUE;

    {
        SIZE_T dotTrimmed = len;
        while (dotTrimmed > 0 && str[dotTrimmed - 1] == '.')
            dotTrimmed--;
        if (dotTrimmed != len && dotTrimmed > 0) {
            if (StrMatchInsensitive(str, dotTrimmed, "to be filled by o.e.m")) return TRUE;
            if (StrMatchInsensitive(str, dotTrimmed, "to be filled by oem")) return TRUE;
        }
    }

    if (StrMatchInsensitive(str, len, "system product name")) return TRUE;
    if (StrMatchInsensitive(str, len, "system manufacturer")) return TRUE;
    if (StrMatchInsensitive(str, len, "type1productconfigid")) return TRUE;
    if (StrMatchInsensitive(str, len, "empty")) return TRUE;
    if (StrMatchInsensitive(str, len, "unknown")) return TRUE;
    if (StrMatchInsensitive(str, len, "no asset information")) return TRUE;
    if (StrMatchInsensitive(str, len, "chassis serial number")) return TRUE;
    if (StrMatchInsensitive(str, len, "base board serial number")) return TRUE;
    if (StrMatchInsensitive(str, len, "no enclosure")) return TRUE;

    {
        BOOLEAN allSame = TRUE;
        SIZE_T i;
        for (i = 1; i < len; i++) {
            if (str[i] != str[0]) { allSame = FALSE; break; }
        }
        if (allSame && len >= 3) return TRUE;
    }

    return FALSE;
}


static UINT32 SpoofHash(UINT8 val, const UINT8* seed, ULONG position)
{
    UINT32 hash = 0x811c9dc5;
    hash ^= val;           hash *= 0x01000193;
    hash ^= seed[position % 16];          hash *= 0x01000193;
    hash ^= seed[(position + 5) % 16];    hash *= 0x01000193;
    hash ^= seed[(position + 11) % 16];   hash *= 0x01000193;
    hash ^= (UINT8)(position & 0xFF);     hash *= 0x01000193;
    hash ^= (UINT8)((position >> 8) & 0xFF); hash *= 0x01000193;
    hash ^= seed[(position * 3 + 7) % 16];   hash *= 0x01000193;
    hash ^= seed[(position * 7 + 13) % 16];  hash *= 0x01000193;

    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;

    return hash;
}


char SpoofCharacter(char c, const UINT8* seed, ULONG position)
{
    UINT32 hash = SpoofHash((UINT8)c, seed, position);
    UINT8 result = (UINT8)(hash & 0xFF);

    if (c >= '0' && c <= '9') return '0' + (result % 10);
    if (c >= 'A' && c <= 'Z') return 'A' + (result % 26);
    if (c >= 'a' && c <= 'z') return 'A' + (result % 26);

    return c;
}


static BOOLEAN ShouldChangePosition(const UINT8* seed, ULONG alphaIdx, SIZE_T totalAlpha)
{
    UINT32 hash = 0x27d4eb2d;
    hash ^= seed[alphaIdx % 16];        hash *= 0x01000193;
    hash ^= seed[(alphaIdx + 3) % 16];  hash *= 0x01000193;
    hash ^= (UINT8)(alphaIdx & 0xFF);   hash *= 0x01000193;
    hash ^= (UINT8)(totalAlpha & 0xFF); hash *= 0x01000193;
    hash ^= hash >> 16;

    return (hash & 0xFF) < 64;
}


BOOLEAN SpoofSerialString(PCHAR serial, SIZE_T maxLen, const UINT8* seed, const char* source)
{
    SIZE_T i, len = 0;
    SIZE_T alphaNum = 0;
    SIZE_T changed = 0;
    char c;
    BOOLEAN atLeastOneChanged = FALSE;

    if (!serial || !seed || maxLen < 4) return FALSE;

    for (i = 0; i < maxLen; i++) {
        c = serial[i];
        if (c == 0 || c < 0x20) break;
        len++;
    }

    if (len < 4) return FALSE;

    if (IsDefaultString(serial, len)) {
        SPOOF_DBG("[Spoofer] %s: Skip default '%.*s'\n", source, (int)len, serial);
        return FALSE;
    }

    for (i = 0; i < len; i++) {
        c = serial[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            alphaNum++;
    }

    if (alphaNum < 4) {
        SPOOF_DBG("[Spoofer] %s: Skip (only %llu alphanumeric in %llu chars)\n",
            source, (UINT64)alphaNum, (UINT64)len);
        return FALSE;
    }

    SPOOF_DBG("[Spoofer] %s: Original='%.*s'\n", source, (int)len, serial);

    if (!g_Spoofer.FirstSerialCaptured) {
        RtlZeroMemory(g_Spoofer.FirstOriginalSerial, sizeof(g_Spoofer.FirstOriginalSerial));
        for (i = 0; i < len && i < 63; i++)
            g_Spoofer.FirstOriginalSerial[i] = serial[i];
    }

    {
        ULONG alphaIdx = 0;
        for (i = 0; i < len; i++) {
            c = serial[i];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                if (ShouldChangePosition(seed, alphaIdx, alphaNum)) {
                    char spoofed = SpoofCharacter(c, seed, alphaIdx);
                    if (spoofed == c) {
                        if (c >= '0' && c <= '9')
                            spoofed = '0' + ((c - '0' + 1) % 10);
                        else if (c >= 'A' && c <= 'Z')
                            spoofed = 'A' + ((c - 'A' + 1) % 26);
                        else
                            spoofed = 'A' + ((c - 'a' + 1) % 26);
                    }
                    serial[i] = spoofed;
                    changed++;
                    atLeastOneChanged = TRUE;
                }
                alphaIdx++;
            }
        }
    }

    if (!atLeastOneChanged && alphaNum > 0) {
        ULONG targetIdx = SpoofHash(seed[0], seed, 0) % (ULONG)alphaNum;
        ULONG alphaIdx = 0;
        for (i = 0; i < len; i++) {
            c = serial[i];
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                if (alphaIdx == targetIdx) {
                    char spoofed = SpoofCharacter(c, seed, alphaIdx);
                    if (spoofed == c) {
                        if (c >= '0' && c <= '9')
                            spoofed = '0' + ((c - '0' + 1) % 10);
                        else
                            spoofed = 'A' + ((c - 'A' + 1) % 26);
                    }
                    serial[i] = spoofed;
                    changed++;
                    break;
                }
                alphaIdx++;
            }
        }
    }

    if (!g_Spoofer.FirstSerialCaptured && len > 0) {
        RtlZeroMemory(g_Spoofer.FirstSpoofedSerial, sizeof(g_Spoofer.FirstSpoofedSerial));
        for (i = 0; i < len && i < 63; i++)
            g_Spoofer.FirstSpoofedSerial[i] = serial[i];
        g_Spoofer.FirstSerialCaptured = TRUE;
    }

    SPOOF_DBG("[Spoofer] %s: Spoofed='%.*s' (%llu/%llu changed)\n",
        source, (int)len, serial, (UINT64)changed, (UINT64)alphaNum);
    return TRUE;
}

void SpoofNvmeSerial(PCHAR serial, const UINT8* seed, const char* source)
{
    SIZE_T i;
    ULONG alphaIdx = 0;
    SIZE_T totalAlpha = 0;
    SIZE_T changed = 0;

    if (!serial || !seed) return;

    for (i = 0; i < 20; i++) {
        char c = serial[i];
        if (c == ' ' || c == 0) continue;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            totalAlpha++;
    }

    if (totalAlpha < 4) return;

    if (IsDefaultString(serial, 20)) return;

    SPOOF_DBG("[Spoofer] %s: Original NVMe SN='%.20s'\n", source, serial);

    for (i = 0; i < 20; i++) {
        char c = serial[i];
        if (c == ' ' || c == 0) continue;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            if (ShouldChangePosition(seed, alphaIdx, totalAlpha)) {
                char spoofed = SpoofCharacter(c, seed, alphaIdx);
                if (spoofed == c) {
                    if (c >= '0' && c <= '9') spoofed = '0' + ((c - '0' + 1) % 10);
                    else spoofed = 'A' + ((c - 'A' + 1) % 26);
                }
                serial[i] = spoofed;
                changed++;
            }
            alphaIdx++;
        }
    }

    SPOOF_DBG("[Spoofer] %s: Spoofed NVMe SN='%.20s' (%llu changed)\n",
        source, serial, (UINT64)changed);
}

void SpoofBinaryData(PUCHAR data, SIZE_T len, const UINT8* seed, const char* source)
{
    SIZE_T i, startOffset;
    SIZE_T zeroCount = 0;
    SIZE_T changed = 0;

    if (!data || !seed || len < 4) return;

    for (i = 0; i < len; i++) {
        if (data[i] == 0x00 || data[i] == 0xFF || data[i] == 0x20)
            zeroCount++;
    }
    if (zeroCount == len) return;

    startOffset = (len > 8) ? 4 : 0;

    for (i = startOffset; i < len; i++) {
        UINT32 hash = SpoofHash(data[i], seed, (ULONG)i);

        if ((hash & 0xFF) < 64) {
            hash = SpoofHash(data[i], seed, (ULONG)(i + 100));
            data[i] = (UINT8)(hash & 0xFF);
            changed++;
        }
    }

    SPOOF_DBG("[Spoofer] %s: Spoofed %llu/%llu binary bytes\n",
        source, (UINT64)changed, (UINT64)(len - startOffset));
}
