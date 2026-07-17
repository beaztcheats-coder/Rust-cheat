#include "../include/spoofer.h"


SPOOFER_GLOBALS g_Spoofer = { 0 };

#ifdef DBG
BOOLEAN g_DebugEnabled = TRUE;
#else
BOOLEAN g_DebugEnabled = FALSE;
#endif

static PDEVICE_OBJECT g_DeviceObject = NULL;
static UNICODE_STRING g_DeviceName;
static UNICODE_STRING g_SymlinkName;


static NTSTATUS HandleInstall(PIRP Irp, PIO_STACK_LOCATION stack)
{
    PSPOOFER_INSTALL_INPUT input;
    ULONG inputLen;
    NTSTATUS status;

    inputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
    if (inputLen < sizeof(SPOOFER_INSTALL_INPUT)) {
        SPOOF_DBG("[Spoofer] HandleInstall: buffer too small: %lu < %lu\n",
            inputLen, (ULONG)sizeof(SPOOFER_INSTALL_INPUT));
        return STATUS_BUFFER_TOO_SMALL;
    }

    input = (PSPOOFER_INSTALL_INPUT)Irp->AssociatedIrp.SystemBuffer;
    if (!input) return STATUS_INVALID_PARAMETER;

    if (g_Spoofer.Installed) {
        UninstallRegistryCallback();
        UninstallDispatchHook();
        RestoreDeviceDescriptors();
        RtlZeroMemory(&g_Spoofer, sizeof(g_Spoofer));
    }

    RtlCopyMemory(g_Spoofer.Seed, input->Seed, SPOOFER_SEED_SIZE);
    InitPoolTag(g_Spoofer.Seed, &g_Spoofer.PoolTag);

    SPOOF_DBG("[Spoofer] Installing with seed: %02X%02X%02X%02X...\n",
        g_Spoofer.Seed[0], g_Spoofer.Seed[1], g_Spoofer.Seed[2], g_Spoofer.Seed[3]);

    status = PatchDeviceDescriptors(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 1 (Descriptor Patch): %s\n",
        NT_SUCCESS(status) ? "OK" : "FAILED");

    status = InstallDispatchHook(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 2 (Dispatch Hook):    %s\n",
        NT_SUCCESS(status) ? "OK" : "FAILED");

    status = InstallRegistryCallback(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 3 (Registry Callback): %s\n",
        NT_SUCCESS(status) ? "OK" : "FAILED");

    status = PatchSmbiosTable(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 4 (SMBIOS Patch):      %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = PatchMacAddresses(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 5 (MAC Patch):         %s\n",
        g_Spoofer.MacPatched ? "OK" : "SKIPPED");

    InstallNicHooks();

    status = ScanKernelModulesForMacs(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 5b (ntoskrnl MAC Scan): %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    InstallNsiHook();

    status = SpoofEfiVariables(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 6 (EFI Variables):    %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = SpoofWindowsIdentifiers(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 7 (Windows IDs):      %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = InstallUsbHooks(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 8 (USB Hooks):        %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = InstallMonitorHook(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 9 (Monitor Hook):     %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = PatchSpacePortCache(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 10 (SpacePort):       %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = SpoofMountedDevicesRegistry(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 11 (MountedDevices):  %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = InitializeTpmSpoofer(g_Spoofer.Seed);
    if (NT_SUCCESS(status))
    {
        status = InstallTpmHook();
        g_Spoofer.TpmHookInstalled = NT_SUCCESS(status);
    }
    SPOOF_DBG("[Spoofer] Schicht 12 (TPM EK Spoof):    %s\n",
        g_Spoofer.TpmHookInstalled ? "OK" : "SKIPPED");

    status = SpoofGpuUuid(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 13 (GPU UUID):        %s\n",
        g_Spoofer.GpuSpoofed ? "OK" : "SKIPPED");

    status = SpoofGptGuids(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 14 (GPT GUIDs):       %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = SpoofBluetoothMac(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 15 (Bluetooth MAC):   %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = SpoofExtraIdentifiers(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 16 (Extra IDs):       %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    status = InstallNtfsHooks(g_Spoofer.Seed);
    SPOOF_DBG("[Spoofer] Schicht 17 (NTFS VolSerial):  %s\n",
        NT_SUCCESS(status) ? "OK" : "SKIPPED");

    g_Spoofer.Installed = TRUE;

    SPOOF_DBG("[Spoofer] ====================================================\n");
    SPOOF_DBG("[Spoofer] INSTALLATION COMPLETE\n");
    SPOOF_DBG("[Spoofer] + Descriptors:  %lu devices patched\n", g_Spoofer.DevicesPatchedCount);
    SPOOF_DBG("[Spoofer] + Dispatch:     %s\n", g_Spoofer.DispatchHooked ? "ACTIVE" : "INACTIVE");
    SPOOF_DBG("[Spoofer] + Registry:     %s\n", g_Spoofer.RegistryCallbackActive ? "ACTIVE" : "INACTIVE");
    SPOOF_DBG("[Spoofer] + SMBIOS:       %s\n", g_SmbiosPatched ? "PATCHED" : "NOT AVAILABLE");
    SPOOF_DBG("[Spoofer] + MAC:          %s\n", g_Spoofer.MacPatched ? "PATCHED" : "NOT AVAILABLE");
    SPOOF_DBG("[Spoofer] + NIC Hooks:    %lu driver(s)\n", g_NicDriverCount);
    if (g_SmbiosSystemUUID[0])
        SPOOF_DBG("[Spoofer] + UUID:         %s\n", g_SmbiosSystemUUID);
    SPOOF_DBG("[Spoofer] + TPM:          %s\n", g_Spoofer.TpmHookInstalled ? "HOOKED" : "NOT AVAILABLE");
    SPOOF_DBG("[Spoofer] + GPU:          %s\n", g_Spoofer.GpuSpoofed ? "SPOOFED" : "NOT AVAILABLE");
    SPOOF_DBG("[Spoofer] ====================================================\n");

    return STATUS_SUCCESS;
}

static NTSTATUS HandleUninstall(void)
{
    if (!g_Spoofer.Installed) {
        SPOOF_DBG("[Spoofer] UNINSTALL: Not installed\n");
        return STATUS_SUCCESS;
    }

    SPOOF_DBG("[Spoofer] UNINSTALLING...\n");

    CleanupTpmSpoofer();
    UninstallMonitorHook();
    UninstallUsbHooks();
    UninstallNsiHook();
    UninstallNicHooks();
    RestoreMacAddresses();
    RestoreSmbiosTable();
    UninstallRegistryCallback();
    UninstallDispatchHook();
    RestoreDeviceDescriptors();

    g_Spoofer.Installed = FALSE;
    SPOOF_DBG("[Spoofer] UNINSTALL COMPLETE\n");

    return STATUS_SUCCESS;
}

static NTSTATUS HandleStatus(PIRP Irp, PIO_STACK_LOCATION stack)
{
    PSPOOFER_STATUS_OUTPUT output;
    ULONG outputLen;

    outputLen = stack->Parameters.DeviceIoControl.OutputBufferLength;
    if (outputLen < sizeof(SPOOFER_STATUS_OUTPUT)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    output = (PSPOOFER_STATUS_OUTPUT)Irp->AssociatedIrp.SystemBuffer;
    if (!output) return STATUS_INVALID_PARAMETER;

    RtlZeroMemory(output, sizeof(SPOOFER_STATUS_OUTPUT));
    output->Installed = g_Spoofer.Installed;
    output->DescriptorPatched = g_Spoofer.DescriptorsPatched;
    output->DispatchHooked = g_Spoofer.DispatchHooked;
    output->RegistryCallbackActive = g_Spoofer.RegistryCallbackActive;
    output->SmbiosPatched = g_SmbiosPatched;
    output->MacPatched = g_Spoofer.MacPatched;
    output->TpmPatched = g_Spoofer.TpmHookInstalled;
    output->GpuSpoofed = g_Spoofer.GpuSpoofed;
    output->DevicesPatched = g_Spoofer.DevicesPatchedCount;
    RtlCopyMemory(output->FirstSpoofedSerial, g_Spoofer.FirstSpoofedSerial,
        sizeof(output->FirstSpoofedSerial));
    RtlCopyMemory(output->SpoofedUUID, g_SmbiosSystemUUID,
        sizeof(output->SpoofedUUID));

    Irp->IoStatus.Information = sizeof(SPOOFER_STATUS_OUTPUT);
    return STATUS_SUCCESS;
}

static NTSTATUS HandleSetOffsets(PIRP Irp, PIO_STACK_LOCATION stack)
{
    PSPOOFER_OFFSETS_INPUT input;
    ULONG inputLen;

    inputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
    if (inputLen < sizeof(SPOOFER_OFFSETS_INPUT))
        return STATUS_BUFFER_TOO_SMALL;

    input = (PSPOOFER_OFFSETS_INPUT)Irp->AssociatedIrp.SystemBuffer;
    if (!input) return STATUS_INVALID_PARAMETER;

    g_Spoofer.SmbiosPhysAddrOffset = input->SmbiosPhysAddrOffset;
    g_Spoofer.SmbiosTableLenOffset = input->SmbiosTableLenOffset;
    g_Spoofer.OffsetsProvided = TRUE;

    SPOOF_DBG("[Spoofer] Offsets received: PhysAddr=0x%llX, Len=0x%llX\n",
        g_Spoofer.SmbiosPhysAddrOffset, g_Spoofer.SmbiosTableLenOffset);

    return STATUS_SUCCESS;
}

static NTSTATUS DriverDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DeviceObject);

    stack = IoGetCurrentIrpStackLocation(Irp);

    switch (stack->MajorFunction) {
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        status = STATUS_SUCCESS;
        break;

    case IRP_MJ_DEVICE_CONTROL:
        switch (stack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_SPOOFER_INSTALL:
            status = HandleInstall(Irp, stack);
            break;
        case IOCTL_SPOOFER_UNINSTALL:
            status = HandleUninstall();
            break;
        case IOCTL_SPOOFER_STATUS:
            status = HandleStatus(Irp, stack);
            break;
        case IOCTL_SPOOFER_SET_OFFSETS:
            status = HandleSetOffsets(Irp, stack);
            break;
        case IOCTL_SPOOFER_SET_NDIS_OFFSETS:
            {
                PSPOOFER_NDIS_OFFSETS_INPUT ndisInput;
                ULONG ndisInputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
                if (ndisInputLen < sizeof(SPOOFER_NDIS_OFFSETS_INPUT)) {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    ndisInput = (PSPOOFER_NDIS_OFFSETS_INPUT)Irp->AssociatedIrp.SystemBuffer;
                    if (!ndisInput) { status = STATUS_INVALID_PARAMETER; break; }
                    RtlCopyMemory(&g_Spoofer.NdisOffsets, ndisInput, sizeof(SPOOFER_NDIS_OFFSETS_INPUT));
                    g_Spoofer.NdisOffsetsProvided = TRUE;
                    SPOOF_DBG("[Spoofer] NDIS offsets: FilterList=0x%llX, IfBlock=0x%X, Miniport=0x%X, ifPhys=0x%X\n",
                        ndisInput->NdisGlobalFilterListOffset,
                        ndisInput->FilterBlockIfBlockOffset,
                        ndisInput->FilterBlockMiniportOffset,
                        ndisInput->IfBlockPhysAddressOffset);
                    status = STATUS_SUCCESS;
                }
            }
            break;
        case IOCTL_SPOOFER_SET_KNOWN_MACS:
            {
                PSPOOFER_KNOWN_MACS_INPUT macsInput;
                ULONG macsInputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
                if (macsInputLen < sizeof(SPOOFER_KNOWN_MACS_INPUT)) {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    macsInput = (PSPOOFER_KNOWN_MACS_INPUT)Irp->AssociatedIrp.SystemBuffer;
                    if (!macsInput) { status = STATUS_INVALID_PARAMETER; break; }
                    RtlCopyMemory(&g_Spoofer.KnownMacs, macsInput, sizeof(SPOOFER_KNOWN_MACS_INPUT));
                    g_Spoofer.KnownMacsProvided = TRUE;
                    SPOOF_DBG("[Spoofer] Known MACs received: %lu adapter(s)\n", macsInput->Count);
                    status = STATUS_SUCCESS;
                }
            }
            break;
        case IOCTL_SPOOFER_SET_STORPORT_OFFSETS:
            {
                PSPOOFER_STORPORT_OFFSETS_INPUT spInput;
                ULONG spInputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
                if (spInputLen < sizeof(SPOOFER_STORPORT_OFFSETS_INPUT)) {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    spInput = (PSPOOFER_STORPORT_OFFSETS_INPUT)Irp->AssociatedIrp.SystemBuffer;
                    if (!spInput) { status = STATUS_INVALID_PARAMETER; break; }
                    RtlCopyMemory(&g_Spoofer.StorPortOffsets, spInput, sizeof(SPOOFER_STORPORT_OFFSETS_INPUT));
                    g_Spoofer.StorPortOffsetsProvided = TRUE;
                    SPOOF_DBG("[Spoofer] StorPort offsets: Serial=0x%X, Adapter=0x%X, NvmeCtrl=0x%X, RegIf=0x%llX\n",
                        spInput->RaidExtSerialStringOff,
                        spInput->RaidExtUnitAdapterOff,
                        spInput->NvmeControllerOff,
                        spInput->RaidUnitRegIfRva);
                    status = STATUS_SUCCESS;
                }
            }
            break;
        case IOCTL_SPOOFER_SET_TPM_OFFSETS:
            {
                PSPOOFER_TPM_OFFSETS_INPUT tpmInput;
                ULONG tpmInputLen = stack->Parameters.DeviceIoControl.InputBufferLength;
                if (tpmInputLen < sizeof(SPOOFER_TPM_OFFSETS_INPUT)) {
                    status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    tpmInput = (PSPOOFER_TPM_OFFSETS_INPUT)Irp->AssociatedIrp.SystemBuffer;
                    if (!tpmInput) { status = STATUS_INVALID_PARAMETER; break; }
                    g_Spoofer.TpmDispatchCommandRva = tpmInput->DispatchCommandRva;
                    g_Spoofer.TpmOffsetsProvided = TRUE;
                    SPOOF_DBG("[Spoofer] TPM offsets: DispatchCommand RVA=0x%llX\n",
                        tpmInput->DispatchCommandRva);
                    status = STATUS_SUCCESS;
                }
            }
            break;
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    if (stack->MajorFunction != IRP_MJ_DEVICE_CONTROL ||
        stack->Parameters.DeviceIoControl.IoControlCode != IOCTL_SPOOFER_STATUS) {
        Irp->IoStatus.Information = 0;
    }
    ((fn_IofCompleteRequest)ApiResolveExport(HASH_IOFCOMPLETEREQUEST))(Irp, IO_NO_INCREMENT);
    return status;
}

static void DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    SPOOF_DBG("[Spoofer] DriverUnload called\n");

    if (g_Spoofer.Installed) {
        CleanupTpmSpoofer();
        RestoreMacAddresses();
        UninstallRegistryCallback();
        UninstallDispatchHook();
        RestoreDeviceDescriptors();
    }

    if (g_DeviceObject) {
        ((fn_IoDeleteSymbolicLink)ApiResolveExport(HASH_IODELETESYMBOLICLINK))(&g_SymlinkName);
        ((fn_IoDeleteDevice)ApiResolveExport(HASH_IODELETEDEVICE))(g_DeviceObject);
        g_DeviceObject = NULL;
    }

    SPOOF_DBG("[Spoofer] Driver unloaded\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    SPOOF_DBG("[Spoofer] DriverEntry — 3-Layer Disk Spoofer\n");
    SPOOF_DBG("[Spoofer] Build: %s %s\n", __DATE__, __TIME__);

    RtlZeroMemory(&g_Spoofer, sizeof(g_Spoofer));

    status = ApiResolverInit();
    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] ApiResolverInit FAILED: 0x%X\n", status);
        return status;
    }

    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&g_DeviceName, SPOOFER_DEVICE_NAME);
    ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&g_SymlinkName, SPOOFER_SYMLINK_NAME);

    status = ((fn_IoCreateDevice)ApiResolveExport(HASH_IOCREATEDEVICE))(
        DriverObject,
        0,
        &g_DeviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] IoCreateDevice failed: 0x%X\n", status);
        return status;
    }

    status = ((fn_IoCreateSymbolicLink)ApiResolveExport(HASH_IOCREATESYMBOLICLINK))(&g_SymlinkName, &g_DeviceName);
    if (!NT_SUCCESS(status)) {
        SPOOF_DBG("[Spoofer] IoCreateSymbolicLink failed: 0x%X\n", status);
        ((fn_IoDeleteDevice)ApiResolveExport(HASH_IODELETEDEVICE))(g_DeviceObject);
        g_DeviceObject = NULL;
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}
