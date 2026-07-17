#pragma once
#include <windows.h>
#include <cstdint>

#define IOCTL_DISPATCH CTL_CODE(FILE_DEVICE_UNKNOWN, 0x3C4F22, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SECURITY_CODE1 0xD93A7A61B8E42C5FULL
#define SECURITY_CODE2 0x4B7E29C2F1A6D3E0ULL

enum class RequestId : ULONG
{
    None = 0,
    GetCr3_1,
    GetCr3_2,
    GetCr3_3,
    GetCr3_VGK,
    GetBase,
    ReadWriteMemory,
    ReadVirtual,
    MouseMovements,
    WindowHide,
    SetCr3Value,
    ReadPhysical,
    GetKernelModuleBase,
    GetModuleBase,
    Spoof_MAC,
    Spoof_DISK,
    Spoof_GPU,
    Spoof_SMBIOS,
    Spoof_BOOTINFO,
    Spoof_FIRMWAREENTRY,
    Spoof_ARP,
    Spoof_SMBIOSNULL,
    Spoof_TPMNULL,
    Spoof_MONITOR,
    Spoof_REGISTRY,
    Set_Spoof_Seed,
};

typedef struct _REQUEST_DATA
{
    ULONG64 SecurityCode1;
    ULONG64 SecurityCode2;
    RequestId Command;

    union
    {
        struct
        {
            ULONG64 processId;
            ULONG64 cr3Value;
            bool sizecheck;
            bool VGKcheck;
        } Cr3Data;

        struct
        {
            ULONG64 base_address;
            ULONG64 found_cr3_value;
            unsigned int process_id;
        } DTBData;

        struct
        {
            ULONG64 Address;
            ULONG64 Buffer;
            ULONG Size;
            ULONGLONG return_size;

            BOOLEAN isWrite;
            BOOLEAN noncachedread;
        } ReadWriteMemoryData;

        struct
        {
            ULONG64 processId;
            ULONG64 baseadressvalue;
        } ProcessBaseData;

        struct
        {
            long MouseX;
            long MouseY;
            unsigned short MouseButtons;
        } MouseData;

        struct
        {
            uintptr_t window;
            unsigned int value;
        } WindowHideData;

        struct
        {
            ULONG64 PhysicalAddress;
            ULONG64 Buffer;
            ULONG Size;
            ULONGLONG return_size;
        } PhysicalReadData;

        struct
        {
            char kernelmodulename[256];
            ULONG64 kernelmodulebase;
        } KernelModuleData;

        struct
        {
            char modulename[256];
            ULONG64 modulebase;
            ULONGLONG ImageSize;
        } ModuleData;

        struct
        {
            ULONG64 BIOS_SEED;
            ULONG64 DISK_SEED;
        } SpoofData;
    };

    bool Result;
} REQUEST_DATA, *PREQUEST_DATA;
