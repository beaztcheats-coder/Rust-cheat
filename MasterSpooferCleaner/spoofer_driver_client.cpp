#include "spoofer_driver_client.h"
#include "offset_database.h"
#include <cstdio>

static HANDLE g_hSpoofer = INVALID_HANDLE_VALUE;

static bool SendIoctl(ULONG code, const void* in, DWORD inSize, void* out = nullptr, DWORD outSize = 0) {
    if (g_hSpoofer == INVALID_HANDLE_VALUE) return false;
    DWORD returned = 0;
    return DeviceIoControl(g_hSpoofer, code, (LPVOID)in, inSize, out, outSize, &returned, nullptr) != 0;
}

bool DriverSpoofer_Connect() {
    g_hSpoofer = CreateFileW(SPOOFER_USERMODE_PATH, GENERIC_READ | GENERIC_WRITE,
                             0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    return g_hSpoofer != INVALID_HANDLE_VALUE;
}

void DriverSpoofer_Disconnect() {
    if (g_hSpoofer != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hSpoofer);
        g_hSpoofer = INVALID_HANDLE_VALUE;
    }
}

static void GenerateSeed(UINT8 seed[16]) {
    LARGE_INTEGER pc;
    QueryPerformanceCounter(&pc);
    UINT64 val = (UINT64)GetTickCount64() ^ pc.QuadPart ^ (UINT64)seed;
    for (int i = 0; i < 16; i++) {
        val = val * 6364136223846793005ULL + 1442695040888963407ULL;
        seed[i] = (UINT8)(val >> 24);
    }
}

static bool CollectCurrentMacs(SPOOFER_KNOWN_MACS_INPUT& macs) {
    macs.Count = 0;
    PIP_ADAPTER_INFO adapterInfo = nullptr;
    ULONG outBufLen = sizeof(IP_ADAPTER_INFO);
    adapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);
    if (!adapterInfo) return false;

    DWORD dwRetVal = GetAdaptersInfo(adapterInfo, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        free(adapterInfo);
        adapterInfo = (PIP_ADAPTER_INFO)malloc(outBufLen);
        if (!adapterInfo) return false;
        dwRetVal = GetAdaptersInfo(adapterInfo, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter && macs.Count < MAX_KNOWN_MACS) {
            if (pAdapter->AddressLength >= 6) {
                memcpy(macs.Macs[macs.Count], pAdapter->Address, 6);
                macs.Macs[macs.Count][6] = 0;
                macs.Macs[macs.Count][7] = 0;
                macs.Count++;
            }
            pAdapter = pAdapter->Next;
        }
    }
    free(adapterInfo);
    return macs.Count > 0;
}

bool DriverSpoofer_SendOffsets(const SPOOFER_OFFSETS_INPUT& smbios,
                                const SPOOFER_NDIS_OFFSETS_INPUT& ndis,
                                const SPOOFER_KNOWN_MACS_INPUT& macs,
                                const SPOOFER_STORPORT_OFFSETS_INPUT& storport,
                                const SPOOFER_TPM_OFFSETS_INPUT& tpm) {
    bool ok = true;
    ok &= SendIoctl(IOCTL_SPOOFER_SET_OFFSETS, &smbios, sizeof(smbios));
    ok &= SendIoctl(IOCTL_SPOOFER_SET_NDIS_OFFSETS, &ndis, sizeof(ndis));
    ok &= SendIoctl(IOCTL_SPOOFER_SET_KNOWN_MACS, &macs, sizeof(macs));
    ok &= SendIoctl(IOCTL_SPOOFER_SET_STORPORT_OFFSETS, &storport, sizeof(storport));
    ok &= SendIoctl(IOCTL_SPOOFER_SET_TPM_OFFSETS, &tpm, sizeof(tpm));
    return ok;
}

bool DriverSpoofer_Install(const UINT8 seed[16]) {
    SPOOFER_INSTALL_INPUT input;
    memcpy(input.Seed, seed, 16);
    return SendIoctl(IOCTL_SPOOFER_INSTALL, &input, sizeof(input));
}

bool DriverSpoofer_Uninstall() {
    return SendIoctl(IOCTL_SPOOFER_UNINSTALL, nullptr, 0);
}

DriverSpooferStatus DriverSpoofer_GetStatus() {
    DriverSpooferStatus result = {};
    result.driverConnected = (g_hSpoofer != INVALID_HANDLE_VALUE);
    if (!result.driverConnected) return result;

    SPOOFER_STATUS_OUTPUT status = {};
    DWORD returned = 0;
    if (DeviceIoControl(g_hSpoofer, IOCTL_SPOOFER_STATUS, nullptr, 0,
                        &status, sizeof(status), &returned, nullptr)) {
        result.installed = status.Installed != 0;
        result.layersActive = status.DescriptorPatched + status.DispatchHooked +
                              status.RegistryCallbackActive + status.SmbiosPatched +
                              status.MacPatched + status.TpmPatched + status.GpuSpoofed;
        strncpy_s(result.firstSerial, status.FirstSpoofedSerial, 63);
        strncpy_s(result.spoofedUUID, status.SpoofedUUID, 63);
    }
    return result;
}

bool RunDriverSpoofer() {
    if (!DriverSpoofer_Connect()) {
        printf("  [!] Cannot connect to driver. Make sure it is loaded.\n");
        return false;
    }

    OffsetData offsets;
    if (!ResolveOffsetsForCurrentBuild(offsets)) {
        printf("  [!] Cannot resolve kernel offsets for this Windows build.\n");
        printf("      Build: %lu (major=%lu minor=%lu)\n", offsets.buildNumber, offsets.major, offsets.minor);
        DriverSpoofer_Disconnect();
        return false;
    }

    SPOOFER_OFFSETS_INPUT smbios = {};
    smbios.SmbiosPhysAddrOffset = offsets.smbiosPhysAddr;
    smbios.SmbiosTableLenOffset = offsets.smbiosTableLen;

    SPOOFER_NDIS_OFFSETS_INPUT ndis = {};
    ndis.NdisGlobalFilterListOffset = offsets.ndisFilterList;
    ndis.FilterBlockIfBlockOffset = offsets.filterBlockIfBlock;
    ndis.FilterBlockMiniportOffset = offsets.filterBlockMiniport;
    ndis.FilterBlockNextGlobalFilterOffset = offsets.filterBlockNextGlobal;
    ndis.FilterInstanceNameOffset = offsets.filterInstanceName;
    ndis.IfBlockPhysAddressOffset = offsets.ifBlockPhysAddr;
    ndis.IfBlockPermPhysAddressOffset = offsets.ifBlockPermPhysAddr;
    ndis.MiniportBlockIfBlockOffset = offsets.miniportIfBlock;

    SPOOFER_KNOWN_MACS_INPUT macs = {};
    CollectCurrentMacs(macs);

    SPOOFER_STORPORT_OFFSETS_INPUT storport = {};
    storport.RaidExtSerialStringOff = offsets.raidSerialString;
    storport.RaidExtRichDescPtrOff = offsets.raidRichDescPtr;
    storport.RaidExtInlineSerialOff = offsets.raidInlineSerial;
    storport.RaidExtUnitAdapterOff = offsets.raidUnitAdapter;
    storport.RaidAdapterMiniportOff = offsets.raidAdapterMiniport;
    storport.MiniportPrivateExtOff = offsets.miniportPrivateExt;
    storport.NvmeControllerOff = offsets.nvmeController;
    storport.IdentifyDataPtrOff = offsets.identifyDataPtr;
    storport.IdentifySerialOff = offsets.identifySerial;
    storport.IdentifySerialLen = offsets.identifySerialLen;
    storport.NvmeAdapterSerialCacheOff = offsets.nvmeAdapterSerialCache;
    storport.RaidAdapterSerialCacheOff = offsets.raidAdapterSerialCache;
    storport.RaidUnitRegIfRva = offsets.raidUnitRegIfRva;
    storport.SpDeviceIdPtrOff = offsets.spDeviceIdPtr;
    storport.SpDeviceIdDataOff = offsets.spDeviceIdData;
    storport.SpDeviceIdDataLen = offsets.spDeviceIdDataLen;

    SPOOFER_TPM_OFFSETS_INPUT tpm = {};
    tpm.DispatchCommandRva = offsets.tpmDispatchRva;

    printf("  [*] Sending offsets to driver...\n");
    if (!DriverSpoofer_SendOffsets(smbios, ndis, macs, storport, tpm)) {
        printf("  [!] Failed to send offsets.\n");
        DriverSpoofer_Disconnect();
        return false;
    }
    printf("  [+] Offsets sent.\n");

    UINT8 seed[16] = {};
    GenerateSeed(seed);
    printf("  [*] Installing spoof (17 layers)...\n");
    if (!DriverSpoofer_Install(seed)) {
        printf("  [!] Install failed.\n");
        DriverSpoofer_Disconnect();
        return false;
    }

    DriverSpooferStatus status = DriverSpoofer_GetStatus();
    printf("  [+] Install complete. Layers active: %d\n", status.layersActive);
    printf("      Disk serial: %s\n", status.firstSerial);
    printf("      SMBIOS UUID: %s\n", status.spoofedUUID);

    DriverSpoofer_Disconnect();
    return true;
}

bool UninstallDriverSpoofer() {
    if (!DriverSpoofer_Connect()) return false;
    bool ok = DriverSpoofer_Uninstall();
    DriverSpoofer_Disconnect();
    return ok;
}
