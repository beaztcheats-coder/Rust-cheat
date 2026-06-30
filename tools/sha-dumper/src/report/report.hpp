#pragma once

#include "../cipher/cipher.hpp"
#include <cstdio>
#include <string>

class g_report
{
public:
    bool open(const char* path);
    void close();
    bool live() const { return m_fp != nullptr; }

    void header();
    void section(const char* name);
    void field(const char* key, uint64_t val);
    void field_str(const char* key, const char* val);
    void blank();

    void cipher_block(const char* tag, uint64_t fn_rva,
                      const cipher_t::chain_t& c, uint64_t hv_off = 0);

private:
    std::FILE* m_fp = nullptr;

    static void fmt_hex(char* buf, size_t cap, uint64_t v)
    {
        std::snprintf(buf, cap, v >= 0x1000 ? "0x%llX" : "0x%llx",
                      (unsigned long long)v);
    }
};
