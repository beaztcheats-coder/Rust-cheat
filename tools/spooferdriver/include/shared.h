#pragma once
#define SPOOFER_DEVICE_NAME     L"\\Device\\WdmPnpBroker"
#define SPOOFER_SYMLINK_NAME    L"\\??\\WdmPnpBroker"
#define SPOOFER_USERMODE_PATH   L"\\\\.\\WdmPnpBroker"

#define SPOOFER_SEED_SIZE       16

#define IOCTL_SPOOFER_INSTALL         (ULONG)0x80002000   // Function 0x800
#define IOCTL_SPOOFER_UNINSTALL       (ULONG)0x80002004   // Function 0x801
#define IOCTL_SPOOFER_STATUS          (ULONG)0x80002008   // Function 0x802
#define IOCTL_SPOOFER_SET_OFFSETS     (ULONG)0x8000200C   // Function 0x803
#define IOCTL_SPOOFER_SET_NDIS_OFFSETS (ULONG)0x80002010  // Function 0x804
#define IOCTL_SPOOFER_SET_KNOWN_MACS  (ULONG)0x80002014   // Function 0x805
#define IOCTL_SPOOFER_SET_STORPORT_OFFSETS (ULONG)0x80002018 // Function 0x806
#define IOCTL_SPOOFER_SET_TPM_OFFSETS  (ULONG)0x8000201C   // Function 0x807

#define MAX_KNOWN_MACS 16

typedef struct _SPOOFER_INSTALL_INPUT {
    UINT8   Seed[SPOOFER_SEED_SIZE];
} SPOOFER_INSTALL_INPUT, *PSPOOFER_INSTALL_INPUT;

typedef struct _SPOOFER_OFFSETS_INPUT {
    UINT64  SmbiosPhysAddrOffset;   // Offset of WmipSMBiosTablePhysicalAddress
    UINT64  SmbiosTableLenOffset;   // Offset of WmipSMBiosTableLength
} SPOOFER_OFFSETS_INPUT, *PSPOOFER_OFFSETS_INPUT;

typedef struct _SPOOFER_NDIS_OFFSETS_INPUT {
    UINT64  NdisGlobalFilterListOffset;       // ndis!ndisGlobalFilterList symbol
    UINT32  FilterBlockIfBlockOffset;         // _NDIS_FILTER_BLOCK.IfBlock
    UINT32  FilterBlockMiniportOffset;        // _NDIS_FILTER_BLOCK.Miniport
    UINT32  FilterBlockNextGlobalFilterOffset; // _NDIS_FILTER_BLOCK.NextGlobalFilter (usually 0x68)
    UINT32  FilterInstanceNameOffset;         // _NDIS_FILTER_BLOCK.FilterInstanceName
    UINT32  IfBlockPhysAddressOffset;         // _NDIS_IF_BLOCK.ifPhysAddress (IF_PHYSICAL_ADDRESS_LH)
    UINT32  IfBlockPermPhysAddressOffset;     // _NDIS_IF_BLOCK.PermanentPhysAddress
    UINT32  MiniportBlockIfBlockOffset;       // _NDIS_MINIPORT_BLOCK.IfBlock
} SPOOFER_NDIS_OFFSETS_INPUT, *PSPOOFER_NDIS_OFFSETS_INPUT;

typedef struct _SPOOFER_KNOWN_MACS_INPUT {
    UINT32  Count;
    UINT8   Macs[MAX_KNOWN_MACS][8]; 
} SPOOFER_KNOWN_MACS_INPUT, *PSPOOFER_KNOWN_MACS_INPUT;

typedef struct _SPOOFER_STORPORT_OFFSETS_INPUT {
    UINT32  RaidExtSerialStringOff;      
    UINT32  RaidExtRichDescPtrOff;        
    UINT32  RaidExtInlineSerialOff;       
    UINT32  RaidExtUnitAdapterOff;        

    UINT32  RaidAdapterMiniportOff;       
    UINT32  MiniportPrivateExtOff;        

    UINT32  NvmeControllerOff;            
    UINT32  IdentifyDataPtrOff;           
    UINT32  IdentifySerialOff;            
    UINT32  IdentifySerialLen;           

    UINT32  NvmeAdapterSerialCacheOff;   
    UINT32  RaidAdapterSerialCacheOff;    

 
    UINT64  RaidUnitRegIfRva;             

 
    UINT32  SpDeviceIdPtrOff;            
    UINT32  SpDeviceIdDataOff;            
    UINT32  SpDeviceIdDataLen;            
} SPOOFER_STORPORT_OFFSETS_INPUT, *PSPOOFER_STORPORT_OFFSETS_INPUT;

typedef struct _SPOOFER_TPM_OFFSETS_INPUT {
    UINT64  DispatchCommandRva;    
} SPOOFER_TPM_OFFSETS_INPUT, *PSPOOFER_TPM_OFFSETS_INPUT;

typedef struct _SPOOFER_STATUS_OUTPUT {
    UINT8   Installed;
    UINT8   DescriptorPatched;      // Schicht 1
    UINT8   DispatchHooked;         // Schicht 2
    UINT8   RegistryCallbackActive; // Schicht 3
    UINT8   SmbiosPatched;          // Schicht 4
    UINT8   MacPatched;             // Schicht 5
    UINT8   TpmPatched;             // Schicht 12
    UINT8   GpuSpoofed;            // Schicht 13
    UINT32  DevicesPatched;
    char    FirstSpoofedSerial[64];
    char    SpoofedUUID[64];
} SPOOFER_STATUS_OUTPUT, *PSPOOFER_STATUS_OUTPUT;

