#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ntddk.h>
#include <wdm.h>
#include <ntstrsafe.h>

#ifdef __cplusplus
}
#endif

#include "shared.h"
#include "classpnp_internal.h"
#include "spoof_engine.h"
#include "tpm.h"
#include "api_resolve.h"
#include "stack_str.h"
#include "skCrypter.h"



#ifdef DBG
  #ifdef __cplusplus
    extern BOOLEAN g_DebugEnabled;
    #define SPOOF_DBG(...) \
        do { if (g_DebugEnabled) DbgPrint(__VA_ARGS__); } while(0)
  #else
    #define SPOOF_DBG DbgPrint
  #endif
#else

  #define SPOOF_DBG(...) ((void)0)
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef NTSTATUS (NTAPI* fn_ObReferenceObjectByName)(
    PUNICODE_STRING ObjectName,
    ULONG Attributes,
    PACCESS_STATE AccessState,
    ACCESS_MASK DesiredAccess,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    PVOID ParseContext,
    PVOID* Object
);



#ifdef __cplusplus
}
#endif


#define IOCTL_STORAGE_QUERY_PROPERTY    0x002D1400
#define IOCTL_SCSI_PASS_THROUGH        0x0004D004
#define IOCTL_SCSI_PASS_THROUGH_DIRECT 0x0004D014
#define IOCTL_ATA_PASS_THROUGH         0x0004D02C
#define IOCTL_ATA_PASS_THROUGH_DIRECT  0x0004D030
#define IOCTL_SCSI_MINIPORT            0x0004D008
#define IOCTL_STORAGE_PROTOCOL_COMMAND 0x002D5140
#define SMART_RCV_DRIVE_DATA           0x0007C088

#define StorageDeviceProperty          0
#define StorageDeviceIdProperty        2


typedef struct _STORAGE_PROPERTY_QUERY_MINI {
    ULONG PropertyId;
    ULONG QueryType;
    UCHAR AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY_MINI;

typedef struct _STORAGE_DEVICE_DESCRIPTOR_MINI {
    ULONG Version;
    ULONG Size;
    UCHAR DeviceType;
    UCHAR DeviceTypeModifier;
    UCHAR RemovableMedia;
    UCHAR CommandQueueing;
    ULONG VendorIdOffset;
    ULONG ProductIdOffset;
    ULONG ProductRevisionOffset;
    ULONG SerialNumberOffset;
    ULONG BusType;
    ULONG RawPropertiesLength;
    UCHAR RawDeviceProperties[1];
} STORAGE_DEVICE_DESCRIPTOR_MINI, *PSTORAGE_DEVICE_DESCRIPTOR_MINI;

typedef struct _STORAGE_IDENTIFIER_MINI {
    ULONG CodeSet;
    ULONG Type;
    USHORT IdentifierSize;
    USHORT NextOffset;
    ULONG Association;
    UCHAR Identifier[1];
} STORAGE_IDENTIFIER_MINI, *PSTORAGE_IDENTIFIER_MINI;

typedef struct _STORAGE_DEVICE_ID_DESCRIPTOR_MINI {
    ULONG Version;
    ULONG Size;
    ULONG NumberOfIdentifiers;
    UCHAR Identifiers[1];
} STORAGE_DEVICE_ID_DESCRIPTOR_MINI, *PSTORAGE_DEVICE_ID_DESCRIPTOR_MINI;

typedef struct _SCSI_PASS_THROUGH_DIRECT {
    USHORT  Length;
    UCHAR   ScsiStatus;
    UCHAR   PathId;
    UCHAR   TargetId;
    UCHAR   Lun;
    UCHAR   CdbLength;
    UCHAR   SenseInfoLength;
    UCHAR   DataIn;
    ULONG   DataTransferLength;
    ULONG   TimeOutValue;
    PVOID   DataBuffer;
    ULONG   SenseInfoOffset;
    UCHAR   Cdb[16];
} SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

typedef struct _ATA_PASS_THROUGH_DIRECT {
    USHORT    Length;
    USHORT    AtaFlags;
    UCHAR     PathId;
    UCHAR     TargetId;
    UCHAR     Lun;
    UCHAR     ReservedAsUchar;
    ULONG     DataTransferLength;
    ULONG     TimeOutValue;
    ULONG     ReservedAsUlong;
    PVOID     DataBuffer;
    UCHAR     PreviousTaskFile[8];
    UCHAR     CurrentTaskFile[8];
} ATA_PASS_THROUGH_DIRECT, *PATA_PASS_THROUGH_DIRECT;


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SPOOFER_GLOBALS {
    UINT8               Seed[SPOOFER_SEED_SIZE];
    BOOLEAN             Installed;
    
    BOOLEAN             DescriptorsPatched;
    ULONG               DevicesPatchedCount;
    
    BOOLEAN             DispatchHooked;
    PDRIVER_OBJECT      DiskDriverObject;
    PVOID               OriginalClassDeviceControl;
    PVOID               DevInfoPointer;
    
    BOOLEAN             RegistryCallbackActive;
    LARGE_INTEGER       RegistryCallbackCookie;
    
    UINT64              SmbiosPhysAddrOffset;
    UINT64              SmbiosTableLenOffset;
    BOOLEAN             OffsetsProvided;
    
    BOOLEAN             MacPatched;
    SPOOFER_NDIS_OFFSETS_INPUT NdisOffsets;
    BOOLEAN             NdisOffsetsProvided;
    
    SPOOFER_KNOWN_MACS_INPUT KnownMacs;
    BOOLEAN             KnownMacsProvided;
    
    SPOOFER_STORPORT_OFFSETS_INPUT StorPortOffsets;
    
    BOOLEAN             TpmHookInstalled;
    UINT64              TpmDispatchCommandRva;
    BOOLEAN             TpmOffsetsProvided;
    BOOLEAN             StorPortOffsetsProvided;
    
    BOOLEAN             GpuSpoofed;
    
    char                FirstOriginalSerial[64];
    char                FirstSpoofedSerial[64];
    BOOLEAN             FirstSerialCaptured;
    
    ULONG               PoolTag;
} SPOOFER_GLOBALS, *PSPOOFER_GLOBALS;

extern SPOOFER_GLOBALS g_Spoofer;
extern BOOLEAN g_DebugEnabled;


NTSTATUS PatchDeviceDescriptors(const UINT8* seed);
void     RestoreDeviceDescriptors(void);

NTSTATUS InstallDispatchHook(const UINT8* seed);
void     UninstallDispatchHook(void);

NTSTATUS InstallRegistryCallback(const UINT8* seed);
void     UninstallRegistryCallback(void);

NTSTATUS PatchSmbiosTable(const UINT8* seed);
void     RestoreSmbiosTable(void);


#define IOCTL_NDIS_QUERY_GLOBAL_STATS 0x00170002

#define OID_802_3_PERMANENT_ADDRESS   0x01010101
#define OID_802_3_CURRENT_ADDRESS     0x01010102
#define OID_802_5_PERMANENT_ADDRESS   0x02010101
#define OID_802_5_CURRENT_ADDRESS     0x02010102
#define OID_WAN_PERMANENT_ADDRESS     0x04010101
#define OID_WAN_CURRENT_ADDRESS       0x04010102
#define OID_ARCNET_PERMANENT_ADDRESS  0x06010101
#define OID_ARCNET_CURRENT_ADDRESS    0x06010102

typedef struct _IOC_REQUEST {
    PIO_COMPLETION_ROUTINE  OldRoutine;
    PVOID                   OldContext;
    UCHAR                   OldControl;
} IOC_REQUEST, *PIOC_REQUEST;

#define MAX_NIC_DRIVERS 16

typedef struct _NIC_DRIVER {
    PDRIVER_OBJECT  DriverObject;
    PDRIVER_DISPATCH Original;
} NIC_DRIVER;

#define MAX_MAC_MAP 32

typedef struct _MAC_MAP_ENTRY {
    UCHAR   Original[6];
    UCHAR   Spoofed[6];
    BOOLEAN Valid;
} MAC_MAP_ENTRY;

NTSTATUS PatchMacAddresses(const UINT8* seed);
void     RestoreMacAddresses(void);
NTSTATUS InstallNicHooks(void);
void     UninstallNicHooks(void);
BOOLEAN  MacMapLookup(const UCHAR* originalMac, UCHAR* spoofedMac);
void     MacMapAdd(const UCHAR* originalMac, const UCHAR* spoofedMac);

NTSTATUS ScanKernelModulesForMacs(const UINT8* seed);

NTSTATUS InstallNsiHook(void);
void     UninstallNsiHook(void);

extern PUCHAR  g_SpoofedSmbiosRaw;
extern ULONG   g_SpoofedSmbiosLen;
extern char    g_SmbiosSystemSerial[128];
extern char    g_SmbiosBaseBoardSerial[128];
extern char    g_SmbiosChassisSerial[128];
extern char    g_SmbiosSystemUUID[64];
extern BOOLEAN g_SmbiosPatched;

extern NIC_DRIVER g_NicDrivers[MAX_NIC_DRIVERS];
extern ULONG      g_NicDriverCount;
extern MAC_MAP_ENTRY g_MacMap[MAX_MAC_MAP];
extern ULONG      g_MacMapCount;

PDRIVER_OBJECT GetDriverObjectByName(const WCHAR* name);
BOOLEAN ValidateDeviceDescriptor(PVOID descriptor);
PVOID   GetDeviceExtensionField(PVOID deviceExtension, ULONG offset);
void    InitPoolTag(const UINT8* seed, PULONG outTag);

NTSTATUS SpoofEfiVariables(const UINT8* seed);

NTSTATUS SpoofWindowsIdentifiers(const UINT8* seed);

NTSTATUS InstallUsbHooks(const UINT8* seed);
void     UninstallUsbHooks(void);

NTSTATUS InstallMonitorHook(const UINT8* seed);
void     UninstallMonitorHook(void);

NTSTATUS PatchSpacePortCache(const UINT8* seed);

NTSTATUS SpoofMountedDevicesRegistry(const UINT8* seed);

NTSTATUS InitializeTpmSpoofer(UINT8* seed);
NTSTATUS InstallTpmHook(void);
void     UninstallTpmHook(void);
void     CleanupTpmSpoofer(void);

NTSTATUS SpoofGpuUuid(const UINT8* seed);

NTSTATUS SpoofGptGuids(const UINT8* seed);
void     GenerateSpoofGuid(GUID* out, const UINT8* seed, ULONG salt);
NTSTATUS SpoofBluetoothMac(const UINT8* seed);

NTSTATUS SpoofExtraIdentifiers(const UINT8* seed);

NTSTATUS InstallNtfsHooks(const UINT8* seed);
void     UninstallNtfsHooks(void);

#ifdef __cplusplus
}
#endif
