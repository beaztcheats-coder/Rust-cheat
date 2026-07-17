#include "../include/spoofer.h"
#include <usbioctl.h>

// ============================================================================
// usb_spoof.c — USB Hub IRP Hook
//
// Hooks IRP_MJ_DEVICE_CONTROL on all USB Hub drivers to intercept:
//   - IOCTL_USB_GET_NODE_CONNECTION_INFORMATION(_EX)
//   - IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION (serial string)
// Spoofs serial strings to match registry-level spoofing.
// Uses IoGetDeviceInterfaces(GUID_USB_HUB) — no patterns, no PDB.
// ============================================================================

// ── Globals ─────────────────────────────────────────────────────────────────

#define MAX_USB_DRIVERS 0x40
#define USB_STRING_DESCRIPTOR_TYPE 0x03

typedef struct _USB_DRIVER {
    PDRIVER_OBJECT   DriverObject;
    PDRIVER_DISPATCH Original;
} USB_DRIVER;

static struct {
    ULONG      Length;
    USB_DRIVER Drivers[MAX_USB_DRIVERS];
} g_UsbHooks = { 0 };

static UINT8 g_UsbSeed[SPOOFER_SEED_SIZE] = { 0 };

/* Spoof a wide-char serial in-place — MUST match SpoofUsbSerialName in registry_intercept.c */
static void SpoofUsbWideSerial(PWCHAR name, ULONG charCount)
{
    ULONG seed = 0x811c9dc5;
    ULONG i;

    /* FNV-1a hash of original serial chars */
    for (i = 0; i < charCount && name[i] != 0; i++) {
        seed ^= (ULONG)name[i];
        seed *= 0x01000193;
    }
    /* Mix in spoofer seed */
    for (i = 0; i < SPOOFER_SEED_SIZE && i < 16; i++) {
        seed ^= (ULONG)g_UsbSeed[i] << ((i % 4) * 8);
        seed *= 0x01000193;
    }

    /* Replace each char deterministically, preserving char class */
    for (i = 0; i < charCount && name[i] != 0; i++) {
        WCHAR c = name[i];
        seed = (seed * 1103515245 + 12345 + i) & 0x7FFFFFFF;
        if (c >= L'0' && c <= L'9') {
            name[i] = L'0' + (WCHAR)(seed % 10);
        }
        else if (c >= L'A' && c <= L'Z') {
            name[i] = L'A' + (WCHAR)(seed % 26);
        }
        else if (c >= L'a' && c <= L'z') {
            name[i] = L'a' + (WCHAR)(seed % 26);
        }
    }
}

// ── Completion Routine ──────────────────────────────────────────────────────

static NTSTATUS UsbHubCompletion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    if (!NT_SUCCESS(Irp->IoStatus.Status))
        goto done;

    PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(Irp);
    if (!ioc || !Irp->AssociatedIrp.SystemBuffer)
        goto done;

    switch (ioc->Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION:
    {
        PUSB_NODE_CONNECTION_INFORMATION pInfo =
            (PUSB_NODE_CONNECTION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

        if (ioc->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(*pInfo)) {
            /* Leave iSerialNumber as-is — the actual serial string is spoofed
             * via IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION below.
             * Only give a non-zero index to devices that originally had none. */
            if (pInfo->DeviceDescriptor.iSerialNumber == 0) {
                ULONG s = 0x811c9dc5;
                for (int i = 0; i < SPOOFER_SEED_SIZE; i++)
                    s ^= ((ULONG)g_UsbSeed[i]) << ((i % 4) * 8);
                s ^= pInfo->DeviceDescriptor.idVendor;
                s ^= (ULONG)pInfo->DeviceDescriptor.idProduct << 16;
                s *= 0x01000193;
                pInfo->DeviceDescriptor.iSerialNumber = (UCHAR)((s % 254) + 1);
            }
        }
        break;
    }
    case IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX:
    {
        PUSB_NODE_CONNECTION_INFORMATION_EX pInfoEx =
            (PUSB_NODE_CONNECTION_INFORMATION_EX)Irp->AssociatedIrp.SystemBuffer;

        if (ioc->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(*pInfoEx)) {
            if (pInfoEx->DeviceDescriptor.iSerialNumber == 0) {
                ULONG s = 0x811c9dc5;
                for (int i = 0; i < SPOOFER_SEED_SIZE; i++)
                    s ^= ((ULONG)g_UsbSeed[i]) << ((i % 4) * 8);
                s ^= pInfoEx->DeviceDescriptor.idVendor;
                s ^= (ULONG)pInfoEx->DeviceDescriptor.idProduct << 16;
                s *= 0x01000193;
                pInfoEx->DeviceDescriptor.iSerialNumber = (UCHAR)((s % 254) + 1);
            }
        }
        break;
    }
    case IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION:
    {
        /* This IOCTL returns the actual USB string descriptor (serial, product, manufacturer).
         * The input/output buffer is USB_DESCRIPTOR_REQUEST:
         *   - SetupPacket.wValue high byte = descriptor type
         *   - SetupPacket.wValue low byte  = descriptor index
         *   - Data[] follows the header with the actual descriptor */
        PUCHAR buf = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
        ULONG outLen = (ULONG)Irp->IoStatus.Information;

        if (!buf || outLen <= sizeof(USB_DESCRIPTOR_REQUEST))
            break;

        PUSB_DESCRIPTOR_REQUEST req = (PUSB_DESCRIPTOR_REQUEST)buf;
        UCHAR descType  = (UCHAR)(req->SetupPacket.wValue >> 8);
        UCHAR descIndex = (UCHAR)(req->SetupPacket.wValue & 0xFF);

        /* Only spoof string descriptors with index > 0 (index 0 = language IDs) */
        if (descType != USB_STRING_DESCRIPTOR_TYPE || descIndex == 0)
            break;

        /* The string descriptor sits right after the USB_DESCRIPTOR_REQUEST header */
        PUSB_STRING_DESCRIPTOR strDesc = (PUSB_STRING_DESCRIPTOR)req->Data;
        ULONG dataLen = outLen - sizeof(USB_DESCRIPTOR_REQUEST);

        if (dataLen < 4 || strDesc->bLength < 4 || strDesc->bDescriptorType != USB_STRING_DESCRIPTOR_TYPE)
            break;

        /* USB string descriptor: bLength, bDescriptorType, then wide chars */
        ULONG strBytes = strDesc->bLength - 2;  /* subtract header */
        ULONG charCount = strBytes / sizeof(WCHAR);

        if (charCount >= 4 && charCount <= 128) {
            /* Spoof the string using same algorithm as registry hook */
            SpoofUsbWideSerial(strDesc->bString, charCount);
        }
        break;
    }
    }

done:
    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    return STATUS_CONTINUE_COMPLETION;
}

// ── Dispatch Hook ───────────────────────────────────────────────────────────

static NTSTATUS UsbHubDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PDRIVER_DISPATCH orig = NULL;

    for (ULONG i = 0; i < g_UsbHooks.Length; i++) {
        if (g_UsbHooks.Drivers[i].DriverObject == DeviceObject->DriverObject) {
            orig = g_UsbHooks.Drivers[i].Original;
            break;
        }
    }

    if (!orig)
        return STATUS_INVALID_DEVICE_REQUEST;

    PIO_STACK_LOCATION ioc = IoGetCurrentIrpStackLocation(Irp);
    if (!ioc)
        return orig(DeviceObject, Irp);

    ULONG code = ioc->Parameters.DeviceIoControl.IoControlCode;

    if (code == IOCTL_USB_GET_NODE_CONNECTION_INFORMATION ||
        code == IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX ||
        code == IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION)
    {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, UsbHubCompletion, NULL, TRUE, TRUE, TRUE);
        return orig(DeviceObject, Irp);
    }

    return orig(DeviceObject, Irp);
}

// ── Install / Uninstall ─────────────────────────────────────────────────────

NTSTATUS InstallUsbHooks(const UINT8* seed)
{
    RtlCopyMemory(g_UsbSeed, seed, SPOOFER_SEED_SIZE);

    static const GUID GUID_USB_HUB =
        { 0xf18a0e88, 0xc30c, 0x11d0, { 0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8 } };

    PWCHAR pDeviceNames = NULL;
    NTSTATUS status = ((fn_IoGetDeviceInterfaces)ApiResolveExport(HASH_IOGETDEVICEINTERFACES))(&GUID_USB_HUB, NULL,
        0, &pDeviceNames);

    if (!NT_SUCCESS(status) || !pDeviceNames) {
        SPOOF_DBG("[Spoofer] USB: IoGetDeviceInterfaces failed (0x%08X)\n", status);
        return status;
    }

    PWCHAR cur = pDeviceNames;
    while (*cur)
    {
        UNICODE_STRING uName;
        ((fn_RtlInitUnicodeString)ApiResolveExport(HASH_RTLINITUNICODESTRING))(&uName, cur);

        PFILE_OBJECT fileObject = NULL;
        PDEVICE_OBJECT deviceObject = NULL;

        status = ((fn_IoGetDeviceObjectPointer)ApiResolveExport(HASH_IOGETDEVICEOBJECTPOINTER))(&uName, FILE_READ_DATA, &fileObject, &deviceObject);
        if (NT_SUCCESS(status) && deviceObject && deviceObject->DriverObject)
        {
            PDRIVER_OBJECT drv = deviceObject->DriverObject;

            // Check if already hooked
            BOOLEAN found = FALSE;
            for (ULONG i = 0; i < g_UsbHooks.Length; i++) {
                if (g_UsbHooks.Drivers[i].DriverObject == drv) {
                    found = TRUE;
                    break;
                }
            }

            if (!found && g_UsbHooks.Length < MAX_USB_DRIVERS) {
                PDRIVER_DISPATCH old = drv->MajorFunction[IRP_MJ_DEVICE_CONTROL];
                drv->MajorFunction[IRP_MJ_DEVICE_CONTROL] = UsbHubDispatch;

                g_UsbHooks.Drivers[g_UsbHooks.Length].DriverObject = drv;
                g_UsbHooks.Drivers[g_UsbHooks.Length].Original = old;
                g_UsbHooks.Length++;
            }

            {
        fn_ObfDereferenceObject _pDeref = (fn_ObfDereferenceObject)ApiResolveExport(HASH_OBFDEREFERENCEOBJECT);
        if (_pDeref) _pDeref(fileObject);
    }
        }

        cur += (((fn_wcslen)ApiResolveExport(HASH_WCSLEN))(cur) + 1);
    }

    ((fn_ExFreePoolWithTag)ApiResolveExport(HASH_EXFREEPOOLWITHTAG))(pDeviceNames, 0);
    SPOOF_DBG("[Spoofer] USB: Hooked %lu hub driver(s)\n", g_UsbHooks.Length);
    return STATUS_SUCCESS;
}

void UninstallUsbHooks(void)
{
    for (ULONG i = 0; i < g_UsbHooks.Length; i++) {
        PDRIVER_OBJECT drv = g_UsbHooks.Drivers[i].DriverObject;
        if (drv && g_UsbHooks.Drivers[i].Original) {
            drv->MajorFunction[IRP_MJ_DEVICE_CONTROL] = g_UsbHooks.Drivers[i].Original;
        }
    }

    SPOOF_DBG("[Spoofer] USB: Unhooked %lu driver(s)\n", g_UsbHooks.Length);
    RtlZeroMemory(&g_UsbHooks, sizeof(g_UsbHooks));
}
