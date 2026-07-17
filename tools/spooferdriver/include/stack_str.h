#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * stack_str.h — C-compatible Stack String Encryption
 *
 * Strings are XOR-encoded as immediate values in .text section (MOV insns).
 * Decoded on the stack at runtime, then wiped after use.
 * No readable strings in .rdata or .data sections.
 * ============================================================================ */

/* ── XOR Decode helpers ───────────────────────────────────────────────────── */

static __forceinline void _stkDecodeW(WCHAR* buf, int len, WCHAR key)
{
    int i;
    for (i = 0; i < len; i++)
        buf[i] ^= (key + (WCHAR)(i * 3));
}

static __forceinline void _stkDecodeA(char* buf, int len, char key)
{
    int i;
    for (i = 0; i < len; i++)
        buf[i] ^= (key + (char)(i * 3));
}

static __forceinline void _stkWipeW(WCHAR* buf, int len)
{
    volatile WCHAR* p = (volatile WCHAR*)buf;
    int i;
    for (i = 0; i < len; i++)
        p[i] = 0;
}

static __forceinline void _stkWipeA(char* buf, int len)
{
    volatile char* p = (volatile char*)buf;
    int i;
    for (i = 0; i < len; i++)
        p[i] = 0;
}

/* ── Macros to init from XOR'd immediate values ──────────────────────────── */

#define STK_INIT_W(buf, len, key, ...) \
    do { \
        const WCHAR _tmp[] = { __VA_ARGS__ }; \
        int _i; for (_i = 0; _i < (len); _i++) (buf)[_i] = _tmp[_i]; \
        _stkDecodeW((buf), (len), (key)); \
    } while(0)

#define STK_INIT_A(buf, len, key, ...) \
    do { \
        const char _tmp[] = { __VA_ARGS__ }; \
        int _i; for (_i = 0; _i < (len); _i++) (buf)[_i] = _tmp[_i]; \
        _stkDecodeA((buf), (len), (key)); \
    } while(0)

#define STK_WIPE_W(buf, len) _stkWipeW((buf), (len))
#define STK_WIPE_A(buf, len) _stkWipeA((buf), (len))

/* ── Include generated encrypted string definitions ──────────────────────── */
#include "stack_str_generated.h"

#ifdef __cplusplus
}
#endif
