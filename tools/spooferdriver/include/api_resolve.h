#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ntddk.h>

/* ============================================================================
 * api_resolve.h — FNV-1a Runtime API Resolver
 *
 * Resolves ntoskrnl exports at runtime by walking the export table and
 * matching FNV-1a 64-bit hashes. No readable API names in the binary.
 * ============================================================================ */

/* FNV-1a 64-bit constants */
#define FNV_OFFSET_BASIS  0xCBF29CE484222325ULL
#define FNV_PRIME         0x100000001B3ULL

/* ntoskrnl module hash */
#define HASH_NTOSKRNL     0xBC8ECBB539643031ULL

/* ── API Hash Definitions (C-computed FNV-1a 64-bit) ──────────────────────── */

#define HASH_OBREFERENCEOBJECTBYNAME        0x9F6A56D86BC44FECULL
#define HASH_OBFDEREFERENCEOBJECT            0xB61941B7EB2F33E5ULL
#define HASH_IODRIVEROBJECTTYPE              0x7E429BA8D4712670ULL
#define HASH_ZWOPENKEY                       0x68728064A1D2D535ULL
#define HASH_ZWSETVALUEKEY                   0x98F5C40CC4762FEEULL
#define HASH_ZWENUMERATEKEY                  0x3C4BF52DD3D6AA5BULL
#define HASH_ZWDELETEKEY                     0x7D4C3ABAA3E42970ULL
#define HASH_ZWDELETEVALUEKEY                0x9082880BF7F32EFDULL
#define HASH_ZWCLOSE                         0x730BEB78DE086D3AULL
#define HASH_ZWCREATEKEY                     0x8DAF12FB2E3254A1ULL
#define HASH_ZWENUMERATEVALUEKEY             0xA7F0D9B45CA5818CULL
#define HASH_EXALLOCATEPOOLWITHTAG           0xB456343AD290F91FULL
#define HASH_EXALLOCATEPOOL2                 0x8F21EF43E24FB4A5ULL
#define HASH_EXFREEPOOLWITHTAG               0x7F6272C86A6EEADCULL
#define HASH_EXFREEPOOL                      0xFB66208BC67B66DCULL
#define HASH_IOCREATEDEVICE                  0x1AF62553DEB3554FULL
#define HASH_IOCREATESYMBOLICLINK            0x205A8A33D7E0AF99ULL
#define HASH_IODELETEDEVICE                  0x3B4B4822507DC0B8ULL
#define HASH_IODELETESYMBOLICLINK            0x1CDC4067F70BD79AULL
#define HASH_IOGETDEVICEOBJECTPOINTER        0x8D49244199AFB66FULL
#define HASH_IOBUILDDEVICEIOCONTROLREQUEST   0x1D9B1572A04955D7ULL
#define HASH_IOFCALLDRIVER                   0xC26886D76545A61BULL
#define HASH_IOFCOMPLETEREQUEST              0x8B081062775EF4FFULL
#define HASH_CMREGISTERCALLBACKEX            0x30D59E31BD19865EULL
#define HASH_CMUNREGISTERCALLBACK            0x7860A3CF7E37E954ULL
#define HASH_CMCALLBACKGETKEYOBJECTIDEX      0x892DF16F016AD1BEULL
#define HASH_MMMAPIOSPACE                    0x40ED8EA987B20683ULL
#define HASH_MMUNMAPIOSPACE                  0x47E1CA0E288956C0ULL
#define HASH_MMISADDRESSVALID                0x052478B3FD7ABC6DULL
#define HASH_KEDELAYEXECUTIONTHREAD          0x8654AA54092A0912ULL
#define HASH_RTLINITUNICODESTRING            0x30E36D1E897986EFULL
#define HASH_RTLRANDOMEX                     0xA12AC26ABE63B26FULL
#define HASH_RTLSTRINGCBPRINTFA              0xAE92284C3296DC6DULL
#define HASH_KEQUERYPERFORMANCECOUNTER       0x20AC1421621BACC9ULL

/* ── Additional IAT-elimination hashes ───────────────────────────────────── */
#define HASH_ZWQUERYVALUEKEY                 0xE8151E4A9CFECBA2ULL
#define HASH_ZWQUERYSYSTEMINFORMATION        0x9B3466F6CE10AC9FULL
#define HASH_EXALLOCATEPOOL                  0x48B8617D81FDD475ULL
#define HASH_IOGETDEVICEINTERFACES           0x8C5FD7DD7EAD5A15ULL
#define HASH_MMMAPLOCKEDPAGES                0xCDF352622C54F7DBULL
#define HASH_RTLUNICODESTRINGTOANSISTRING    0xE27349604A7ABD8AULL
#define HASH_RTLFREEANSISTRING               0x2C14C65C6B0F6BF1ULL
#define HASH_RTLGUIDFROMSTRING               0x4337968171036BCDULL
#define HASH_EXGETFIRMWAREENVIRONMENTVARIABLE 0x8856EFC127411486ULL
#define HASH_EXSETFIRMWAREENVIRONMENTVARIABLE 0x3DBEC3CC5D6C3EC2ULL
#define HASH_RTLCOMPAREMEMORY                0x713C67B5308446AFULL
#define HASH_IOGETCURRENTPROCESS             0xE921EFA903D5D6A1ULL
#define HASH_WCSLEN                          0x52B13D013C85853BULL
#define HASH_STRLEN                          0x8CDF54D90B9B8A37ULL
#define HASH_WCSSTR                          0xBEAE6800E8791C23ULL
#define HASH_IOGETLOWERDEVICEOBJECT           0x3C8643058F856A51ULL

/* ── MDL API hashes (for hypervisor-safe write bypass) ───────────────────── */
#define HASH_IOALLOCATEMDL                   0xC51C45581F224A39ULL
#define HASH_MMBUILDMDLFORNONPAGEDPOOL        0x4072210C6BB6A8B9ULL
#define HASH_MMMAPLOCKEDPAGESSPECIFYCACHE     0x96B081538803F1A4ULL
#define HASH_MMUNMAPLOCKEDPAGES              0x2A9366C7EA2547B0ULL
#define HASH_IOFREEMDL                       0x8E6C891B6ACC6C0EULL

/* ── File API hashes (for MachineGuid.txt / setupapi.dev.log) ────────────── */
#define HASH_ZWCREATEFILE                    0xB2652D28DDC25A6CULL
#define HASH_ZWWRITEFILE                     0x0BA1615476BBC9AFULL
#define HASH_ZWQUERYDIRECTORYFILE            0x79B29145D759B577ULL
#define HASH_ZWDELETEFILE                    0x8CF9E88F850D18FFULL
#define HASH_ZWSETINFORMATIONFILE            0x5BA5254C5F2CAE2EULL

/* ── Resolver Functions ───────────────────────────────────────────────────── */

/* Find ntoskrnl base address via IDT scan */
UINT_PTR ApiFindKernelBase(void);

/* Resolve a function export by FNV-1a hash */
PVOID ApiResolveExport(UINT64 hash);

/* Resolve a data export by FNV-1a hash (e.g., IoDriverObjectType) */
PVOID ApiResolveData(UINT64 hash);

/* Initialize the resolver (call once from DriverEntry) */
NTSTATUS ApiResolverInit(void);

/* ── Convenience Macros ───────────────────────────────────────────────────── */

/* Cast-resolve: API(ReturnType, HASH_NAME, args...) */
#define API_CALL(ret_type, hash, ...) \
    ((ret_type(NTAPI*)(__VA_ARGS__))ApiResolveExport(hash))

/* Cached resolve for hot paths */
#define API_CACHE(var, ret_type, hash, param_types) \
    static ret_type (NTAPI* var) param_types = NULL; \
    if (!(var)) (var) = (ret_type(NTAPI*) param_types)ApiResolveExport(hash);

/* ── Function Pointer Typedefs ───────────────────────────────────────────── */

typedef VOID (NTAPI* fn_RtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
typedef BOOLEAN (NTAPI* fn_MmIsAddressValid)(PVOID);
typedef PVOID (NTAPI* fn_MmMapIoSpace)(PHYSICAL_ADDRESS, SIZE_T, MEMORY_CACHING_TYPE);
typedef VOID (NTAPI* fn_MmUnmapIoSpace)(PVOID, SIZE_T);

typedef NTSTATUS (NTAPI* fn_ZwOpenKey)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
typedef NTSTATUS (NTAPI* fn_ZwClose)(HANDLE);
typedef NTSTATUS (NTAPI* fn_ZwCreateKey)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG, PUNICODE_STRING, ULONG, PULONG);
typedef NTSTATUS (NTAPI* fn_ZwSetValueKey)(HANDLE, PUNICODE_STRING, ULONG, ULONG, PVOID, ULONG);
typedef NTSTATUS (NTAPI* fn_ZwEnumerateKey)(HANDLE, ULONG, KEY_INFORMATION_CLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS (NTAPI* fn_ZwDeleteKey)(HANDLE);
typedef NTSTATUS (NTAPI* fn_ZwDeleteValueKey)(HANDLE, PUNICODE_STRING);
typedef NTSTATUS (NTAPI* fn_ZwEnumerateValueKey)(HANDLE, ULONG, KEY_VALUE_INFORMATION_CLASS, PVOID, ULONG, PULONG);

typedef NTSTATUS (NTAPI* fn_CmRegisterCallbackEx)(PEX_CALLBACK_FUNCTION, PCUNICODE_STRING, PVOID, PVOID, PLARGE_INTEGER, PVOID);
typedef NTSTATUS (NTAPI* fn_CmUnRegisterCallback)(LARGE_INTEGER);
typedef NTSTATUS (NTAPI* fn_CmCallbackGetKeyObjectIDEx)(PLARGE_INTEGER, PVOID, PULONG_PTR, PCUNICODE_STRING*, ULONG);

typedef LONG_PTR (FASTCALL* fn_ObfDereferenceObject)(PVOID);
typedef NTSTATUS (NTAPI* fn_KeDelayExecutionThread)(KPROCESSOR_MODE, BOOLEAN, PLARGE_INTEGER);
typedef ULONG (NTAPI* fn_RtlRandomEx)(PULONG);

/* ── Additional fn_ typedefs for IAT elimination ─────────────────────────── */
typedef NTSTATUS (NTAPI* fn_ZwQueryValueKey)(HANDLE, PUNICODE_STRING, KEY_VALUE_INFORMATION_CLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS (NTAPI* fn_ZwQuerySystemInformation)(ULONG, PVOID, ULONG, PULONG);
typedef PVOID (NTAPI* fn_ExAllocatePoolWithTag)(ULONG, SIZE_T, ULONG);
typedef PVOID (NTAPI* fn_ExAllocatePool2)(ULONG64, SIZE_T, ULONG);
typedef PVOID (NTAPI* fn_ExAllocatePool)(ULONG, SIZE_T);
typedef VOID (NTAPI* fn_ExFreePoolWithTag)(PVOID, ULONG);
typedef NTSTATUS (NTAPI* fn_IoGetDeviceObjectPointer)(PUNICODE_STRING, ACCESS_MASK, PFILE_OBJECT*, PDEVICE_OBJECT*);
typedef NTSTATUS (NTAPI* fn_IoGetDeviceInterfaces)(CONST GUID*, PDEVICE_OBJECT, ULONG, PWSTR*);
typedef PVOID (NTAPI* fn_MmMapLockedPages)(PMDL, KPROCESSOR_MODE);
typedef NTSTATUS (NTAPI* fn_RtlUnicodeStringToAnsiString)(PANSI_STRING, PCUNICODE_STRING, BOOLEAN);
typedef VOID (NTAPI* fn_RtlFreeAnsiString)(PANSI_STRING);
typedef NTSTATUS (NTAPI* fn_RtlGUIDFromString)(PCUNICODE_STRING, GUID*);
typedef NTSTATUS (NTAPI* fn_ExGetFirmwareEnvironmentVariable)(PUNICODE_STRING, LPGUID, PVOID, PULONG, PULONG);
typedef NTSTATUS (NTAPI* fn_ExSetFirmwareEnvironmentVariable)(PUNICODE_STRING, LPGUID, PVOID, ULONG, ULONG);
typedef SIZE_T (NTAPI* fn_RtlCompareMemory)(const VOID*, const VOID*, SIZE_T);
typedef PEPROCESS (NTAPI* fn_IoGetCurrentProcess)(VOID);
typedef NTSTATUS (NTAPI* fn_IoCreateDevice)(PDRIVER_OBJECT, ULONG, PUNICODE_STRING, DEVICE_TYPE, ULONG, BOOLEAN, PDEVICE_OBJECT*);
typedef NTSTATUS (NTAPI* fn_IoCreateSymbolicLink)(PUNICODE_STRING, PUNICODE_STRING);
typedef VOID (NTAPI* fn_IoDeleteDevice)(PDEVICE_OBJECT);
typedef VOID (NTAPI* fn_IoDeleteSymbolicLink)(PUNICODE_STRING);
typedef VOID (FASTCALL* fn_IofCompleteRequest)(PIRP, CCHAR);
typedef SIZE_T (NTAPI* fn_wcslen)(const wchar_t*);
typedef SIZE_T (NTAPI* fn_strlen)(const char*);
typedef wchar_t* (NTAPI* fn_wcsstr)(const wchar_t*, const wchar_t*);
typedef PDEVICE_OBJECT (NTAPI* fn_IoGetLowerDeviceObject)(PDEVICE_OBJECT);

/* ── MDL function typedefs ─────────────────────────────────────────────── */
typedef PMDL (NTAPI* fn_IoAllocateMdl)(PVOID, ULONG, BOOLEAN, BOOLEAN, PIRP);
typedef VOID (NTAPI* fn_MmBuildMdlForNonPagedPool)(PMDL);
typedef PVOID (NTAPI* fn_MmMapLockedPagesSpecifyCache)(PMDL, KPROCESSOR_MODE, MEMORY_CACHING_TYPE, PVOID, ULONG, ULONG);
typedef VOID (NTAPI* fn_MmUnmapLockedPages)(PVOID, PMDL);
typedef VOID (NTAPI* fn_IoFreeMdl)(PMDL);

/* ── File API typedefs ───────────────────────────────────────────────────── */
typedef NTSTATUS (NTAPI* fn_ZwCreateFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,
    PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG);
typedef NTSTATUS (NTAPI* fn_ZwWriteFile)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID,
    PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS (NTAPI* fn_ZwQueryDirectoryFile)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID,
    PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN);
typedef NTSTATUS (NTAPI* fn_ZwDeleteFile)(POBJECT_ATTRIBUTES);
typedef NTSTATUS (NTAPI* fn_ZwSetInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG,
    FILE_INFORMATION_CLASS);

/* Helper: ObReferenceObjectByName via hash-resolved IoDriverObjectType */
static __forceinline NTSTATUS ResolveObRefByName(
    PUNICODE_STRING name, ULONG attr, PACCESS_STATE access, ACCESS_MASK desired,
    KPROCESSOR_MODE mode, PVOID parseCtx, PVOID* object)
{
    typedef NTSTATUS (NTAPI* fn_ObRefByName)(PUNICODE_STRING, ULONG, PACCESS_STATE,
        ACCESS_MASK, POBJECT_TYPE, KPROCESSOR_MODE, PVOID, PVOID*);
    fn_ObRefByName pObRef;
    POBJECT_TYPE* ppType;

    pObRef = (fn_ObRefByName)ApiResolveExport(HASH_OBREFERENCEOBJECTBYNAME);
    ppType = (POBJECT_TYPE*)ApiResolveData(HASH_IODRIVEROBJECTTYPE);
    if (!pObRef || !ppType || !*ppType) return STATUS_NOT_FOUND;

    return pObRef(name, attr, access, desired, *ppType, mode, parseCtx, object);
}

#ifdef __cplusplus
}
#endif
