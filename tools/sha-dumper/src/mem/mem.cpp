#include "mem.hpp"
#include "../glue/glue.hpp"
#include "../disasm/hde64.h"

#include <windows.h>
#include <cstring>
#include <vector>

bool g_mem::parse_pattern(const char* pat, std::vector<int>& out)
{
    out.clear();
    for (const char* p = pat; *p; )
    {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        if (*p == '?')
        {
            out.push_back(-1);
            p++;
            if (*p == '?') p++;
            continue;
        }

        int v = 0, d = 0;
        while (d < 2 && *p && *p != ' ')
        {
            char c = *p;
            int  x = (c >= '0' && c <= '9') ? c - '0'
                   : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                   : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
            if (x < 0) break;
            v = v * 16 + x;
            d++; p++;
        }
        out.push_back(v);
    }
    return !out.empty();
}

uintptr_t g_mem::find_sig(const char* pat)
{
    auto base = (const uint8_t*)g_rt::base();
    auto size = (size_t)g_rt::imgsize();

    std::vector<int> bytes;
    if (!parse_pattern(pat, bytes) || bytes.size() > size)
        return 0;

    size_t lim = size - bytes.size();
    for (size_t i = 0; i <= lim; i++)
    {
        bool ok = true;
        for (size_t j = 0; j < bytes.size(); j++)
        {
            if (bytes[j] >= 0 && base[i + j] != (uint8_t)bytes[j])
            { ok = false; break; }
        }
        if (ok) return g_rt::base() + i;
    }
    return 0;
}

bool g_mem::map_region(uintptr_t addr, uint32_t len, region_t& out)
{
    if (!addr || IsBadReadPtr((void*)addr, len)) return false;
    out.ptr  = (const uint8_t*)addr;
    out.len  = len;
    out.base = addr;
    return true;
}

bool g_mem::find_next_call(
    const region_t& r, uint32_t from, uint32_t nth,
    uint32_t& out_pos, uintptr_t& out_target)
{
    int hit = 0;
    for (uint32_t i = from; i + 5 <= r.len; i++)
    {
        if (r.ptr[i] != 0xE8) continue;
        if (++hit < nth) continue;
        int32_t rel; memcpy(&rel, &r.ptr[i + 1], 4);
        out_pos    = i;
        out_target = r.base + i + 5 + rel;
        return true;
    }
    return false;
}

static uint8_t* thunk_follow(uint8_t* fn)
{
    for (int h = 0; h < 3 && fn && !IsBadReadPtr(fn, 0x20); h++)
    {
        if (fn[0] == 0xFF && fn[1] == 0x25)
        {
            int32_t d; memcpy(&d, fn + 2, 4);
            auto slot = (uintptr_t)fn + 6 + d;
            if (IsBadReadPtr((void*)slot, 8)) return fn;
            fn = *(uint8_t**)slot;
            continue;
        }
        if (fn[0] == 0xE9)
        {
            int32_t d; memcpy(&d, fn + 1, 4);
            fn = fn + 5 + d;
            continue;
        }
        break;
    }
    return fn;
}

static bool is_writable_rva(uint64_t rva)
{
    uint64_t base = g_rt::base();
    auto dos = (PIMAGE_DOS_HEADER)base;
    auto nt  = (PIMAGE_NT_HEADERS)(base + dos->e_lfanew);
    auto sb  = (uint8_t*)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader;

    for (uint32_t i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        auto sec = (PIMAGE_SECTION_HEADER)(sb + i * sizeof(IMAGE_SECTION_HEADER));
        if (rva >= sec->VirtualAddress && rva < sec->VirtualAddress + sec->Misc.VirtualSize)
            return (sec->Characteristics & IMAGE_SCN_MEM_WRITE) != 0;
    }
    return false;
}

static uint64_t first_writable_lea(uint8_t* fn, size_t window)
{
    uint64_t game_base = g_rt::base();
    uint8_t* end = fn + window;
    uint8_t* p   = fn;

    while (p < end)
    {
        hde64s hs;
        auto len = hde64_disasm(p, &hs);
        if (!len) { p++; continue; }
        if (hs.flags & F_ERROR) { p += len; continue; }

        if ((p[0] == 0x48 || p[0] == 0x4C) && len >= 7)
        {
            uint8_t op = p[1], modrm = p[2];
            if ((op == 0x8D || op == 0x8B) && (modrm & 0xC7) == 0x05)
            {
                int32_t disp; memcpy(&disp, p + 3, 4);
                auto tgt = (uintptr_t)p + 7 + disp;
                if (tgt >= game_base)
                {
                    auto rva = tgt - game_base;
                    if (is_writable_rva(rva)) return rva;
                }
            }
        }
        p += len;
    }
    return 0;
}

uint64_t g_mem::gchandle_rva()
{
    uint64_t game_base = g_rt::base();
    auto     mod       = GetModuleHandleA("GameAssembly.dll");

    static const char* exports[] = {
        "il2cpp_gchandle_new",
        "il2cpp_gchandle_free",
        "il2cpp_gchandle_get_target",
        "il2cpp_gchandle_new_weakref",
    };

    for (auto name : exports)
    {
        auto fn = (uint8_t*)GetProcAddress(mod, name);
        if (!fn) continue;
        fn = thunk_follow(fn);
        if (!fn || IsBadReadPtr(fn, 0x400)) continue;
        if ((uintptr_t)fn < game_base) continue;

        uint8_t* targets[32]{};
        int tc = 0;
        targets[tc++] = fn;

        for (int level = 0; level < 2; level++)
        {
            int prev_count = tc;
            for (int t = 0; t < prev_count && tc < 32; t++)
            {
                auto base_p = targets[t];
                if (IsBadReadPtr(base_p, 0x200)) continue;

                for (size_t i = 0; i < 0x200; )
                {
                    hde64s hs;
                    auto len = hde64_disasm(base_p + i, &hs);
                    if (!len || (hs.flags & F_ERROR)) break;
                    if (hs.opcode == 0xC3 || hs.opcode == 0xCC) break;

                    if (base_p[i] == 0xE8 && len >= 5 && tc < 32)
                    {
                        int32_t d; memcpy(&d, base_p + i + 1, 4);
                        auto tgt = thunk_follow(base_p + i + 5 + d);
                        if (tgt && !IsBadReadPtr(tgt, 0x100) && (uintptr_t)tgt >= game_base)
                        {
                            bool dup = false;
                            for (int k = 0; k < tc; k++) if (targets[k] == tgt) { dup = true; break; }
                            if (!dup) targets[tc++] = tgt;
                        }
                    }
                    i += len;
                }
            }
        }

        for (int t = 0; t < tc; t++)
        {
            auto sf = targets[t];
            if (IsBadReadPtr(sf, 0x200)) continue;

            bool has_and7 = false;
            for (size_t i = 0; i < 0x200; )
            {
                hde64s hs;
                auto len = hde64_disasm(sf + i, &hs);
                if (!len || (hs.flags & F_ERROR)) break;
                if (hs.opcode == 0xC3 || hs.opcode == 0xCC) break;

                if (sf[i] == 0x83 && len >= 3 && (sf[i+1] & 0xF8) == 0xE0 && sf[i+2] == 0x07)
                { has_and7 = true; break; }

                i += len;
            }

            if (!has_and7) continue;
            if (auto rva = first_writable_lea(sf, 0x300)) return rva;
        }
    }

    for (auto name : exports)
    {
        auto fn = (uint8_t*)GetProcAddress(mod, name);
        if (!fn) continue;
        fn = thunk_follow(fn);
        if (!fn || IsBadReadPtr(fn, 0x400)) continue;
        if ((uintptr_t)fn < game_base) continue;
        if (auto rva = first_writable_lea(fn, 0x400)) return rva;
    }

    return 0;
}
