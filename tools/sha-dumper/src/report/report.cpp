#include "report.hpp"

bool g_report::open(const char* path)
{
    m_fp = std::fopen(path, "w");
    return m_fp != nullptr;
}

void g_report::close()
{
    if (m_fp) { std::fclose(m_fp); m_fp = nullptr; }
}

void g_report::header()
{
    if (!m_fp) return;
    std::fprintf(m_fp, "// dumped by sha\n\n");
}

void g_report::section(const char* name)
{
    if (!m_fp) return;
    std::fprintf(m_fp, "[%s]\n", name);
}

void g_report::field(const char* key, uint64_t val)
{
    if (!m_fp) return;
    char buf[32];
    fmt_hex(buf, sizeof(buf), val);
    std::fprintf(m_fp, "%s = %s\n", key, buf);
}

void g_report::field_str(const char* key, const char* val)
{
    if (!m_fp) return;
    std::fprintf(m_fp, "%s = %s\n", key, val);
}

void g_report::blank()
{
    if (m_fp) std::fputc('\n', m_fp);
}

void g_report::cipher_block(const char* tag, uint64_t fn_rva,
                             const cipher_t::chain_t& c, uint64_t hv_off)
{
    if (!m_fp) return;

    char buf[32];
    fmt_hex(buf, sizeof(buf), fn_rva);
    std::fprintf(m_fp, "[cipher_%s]\n", tag);
    std::fprintf(m_fp, "fn_rva    = %s\n", buf);

    if (hv_off)
    {
        fmt_hex(buf, sizeof(buf), hv_off);
        std::fprintf(m_fp, "hv_offset = %s\n", buf);
    }

    std::fprintf(m_fp, "op_count  = %d\n", c.count);
    std::fprintf(m_fp, "unrolled  = %s\n", c.unrolled ? "true" : "false");

    for (int i = 0; i < c.count; i++)
        std::fprintf(m_fp, "op[%d]     = %s 0x%X\n",
            i, cipher_t::op_name(c.ops[i].kind), c.ops[i].val);

    std::fputc('\n', m_fp);
}
