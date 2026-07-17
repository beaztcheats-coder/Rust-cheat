#include <windows.h>
#include <cstdio>
#include <conio.h>
#include <cstring>

#include "spoofer_driver_client.h"
#include "spoofer_registry.h"
#include "cleaner.h"

static void SetColor(WORD attr) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h && h != INVALID_HANDLE_VALUE) SetConsoleTextAttribute(h, attr);
}

static void PrintBanner() {
    SetColor(0x0B);
    printf("\n");
    printf("  ================================================\n");
    printf("             HWID Utility - Master Tool           \n");
    printf("  ================================================\n");
    SetColor(0x07);
    printf("  Features: 17-layer kernel spoof + registry spoof\n");
    printf("            8-phase deep cleaner\n\n");
}

static void PrintMenu() {
    printf("  ================================================\n");
    printf("                    MAIN MENU\n");
    printf("  ================================================\n\n");
    printf("    [1]  Spoof  (Kernel driver + Registry)\n");
    printf("    [2]  Clean  (8-Phase Deep Cleaner)\n");
    printf("    [3]  Spoof + Clean  (Recommended)\n");
    printf("    [4]  Uninstall Spoof  (Restore originals)\n");
    printf("    [5]  Driver Status\n");
    printf("    [6]  Exit\n\n");
    printf("  ================================================\n");
    printf("  Choice [1-6]: ");
    fflush(stdout);
}

static void DoSpoof() {
    SetColor(0x0E);
    printf("\n  === SPOOFING ===\n\n");
    SetColor(0x07);

    // Phase 1: Kernel driver spoof (17 layers)
    printf("  [Phase 1] Kernel driver spoof...\n");
    bool driverOk = RunDriverSpoofer();

    // Phase 2: Registry spoof (supplementary)
    printf("\n  [Phase 2] Registry spoof...\n");
    SpooferStatus regStatus = {};
    RunSpoofer();
    regStatus = GetLastSpooferStatus();
    printf("  Registry: %d/%d spoofs completed\n", regStatus.okCount, regStatus.total);

    SetColor(0x0A);
    printf("\n  [+] Spoof complete!\n");
    printf("      Driver layers: %s\n", driverOk ? "ACTIVE" : "Not available");
    printf("      Registry: %s\n", regStatus.okCount > 0 ? "ACTIVE" : "Failed");
    SetColor(0x07);
    printf("\n  All changes revert on reboot.\n\n");
}

static void DoClean() {
    SetColor(0x0E);
    printf("\n  === CLEANING ===\n\n");
    SetColor(0x07);
    CleanGame();
    SetColor(0x0A);
    printf("\n  [+] Cleaning complete!\n");
    printf("      Restart your PC for full effect.\n\n");
    SetColor(0x07);
}

static void DoSpoofAndClean() {
    printf("\n  Running clean first, then spoof...\n\n");
    DoClean();
    printf("\n  Now spoofing...\n\n");
    DoSpoof();
}

static void DoUninstall() {
    SetColor(0x0E);
    printf("\n  === UNINSTALLING SPOOF ===\n\n");
    SetColor(0x07);

    if (UninstallDriverSpoofer()) {
        SetColor(0x0A);
        printf("  [+] Kernel spoof uninstalled. Originals restored.\n");
    } else {
        SetColor(0x0C);
        printf("  [!] Could not connect to driver (not loaded or already restored).\n");
    }
    SetColor(0x07);
    printf("\n");
}

static void DoStatus() {
    SetColor(0x0E);
    printf("\n  === DRIVER STATUS ===\n\n");
    SetColor(0x07);

    if (DriverSpoofer_Connect()) {
        DriverSpooferStatus st = DriverSpoofer_GetStatus();
        printf("  Driver connected:    YES\n");
        printf("  Installed:           %s\n", st.installed ? "YES" : "NO");
        printf("  Layers active:       %d\n", st.layersActive);
        if (st.firstSerial[0])
            printf("  Disk serial:         %s\n", st.firstSerial);
        if (st.spoofedUUID[0])
            printf("  SMBIOS UUID:         %s\n", st.spoofedUUID);
        DriverSpoofer_Disconnect();
    } else {
        SetColor(0x0C);
        printf("  [!] Driver not connected.\n");
        printf("      Make sure SpooferDriver is loaded before running this tool.\n");
        SetColor(0x07);
    }
    printf("\n");
}

int main() {
    SetConsoleTitleA("System Utility");

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut && hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        GetConsoleMode(hOut, &mode);
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    PrintBanner();

    // Check if driver is available
    bool driverAvailable = DriverSpoofer_Connect();
    if (driverAvailable) {
        SetColor(0x0A);
        printf("  [+] Driver detected.\n\n");
        SetColor(0x07);
        DriverSpoofer_Disconnect();
    } else {
        SetColor(0x0E);
        printf("  [!] Driver not detected — kernel spoof unavailable.\n");
        printf("      Registry spoof and cleaner still work.\n\n");
        SetColor(0x07);
    }

    while (true) {
        PrintMenu();

        int choice = _getch();
        printf("%c\n\n", choice);

        switch (choice) {
            case '1': DoSpoof(); break;
            case '2': DoClean(); break;
            case '3': DoSpoofAndClean(); break;
            case '4': DoUninstall(); break;
            case '5': DoStatus(); break;
            case '6':
            case 27:  // ESC
                printf("  Goodbye.\n");
                return 0;
            default:
                SetColor(0x0C);
                printf("  Invalid choice.\n\n");
                SetColor(0x07);
                break;
        }
    }
    return 0;
}
