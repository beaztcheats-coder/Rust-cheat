#pragma once

#include "../il2cpp/sdk.hpp"
#include "../il2cpp/util.hpp"

#include <windows.h>
#include <cstdint>
#include <cstring>

#define _self (uintptr_t)this

namespace sys
{
    class str_t
    {
        char pad_[0x10];
    public:
        int     size;
        wchar_t buf[257];

        static str_t* make(const wchar_t* s)
        {
            static str_t*(*alloc)(uint32_t) = nullptr;
            if (!alloc)
            {
                auto k = il2cpp::get_class_by_name("String", "System");
                if (!k) return nullptr;
                auto m = il2cpp::get_method_by_name(k, "FastAllocateString");
                if (!m) return nullptr;
                alloc = (decltype(alloc))m->get_fn_ptr<void*>();
            }
            if (!alloc) return nullptr;

            int len = (int)wcslen(s);
            str_t* out = alloc(len);
            if (!out) return nullptr;
            out->size = len;
            wcscpy(out->buf, s);
            return out;
        }
    };
}

namespace gfx
{
    struct vec3_t { float x, y, z; };
    struct vec4_t { float x, y, z, w; };
    struct quat_t { float x, y, z, w; };
    struct mat4_t { float m[4][4]; };
}

namespace cvar
{
    inline size_t get_off = 0;
    inline size_t set_off = 0;

    class cmd_t
    {
    public:
        uint64_t getter()
        {
            uint64_t fn = *(uint64_t*)(_self + get_off);
            return fn ? *(uint64_t*)(fn + 0x10) : 0;
        }
        uint64_t setter_fn()
        {
            uint64_t act = *(uint64_t*)(_self + set_off);
            return act ? *(uint64_t*)(act + 0x18) : 0;
        }
    };

    inline cmd_t*(*find_cmd)(sys::str_t*) = nullptr;
}

#undef _self

class g_rt
{
public:
    static bool setup()
    {
        il2cpp::init();
        s_base = (uint64_t)GetModuleHandleA("GameAssembly.dll");
        if (!s_base) return false;
        auto dos  = (PIMAGE_DOS_HEADER)s_base;
        auto nt   = (PIMAGE_NT_HEADERS)(s_base + dos->e_lfanew);
        s_imgsize = nt->OptionalHeader.SizeOfImage;
        return true;
    }

    static uint64_t base()    { return s_base; }
    static uint32_t imgsize() { return s_imgsize; }

    static il2cpp::il2cpp_class_t* klass(const char* name, const char* ns = "")
    {
        return il2cpp::get_class_by_name(name, ns);
    }

    static uint64_t klass_rva(il2cpp::il2cpp_class_t* k)
    {
        if (!k) return 0;

        auto dos = (PIMAGE_DOS_HEADER)s_base;
        auto nt  = (PIMAGE_NT_HEADERS)(s_base + dos->e_lfanew);
        auto sb  = (uint8_t*)&nt->OptionalHeader + nt->FileHeader.SizeOfOptionalHeader;

        uint64_t needle = (uint64_t)k;

        auto scan = [&](PIMAGE_SECTION_HEADER sec) -> uint64_t
        {
            uint64_t start = s_base + sec->VirtualAddress;
            uint64_t end   = start + max(sec->SizeOfRawData, sec->Misc.VirtualSize);
            while (start + 8 <= end)
            {
                if (*(uint64_t*)start == needle) return start - s_base;
                start += 8;
            }
            return 0;
        };

        for (uint32_t i = 0; i < nt->FileHeader.NumberOfSections; i++)
        {
            auto sec = (PIMAGE_SECTION_HEADER)(sb + i * sizeof(IMAGE_SECTION_HEADER));
            char nm[9]{}; memcpy(nm, sec->Name, 8);
            if (!strcmp(nm, ".data")) if (auto r = scan(sec)) return r;
        }

        for (uint32_t i = 0; i < nt->FileHeader.NumberOfSections; i++)
        {
            auto sec = (PIMAGE_SECTION_HEADER)(sb + i * sizeof(IMAGE_SECTION_HEADER));
            if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)   continue;
            if (sec->Characteristics & IMAGE_SCN_MEM_DISCARDABLE) continue;
            char nm[9]{}; memcpy(nm, sec->Name, 8);
            if (!strcmp(nm, ".data"))  continue;
            if (!strcmp(nm, ".pdata")) continue;
            if (!strcmp(nm, ".rsrc"))  continue;
            if (!strcmp(nm, ".reloc")) continue;
            if (auto r = scan(sec)) return r;
        }
        return 0;
    }

    static il2cpp::il2cpp_class_t* static_nested(il2cpp::il2cpp_class_t* k)
    {
        if (!k) return nullptr;
        void* it = nullptr;
        while (auto n = k->nested_types(&it))
        {
            if (!n) continue;
            auto m = il2cpp::get_method_by_name(n, ".cctor");
            if (m && n->method_count() <= 2) return n;
        }
        return nullptr;
    }

private:
    inline static uint64_t s_base    = 0;
    inline static uint32_t s_imgsize = 0;
};
