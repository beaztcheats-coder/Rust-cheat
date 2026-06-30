#pragma once

#include <windows.h>
#include <cstdint>
#include <vector>
#include "../disasm/hde64.h"

namespace sdk_util
{
    inline bool is_valid_ptr(const void* p)
    {
        auto addr = (uintptr_t)p;
        return addr > 0x10000 && addr < 0x7FFFFFFFFFFull && !IsBadReadPtr(p, 8);
    }

    struct fn_attrs_t
    {
        std::vector<uint64_t> calls;
        std::vector<uint64_t> jmps;
        size_t length = 0;

        bool calls_or_jumps_to(void* target) const
        {
            uint64_t t = (uint64_t)target;
            for (auto c : calls) if (c == t) return true;
            for (auto j : jmps)  if (j == t) return true;
            return false;
        }
    };

    inline fn_attrs_t get_fn_attrs(void* address, size_t limit)
    {
        fn_attrs_t out;
        size_t pos = 0;

        while (pos < limit)
        {
            uint8_t* inst = (uint8_t*)address + pos;
            if (*inst == 0 || *inst == 0xCC) break;

            hde64s hs;
            size_t ilen = hde64_disasm(inst, &hs);
            if (!ilen || (hs.flags & F_ERROR)) break;

            if (hs.opcode == 0xE8)
            {
                int32_t d; memcpy(&d, inst + 1, 4);
                out.calls.push_back((uint64_t)inst + ilen + d);
            }
            else if (hs.opcode == 0xE9)
            {
                int32_t d; memcpy(&d, inst + 1, 4);
                out.jmps.push_back((uint64_t)inst + ilen + d);
            }

            pos += ilen;
        }

        out.length = pos;
        return out;
    }

    // Compatibility aliases for sdk.hpp
    using function_attributes_t = fn_attrs_t;
    inline fn_attrs_t get_function_attributes(void* p, size_t l) { return get_fn_attrs(p, l); }
}

namespace util = sdk_util;
