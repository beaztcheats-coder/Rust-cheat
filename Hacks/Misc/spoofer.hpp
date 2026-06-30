#pragma once

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

// Called from MainThreadImpl before the cheat starts.
// Shows a MessageBox asking whether to spoof.
// If user clicks Yes, runs all HWID spoof operations via the kernel driver.
// Returns true if spoofer ran successfully or was skipped.
bool RunEmbeddedSpoofer();

const SpooferStatus& GetLastSpooferStatus();
