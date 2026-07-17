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

bool RunSpoofer();
const SpooferStatus& GetLastSpooferStatus();
