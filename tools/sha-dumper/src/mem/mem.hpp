#pragma once

#include <cstdint>
#include <vector>

class g_mem
{
public:
    struct region_t
    {
        const uint8_t* ptr;
        uint32_t       len;
        uintptr_t      base;
    };

    static uintptr_t find_sig(const char* pat);
    static bool      map_region(uintptr_t addr, uint32_t len, region_t& out);
    static bool      find_next_call(const region_t& r, uint32_t from, uint32_t nth,
                                    uint32_t& out_pos, uintptr_t& out_target);
    static uint64_t  gchandle_rva();

private:
    static bool parse_pattern(const char* pat, std::vector<int>& out);
};
