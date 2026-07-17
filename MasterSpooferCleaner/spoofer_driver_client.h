#pragma once
#include <windows.h>

// IOCTL codes — matches SpooferDriver.sys shared.h
#define SPOOFER_USERMODE_PATH   L"\\\\.\\WdmPnpBroker"
#define SPOOFER_SEED_SIZE       16
#define MAX_KNOWN_MACS          16

#define IOCTL_SPOOFER_INSTALL              (ULONG)0x80002000
#define IOCTL_SPOOFER_UNINSTALL            (ULONG)0x80002004
#define IOCTL_SPOOFER_STATUS               (ULONG)0x80002008
#define IOCTL_SPOOFER_SET_OFFSETS          (ULONG)0x8000200C
#define IOCTL_SPOOFER_SET_NDIS_OFFSETS     (ULONG)0x80002010
#define IOCTL_SPOOFER_SET_KNOWN_MACS       (ULONG)0x80002014
#define IOCTL_SPOOFER_SET_STORPORT_OFFSETS (ULONG)0x80002018
#define IOCTL_SPOOFER_SET_TPM_OFFSETS      (ULONG)0x8000201C

#pragma pack(push, 1)
typedef struct _SPOOFER_INSTALL_INPUT {
    UINT8 Seed[SPOOFER_SEED_SIZE];
} SPOOFER_INSTALL_INPUT;

typedef struct _SPOOFER_OFFSETS_INPUT {
    UINT64 SmbiosPhysAddrOffset;
    UINT64 SmbiosTableLenOffset;
} SPOOFER_OFFSETS_INPUT;

typedef struct _SPOOFER_NDIS_OFFSETS_INPUT {
    UINT64 NdisGlobalFilterListOffset;
    UINT32 FilterBlockIfBlockOffset;
    UINT32 FilterBlockMiniportOffset;
    UINT32 FilterBlockNextGlobalFilterOffset;
    UINT32 FilterInstanceNameOffset;
    UINT32 IfBlockPhysAddressOffset;
    UINT32 IfBlockPermPhysAddressOffset;
    UINT32 MiniportBlockIfBlockOffset;
} SPOOFER_NDIS_OFFSETS_INPUT;

typedef struct _SPOOFER_KNOWN_MACS_INPUT {
    UINT32 Count;
    UINT8  Macs[MAX_KNOWN_MACS][8];
} SPOOFER_KNOWN_MACS_INPUT;

typedef struct _SPOOFER_STORPORT_OFFSETS_INPUT {
    UINT32 RaidExtSerialStringOff;
    UINT32 RaidExtRichDescPtrOff;
    UINT32 RaidExtInlineSerialOff;
    UINT32 RaidExtUnitAdapterOff;
    UINT32 RaidAdapterMiniportOff;
    UINT32 MiniportPrivateExtOff;
    UINT32 NvmeControllerOff;
    UINT32 IdentifyDataPtrOff;
    UINT32 IdentifySerialOff;
    UINT32 IdentifySerialLen;
    UINT32 NvmeAdapterSerialCacheOff;
    UINT32 RaidAdapterSerialCacheOff;
    UINT64 RaidUnitRegIfRva;
    UINT32 SpDeviceIdPtrOff;
    UINT32 SpDeviceIdDataOff;
    UINT32 SpDeviceIdDataLen;
} SPOOFER_STORPORT_OFFSETS_INPUT;

typedef struct _SPOOFER_TPM_OFFSETS_INPUT {
    UINT64 DispatchCommandRva;
} SPOOFER_TPM_OFFSETS_INPUT;

typedef struct _SPOOFER_STATUS_OUTPUT {
    UINT8  Installed;
    UINT8  DescriptorPatched;
    UINT8  DispatchHooked;
    UINT8  RegistryCallbackActive;
    UINT8  SmbiosPatched;
    UINT8  MacPatched;
    UINT8  TpmPatched;
    UINT8  GpuSpoofed;
    UINT32 DevicesPatched;
    char   FirstSpoofedSerial[64];
    char   SpoofedUUID[64];
} SPOOFER_STATUS_OUTPUT;
#pragma pack(pop)

struct DriverSpooferStatus {
    bool driverConnected;
    bool installed;
    int  layersActive;
    char firstSerial[64];
    char spoofedUUID[64];
};

bool DriverSpoofer_Connect();
void DriverSpoofer_Disconnect();
bool DriverSpoofer_SendOffsets(const SPOOFER_OFFSETS_INPUT& smbios,
                                const SPOOFER_NDIS_OFFSETS_INPUT& ndis,
                                const SPOOFER_KNOWN_MACS_INPUT& macs,
                                const SPOOFER_STORPORT_OFFSETS_INPUT& storport,
                                const SPOOFER_TPM_OFFSETS_INPUT& tpm);
bool DriverSpoofer_Install(const UINT8 seed[16]);
bool DriverSpoofer_Uninstall();
DriverSpooferStatus DriverSpoofer_GetStatus();

bool RunDriverSpoofer();
bool UninstallDriverSpoofer();
