#include "cipher.hpp"
#include "../disasm/hde64.h"
#include <windows.h>

static void try_unroll(cipher_t::chain_t& c)
{
    if (c.count < 4 || (c.count % 2) != 0) return;
    int half = c.count / 2;
    for (int i = 0; i < half; i++)
        if (c.ops[i].kind != c.ops[i + half].kind || c.ops[i].val != c.ops[i + half].val)
            return;
    c.count    = half;
    c.unrolled = true;
}

cipher_t::chain_t cipher_t::from_loop(uint8_t* fn, size_t window)
{
    chain_t c{};
    if (IsBadReadPtr(fn, window)) return c;

    size_t loop_start = 0, loop_end = 0;

    for (size_t i = 0; i + 6 <= window && i < 0x120; i++)
    {
        if (fn[i] == 0x41 && fn[i+1] >= 0xB8 && fn[i+1] <= 0xBF &&
            fn[i+2] == 0x02 && fn[i+3] == 0 && fn[i+4] == 0 && fn[i+5] == 0)
        { loop_start = i + 6; break; }

        if (fn[i] >= 0xB8 && fn[i] <= 0xBF &&
            fn[i+1] == 0x02 && fn[i+2] == 0 && fn[i+3] == 0 && fn[i+4] == 0)
        { loop_start = i + 5; break; }
    }

    if (!loop_start) return c;

    for (size_t i = loop_start; i + 3 <= window && i < loop_start + 0x90; i++)
    {
        if ((fn[i] == 0x41 || fn[i] == 0x83) && (fn[i+1] >= 0xE8 && fn[i+1] <= 0xEF) && fn[i+2] == 0x01)
        { loop_end = i; break; }
        if ((fn[i] == 0x41 || fn[i] == 0xFF) && fn[i+1] >= 0xC8 && fn[i+1] <= 0xCF)
        { loop_end = i; break; }
    }

    if (!loop_end || loop_end <= loop_start) return c;

    for (size_t i = loop_start; i < loop_end && c.count < k_max_ops; )
    {
        bool has_rex = fn[i] >= 0x40 && fn[i] <= 0x4F;
        bool rex_w   = has_rex && (fn[i] & 0x08);
        size_t p     = has_rex ? i + 1 : i;

        if (rex_w) { hde64s hs; auto l = hde64_disasm(fn + i, &hs); i += l ? l : 1; continue; }

        auto push_op = [&](uint8_t kind, uint32_t v, size_t advance) {
            c.ops[c.count++] = { kind, v }; i += advance;
        };

        if (fn[p] == 0x05 && p + 5 <= loop_end) { uint32_t v; memcpy(&v, fn+p+1, 4); push_op(op_t::k_add, v, (p-i)+5); continue; }
        if (fn[p] == 0x2D && p + 5 <= loop_end) { uint32_t v; memcpy(&v, fn+p+1, 4); push_op(op_t::k_sub, v, (p-i)+5); continue; }
        if (fn[p] == 0x35 && p + 5 <= loop_end) { uint32_t v; memcpy(&v, fn+p+1, 4); push_op(op_t::k_xor, v, (p-i)+5); continue; }

        if (fn[p] == 0x81 && p + 6 <= loop_end)
        {
            uint8_t mod = (fn[p+1] >> 6) & 3, reg = (fn[p+1] >> 3) & 7;
            if (mod == 3)
            {
                uint32_t v; memcpy(&v, fn+p+2, 4);
                if (reg == 0) push_op(op_t::k_add, v, (p-i)+6);
                else if (reg == 5) push_op(op_t::k_sub, v, (p-i)+6);
                else if (reg == 6) push_op(op_t::k_xor, v, (p-i)+6);
                else i++;
                continue;
            }
        }

        if (fn[p] == 0xC1 && p + 3 <= loop_end)
        {
            uint8_t mod = (fn[p+1] >> 6) & 3, reg = (fn[p+1] >> 3) & 7;
            if (mod == 3 && (reg == 0 || reg == 4) && fn[p+2] < 32)
            { push_op(op_t::k_rol, fn[p+2], (p-i)+3); continue; }
        }

        i++;
    }

    try_unroll(c);
    c.valid = (c.count >= 2);
    return c;
}

cipher_t::chain_t cipher_t::from_inline(uint8_t* fn, size_t window)
{
    chain_t c{};
    if (IsBadReadPtr(fn, window)) return c;

    size_t pos = 0;
    while (pos < window && c.count < k_max_ops)
    {
        hde64s hs;
        auto len = hde64_disasm(fn + pos, &hs);
        if (!len) break;
        if (hs.flags & F_ERROR) { pos += len; continue; }
        if (hs.opcode == 0xC3 || hs.opcode == 0xCC) break;
        if (hs.opcode == 0xE9 || hs.opcode == 0xEB) break;

        if (!hs.rex_w)
        {
            auto p = (uint8_t*)(fn + pos);
            if (hs.opcode == 0x81 && hs.modrm_mod == 3 && (hs.flags & F_IMM32))
            {
                if      (hs.modrm_reg == 6) c.ops[c.count++] = { op_t::k_xor, hs.imm.imm32 };
                else if (hs.modrm_reg == 0) c.ops[c.count++] = { op_t::k_add, hs.imm.imm32 };
                else if (hs.modrm_reg == 5) c.ops[c.count++] = { op_t::k_sub, hs.imm.imm32 };
            }
            if (hs.opcode == 0x35 && (hs.flags & F_IMM32)) c.ops[c.count++] = { op_t::k_xor, hs.imm.imm32 };
            if (hs.opcode == 0x05 && (hs.flags & F_IMM32)) c.ops[c.count++] = { op_t::k_add, hs.imm.imm32 };
            if (hs.opcode == 0x2D && (hs.flags & F_IMM32)) c.ops[c.count++] = { op_t::k_sub, hs.imm.imm32 };
            if (hs.opcode == 0xC1 && hs.modrm_mod == 3 &&
                (hs.modrm_reg == 0 || hs.modrm_reg == 4) && hs.imm.imm8 < 32)
                c.ops[c.count++] = { op_t::k_rol, (uint32_t)hs.imm.imm8 };
        }
        pos += len;
    }

    try_unroll(c);

    if (c.count >= 2)
    {
        bool has_large = false;
        for (int i = 0; i < c.count; i++)
            if (c.ops[i].kind != op_t::k_rol && c.ops[i].val > 0x10000) has_large = true;
        c.valid = has_large;
    }

    return c;
}
