#pragma once

#include <cstdint>
#include <cstring>
#include <windows.h>

class cipher_t
{
public:
    struct op_t
    {
        enum : uint8_t { k_add = 0, k_xor, k_rol, k_sub };
        uint8_t  kind;
        uint32_t val;
    };

    static constexpr int k_max_ops = 16;

    struct chain_t
    {
        op_t ops[k_max_ops]{};
        int  count    = 0;
        bool valid    = false;
        bool unrolled = false;
    };

    static chain_t from_loop  (uint8_t* fn, size_t window);
    static chain_t from_inline(uint8_t* fn, size_t window);

    static const char* op_name(uint8_t k)
    {
        switch (k)
        {
        case op_t::k_add: return "ADD";
        case op_t::k_xor: return "XOR";
        case op_t::k_rol: return "ROL";
        case op_t::k_sub: return "SUB";
        default:          return "UNK";
        }
    }

    static bool chains_equal(const chain_t& a, const chain_t& b)
    {
        if (a.count != b.count) return false;
        for (int i = 0; i < a.count; i++)
            if (a.ops[i].kind != b.ops[i].kind || a.ops[i].val != b.ops[i].val)
                return false;
        return true;
    }
};
