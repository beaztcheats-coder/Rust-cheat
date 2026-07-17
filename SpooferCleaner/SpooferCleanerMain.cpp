#include <windows.h>
#include <cstdio>
#include <conio.h>
#include <cstring>
#include "spoofer.hpp"

static void SetColor(WORD attr) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h && h != INVALID_HANDLE_VALUE) SetConsoleTextAttribute(h, attr);
}

static void PrintBanner() {
    SetColor(0x0B);
    printf("\n");
    printf("  ================================================\n");
    printf("                                                  \n");
    printf("            B B B   E   E   Z   Z   T T T T       \n");
    printf("            B   B  E   E     Z       T           \n");
    printf("            B B B  E   E     Z       T           \n");
    printf("            B   B  E   E     Z       T           \n");
    printf("            B   B  E   E   Z   Z     T           \n");
    printf("                                                  \n");
    printf("          HWID Spoofer  -  Temporary Mode        \n");
    printf("                                                  \n");
    printf("  ================================================\n");
    SetColor(0x07);
    printf("\n  All changes are temporary and revert on reboot.\n");
    printf("  Win11: Full kernel spoof (9 IOCTLs + registry fallback)\n");
    printf("  Win10:  Safe mode (registry-level only, no BSOD risk)\n\n");
}

static bool CheckPasscode() {
    printf("  Enter passcode: ");

    char input[64] = {};
    int idx = 0;

    while (true) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') {
            printf("\n");
            break;
        }
        if (ch == '\b' || ch == 127) {
            if (idx > 0) {
                idx--;
                printf("\b \b");
            }
            continue;
        }
        if (ch == 0 || ch == 0xE0) {
            _getch();
            continue;
        }
        if (idx < (int)sizeof(input) - 1) {
            input[idx++] = (char)ch;
            printf("*");
        }
    }

    input[idx] = 0;
    return lstrcmpA(input, "hwid2024") == 0;
}

static void PrintResults(const SpooferStatus& st) {
    printf("\n");
    printf("  ================================================\n");
    printf("                    RESULTS\n");
    printf("  ================================================\n\n");

    if (!st.driverConnected) {
        SetColor(0x0C);
        printf("  [!] Driver not connected.\n");
        printf("      Make sure the kernel driver is loaded.\n");
        printf("      If HVCI/Memory Integrity is ON, disable it\n");
        printf("      in Windows Security > Device Security.\n");
        SetColor(0x07);
    } else {
        printf("  Driver connected:    YES\n");
        printf("  Seed set:            %s\n", st.seedSet ? "YES" : "FAILED");
        printf("  Spoofs successful:   %d / %d\n", st.okCount, st.total);

        if (st.driverMismatch) {
            SetColor(0x0E);
            printf("\n  [!] Driver mismatch detected on this PC.\n");
            printf("      Spoof opcodes were rejected by the driver.\n");
            SetColor(0x07);
        }

        if (st.okCount == st.total) {
            SetColor(0x0A);
            printf("\n  [+] All spoofs successful!\n");
            SetColor(0x07);
        } else if (st.okCount > 0) {
            SetColor(0x0E);
            printf("\n  [~] Partial spoof completed (%d failed).\n", st.total - st.okCount);
            SetColor(0x07);
        } else {
            SetColor(0x0C);
            printf("\n  [!] All spoofs failed.\n");
            SetColor(0x07);
        }
    }

    printf("\n  User-mode cleanup:   DONE (minimal, Win32 API)\n");
    printf("  Self-cleanup:        DONE (prefetch entry deleted)\n");
    printf("  Trace files:         DELETED\n");
    printf("\n  ================================================\n");
    printf("  All changes revert on reboot.\n");
    printf("  ================================================\n");
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

    if (!CheckPasscode()) {
        SetColor(0x0C);
        printf("\n  [!] Invalid passcode. Exiting.\n");
        SetColor(0x07);
        printf("\n  Press any key to exit...\n");
        _getch();
        return 1;
    }

    SetColor(0x0A);
    printf("  [+] Passcode accepted.\n\n");
    SetColor(0x07);

    printf("  Press any key to start spoofing...\n");
    _getch();
    printf("\n");

    RunSpoofer();

    const SpooferStatus& st = GetLastSpooferStatus();
    PrintResults(st);

    printf("\n  Press any key to exit...\n");
    _getch();
    return 0;
}
