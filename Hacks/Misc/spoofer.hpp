#pragma once
#include <windows.h>

struct SpooferStatus {
    bool attempted = false;
    bool skipped = false;
    bool driverConnected = false;
    bool seedSet = false;
    bool driverMismatch = false;
    bool completed = false;
    int okCount = 0;
    int total = 0;
    int attemptedCount = 0;
};

// Called from MainThreadImpl AFTER driver is initialized (CR3 set).
// hDriver must be a valid handle to \\.\Bfo64 with CR3 already configured.
// Runs HWID spoof IOCTLs via the kernel driver, then user-mode cleanup.
bool RunEmbeddedSpoofer(HANDLE hDriver);

const SpooferStatus& GetLastSpooferStatus();
