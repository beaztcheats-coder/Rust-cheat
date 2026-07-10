#pragma once
#include <vector>
#include <cstdint>
#include <atomic>
#include <Windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <psapi.h>
#include <winternl.h>
#include <mutex>
#include "globals.h"
#include "Logger.hpp"

inline std::atomic<bool> g_process_dead{ false };
inline std::atomic<bool> g_shutting_down{ false };

// Render thread bypasses IOCTL mutex — prevents ESP sticking/menu lag on slow PCs.
// Render thread does ~12 IOCTLs/frame (camera + aimbot), negligible concurrent load.
// All other threads (cache/position/skeleton/misc) still use the mutex.
thread_local bool g_render_thread = false;
<<<<<<< HEAD
=======

>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6


#define IOCTL_DISPATCH CTL_CODE(FILE_DEVICE_UNKNOWN, 0x3C4F22, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define SECURITY_CODE1 0xD93A7A61B8E42C5FULL
#define SECURITY_CODE2 0x4B7E29C2F1A6D3E0ULL

enum class RequestId : ULONG
{
    None = 0,
    GetCr3_1,
    GetCr3_2,
    GetCr3_3,
    GetCr3_VGK,
    GetBase,
    ReadWriteMemory,
    ReadVirtual,
    MouseMovements,
    WindowHide,
    SetCr3Value,
    ReadPhysical,
    GetKernelModuleBase,
    GetModuleBase,
    Spoof_MAC,
    Spoof_DISK,
    Spoof_GPU,
    Spoof_SMBIOS,
    Spoof_BOOTINFO,
    Spoof_FIRMWAREENTRY,
    Spoof_ARP,
    Spoof_SMBIOSNULL,
    Spoof_TPMNULL,
    Spoof_MONITOR,
    Spoof_REGISTRY,
    Set_Spoof_Seed,
};

typedef struct _REQUEST_DATA
{
    ULONG64 SecurityCode1;
    ULONG64 SecurityCode2;
    RequestId Command;

    union
    {
        struct
        {
            ULONG64 processId;
            ULONG64 cr3Value;
            bool sizecheck;
            bool VGKcheck;
        } Cr3Data;

        struct
        {
            ULONG64 base_address;
            ULONG64 found_cr3_value;
            unsigned int process_id;
        } DTBData;

        struct
        {
            ULONG64 Address;
            ULONG64 Buffer;
            ULONG Size;
            ULONGLONG return_size;

            BOOLEAN isWrite;
            BOOLEAN noncachedread;
        } ReadWriteMemoryData;

        struct
        {
            ULONG64 processId;
            ULONG64 baseadressvalue;
        } ProcessBaseData;

        struct
        {
            long MouseX;
            long MouseY;
            unsigned short MouseButtons;
        } MouseData;

        struct
        {
            uintptr_t window;
            unsigned int value;
        } WindowHideData;

        struct
        {
            ULONG64 PhysicalAddress;
            ULONG64 Buffer;
            ULONG Size;
            ULONGLONG return_size;
        } PhysicalReadData;

        struct
        {
            char kernelmodulename[256];
            ULONG64 kernelmodulebase;
        } KernelModuleData;

        struct
        {
            char modulename[256];
            ULONG64 modulebase;
            ULONGLONG ImageSize;
        } ModuleData;

        struct
        {
            ULONG64 BIOS_SEED;
            ULONG64 DISK_SEED;
        } SpoofData;
    };

    bool Result;
} REQUEST_DATA, * PREQUEST_DATA;

class DriverInterface
{
public:
    HANDLE hDriver;
    int processid = 0;
    uintptr_t process_base = 0x0;
    uintptr_t process_cr3;
    uintptr_t m_game_assembly = 0;

    // IOCTL serialization — prevents concurrent driver access from multiple threads
    // Without this, 5 threads call DeviceIoControl() simultaneously, causing race
    // conditions in the kernel driver's IOCTL handler -> BSOD
    std::mutex ioctl_mutex;

    // IOCTL failure tracking — prevents BSOD from stale CR3 after game closes
    std::atomic<int> ioctl_fail_count{ 0 };
    static constexpr int IOCTL_FAIL_LIMIT = 500;        // block reads when fail count exceeds this
    std::atomic<bool> ioctl_blocked{ false };
    std::atomic<uint64_t> ioctl_blocked_time{ 0 };
    std::atomic<uint64_t> ioctl_last_decay_time{ 0 };
    static constexpr ULONGLONG IOCTL_DECAY_INTERVAL_MS = 500;
    static constexpr int IOCTL_DECAY_AMOUNT = 50;        // was 10 — 5x faster recovery
    static constexpr ULONGLONG IOCTL_BLOCK_DURATION_MS = 500; // was 1000 — shorter blocks
    static constexpr uint32_t IOCTL_MAX_SIZE = 0x100000;
    std::atomic<bool> silent_reads{ false };

    DriverInterface()
    {
        hDriver = CreateFileW(L"\\\\.\\Bfo64", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    }

    ~DriverInterface()
    {
        if (hDriver != INVALID_HANDLE_VALUE)
            CloseHandle(hDriver);
    }

    bool IsValid() const
    {
        return hDriver != INVALID_HANDLE_VALUE;
    }

    bool IsProcessAlive()
    {
        if (!processid) return false;
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processid);
        if (!h || h == INVALID_HANDLE_VALUE) return false;
        DWORD exitCode = 0;
        bool alive = GetExitCodeProcess(h, &exitCode) && exitCode == STILL_ACTIVE;
        CloseHandle(h);
        return alive;
    }

    bool SendIoctl(PREQUEST_DATA request)
    {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        if (g_process_dead.load(std::memory_order_relaxed)) return false;
        if (g_shutting_down.load(std::memory_order_relaxed)) return false;
        DWORD bytesReturned = 0;
        // No mutex — driver handles concurrent IOCTLs internally.
        // Mutex was causing 20+ second freezes on slow PCs (40K IOCTLs/cycle serialized).
        // SHA source (Timeless Rust) uses no mutex and never BSODs.
        // Process-death watchdog + ioctl_blocked flag still protect against stale CR3.
        BOOL status = DeviceIoControl(hDriver, IOCTL_DISPATCH, request, sizeof(REQUEST_DATA), request, sizeof(REQUEST_DATA), &bytesReturned, nullptr);
        return status && bytesReturned == sizeof(REQUEST_DATA);
    }

    bool GetCR3_1(uint64_t pid, bool sizecheck, uint64_t& cr3)
    {
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::GetCr3_1;
        req.Cr3Data.processId = pid;
        req.Cr3Data.sizecheck = sizecheck;

        if (SendIoctl(&req) && req.Result)
        {
            cr3 = req.Cr3Data.cr3Value;
            return true;
        }
        return false;
    }

    bool GetCR3_2(uint64_t pid, uint64_t& cr3)
    {
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::GetCr3_2;
        req.Cr3Data.processId = pid;

        if (SendIoctl(&req) && req.Result)
        {
            cr3 = req.Cr3Data.cr3Value;
            return true;
        }
        return false;
    }

    bool GetProcessBase(uint64_t pid, uint64_t& baseAddress)
    {
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::GetBase;
        req.ProcessBaseData.processId = pid;

        if (SendIoctl(&req))
        {
            baseAddress = req.ProcessBaseData.baseadressvalue;
            return true;
        }
        return false;
    }

    void DecayFailCount()
    {
        uint64_t now = GetTickCount64();
        uint64_t last = ioctl_last_decay_time.load(std::memory_order_relaxed);
        if (now - last >= IOCTL_DECAY_INTERVAL_MS) {
            if (ioctl_last_decay_time.compare_exchange_strong(last, now, std::memory_order_relaxed)) {
                int prev = ioctl_fail_count.load(std::memory_order_relaxed);
                int decayed = prev > IOCTL_DECAY_AMOUNT ? prev - IOCTL_DECAY_AMOUNT : 0;
                while (prev > decayed && !ioctl_fail_count.compare_exchange_weak(prev, decayed, std::memory_order_relaxed)) {}
            }
        }
    }

    bool ReadMemory_ACE(PVOID address, PVOID buffer, uint32_t size)
    {
        if (size > IOCTL_MAX_SIZE) return false;
        if (g_process_dead.load(std::memory_order_relaxed)) return false;
        if (g_shutting_down.load(std::memory_order_relaxed)) return false;
        if (ioctl_blocked.load(std::memory_order_relaxed)) {
            uint64_t blocked_at = ioctl_blocked_time.load(std::memory_order_relaxed);
            if (blocked_at && GetTickCount64() - blocked_at > IOCTL_BLOCK_DURATION_MS) {
                ioctl_blocked.store(false, std::memory_order_relaxed);
                ioctl_fail_count.store(0, std::memory_order_relaxed);
            }
            if (ioctl_blocked.load(std::memory_order_relaxed)) return false;
        }
        DecayFailCount();
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::ReadWriteMemory;
        req.ReadWriteMemoryData.Address = (ULONG64)address;
        req.ReadWriteMemoryData.Buffer = (ULONG64)buffer;
        req.ReadWriteMemoryData.Size = size;
        req.ReadWriteMemoryData.isWrite = false;
        req.ReadWriteMemoryData.noncachedread = false;

        if (SendIoctl(&req))
        {
            int prev = ioctl_fail_count.load(std::memory_order_relaxed);
            while (prev > 0 && !ioctl_fail_count.compare_exchange_weak(prev, prev - 1, std::memory_order_relaxed)) {}
            return true;
        }
        else
        {
            if (!silent_reads.load(std::memory_order_relaxed)) {
                int fails = ioctl_fail_count.fetch_add(1, std::memory_order_relaxed) + 1;
                if (fails >= IOCTL_FAIL_LIMIT) {
                    ioctl_blocked_time.store(GetTickCount64(), std::memory_order_relaxed);
                    ioctl_blocked.store(true, std::memory_order_relaxed);
                }
            }
        }
        return false;
    }

    // Minimal read for PE scanning — bypasses all fail-counting/blocking logic
    // Uses noncachedread=false (MmCopyVirtualMemory) — safe for all reads.
    // ReadMemory_ACE/WriteMemory_ACE also use noncachedread=false — MmCopyVirtualMemory
    // handles freed/invalid pages gracefully (returns false instead of BSOD).
    // Previous noncachedread=true (direct physical read) caused BSODs on stale pointers.
    bool ReadMemory_Raw(PVOID address, PVOID buffer, uint32_t size)
    {
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::ReadWriteMemory;
        req.ReadWriteMemoryData.Address = (ULONG64)address;
        req.ReadWriteMemoryData.Buffer = (ULONG64)buffer;
        req.ReadWriteMemoryData.Size = size;
        req.ReadWriteMemoryData.isWrite = false;
<<<<<<< HEAD
        req.ReadWriteMemoryData.noncachedread = true;
=======
        req.ReadWriteMemoryData.noncachedread = false;
>>>>>>> 25ff9416c9ef7560696ffe11ac63cc83810d43e6
        DWORD bytesReturned = 0;
        return DeviceIoControl(hDriver, IOCTL_DISPATCH, &req, sizeof(REQUEST_DATA), &req, sizeof(REQUEST_DATA), &bytesReturned, nullptr)
            && bytesReturned == sizeof(REQUEST_DATA);
    }

    bool WriteMemory_ACE(PVOID address, PVOID buffer, uint32_t size)
    {
        if (size > IOCTL_MAX_SIZE) return false;
        if (g_process_dead.load(std::memory_order_relaxed)) return false;
        if (g_shutting_down.load(std::memory_order_relaxed)) return false;
        if (ioctl_blocked.load(std::memory_order_relaxed)) {
            uint64_t blocked_at = ioctl_blocked_time.load(std::memory_order_relaxed);
            if (blocked_at && GetTickCount64() - blocked_at > IOCTL_BLOCK_DURATION_MS) {
                ioctl_blocked.store(false, std::memory_order_relaxed);
                ioctl_fail_count.store(0, std::memory_order_relaxed);
            }
            if (ioctl_blocked.load(std::memory_order_relaxed)) return false;
        }
        DecayFailCount();
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::ReadWriteMemory;
        req.ReadWriteMemoryData.Address = (ULONG64)address;
        req.ReadWriteMemoryData.Buffer = (ULONG64)buffer;
        req.ReadWriteMemoryData.Size = size;
        req.ReadWriteMemoryData.isWrite = true;
        req.ReadWriteMemoryData.noncachedread = false;

        if (SendIoctl(&req))
        {
            int prev = ioctl_fail_count.load(std::memory_order_relaxed);
            while (prev > 0 && !ioctl_fail_count.compare_exchange_weak(prev, prev - 1, std::memory_order_relaxed)) {}
            return true;
        }
        else
        {
            int fails = ioctl_fail_count.fetch_add(1, std::memory_order_relaxed) + 1;
            if (fails >= IOCTL_FAIL_LIMIT) {
                ioctl_blocked_time.store(GetTickCount64(), std::memory_order_relaxed);
                ioctl_blocked.store(true, std::memory_order_relaxed);
            }
        }
        return false;
    }

    bool MouseMove(int x, int y, unsigned short buttons)
    {
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::MouseMovements;
        req.MouseData.MouseX = x;
        req.MouseData.MouseY = y;
        req.MouseData.MouseButtons = buttons;

        return SendIoctl(&req);
    }

    // Cached process check — avoids CreateToolhelp32Snapshot storm (20-60ms each)
    std::atomic<uint64_t> findproc_last_check_ms{ 0 };
    std::atomic<INT32> findproc_cached_pid{ 0 };
    static constexpr uint64_t FINDPROC_CACHE_MS = 500;

    INT32 find_process(LPCTSTR process_name) {
        // If process is dead, invalidate cache and return 0
        if (g_process_dead.load(std::memory_order_relaxed)) {
            findproc_cached_pid.store(0, std::memory_order_relaxed);
            return 0;
        }
        // Return cached PID if checked recently
        uint64_t now = GetTickCount64();
        uint64_t last = findproc_last_check_ms.load(std::memory_order_relaxed);
        if (now - last < FINDPROC_CACHE_MS) {
            INT32 cached = findproc_cached_pid.load(std::memory_order_relaxed);
            if (cached != 0) return cached;
        }

        PROCESSENTRY32 pt;
        HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        INT32 foundPid = 0;
        if (hsnap != INVALID_HANDLE_VALUE) {
            pt.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hsnap, &pt)) {
                do {
                    if (!lstrcmpi(pt.szExeFile, process_name)) {
                        foundPid = pt.th32ProcessID;
                        break;
                    }
                } while (Process32Next(hsnap, &pt));
            }
            CloseHandle(hsnap);

            // Fallback: also check for Rust.exe if looking for RustClient.exe
            if (!foundPid && lstrcmpi(process_name, L"RustClient.exe") == 0) {
                hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                if (hsnap != INVALID_HANDLE_VALUE) {
                    pt.dwSize = sizeof(PROCESSENTRY32);
                    if (Process32First(hsnap, &pt)) {
                        do {
                            if (!lstrcmpi(pt.szExeFile, L"Rust.exe")) {
                                foundPid = pt.th32ProcessID;
                                break;
                            }
                        } while (Process32Next(hsnap, &pt));
                    }
                    CloseHandle(hsnap);
                }
            }
        }

        findproc_cached_pid.store(foundPid, std::memory_order_relaxed);
        findproc_last_check_ms.store(now, std::memory_order_relaxed);
        return foundPid;
    }

    bool SetCr3Value(uint64_t cr3Value)
    {
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::SetCr3Value;
        req.Cr3Data.cr3Value = cr3Value;

        if (SendIoctl(&req) && req.Result)
        {
            return true;
        }
        return false;
    }

    bool ReadPhysical(uint64_t physicalAddress, void* buffer, uint32_t size)
    {
        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::ReadPhysical;
        req.PhysicalReadData.PhysicalAddress = physicalAddress;
        req.PhysicalReadData.Size = size;
        req.PhysicalReadData.Buffer = reinterpret_cast<ULONG64>(buffer);  // Assuming driver reads into this user-mode buffer address

        if (SendIoctl(&req) && req.Result)
        {
            // If your driver fills buffer at given pointer (pass-through), nothing else to do
            return true;
        }

        return false;
    }


    bool GetKernelModuleBase(const char* moduleName, uint64_t& baseAddress)
    {
        if (!moduleName || strlen(moduleName) >= 256)
        {
            std::cerr << "Invalid module name" << std::endl;
            return false;
        }

        REQUEST_DATA req{};
        req.SecurityCode1 = SECURITY_CODE1;
        req.SecurityCode2 = SECURITY_CODE2;
        req.Command = RequestId::GetKernelModuleBase;

        // Copy module name safely
        strcpy_s(req.KernelModuleData.kernelmodulename, moduleName);

        if (SendIoctl(&req) && req.Result)
        {
            baseAddress = req.KernelModuleData.kernelmodulebase;
            return true;
        }

        return false;
    }

    inline std::uintptr_t get_module(const wchar_t* name, uintptr_t skipAddr = 0) {
        // Original proven approach: OpenProcess + VirtualQueryEx + NtQueryVirtualMemory
        // This is the EXACT method from the stable version that works 100%.
        // Do NOT add CreateToolhelp32Snapshot — it triggers EAC detection which
        // then blocks all subsequent OpenProcess/NtQueryVirtualMemory calls.
        // Method 1: OpenProcess + VirtualQueryEx + NtQueryVirtualMemory (usermode)
        // Falls through to PE scan if OpenProcess fails or module not found
        {
            const auto handle = OpenProcess(PROCESS_QUERY_INFORMATION, 0, processid);
            if (handle) {
                auto current = 0ull;
                auto mbi = MEMORY_BASIC_INFORMATION();
                while (VirtualQueryEx(handle, reinterpret_cast<void*>(current), &mbi, sizeof(MEMORY_BASIC_INFORMATION))) {
                    if (mbi.Type == MEM_MAPPED || mbi.Type == MEM_IMAGE) {
                        const auto buffer = malloc(1024);
                        if (!buffer) goto skip;
                        auto bytes = std::size_t();
                        const static auto ntdll = GetModuleHandle(L"ntdll");
                        const static auto nt_query_virtual_memory_fn =
                            reinterpret_cast<NTSTATUS(__stdcall*)(HANDLE, void*, std::int32_t, void*, std::size_t, std::size_t*)> (
                                GetProcAddress(ntdll, "NtQueryVirtualMemory"));

                        if (!nt_query_virtual_memory_fn ||
                            nt_query_virtual_memory_fn(handle, mbi.BaseAddress, 2, buffer, 1024, &bytes) != 0 ||
                            !wcsstr(static_cast<UNICODE_STRING*>(buffer)->Buffer, name) ||
                            wcsstr(static_cast<UNICODE_STRING*>(buffer)->Buffer, L".mui")) {
                            free(buffer);
                            goto skip;
                        }
                        free(buffer);
                        CloseHandle(handle);
                        return reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
                    }
                skip:
                    current = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
                }
                CloseHandle(handle);
            }
            // Fall through to PE scan — runs regardless of OpenProcess result
        }

        // PE scan fallback — uses ReadMemory_Raw (noncachedread=false)
        // Binary search approach: 1GB coarse scan → 64KB fine scan in hit region
        {
            constexpr uint32_t GA_TIMESTAMP = 0x6A4CF525;
            // When skipAddr is set (searching for UnityPlayer), use lower size threshold
            // UnityPlayer.dll is ~50-100MB, GameAssembly.dll is ~268MB
            uint32_t sizeThreshold = skipAddr ? 0x1000000 : 0x10000000;
            LOG("PE scan: skipAddr=0x%llX sizeThreshold=0x%X", (unsigned long long)skipAddr, sizeThreshold);

            // Diagnostic: test if ReadMemory_Raw works at all
            uint16_t testMZ = 0;
            bool driverWorks = ReadMemory_Raw((PVOID)process_base, &testMZ, 2) && testMZ == 0x5A4D;
            LOG("PE scan: driverWorks=%d process_base=0x%llX", driverWorks ? 1 : 0, (unsigned long long)process_base);

            // Scan the ENTIRE system DLL load region: 0x7FF000000000 - 0x7FFF00000000
            // Step 1: Coarse scan at 1GB intervals to find approximate location (16 reads)
            uintptr_t coarseStart = 0x7FF000000000ULL;
            uintptr_t coarseEnd   = 0x7FFF00000000ULL;
            uintptr_t hitRegion = 0;
            int coarseMZcount = 0;

            for (uintptr_t addr = coarseStart; addr < coarseEnd; addr += 0x40000000ULL) {
                uint16_t dosSig = 0;
                if (!ReadMemory_Raw((PVOID)addr, &dosSig, 2)) continue;
                if (dosSig != 0x5A4D) continue;
                if (skipAddr && addr == skipAddr) continue;
                coarseMZcount++;
                uint32_t peOff = 0;
                ReadMemory_Raw((PVOID)(addr + 0x3C), &peOff, 4);
                if (peOff == 0 || peOff > 0x400) continue;
                uint32_t peSig = 0;
                ReadMemory_Raw((PVOID)(addr + peOff), &peSig, 4);
                if (peSig != 0x00004550) continue;
                uint32_t ts = 0;
                ReadMemory_Raw((PVOID)(addr + peOff + 4), &ts, 4);
                uint32_t sizeOfImage = 0;
                ReadMemory_Raw((PVOID)(addr + peOff + 0x50), &sizeOfImage, 4);
                LOG("PE scan coarse: addr=0x%llX ts=0x%X size=0x%X", (unsigned long long)addr, ts, sizeOfImage);
                if (!skipAddr && ts == GA_TIMESTAMP) return addr;
                // Check size — GameAssembly is ~268MB, UnityPlayer is ~50-100MB
                if (sizeOfImage > sizeThreshold) { hitRegion = addr; break; }
            }
            LOG("PE scan: coarseMZcount=%d hitRegion=0x%llX", coarseMZcount, (unsigned long long)hitRegion);

            // Step 2: Fine scan at 64KB intervals in a ±512MB window around hit
            // If no coarse hit, scan ±8GB from process_base
            uintptr_t fineStart, fineEnd;
            if (hitRegion) {
                fineStart = hitRegion > 0x20000000ULL ? hitRegion - 0x20000000ULL : coarseStart;
                fineEnd = hitRegion + 0x20000000ULL;
            } else {
                fineStart = process_base + 0x80000000ULL;  // +2GB
                fineEnd = process_base + 0x380000000ULL;    // +14GB
            }
            LOG("PE scan: fine scan 0x%llX-0x%llX", (unsigned long long)fineStart, (unsigned long long)fineEnd);

            for (uintptr_t addr = fineStart; addr < fineEnd; addr += 0x10000ULL) {
                uint16_t dosSig = 0;
                if (!ReadMemory_Raw((PVOID)addr, &dosSig, 2)) continue;
                if (dosSig != 0x5A4D) continue;
                if (skipAddr && addr == skipAddr) continue;
                uint32_t peOff = 0;
                ReadMemory_Raw((PVOID)(addr + 0x3C), &peOff, 4);
                if (peOff == 0 || peOff > 0x400) continue;
                uint32_t peSig = 0;
                ReadMemory_Raw((PVOID)(addr + peOff), &peSig, 4);
                if (peSig != 0x00004550) continue;
                uint32_t ts = 0;
                ReadMemory_Raw((PVOID)(addr + peOff + 4), &ts, 4);
                if (!skipAddr && ts == GA_TIMESTAMP) { LOG("PE scan: fine found by timestamp at 0x%llX", (unsigned long long)addr); return addr; }
                uint32_t sizeOfImage = 0;
                ReadMemory_Raw((PVOID)(addr + peOff + 0x50), &sizeOfImage, 4);
                if (sizeOfImage > sizeThreshold) { LOG("PE scan: fine found by size at 0x%llX (size=0x%X)", (unsigned long long)addr, sizeOfImage); return addr; }
            }
            LOG("PE scan: fine scan failed");

            // Step 3: If still not found and driver works, scan wider from process_base
            if (driverWorks) {
                uintptr_t wideStart = process_base;
                uintptr_t wideEnd = process_base + 0x800000000ULL; // +32GB
                LOG("PE scan: wide scan 0x%llX-0x%llX", (unsigned long long)wideStart, (unsigned long long)wideEnd);
                for (uintptr_t addr = wideStart; addr < wideEnd; addr += 0x10000ULL) {
                    uint16_t dosSig = 0;
                    if (!ReadMemory_Raw((PVOID)addr, &dosSig, 2)) continue;
                    if (dosSig != 0x5A4D) continue;
                    if (skipAddr && addr == skipAddr) continue;
                    uint32_t peOff = 0;
                    ReadMemory_Raw((PVOID)(addr + 0x3C), &peOff, 4);
                    if (peOff == 0 || peOff > 0x400) continue;
                    uint32_t peSig = 0;
                    ReadMemory_Raw((PVOID)(addr + peOff), &peSig, 4);
                    if (peSig != 0x00004550) continue;
                    uint32_t ts = 0;
                    ReadMemory_Raw((PVOID)(addr + peOff + 4), &ts, 4);
                    if (!skipAddr && ts == GA_TIMESTAMP) { LOG("PE scan: wide found by timestamp at 0x%llX", (unsigned long long)addr); return addr; }
                    uint32_t sizeOfImage = 0;
                    ReadMemory_Raw((PVOID)(addr + peOff + 0x50), &sizeOfImage, 4);
                    if (sizeOfImage > sizeThreshold) { LOG("PE scan: wide found by size at 0x%llX (size=0x%X)", (unsigned long long)addr, sizeOfImage); return addr; }
                }
                LOG("PE scan: wide scan failed");

                // Step 4: Finer scan at 4KB intervals from process_base to +4GB
                // Catches cases where GameAssembly.dll is loaded at unusual alignment
                uintptr_t finerStart = process_base;
                uintptr_t finerEnd = process_base + 0x100000000ULL; // +4GB
                LOG("PE scan: finer scan 0x%llX-0x%llX", (unsigned long long)finerStart, (unsigned long long)finerEnd);
                for (uintptr_t addr = finerStart; addr < finerEnd; addr += 0x1000ULL) {
                    uint16_t dosSig = 0;
                    if (!ReadMemory_Raw((PVOID)addr, &dosSig, 2)) continue;
                    if (dosSig != 0x5A4D) continue;
                    if (skipAddr && addr == skipAddr) continue;
                    uint32_t peOff = 0;
                    ReadMemory_Raw((PVOID)(addr + 0x3C), &peOff, 4);
                    if (peOff == 0 || peOff > 0x400) continue;
                    uint32_t peSig = 0;
                    ReadMemory_Raw((PVOID)(addr + peOff), &peSig, 4);
                    if (peSig != 0x00004550) continue;
                    uint32_t sizeOfImage = 0;
                    ReadMemory_Raw((PVOID)(addr + peOff + 0x50), &sizeOfImage, 4);
                    if (sizeOfImage > sizeThreshold) { LOG("PE scan: finer found by size at 0x%llX (size=0x%X)", (unsigned long long)addr, sizeOfImage); return addr; }
                }
                LOG("PE scan: finer scan failed — module NOT FOUND");
            } else {
                LOG("PE scan: driverWorks=false, skipping wide/finer scans");
            }
        }

        LOG("PE scan: all steps exhausted, returning 0");
        return 0ull;
    }

};  inline DriverInterface* Drv = nullptr;

inline bool is_valid(const uint64_t address)
{
    if (address == 0 ||
        address == 0xCCCCCCCCCCCCCCCC ||
        address == 0xFFFFFFFFFFFFFFFF)
        return false;

    if (address < 0x10000 || address > 0x7FFFFFFFFFFFFFFF)
        return false;

    return true;
}

inline bool g_UseInternalReads = false;

__declspec(noinline) inline bool ReadMemory_Internal(PVOID address, PVOID buffer, SIZE_T size) {
    __try {
        memcpy(buffer, address, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

inline bool ReadBatch_Internal(PVOID address, PVOID buffer, SIZE_T size) {
    return ReadMemory_Internal(address, buffer, size);
}

template <typename T>
T read(uint64_t address) {
    T buffer{ };
    if (is_valid(address)) {
        if (g_UseInternalReads && ReadMemory_Internal((PVOID)address, &buffer, sizeof(T)))
            return buffer;
        if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed))
            Drv->ReadMemory_ACE((PVOID)address, &buffer, sizeof(T));
    }
    return buffer;
}

// example write
template <typename T>
void write(uint64_t address, T buffer) {
    if (is_valid(address))
        if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed))
            Drv->WriteMemory_ACE((PVOID)address, &buffer, sizeof(T));
}

template <typename T>
bool write_checked(uint64_t address, const T& buffer) {
    if (!Drv || !is_valid(address))
        return false;
    T tmp = buffer;
    return Drv->WriteMemory_ACE((PVOID)address, &tmp, sizeof(T));
}

inline std::string read_wstr(uintptr_t address) {
    if (!is_valid(address)) return "";
    wchar_t buf[256] = {};
    if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(address), buf, 255 * sizeof(wchar_t))))
        if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(address), buf, 255 * sizeof(wchar_t));
    buf[255] = 0;
    std::string out;
    for (int i = 0; i < 255 && buf[i]; i++) {
        if (buf[i] >= 32 && buf[i] < 127) out += (char)buf[i];
    }
    return out;
}

inline std::string read_il2cpp_string(uintptr_t stringObj) {
    if (!is_valid(stringObj)) return "";
    uint32_t len = read<uint32_t>(stringObj + 0x10);
    if (len == 0 || len > 255) return "";
    wchar_t buf[256] = {};
    if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(stringObj + 0x14), buf, len * sizeof(wchar_t))))
        if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(stringObj + 0x14), buf, len * sizeof(wchar_t));
    buf[len] = 0;
    std::string out;
    out.reserve(len);
    for (uint32_t i = 0; i < len && buf[i]; i++) {
        if (buf[i] >= 32 && buf[i] < 127) out += (char)buf[i];
    }
    return out;
}

inline std::string readstring(uint64_t Address) {
    if (!is_valid(Address)) return "";
    std::unique_ptr<char[]> buffer(new char[64]);
    if (!(g_UseInternalReads && ReadMemory_Internal((PVOID)Address, buffer.get(), 64)))
        if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE((PVOID)Address, buffer.get(), 64);
    buffer[63] = '\0';
    return buffer.get();
}

template <typename T>
inline T read_chain(std::uint64_t address, std::vector<std::uint64_t> chain) {
    if (chain.empty()) return T{};
    uint64_t cur_read = address;

    for (size_t r = 0; r < chain.size() - 1; ++r) {
        cur_read = read<std::uint64_t>(cur_read + chain[r]);
        if (!is_valid(cur_read)) return T{};
    }

    return read<T>(cur_read + chain[chain.size() - 1]);
}

namespace mem {


    template <typename T>
    T read(uint64_t address) {
        T buffer{ };
        if (is_valid(address)) {
            if (g_UseInternalReads && ReadMemory_Internal((PVOID)address, &buffer, sizeof(T)))
                return buffer;
            if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed)) Drv->ReadMemory_ACE((PVOID)address, &buffer, sizeof(T));
        }
        return buffer;
    }
};