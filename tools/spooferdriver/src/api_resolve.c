#include <ntddk.h>
#include <intrin.h>
#include "../include/api_resolve.h"

/* ============================================================================
 * api_resolve.c — FNV-1a Runtime API Resolver
 *
 * Technique (identical to Amd_RiverHelper.sys):
 *   1. SIDT → read IDT base address
 *   2. Extract ISR address from IDT entry → lives inside ntoskrnl
 *   3. Scan backwards from ISR in 2MB steps, check for MZ + PE header
 *   4. Parse export directory, hash each export name with FNV-1a
 *   5. Match against target hash → return function pointer
 *   6. Cache ntoskrnl base for subsequent calls
 * ============================================================================ */

/* Cached ntoskrnl base */
static UINT_PTR g_KernelBase = 0;

/* ── FNV-1a Hash (runtime, for export name matching) ─────────────────────── */

static UINT64 FnvHashString(const char* str)
{
    UINT64 hash = FNV_OFFSET_BASIS;

    while (*str)
    {
        hash ^= (UINT64)(UINT8)*str;
        hash *= FNV_PRIME;
        str++;
    }

    return hash;
}


/* ── Find ntoskrnl base via IDT ──────────────────────────────────────────── */

#pragma pack(push, 1)
typedef struct _IDT_DESCRIPTOR {
    UINT16 Limit;
    UINT64 Base;
} IDT_DESCRIPTOR;

typedef struct _IDT_ENTRY_64 {
    UINT16  OffsetLow;
    UINT16  Selector;
    UINT8   Ist;
    UINT8   TypeAttr;
    UINT16  OffsetMid;
    UINT32  OffsetHigh;
    UINT32  Reserved;
} IDT_ENTRY_64;
#pragma pack(pop)

UINT_PTR ApiFindKernelBase(void)
{
    IDT_DESCRIPTOR idt;
    IDT_ENTRY_64*  entry;
    UINT_PTR       isrAddr;
    UINT_PTR       probe;

    if (g_KernelBase)
        return g_KernelBase;

    /* Read IDT register */
    __sidt(&idt);

    /* Extract ISR address from IDT entry 14 (page fault handler — always in ntoskrnl) */
    entry = (IDT_ENTRY_64*)(idt.Base + 14 * sizeof(IDT_ENTRY_64));
    isrAddr = ((UINT_PTR)entry->OffsetHigh << 32)
            | ((UINT_PTR)entry->OffsetMid  << 16)
            |  (UINT_PTR)entry->OffsetLow;

    /* Scan backwards in 2MB steps for PE header */
    probe = isrAddr & ~0x1FFFFFULL;  /* Align to 2MB boundary */

    while (probe > 0)
    {
        __try
        {
            /* Check MZ signature */
            if (*(UINT16*)probe == 0x5A4D)  /* 'MZ' */
            {
                LONG peOff = *(LONG*)(probe + 0x3C);

                if (peOff > 0 && peOff < 0x1000)
                {
                    UINT_PTR peHdr = probe + peOff;

                    /* Check PE signature + x64 machine type */
                    if (*(UINT32*)peHdr == 0x4550 &&        /* 'PE\0\0' */
                        *(UINT16*)(peHdr + 4) == 0x8664)    /* AMD64 */
                    {
                        g_KernelBase = probe;
                        return probe;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) { /* skip invalid pages */ }

        probe -= 0x200000;  /* 2MB step */
    }

    return 0;
}


/* ── Resolve export by FNV-1a hash ───────────────────────────────────────── */

static PVOID ResolveExportInternal(UINT_PTR base, UINT64 targetHash, BOOLEAN isData)
{
    LONG                     peOff;
    UINT_PTR                 peHdr;
    UINT32                   exportRva;
    UINT_PTR                 exportDir;
    UINT32                   numNames;
    UINT32*                  nameRvas;
    UINT16*                  ordinals;
    UINT32*                  funcRvas;
    UINT32                   i;

    if (!base)
        return NULL;

    peOff     = *(LONG*)(base + 0x3C);
    peHdr     = base + peOff;

    /* Export directory RVA is at PE+0x88 (offset 136) in optional header */
    exportRva = *(UINT32*)(peHdr + 0x88);
    if (!exportRva)
        return NULL;

    exportDir = base + exportRva;

    numNames = *(UINT32*)(exportDir + 0x18);     /* NumberOfNames */
    funcRvas = (UINT32*)(base + *(UINT32*)(exportDir + 0x1C));  /* AddressOfFunctions */
    nameRvas = (UINT32*)(base + *(UINT32*)(exportDir + 0x20));  /* AddressOfNames */
    ordinals = (UINT16*)(base + *(UINT32*)(exportDir + 0x24));  /* AddressOfNameOrdinals */

    for (i = 0; i < numNames; i++)
    {
        const char* name = (const char*)(base + nameRvas[i]);
        UINT64 hash = FnvHashString(name);

        if (hash == targetHash)
        {
            UINT32 funcRva = funcRvas[ordinals[i]];

            if (isData)
            {
                /* For data exports (e.g., IoDriverObjectType), return the address itself */
                return (PVOID)(base + funcRva);
            }

            return (PVOID)(base + funcRva);
        }
    }

    return NULL;
}


PVOID ApiResolveExport(UINT64 hash)
{
    UINT_PTR base = ApiFindKernelBase();
    return ResolveExportInternal(base, hash, FALSE);
}

PVOID ApiResolveData(UINT64 hash)
{
    UINT_PTR base = ApiFindKernelBase();
    return ResolveExportInternal(base, hash, TRUE);
}


/* ── Init (validate we can find ntoskrnl) ────────────────────────────────── */

NTSTATUS ApiResolverInit(void)
{
    UINT_PTR base = ApiFindKernelBase();

    if (!base)
        return STATUS_NOT_FOUND;

    return STATUS_SUCCESS;
}
