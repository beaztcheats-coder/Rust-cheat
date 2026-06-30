#include "mat.hpp"
#include "../glue/glue.hpp"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using find_all_fn = void*(*)(void*);
using get_name_fn = sys::str_t*(*)(void*);

static find_all_fn s_find_all = nullptr;
static get_name_fn s_get_name = nullptr;

static bool init_fns()
{
    if (!s_find_all)
    {
        auto res = il2cpp::get_class_by_name("Resources", "UnityEngine");
        if (res)
        {
            auto m = il2cpp::get_method_by_name(res, "FindObjectsOfTypeAll", 1);
            if (m) s_find_all = (find_all_fn)m->get_fn_ptr<void*>();
        }
    }

    if (!s_find_all)
        s_find_all = (find_all_fn)il2cpp::resolve_icall(
            "UnityEngine.Resources::FindObjectsOfTypeAll(System.Type)");

    if (!s_get_name)
    {
        auto obj_k = il2cpp::get_class_by_name("Object", "UnityEngine");
        if (obj_k)
        {
            auto m = il2cpp::get_method_by_name(obj_k, "get_name", 0);
            if (m) s_get_name = (get_name_fn)m->get_fn_ptr<void*>();
        }
    }

    if (!s_get_name)
        s_get_name = (get_name_fn)il2cpp::resolve_icall("UnityEngine.Object::get_name()");

    return s_find_all && s_get_name;
}

static std::string sanitize(const wchar_t* raw, int len)
{
    if (!raw || len <= 0) return {};
    std::string out;
    out.reserve(len);

    auto valid_first = [](wchar_t c) {
        return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c == L'_';
    };
    auto valid_rest = [](wchar_t c) {
        return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
               (c >= L'0' && c <= L'9') || c == L'_';
    };

    out += valid_first(raw[0]) ? (char)raw[0] : '_';
    for (int i = 1; i < len; i++)
        out += valid_rest(raw[i]) ? (char)raw[i] : '_';

    return out;
}

bool g_mat::append(const char* path)
{
    if (!init_fns()) return false;

    auto mat_k = il2cpp::get_class_by_name("Material", "UnityEngine");
    if (!mat_k) return false;

    auto mat_type = mat_k->type();
    if (!mat_type) return false;

    auto type_obj = il2cpp::type_get_object(mat_type);
    if (!type_obj) return false;

    void* arr = nullptr;
    if (!IsBadReadPtr((void*)s_find_all, 8)) arr = s_find_all(type_obj);
    if (!arr || IsBadReadPtr(arr, 0x24)) return false;

    int32_t  count    = *(int32_t*)((uint8_t*)arr + 0x18);
    uint64_t* elems   = (uint64_t*)((uint8_t*)arr + 0x20);
    if (count <= 0 || count > 500000) return false;

    struct entry_t { std::string name; int32_t id; };
    std::vector<entry_t> entries;
    entries.reserve(count);

    for (int32_t i = 0; i < count; i++)
    {
        auto obj = (void*)elems[i];
        if (!obj || IsBadReadPtr(obj, 0x20)) continue;

        sys::str_t* ns = nullptr;
        if (!IsBadReadPtr((void*)s_get_name, 8)) ns = s_get_name(obj);
        if (!ns || IsBadReadPtr(ns, 0x14)) continue;
        if (ns->size <= 0 || ns->size > 256) continue;

        auto name = sanitize(ns->buf, ns->size);
        if (name.empty()) continue;

        int32_t id = 0;
        {
            auto slot = *(uintptr_t*)((uint8_t*)obj + 0x10);
            id = slot && !IsBadReadPtr((void*)slot, 0x10)
               ? *(int32_t*)(slot + 0x08)
               : *(int32_t*)((uint8_t*)obj + 0x18);
        }

        entries.push_back({ std::move(name), id });
    }

    std::printf("[mat] %zu entries\n", entries.size());

    auto fp = std::fopen(path, "a");
    if (!fp) return false;

    std::fprintf(fp, "[materials]\n");
    for (auto& e : entries)
        std::fprintf(fp, "%s = %d\n", e.name.c_str(), e.id);
    std::fprintf(fp, "\n");
    std::fclose(fp);

    return true;
}
