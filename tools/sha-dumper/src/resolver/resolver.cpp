#include "resolver.hpp"
#include "../disasm/hde64.h"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>

static const char* k_bn_sigs[] = {
    "48 8B 89 ? ? ? ? 48 8B ? ? ? ? ? 48 8B 49 ? E8 ? ? ? ? 48 85 C0 0F 84",
    "48 8B 89 ? ? ? ? 48 8B ? ? ? ? ? E8 ? ? ? ? 48 85 C0 0F 84",
};

void g_resolver::resolve_bn()
{
    uintptr_t hit = 0;
    for (auto sig : k_bn_sigs)
    {
        hit = g_mem::find_sig(sig);
        if (hit) break;
    }

    if (!hit)
    {
        std::printf("[sha] bn sig: not found\n");
        return;
    }

    g_mem::region_t r{};
    if (!g_mem::map_region(hit, 0xA0, r)) return;

    uint32_t pos = 0;
    uintptr_t tgt = 0;

    if (!g_mem::find_next_call(r, 0, 1, pos, tgt)) return;
    m_bn.dec0_addr = tgt;

    if (!g_mem::find_next_call(r, pos + 5, 1, pos, tgt)) return;

    g_mem::region_t gl{};
    if (!g_mem::map_region(tgt, 0x300, gl)) return;

    if (!g_mem::find_next_call(gl, 0, 1, pos, tgt)) return;
    m_bn.dec1_addr = tgt;

    m_bn.dec0 = cipher_t::from_loop  ((uint8_t*)m_bn.dec0_addr, 0x200);
    if (!m_bn.dec0.valid)
        m_bn.dec0 = cipher_t::from_inline((uint8_t*)m_bn.dec0_addr, 0x200);

    m_bn.dec1 = cipher_t::from_loop  ((uint8_t*)m_bn.dec1_addr, 0x200);
    if (!m_bn.dec1.valid)
        m_bn.dec1 = cipher_t::from_inline((uint8_t*)m_bn.dec1_addr, 0x200);

    auto bn_k = g_rt::klass("BaseNetworkable");
    auto bn_s = bn_k ? g_rt::static_nested(bn_k) : nullptr;

    if (bn_s)
    {
        m_bn.slot_rva = g_rt::klass_rva(bn_s);

        // Try type-based resolution first, fall back to Morphine-confirmed offsets
        bool found = false;
        void* it = nullptr;
        while (auto f = bn_s->fields(&it))
        {
            if (!f || !(f->flags() & FIELD_ATTRIBUTE_STATIC)) continue;
            auto tn = f->type() ? f->type()->name() : nullptr;
            if (!tn || !strstr(tn, "BaseNetworkable")) continue;
            if (!f->static_get_value<void*>()) continue;

            m_bn.wrapper_off = (uint32_t)foff(f);
            found = true;

            auto realm_k = f->type()->klass()
                           ? f->type()->klass()->get_generic_argument_at(0)
                           : nullptr;

            if (realm_k)
            {
                auto pf = il2cpp::get_field_by_name(realm_k, "clientEntities");
                if (pf) m_bn.parent_off = (uint32_t)foff(pf);

                // Try to find entity list field by name
                auto ef = il2cpp::get_field_by_name(realm_k, "entityList");
                if (ef) m_bn.ents_off = (uint32_t)foff(ef);
            }
            break;
        }

        // Morphine-confirmed fallbacks if type/name resolution failed
        if (!m_bn.wrapper_off) m_bn.wrapper_off = 0x20;
        if (!m_bn.parent_off)  m_bn.parent_off = 0x10;
    }

    if (!m_bn.ents_off) m_bn.ents_off = 0x18;

    std::printf("[sha] bn: rva=0x%llX wrap=0x%X parent=0x%X ents=0x%X dec0=%s dec1=%s\n",
        m_bn.slot_rva, m_bn.wrapper_off, m_bn.parent_off, m_bn.ents_off,
        m_bn.dec0.valid ? "ok" : "?", m_bn.dec1.valid ? "ok" : "?");
}

void g_resolver::resolve_cam()
{
    auto mc_k = g_rt::klass("MainCamera");
    if (!mc_k) { std::printf("[sha] camera: not found\n"); return; }

    m_cam.rva = g_rt::klass_rva(mc_k);

    // Name-based fallback (Unity 6 fix)
    auto f = il2cpp::get_field_by_name(mc_k, "_main");
    if (!f) f = il2cpp::get_field_by_name(mc_k, "mainCamera");
    if (!f) f = il2cpp::get_field_if_type_contains(mc_k, "Camera");
    if (f) m_cam.obj_off = (uint32_t)foff(f);

    std::printf("[sha] camera: rva=0x%llX obj=0x%X\n", m_cam.rva, m_cam.obj_off);
}

void g_resolver::resolve_console()
{
    auto host = il2cpp::search_for_class_by_method_in_assembly(
        "Facepunch.Console", "<Find>b__0", nullptr, -1);

    if (host)
    {
        auto m = il2cpp::get_method_by_name(host, "<Find>b__0");
        if (m)
        {
            auto p = m->get_param(0);
            m_cmd_klass = p ? p->klass() : nullptr;
        }
    }

    if (!m_cmd_klass) { std::printf("[sha] console: cmd klass not found\n"); return; }

    {
        int idx = 0;
        void* it = nullptr;
        while (auto f = m_cmd_klass->fields(&it))
        {
            if (!f || (f->flags() & FIELD_ATTRIBUTE_STATIC)) continue;
            auto tn = f->type() ? f->type()->name() : nullptr;
            if (!tn) continue;
            if (!idx && strstr(tn, "Func") && strstr(tn, "String"))
            { cvar::get_off = f->offset(); idx++; }
            else if (strstr(tn, "Action") && strstr(tn, "String"))
            { cvar::set_off = f->offset(); break; }
        }
    }

    auto show_k = g_rt::klass("ShowIfConvarEnabled");
    auto on_en  = show_k ? il2cpp::get_method_by_name(show_k, "OnEnable") : nullptr;
    uint64_t fn = on_en ? on_en->get_fn_ptr<uint64_t>() : 0;

    if (fn && m_cmd_klass)
    {
        auto str_k = il2cpp::get_class_by_name("String", "System");
        if (str_k)
        {
            il2cpp::il2cpp_type_t* params[] = { str_k->type() };
            auto found = il2cpp::get_method_in_method_by_return_type_and_param_types(
                WILDCARD_VALUE(il2cpp::il2cpp_class_t*),
                il2cpp::method_filter_t(fn, 0, 1),
                m_cmd_klass->type(),
                METHOD_ATTRIBUTE_PUBLIC,
                METHOD_ATTRIBUTE_STATIC,
                params, 1);

            if (found)
                cvar::find_cmd =
                    (decltype(cvar::find_cmd))found->get_fn_ptr<uint64_t>();
        }
    }

    std::printf("[sha] console: find=%p get=0x%zX set=0x%zX\n",
        cvar::find_cmd, cvar::get_off, cvar::set_off);
}

void g_resolver::resolve_fov()
{
    if (!cvar::find_cmd || !cvar::set_off) return;

    auto s = sys::str_t::make(L"graphics.fov");
    if (!s) return;

    cvar::cmd_t* cmd = nullptr;
    if (!IsBadReadPtr((void*)cvar::find_cmd, 8)) cmd = cvar::find_cmd(s);
    if (!cmd) return;

    uint64_t setter = 0;
    {
        uint64_t act = *(uint64_t*)((uintptr_t)cmd + cvar::set_off);
        if (act && !IsBadReadPtr((void*)act, 0x20))
            setter = *(uint64_t*)(act + 0x18);
    }

    if (!setter || IsBadReadPtr((void*)setter, 0x80)) return;

    auto try_chain = [](uint8_t* p) -> cipher_t::chain_t {
        auto c = cipher_t::from_inline(p, 0x100);
        if (!c.valid) c = cipher_t::from_loop(p, 0x100);
        return c;
    };

    auto is_bn = [&](const cipher_t::chain_t& c) {
        if (m_bn.dec0.valid && cipher_t::chains_equal(c, m_bn.dec0)) return true;
        if (m_bn.dec1.valid && cipher_t::chains_equal(c, m_bn.dec1)) return true;
        return false;
    };

    struct cand_t { cipher_t::chain_t chain; uint8_t* fn; };
    std::vector<cand_t> cands;

    auto* fn = (uint8_t*)setter;
    auto direct = try_chain(fn);
    if (direct.valid && !is_bn(direct)) cands.push_back({ direct, fn });

    uint8_t* p = fn;
    uint8_t* end = fn + 0x280;
    while (p < end)
    {
        hde64s hs;
        auto len = hde64_disasm(p, &hs);
        if (!len || (hs.flags & F_ERROR)) break;
        if (hs.opcode == 0xC3 || hs.opcode == 0xCC) break;

        if (p[0] == 0xE8 && len >= 5)
        {
            int32_t d; memcpy(&d, p + 1, 4);
            auto tgt = p + 5 + d;
            if (!IsBadReadPtr(tgt, 0x80) &&
                (uintptr_t)tgt != m_bn.dec0_addr &&
                (uintptr_t)tgt != m_bn.dec1_addr)
            {
                auto cc = try_chain(tgt);
                if (cc.valid && !is_bn(cc)) cands.push_back({ cc, tgt });
            }
        }
        p += len;
    }

    if (cands.empty()) return;

    auto* best = &cands.back();
    for (auto it = cands.rbegin(); it != cands.rend(); ++it)
        if (it->chain.count == 4 && !it->chain.unrolled) { best = &(*it); break; }

    m_fov.fn_addr = (uintptr_t)best->fn;
    m_fov.chain   = best->chain;

    std::printf("[sha] fov: rva=0x%llX ops=%d\n",
        m_fov.fn_addr - g_rt::base(), m_fov.chain.count);
}

void g_resolver::resolve_item_klasses()
{
    // Direct class lookup (Unity 6 fix — type matching in fields crashes)
    m_container_klass = g_rt::klass("ItemContainer");
    m_item_klass = g_rt::klass("Item");

    if (m_item_klass)
    {
        auto f = il2cpp::get_field_by_name(m_item_klass, "info");
        m_item_id_klass = (f && f->type()) ? f->type()->klass() : nullptr;
    }

    std::printf("[sha] klasses: item=%p def=%p cont=%p\n",
        m_item_klass, m_item_id_klass, m_container_klass);
}

void g_resolver::resolve_extra_ciphers()
{
    // Scan GameAssembly.dll for additional decrypt functions (ClActiveItem, inventory, eyes)
    // These are cipher loops with mov reg,2 + XOR/ADD/ROL/SUB body + decrement+jnz
    uint8_t* base = (uint8_t*)g_rt::base();
    uint32_t size = g_rt::imgsize();

    // Known op TYPE sequences for identification (validict build 23824285)
    static const uint8_t cla_pat[] = { cipher_t::op_t::k_add, cipher_t::op_t::k_rol, cipher_t::op_t::k_add, cipher_t::op_t::k_rol };
    static const uint8_t inv_pat[] = { cipher_t::op_t::k_rol, cipher_t::op_t::k_sub, cipher_t::op_t::k_xor };
    static const uint8_t ey_pat[]  = { cipher_t::op_t::k_sub, cipher_t::op_t::k_xor, cipher_t::op_t::k_rol, cipher_t::op_t::k_add };

    auto match_types = [](const cipher_t::chain_t& c, const uint8_t* pat, int n) -> bool {
        if (c.count != n) return false;
        for (int i = 0; i < n; i++) if (c.ops[i].kind != pat[i]) return false;
        return true;
    };

    auto is_known = [&](const cipher_t::chain_t& c) -> bool {
        if (m_bn.dec0.valid && cipher_t::chains_equal(c, m_bn.dec0)) return true;
        if (m_bn.dec1.valid && cipher_t::chains_equal(c, m_bn.dec1)) return true;
        if (m_fov.chain.valid && cipher_t::chains_equal(c, m_fov.chain)) return true;
        for (const auto& ec : m_extra_ciphers)
            if (cipher_t::chains_equal(c, ec.chain)) return true;
        return false;
    };

    int found = 0;

    for (uint32_t off = 0; off + 0x200 < size; off++)
    {
        bool is_mov2 = false;

        if (off + 6 < size && base[off] == 0x41 && base[off+1] >= 0xB8 && base[off+1] <= 0xBF &&
            base[off+2] == 0x02 && base[off+3] == 0 && base[off+4] == 0 && base[off+5] == 0)
            is_mov2 = true;
        else if (off + 5 < size && base[off] >= 0xB8 && base[off] <= 0xBF &&
                 base[off+1] == 0x02 && base[off+2] == 0 && base[off+3] == 0 && base[off+4] == 0)
            is_mov2 = true;

        if (!is_mov2) continue;

        auto chain = cipher_t::from_loop(base + off, 0x200);
        if (!chain.valid) continue;
        if (is_known(chain)) continue;

        extra_cipher_t ec;
        ec.fn_addr = (uintptr_t)(base + off);
        ec.chain = chain;

        if (match_types(chain, cla_pat, 4))
            ec.tag = "cl_active_item";
        else if (match_types(chain, inv_pat, 3))
            ec.tag = "inventory";
        else if (match_types(chain, ey_pat, 4))
            ec.tag = "eyes";
        else
        {
            char* t = new char[32];
            std::snprintf(t, 32, "unknown_%d", found);
            ec.tag = t;
        }

        m_extra_ciphers.push_back(ec);
        found++;

        std::printf("[sha] extra cipher: %s rva=0x%llX ops=%d\n",
            ec.tag, (unsigned long long)(ec.fn_addr - g_rt::base()), ec.chain.count);
    }

    std::printf("[sha] extra ciphers found: %d\n", found);
}

void g_resolver::write_bn(g_report& r)
{
    r.section("base_networkable");
    r.field("slot_rva",       m_bn.slot_rva);
    r.field("static_fields",  0xB8);
    r.field("wrapper_off",    m_bn.wrapper_off);
    r.field("parent_off",     m_bn.parent_off);
    r.field("entities",       m_bn.ents_off);
    r.field("hv_offset",      m_bn.hv_off);
    r.blank();
}

void g_resolver::write_cam(g_report& r)
{
    r.section("camera");
    r.field("slot_rva",   m_cam.rva);
    r.field("cam_object", m_cam.obj_off);
    r.blank();
}

void g_resolver::write_bp(g_report& r)
{
    auto k = g_rt::klass("BasePlayer");
    if (k)
    {
        // Use field-name matching (Unity 6 type_get_name crashes/fails)
        auto fn = [&](const char* s) { return il2cpp::get_field_by_name(k, s); };

        m_bp.cl_active_item   = foff(fn("clActiveItem"), k);
        m_bp.player_eyes      = foff(fn("eyes"), k);
        m_bp.player_inventory = foff(fn("inventory"), k);
        m_bp.current_team     = foff(fn("currentTeam"), k);
        m_bp.base_movement    = foff(fn("baseMovement"), k);
        m_bp.player_model     = foff(fn("model"), k);
        m_bp.player_flags     = foff(fn("playerFlags"), k);
        m_bp.display_name     = foff(fn("displayName"), k);
        m_bp.player_input     = foff(fn("input"), k);
        m_bp.metabolism       = foff(fn("metabolism"), k);
        m_bp.model_state      = foff(fn("modelState"), k);

        auto lp = g_rt::static_nested(k);
        if (lp) m_bp.local_rva = g_rt::klass_rva(lp);
    }

    r.section("base_player");
    r.field("cl_active_item",   m_bp.cl_active_item);
    r.field("player_eyes",      m_bp.player_eyes);
    r.field("player_inventory", m_bp.player_inventory);
    r.field("current_team",     m_bp.current_team);
    r.field("base_movement",    m_bp.base_movement);
    r.field("player_model",     m_bp.player_model);
    r.field("player_flags",     m_bp.player_flags);
    r.field("display_name",     m_bp.display_name);
    r.field("player_input",     m_bp.player_input);
    r.field("metabolism",       m_bp.metabolism);
    r.field("model_state",      m_bp.model_state);
    r.field("local_player_rva", m_bp.local_rva);
    r.blank();
}

void g_resolver::write_bce(g_report& r)
{
    auto bce = g_rt::klass("BaseCombatEntity");
    auto be  = g_rt::klass("BaseEntity");

    if (bce)
    {
        auto ls = il2cpp::get_field_by_name(bce, "lifestate");
        if (!ls) ls = il2cpp::get_field_by_name(bce, "_lifestate");
        m_bce.lifestate = foff(ls, bce);

        auto h  = il2cpp::get_field_by_name(bce, "_health");
        if (!h)  h = il2cpp::get_field_by_name(bce, "health");
        m_bce.health = foff(h, bce);

        auto mh = il2cpp::get_field_by_name(bce, "_maxHealth");
        if (!mh) mh = il2cpp::get_field_by_name(bce, "maxHealth");
        m_bce.max_health = foff(mh, bce);
    }

    if (be)
        m_bce.model = foff(il2cpp::get_field_by_name(be, "model"), be);

    r.section("base_combat_entity");
    r.field("lifestate",   m_bce.lifestate);
    r.field("model",       m_bce.model);
    r.field("_health",     m_bce.health);
    r.field("_maxHealth",  m_bce.max_health);
    r.blank();
}

void g_resolver::write_entity(g_report& r)
{
    auto k = g_rt::klass("BaseEntity");
    if (k)
    {
        m_ent.flags     = foff(il2cpp::get_field_by_name(k, "flags"), k);
        m_ent.net       = foff(il2cpp::get_field_by_name(k, "net"), k);
        m_ent.prefab_id = foff(il2cpp::get_field_by_name(k, "prefabID"), k);
    }

    r.section("base_entity");
    r.field("flags",     m_ent.flags);
    r.field("net",       m_ent.net);
    r.field("prefab_id", m_ent.prefab_id);
    r.blank();
}

void g_resolver::write_eyes(g_report& r)
{
    auto k = g_rt::klass("PlayerEyes");
    if (k)
    {
        m_eyes.body_rotation = foff(il2cpp::get_field_by_name(k, "bodyRotation"), k);
        m_eyes.eye_offset    = foff(il2cpp::get_field_by_name(k, "offset"), k);
    }

    r.section("player_eyes");
    r.field("body_rotation", m_eyes.body_rotation);
    r.field("eye_offset",    m_eyes.eye_offset);
    r.blank();
}

void g_resolver::write_movement(g_report& r)
{
    auto k = g_rt::klass("BaseMovement");
    if (k)
    {
        void* it = nullptr;
        while (auto f = k->fields(&it))
        {
            if (!f || (f->flags() & FIELD_ATTRIBUTE_STATIC)) continue;
            const char* fn = f->name();
            if (!fn) continue;
            if (!strcmp(fn, "velocity"))
                m_move.velocity = foff(f, k);
            else if (!strcmp(fn, "isGrounded") || !strcmp(fn, "grounded"))
                m_move.grounded = foff(f, k);
        }
    }

    r.section("base_movement");
    r.field("velocity", m_move.velocity);
    r.field("grounded", m_move.grounded);
    r.blank();
}

void g_resolver::write_item(g_report& r)
{
    auto k = m_item_klass ? m_item_klass : g_rt::klass("Item");
    if (k)
    {
        auto fn = [&](const char* s) { return foff(il2cpp::get_field_by_name(k, s)); };
        m_item.info      = fn("info");
        m_item.amount    = fn("amount");
        m_item.uid       = fn("uid");
        m_item.skin      = fn("skin");
        m_item.condition = fn("condition");
        m_item.max_cond  = fn("maxCondition");
        m_item.position  = fn("position");
        m_item.parent    = fn("parent");
    }

    r.section("item");
    r.field("info",         m_item.info);
    r.field("amount",       m_item.amount);
    r.field("uid",          m_item.uid);
    r.field("skin",         m_item.skin);
    r.field("condition",    m_item.condition);
    r.field("maxCondition", m_item.max_cond);
    r.field("position",     m_item.position);
    r.field("parent",       m_item.parent);
    r.blank();
}

void g_resolver::write_itemdef(g_report& r)
{
    auto k = g_rt::klass("ItemDefinition");
    if (k)
    {
        auto fn = [&](const char* s) { return foff(il2cpp::get_field_by_name(k, s), k); };
        m_idef.shortname = fn("shortname");
        m_idef.itemid    = fn("itemid");
        m_idef.category  = fn("category");
        m_idef.stackable = fn("stackable");
    }

    r.section("item_definition");
    r.field("shortname", m_idef.shortname);
    r.field("itemid",    m_idef.itemid);
    r.field("category",  m_idef.category);
    r.field("stackable", m_idef.stackable);
    r.blank();
}

void g_resolver::write_container(g_report& r)
{
    auto k = m_container_klass ? m_container_klass : g_rt::klass("ItemContainer");
    if (k)
    {
        // Use field name instead of type search (Unity 6 fix)
        auto fl = il2cpp::get_field_by_name(k, "itemList");
        if (!fl) fl = il2cpp::get_field_by_name(k, "itemList< Item >");
        if (!fl) fl = il2cpp::get_field_if_type_contains(k, "List");
        m_cont.item_list = foff(fl);

        auto fn = [&](const char* s) { return foff(il2cpp::get_field_by_name(k, s)); };
        m_cont.capacity = fn("capacity");
        m_cont.uid      = fn("uid");
        m_cont.flags    = fn("flags");
    }

    r.section("item_container");
    r.field("item_list", m_cont.item_list);
    r.field("capacity",  m_cont.capacity);
    r.field("uid",       m_cont.uid);
    r.field("flags",     m_cont.flags);
    r.blank();
}

void g_resolver::write_inventory(g_report& r)
{
    auto inv_k = g_rt::klass("PlayerInventory");
    if (inv_k)
    {
        // Use field name matching (Unity 6 fix — type matching crashes)
        auto fn = [&](const char* s) { return foff(il2cpp::get_field_by_name(inv_k, s)); };
        m_inv.belt = fn("containerBelt");
        m_inv.wear = fn("containerWear");
    }

    r.section("player_inventory");
    r.field("belt", m_inv.belt);
    r.field("wear", m_inv.wear);
    r.blank();
}

void g_resolver::write_weapon(g_report& r)
{
    auto held = g_rt::klass("HeldEntity");
    auto proj = g_rt::klass("BaseProjectile");
    auto mag  = g_rt::klass("Magazine");

    if (held)
        m_weapon.owner_item_uid = foff(il2cpp::get_field_by_name(held, "ownerItemUID"), held);

    if (proj)
    {
        m_weapon.primary_mag = foff(il2cpp::get_field_by_name(proj, "primaryMagazine"), proj);
        m_weapon.recoil      = foff(il2cpp::get_field_by_name(proj, "recoil"), proj);
    }

    if (mag)
    {
        m_weapon.mag_contents = foff(il2cpp::get_field_by_name(mag, "contents"), mag);
        m_weapon.mag_capacity = foff(il2cpp::get_field_by_name(mag, "capacity"), mag);
    }

    r.section("weapon");
    r.field("owner_item_uid", m_weapon.owner_item_uid);
    r.field("primary_mag",    m_weapon.primary_mag);
    r.field("recoil",         m_weapon.recoil);
    r.field("mag_contents",   m_weapon.mag_contents);
    r.field("mag_capacity",   m_weapon.mag_capacity);
    r.blank();
}

// === NEW: Static pointer RVA dump ===
void g_resolver::write_static_pointers(g_report& r)
{
    r.section("static_pointers");

    auto dump_rva = [&](const char* name, const char* klass_name) {
        auto k = g_rt::klass(klass_name);
        if (!k) { r.field(name, (uint64_t)0); return; }
        auto s = g_rt::static_nested(k);
        if (s) r.field(name, g_rt::klass_rva(s));
        else r.field(name, (uint64_t)0);
    };

    // Already resolved via resolve_bn/resolve_cam
    r.field("basenetworkable", m_bn.slot_rva);
    r.field("main_camera", m_cam.rva);

    // Resolve TOD_Sky
    dump_rva("tod_sky", "TOD_Sky");

    // Resolve FOV ConVar_Graphics
    {
        auto fov_k = g_rt::klass("ConVar_Graphics");
        if (!fov_k) fov_k = g_rt::klass("Graphics");
        if (fov_k) {
            auto s = g_rt::static_nested(fov_k);
            r.field("convar_graphics", s ? g_rt::klass_rva(s) : 0);
        } else r.field("convar_graphics", (uint64_t)0);
    }

    // Resolve EffectNetwork
    dump_rva("effect_network", "EffectNetwork");

    // Resolve UI_LoadingScreen singleton
    {
        auto k = g_rt::klass("UI_LoadingScreen");
        if (!k) k = g_rt::klass("LoadingScreen");
        if (k) {
            auto s = g_rt::static_nested(k);
            r.field("ui_loading_screen", s ? g_rt::klass_rva(s) : 0);
        } else r.field("ui_loading_screen", (uint64_t)0);
    }

    // Resolve MixerSnapshotManager singleton
    {
        auto k = g_rt::klass("MixerSnapshotManager");
        if (k) {
            auto s = g_rt::static_nested(k);
            r.field("mixer_snapshot_mgr", s ? g_rt::klass_rva(s) : 0);
        } else r.field("mixer_snapshot_mgr", (uint64_t)0);
    }

    // Resolve convar_admin
    dump_rva("convar_admin", "ConsoleSystem");

    // GCHandle RVA
    r.field("gchandle_array", m_gchandle_rva);

    // PlayerInventory typenav
    dump_rva("player_inventory_typenav", "PlayerInventory");

    r.blank();
}

// === NEW: Native camera struct scanner — uses direct RVA (no Il2Cpp class lookup) ===
void g_resolver::write_native_camera(g_report& r)
{
    r.section("native_camera");

    // Use direct RVA read — same chain as the cheat code
    // GameAssembly + camera_rva → TypeInfo → +0xB8 → static_fields → +0x90 → instance → +0x10 → native
    uint64_t ga = g_rt::base();

    // Try MainCamera RVA (sha-dumper build 24037537: 0x0FD0A5C0)
    uint64_t cam_rva = m_cam.rva;
    if (!cam_rva) cam_rva = 0x0FD0A5C0; // fallback

    uint64_t typeinfo = *(uint64_t*)(ga + cam_rva);
    if (!typeinfo || IsBadReadPtr((void*)typeinfo, 0x100)) {
        r.field("error", (uint64_t)1);
        r.field("detail", (uint64_t)1); // typeinfo failed
        r.blank();
        return;
    }

    uint64_t static_fields = *(uint64_t*)(typeinfo + 0xB8);
    if (!static_fields || IsBadReadPtr((void*)static_fields, 0x100)) {
        r.field("error", (uint64_t)2);
        r.field("detail", (uint64_t)2); // static_fields failed
        r.blank();
        return;
    }

    // Try wrapper_class offset (Morphine: 0x90)
    uint64_t cam_instance = *(uint64_t*)(static_fields + 0x90);
    if (!cam_instance || IsBadReadPtr((void*)cam_instance, 0x20)) {
        // Try other offsets
        for (int off : { 0x48, 0x18, 0x20, 0x30 }) {
            cam_instance = *(uint64_t*)(static_fields + off);
            if (cam_instance && !IsBadReadPtr((void*)cam_instance, 0x20)) break;
            cam_instance = 0;
        }
    }
    if (!cam_instance) {
        r.field("error", (uint64_t)3);
        r.field("detail", (uint64_t)3); // cam_instance failed
        r.blank();
        return;
    }

    // Try +0x10 deref to get native camera
    uint64_t native_cam = *(uint64_t*)(cam_instance + 0x10);
    if (!native_cam || IsBadReadPtr((void*)native_cam, 0x800)) {
        native_cam = cam_instance; // use directly
    }

    r.field("cam_rva", cam_rva);
    r.field("typeinfo_addr", typeinfo);
    r.field("static_fields_addr", static_fields);
    r.field("cam_instance_addr", cam_instance);
    r.field("native_cam_addr", native_cam);

    // ================================================================
    // Structural validation scan (replaces heuristic first-match scan)
    // ================================================================
    // Instead of taking the first plausible float, we use specific
    // structural patterns that are unique to each field type:
    //
    // View matrix:      4x4 float matrix with last row [0,0,0,1]
    //                  + translation column finite (world-bounds)
    //                  + minimum offset 0x100 (skip struct header)
    // Projection matrix: 4x4 float matrix with m[11]=-1.0, m[15]=0.0
    //                    + m[10] non-zero, m[14] negative (near/far)
    //                    + minimum offset 0x100 (skip struct header)
    // Field of view:   float in [30,120] NOT overlapping any matrix,
    //                  followed by aspect ratio in [0.5, 4.0]
    //                  + minimum offset 0x40
    // Culling mask:    uint32 not 0, not 0xFFFFFFFF, >=2 bits set
    //                  + minimum offset 0x40
    // World position:  Vector3 finite, non-zero, within world bounds,
    //                  NOT overlapping any found matrix or FOV
    //                  + minimum offset 0x100
    //
    // validated flag: 2 = ALL 5 offsets found (matrices + FOV + mask + pos)
    //                 1 = matrices found but some fields missing
    //                 0 = matrices NOT found → do NOT auto-apply
    // ================================================================

    uint64_t fov_off = 0, cullmask_off = 0, viewmat_off = 0, projmat_off = 0, worldpos_off = 0;
    auto* cam = (uint8_t*)native_cam;

    auto in_matrix = [&](uint64_t off, int len) -> bool {
        if (viewmat_off && off < viewmat_off + 64 && off + len > viewmat_off) return true;
        if (projmat_off && off < projmat_off + 64 && off + len > projmat_off) return true;
        return false;
    };
    auto in_worldpos = [&](uint64_t off, int len) -> bool {
        if (worldpos_off && off < worldpos_off + 12 && off + len > worldpos_off) return true;
        return false;
    };

    // 1. View matrix: orthonormal rotation + [0,0,0,1] last row + finite translation.
    //    Orthonormality (unit-length columns, perpendicular) eliminates non-rotation
    //    affine matrices that share the [0,0,0,1] pattern.
    for (uint64_t off = 0x100; off + 64 <= 0x600; off += 4) {
        float* m = (float*)(cam + off);
        if (!(m[3] == 0.0f && m[7] == 0.0f && m[11] == 0.0f && m[15] == 1.0f)) continue;
        if (m[0] == 0.0f || m[5] == 0.0f || m[10] == 0.0f) continue;
        bool rot_ok = true;
        for (int i = 0; i < 12; i++) {
            if (i == 3 || i == 7 || i == 11) continue;
            if (!std::isfinite(m[i]) || fabsf(m[i]) > 1.0f) { rot_ok = false; break; }
        }
        if (!rot_ok) continue;
        if (!std::isfinite(m[12]) || !std::isfinite(m[13]) || !std::isfinite(m[14])) continue;
        if (fabsf(m[12]) > 50000.0f || fabsf(m[13]) > 50000.0f || fabsf(m[14]) > 50000.0f) continue;
        float c0l = sqrtf(m[0]*m[0] + m[1]*m[1] + m[2]*m[2]);
        float c1l = sqrtf(m[4]*m[4] + m[5]*m[5] + m[6]*m[6]);
        float c2l = sqrtf(m[8]*m[8] + m[9]*m[9] + m[10]*m[10]);
        if (fabsf(c0l - 1.0f) > 0.01f || fabsf(c1l - 1.0f) > 0.01f || fabsf(c2l - 1.0f) > 0.01f) continue;
        float d01 = m[0]*m[4] + m[1]*m[5] + m[2]*m[6];
        float d02 = m[0]*m[8] + m[1]*m[9] + m[2]*m[10];
        float d12 = m[4]*m[8] + m[5]*m[9] + m[6]*m[10];
        if (fabsf(d01) > 0.01f || fabsf(d02) > 0.01f || fabsf(d12) > 0.01f) continue;
        viewmat_off = off;
        break;
    }

    // 2. Projection matrix: perspective with m[11]=-1, m[15]=0,
    //    m[10] in [-1.01, -0.99] (near-zero near/far ratio), m[14] < 0.
    for (uint64_t off = 0x100; off + 64 <= 0x600; off += 4) {
        if (off == viewmat_off) continue;
        float* m = (float*)(cam + off);
        if (m[11] != -1.0f || m[15] != 0.0f) continue;
        if (m[0] < 0.1f || m[0] > 100.0f) continue;
        if (m[5] < 0.1f || m[5] > 100.0f) continue;
        if (!std::isfinite(m[10]) || m[10] < -1.01f || m[10] > -0.99f) continue;
        if (!std::isfinite(m[14]) || m[14] >= 0.0f) continue;
        projmat_off = off;
        break;
    }

    // 3. World position (scanned BEFORE FOV so FOV can exclude this region).
    for (uint64_t off = 0x100; off + 12 <= 0x600; off += 4) {
        if (in_matrix(off, 12)) continue;
        float x = *(float*)(cam + off);
        float y = *(float*)(cam + off + 4);
        float z = *(float*)(cam + off + 8);
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) continue;
        if (x == 0.0f && y == 0.0f && z == 0.0f) continue;
        if (fabsf(x) > 50000.0f || fabsf(y) > 50000.0f || fabsf(z) > 50000.0f) continue;
        worldpos_off = off;
        break;
    }

    // 4. Field of view: float in [30, 120], not overlapping matrices or world_position.
    uint64_t fov_fallback = 0;
    for (uint64_t off = 0x40; off + 8 <= 0x600; off += 4) {
        if (in_matrix(off, 8)) continue;
        if (in_worldpos(off, 8)) continue;
        float val = *(float*)(cam + off);
        if (!std::isfinite(val) || val < 30.0f || val > 120.0f) continue;
        float next = *(float*)(cam + off + 4);
        if (std::isfinite(next) && next >= 0.5f && next <= 4.0f) {
            fov_off = off;
            break;
        }
        if (!fov_fallback) fov_fallback = off;
    }
    if (!fov_off && fov_fallback) fov_off = fov_fallback;

    // 5. Culling mask: uint32 >= 0xFF (camera renders many layers), not 0xFFFFFFFF.
    for (uint64_t off = 0x40; off + 4 <= 0x600; off += 4) {
        if (in_matrix(off, 4)) continue;
        if (in_worldpos(off, 4)) continue;
        if (fov_off && off < fov_off + 8 && off + 4 > fov_off) continue;
        uint32_t val = *(uint32_t*)(cam + off);
        if (val < 0xFF || val == 0xFFFFFFFF) continue;
        cullmask_off = off;
        break;
    }

    // Validated flag: requires ALL 5 offsets non-zero.
    int camera_validated = 0;
    if (viewmat_off && projmat_off) {
        camera_validated = 1;
        if (fov_off && cullmask_off && worldpos_off)
            camera_validated = 2;
    }

    r.field("native_cam_addr", native_cam);
    r.field("field_of_view", fov_off);
    r.field("culling_mask", cullmask_off);
    r.field("view_matrix", viewmat_off);
    r.field("projection_matrix", projmat_off);
    r.field("world_position", worldpos_off);
    r.field("validated", (uint64_t)camera_validated);

    std::printf("[sha] native_camera: validated=%d fov=0x%llX cull=0x%llX view=0x%llX proj=0x%llX pos=0x%llX\n",
        camera_validated, fov_off, cullmask_off, viewmat_off, projmat_off, worldpos_off);

    r.blank();
}

// === Camera icall disassembly — extract native struct offsets from engine code ===
// Disassembles Camera property getter functions (get_fieldOfView, get_worldToCameraMatrix,
// etc.) to find the exact offset the engine uses to read each field from the native Camera struct.
// This is 100% reliable — the offset is literally in the mov instruction.
void g_resolver::resolve_camera_icalls()
{
    // In Unity 6, Camera property GETTERS are often NOT icalls — they read from
    // managed cached fields. SETTERS ARE icalls and write directly to the native struct.
    // So we try setter icalls first (looking for STORE instructions), then fall back
    // to getter icalls (looking for LOAD instructions).
    enum RetType { FLOAT_LOAD, INT_LOAD, MATRIX_LOAD, FLOAT_STORE, INT_STORE, MATRIX_STORE };
    struct prop_t {
        const char* dbg_name;
        const char* getter_icall;
        const char* getter_injected;
        const char* setter_icall;
        const char* setter_injected;
        uint32_t cam_icall_t::* field;
        RetType ret_type;
        int dbg;
    };
    prop_t props[] = {
        {"fieldOfView",
         "UnityEngine.Camera::get_fieldOfView()", nullptr,
         "UnityEngine.Camera::set_fieldOfView(System.Single)", nullptr,
         &cam_icall_t::field_of_view, FLOAT_LOAD, 0},
        {"worldToCameraMatrix",
         "UnityEngine.Camera::get_worldToCameraMatrix()",
         "UnityEngine.Camera::get_worldToCameraMatrix_Injected(UnityEngine.Matrix4x4&)",
         "UnityEngine.Camera::set_worldToCameraMatrix(UnityEngine.Matrix4x4)",
         "UnityEngine.Camera::set_worldToCameraMatrix_Injected(UnityEngine.Matrix4x4&)",
         &cam_icall_t::view_matrix, MATRIX_LOAD, 1},
        {"projectionMatrix",
         "UnityEngine.Camera::get_projectionMatrix()",
         "UnityEngine.Camera::get_projectionMatrix_Injected(UnityEngine.Matrix4x4&)",
         "UnityEngine.Camera::set_projectionMatrix(UnityEngine.Matrix4x4)",
         "UnityEngine.Camera::set_projectionMatrix_Injected(UnityEngine.Matrix4x4&)",
         &cam_icall_t::projection_matrix, MATRIX_LOAD, 2},
        {"cullingMask",
         "UnityEngine.Camera::get_cullingMask()", nullptr,
         "UnityEngine.Camera::set_cullingMask(System.Int32)", nullptr,
         &cam_icall_t::culling_mask, INT_LOAD, 3},
        {"worldPosition",
         "UnityEngine.Camera::get_worldPosition()", nullptr,
         nullptr, nullptr,
         &cam_icall_t::world_position, FLOAT_LOAD, 4},
    };

    auto camera_klass = g_rt::klass("Camera", "UnityEngine");

    auto valid_offset = [](uint32_t off) -> bool {
        return off >= 0x40 && off <= 0x600;
    };

    auto reads_from_rcx = [](const hde64s& hs) -> bool {
        return hs.modrm_rm == 1 && !hs.rex_b;
    };

    // LOAD instruction checks (movss/movups/movaps xmm, [reg+disp])
    auto is_movss_load = [](const hde64s& hs) -> bool {
        return hs.p_rep && hs.opcode == 0x0F && hs.opcode2 == 0x10;
    };
    auto is_movups_load = [](const hde64s& hs) -> bool {
        return !hs.p_rep && !hs.p_66 && hs.opcode == 0x0F && hs.opcode2 == 0x10;
    };
    auto is_movaps_load = [](const hde64s& hs) -> bool {
        return hs.opcode == 0x0F && hs.opcode2 == 0x28;
    };

    // STORE instruction checks (movss/movups/movaps [reg+disp], xmm)
    auto is_movss_store = [](const hde64s& hs) -> bool {
        return hs.p_rep && hs.opcode == 0x0F && hs.opcode2 == 0x11;
    };
    auto is_movups_store = [](const hde64s& hs) -> bool {
        return !hs.p_rep && !hs.p_66 && hs.opcode == 0x0F && hs.opcode2 == 0x11;
    };
    auto is_movaps_store = [](const hde64s& hs) -> bool {
        return hs.opcode == 0x0F && hs.opcode2 == 0x29;
    };

    // Integer LOAD: mov eax, [reg+disp] (opcode 0x8B, reg=0)
    auto is_mov_eax_load = [](const hde64s& hs) -> bool {
        return hs.opcode == 0x8B && hs.modrm_reg == 0;
    };
    // Integer STORE: mov [reg+disp], eax (opcode 0x89, reg=0)
    auto is_mov_eax_store = [](const hde64s& hs) -> bool {
        return hs.opcode == 0x89 && hs.modrm_reg == 0;
    };

    auto has_disp32 = [](const hde64s& hs) -> bool {
        return hs.modrm_mod == 0x02 && (hs.flags & F_DISP32);
    };
    auto has_disp8 = [](const hde64s& hs) -> bool {
        return hs.modrm_mod == 0x01 && (hs.flags & F_DISP8);
    };
    auto get_offset = [](const hde64s& hs) -> uint32_t {
        if (hs.flags & F_DISP32) return hs.disp.disp32;
        if (hs.flags & F_DISP8) return (uint32_t)(int32_t)(int8_t)hs.disp.disp8;
        return 0;
    };

    auto matches_type = [&](const hde64s& hs, RetType rt) -> bool {
        if (reads_from_rcx(hs)) return false;
        if (!(has_disp32(hs) || has_disp8(hs))) return false;
        switch (rt) {
            case FLOAT_LOAD:   return is_movss_load(hs);
            case MATRIX_LOAD:  return is_movss_load(hs) || is_movups_load(hs) || is_movaps_load(hs);
            case INT_LOAD:     return is_mov_eax_load(hs);
            case FLOAT_STORE:  return is_movss_store(hs);
            case MATRIX_STORE: return is_movss_store(hs) || is_movups_store(hs) || is_movaps_store(hs);
            case INT_STORE:    return is_mov_eax_store(hs);
        }
        return false;
    };

    int found = 0;
    bool world_pos_found = false;

    for (const auto& prop : props)
    {
        if (prop.field == &cam_icall_t::world_position && world_pos_found)
            continue;

        uint8_t* fn = nullptr;
        const char* source = "none";
        RetType scan_type = prop.ret_type;

        auto try_resolve = [&](const char* icall_name) -> uint8_t* {
            if (!icall_name) return nullptr;
            auto ptr = il2cpp::resolve_icall(icall_name);
            if (ptr && !IsBadReadPtr((void*)ptr, 0x200))
                return (uint8_t*)ptr;
            return nullptr;
        };

        // Try SETTER icalls first (they write to native struct directly)
        if (prop.setter_icall) {
            fn = try_resolve(prop.setter_icall);
            if (fn) { source = "setter_icall"; scan_type = (RetType)(prop.ret_type + 3); }
        }
        if (!fn && prop.setter_injected) {
            fn = try_resolve(prop.setter_injected);
            if (fn) { source = "setter_injected"; scan_type = (RetType)(prop.ret_type + 3); }
        }

        // Fall back to GETTER icalls
        if (!fn && prop.getter_icall) {
            fn = try_resolve(prop.getter_icall);
            if (fn) source = "getter_icall";
        }
        if (!fn && prop.getter_injected) {
            fn = try_resolve(prop.getter_injected);
            if (fn) source = "getter_injected";
        }

        // Last resort: get_method_by_name (IL2CPP stub — often won't work)
        if (!fn && camera_klass) {
            auto method = il2cpp::get_method_by_name(camera_klass, prop.dbg_name);
            if (method) {
                auto ptr = method->get_fn_ptr<uint64_t>();
                if (ptr && !IsBadReadPtr((void*)ptr, 0x200)) {
                    fn = (uint8_t*)ptr;
                    source = "get_method_by_name";
                }
            }
        }

        m_cam_icall_debug.source[prop.dbg] = source;
        m_cam_icall_debug.fn_addr[prop.dbg] = (uint64_t)fn;
        if (fn) {
            memcpy(m_cam_icall_debug.first_bytes[prop.dbg], fn, 32);
        }

        if (!fn) continue;

        uint32_t offset = 0;
        uint8_t* scan_start = fn;

        for (int pass = 0; pass < 2 && !offset; pass++)
        {
            uint8_t* p = scan_start;
            uint8_t* end = scan_start + 0x300;

            while (p < end && !offset)
            {
                hde64s hs;
                auto len = hde64_disasm(p, &hs);
                if (!len || (hs.flags & F_ERROR)) break;
                if (hs.opcode == 0xC3 || hs.opcode == 0xCC) break;

                if (matches_type(hs, scan_type)) {
                    uint32_t candidate = get_offset(hs);
                    if (valid_offset(candidate)) {
                        offset = candidate;
                        break;
                    }
                }

                if (p[0] == 0xE9 && len >= 5) {
                    int32_t rel; memcpy(&rel, p + 1, 4);
                    uint8_t* target = p + 5 + rel;
                    if (!IsBadReadPtr(target, 0x200)) {
                        scan_start = target;
                        break;
                    }
                }

                if (p[0] == 0xE8 && len >= 5) {
                    int32_t rel; memcpy(&rel, p + 1, 4);
                    uint8_t* target = p + 5 + rel;
                    if (!IsBadReadPtr(target, 0x100)) {
                        uint8_t* tp = target;
                        uint8_t* tend = target + 0x100;
                        while (tp < tend) {
                            hde64s ts;
                            auto tlen = hde64_disasm(tp, &ts);
                            if (!tlen || (ts.flags & F_ERROR)) break;
                            if (ts.opcode == 0xC3 || ts.opcode == 0xCC) break;

                            if (matches_type(ts, scan_type)) {
                                uint32_t c = get_offset(ts);
                                if (valid_offset(c)) { offset = c; break; }
                            }
                            tp += tlen;
                        }
                        if (offset) break;
                    }
                }

                p += len;
            }
        }

        if (offset) {
            m_cam_icall.*(prop.field) = offset;
            if (prop.field == &cam_icall_t::world_position)
                world_pos_found = true;
            found++;
        }
    }

    bool all_same = false;
    if (m_cam_icall.field_of_view && m_cam_icall.view_matrix &&
        m_cam_icall.field_of_view == m_cam_icall.view_matrix &&
        m_cam_icall.field_of_view == m_cam_icall.projection_matrix &&
        m_cam_icall.field_of_view == m_cam_icall.culling_mask) {
        all_same = true;
    }

    bool has_fov = m_cam_icall.field_of_view != 0;
    bool has_viewmat = m_cam_icall.view_matrix != 0;
    m_cam_icall.resolved = (has_fov && has_viewmat && found >= 3 && !all_same);
}

void g_resolver::write_camera_icalls(g_report& r)
{
    r.section("camera_icall_offsets");
    r.field("field_of_view",      (uint64_t)m_cam_icall.field_of_view);
    r.field("view_matrix",        (uint64_t)m_cam_icall.view_matrix);
    r.field("projection_matrix",  (uint64_t)m_cam_icall.projection_matrix);
    r.field("culling_mask",       (uint64_t)m_cam_icall.culling_mask);
    r.field("world_position",     (uint64_t)m_cam_icall.world_position);
    r.field("resolved",           (uint64_t)(m_cam_icall.resolved ? 1 : 0));
    r.blank();

    r.section("camera_icall_debug");
    uint32_t cam_icall_t::* fields[] = {
        &cam_icall_t::field_of_view,
        &cam_icall_t::view_matrix,
        &cam_icall_t::projection_matrix,
        &cam_icall_t::culling_mask,
        &cam_icall_t::world_position,
    };
    const char* names[] = { "field_of_view", "view_matrix", "projection_matrix", "culling_mask", "world_position" };
    for (int i = 0; i < 5; i++) {
        char key[128], val[256];
        snprintf(key, sizeof(key), "%s_source", names[i]);
        r.field_str(key, m_cam_icall_debug.source[i]);

        snprintf(key, sizeof(key), "%s_addr", names[i]);
        snprintf(val, sizeof(val), "0x%llX", (unsigned long long)m_cam_icall_debug.fn_addr[i]);
        r.field_str(key, val);

        if (m_cam_icall_debug.fn_addr[i]) {
            snprintf(key, sizeof(key), "%s_bytes", names[i]);
            val[0] = '\0';
            for (int b = 0; b < 32; b++) {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X", m_cam_icall_debug.first_bytes[i][b]);
                if (b > 0) strcat(val, " ");
                strcat(val, hex);
            }
            r.field_str(key, val);
        }
    }
    r.blank();
}

// === NEW: TOD_Sky instances offset scanner ===
void g_resolver::write_tod_sky_instances(g_report& r)
{
    r.section("tod_sky_instances");

    auto tod_k = g_rt::klass("TOD_Sky");
    if (!tod_k) { r.field("error", (uint64_t)1); r.blank(); return; }

    auto tod_static = g_rt::static_nested(tod_k);
    if (!tod_static) { r.field("error", (uint64_t)2); r.blank(); return; }

    // Get static fields data
    void* it = nullptr;
    auto f = tod_static->fields(&it);
    if (!f) { r.field("error", (uint64_t)3); r.blank(); return; }

    // Read static field value (the static_fields pointer)
    // The static_nested class has static fields at its offset
    // We need to read the actual static data
    // For Il2Cpp, static fields are at klass + 0xB8
    uint64_t static_data = *(uint64_t*)((uint8_t*)tod_static + 0xB8);
    if (!static_data || IsBadReadPtr((void*)static_data, 0x400)) {
        // Try reading via static_get_value
        void* sf_val = f->static_get_value<void*>();
        if (sf_val) static_data = (uint64_t)sf_val;
        else { r.field("error", (uint64_t)4); r.blank(); return; }
    }

    // Known TOD_Sky field offsets for validation
    const uint64_t TOD_Night = 0x60;
    const uint64_t TOD_AmbientMult = 0x5C;

    uint64_t found_off = 0;

    // Scan static_data for a reference that leads to valid TOD_Sky
    for (uint64_t off = 0; off < 0x200; off += 8) {
        uint64_t candidate = *(uint64_t*)(static_data + off);
        if (!candidate || IsBadReadPtr((void*)candidate, 0x300)) continue;

        // Try list traversal: candidate + 0x10 → array, array + 0x20 → TOD_Sky
        uint64_t arr_ptr = *(uint64_t*)((uint8_t*)candidate + 0x10);
        if (arr_ptr && !IsBadReadPtr((void*)arr_ptr, 0x30)) {
            uint64_t sky = *(uint64_t*)((uint8_t*)arr_ptr + 0x20);
            if (sky && !IsBadReadPtr((void*)sky, 0x300)) {
                uint64_t night = *(uint64_t*)((uint8_t*)sky + TOD_Night);
                if (night && !IsBadReadPtr((void*)night, 0x60)) {
                    float am = *(float*)((uint8_t*)night + TOD_AmbientMult);
                    if (am >= 0.0f && am <= 10.0f) {
                        found_off = off;
                        r.field("instances_offset", off);
                        r.field("instances_type", (uint64_t)1); // list
                        r.field("tod_sky_addr", sky);
                        r.field("ambient_multiplier_val", (uint64_t)*(uint32_t*)&am);
                        break;
                    }
                }
            }
        }

        // Try direct: candidate IS TOD_Sky
        uint64_t night = *(uint64_t*)((uint8_t*)candidate + TOD_Night);
        if (night && !IsBadReadPtr((void*)night, 0x60)) {
            float am = *(float*)((uint8_t*)night + TOD_AmbientMult);
            if (am >= 0.0f && am <= 10.0f) {
                found_off = off;
                r.field("instances_offset", off);
                r.field("instances_type", (uint64_t)0); // direct
                r.field("tod_sky_addr", candidate);
                r.field("ambient_multiplier_val", (uint64_t)*(uint32_t*)&am);
                break;
            }
        }
    }

    if (!found_off) r.field("instances_offset", (uint64_t)0);

    std::printf("[sha] tod_sky_instances: offset=0x%llX\n", found_off);
    r.blank();
}

// === NEW: Unity engine constants (for documentation/verification) ===
void g_resolver::write_unity_constants(g_report& r)
{
    r.section("unity_engine_constants");
    r.field("system_list_items", (uint64_t)0x10);
    r.field("system_list_size", (uint64_t)0x18);
    r.field("system_list_array_first", (uint64_t)0x20);
    r.field("go_components", (uint64_t)0x20);
    r.field("go_component_count", (uint64_t)0x30);
    r.field("go_component_stride", (uint64_t)0x10);
    r.field("go_component_ptr_in_entry", (uint64_t)0x8);
    r.field("go_component_handle_in_native", (uint64_t)0x18);
    r.field("unity_object_cached_ptr", (uint64_t)0x10);
    r.field("unity_component_game_object", (uint64_t)0x20);
    r.field("unity_transform_indirect_ptr", (uint64_t)0x28);
    r.field("unity_transform_world_pos", (uint64_t)0x90);
    r.field("transform_data_pos_base", (uint64_t)0x18);
    r.field("transform_data_indices", (uint64_t)0x20);
    r.field("transform_data_root_pos", (uint64_t)0x90);
    r.field("transform_data_root_quat", (uint64_t)0xA0);
    r.field("transform_data_matrix_stride", (uint64_t)0x30);
    r.field("unity_string_length", (uint64_t)0x10);
    r.field("unity_string_first_char", (uint64_t)0x14);
    r.field("klass_layout_name_ptr", (uint64_t)0x10);
    r.field("klass_layout_static_fields", (uint64_t)0xB8);
    r.field("gchandle_page_mask", (uint64_t)0xFFFFFFFFFFFFE000ULL);
    r.field("gchandle_type_off", (uint64_t)0x20);
    r.field("gchandle_size_off", (uint64_t)0x1C);
    r.field("gchandle_bitmap_off", (uint64_t)0x10);
    r.field("gchandle_slot_base", (uint64_t)0x28);
    r.field("layer_trees", (uint64_t)30);
    r.field("layer_grass", (uint64_t)25);
    r.field("layer_water", (uint64_t)4);
    r.field("layer_sky", (uint64_t)0);
    r.field("entity_name_chain_1", (uint64_t)0x10);
    r.field("entity_name_chain_2", (uint64_t)0x20);
    r.field("entity_name_chain_3", (uint64_t)0x50);
    r.blank();
}

void g_resolver::write_ciphers(g_report& r)
{
    if (m_bn.dec0.valid)
        r.cipher_block("bn_0", m_bn.dec0_addr - g_rt::base(), m_bn.dec0, m_bn.hv_off);
    if (m_bn.dec1.valid)
        r.cipher_block("bn_1", m_bn.dec1_addr - g_rt::base(), m_bn.dec1, m_bn.hv_off);
    if (m_fov.chain.valid)
        r.cipher_block("fov",  m_fov.fn_addr  - g_rt::base(), m_fov.chain);
    for (const auto& ec : m_extra_ciphers)
        if (ec.chain.valid)
            r.cipher_block(ec.tag, ec.fn_addr - g_rt::base(), ec.chain);
}

bool g_resolver::run(const char* out_path)
{
    g_dlog("[run] g_rt::setup()...");
    if (!g_rt::setup())
    {
        g_dlog("[run] g_rt::setup FAILED");
        std::printf("[sha] g_rt::setup failed\n");
        return false;
    }
    g_dlog("[run] g_rt::setup OK: base=0x%llX size=0x%X", g_rt::base(), g_rt::imgsize());

    std::printf("[sha] base=0x%llX size=0x%X\n", g_rt::base(), g_rt::imgsize());

    g_dlog("[run] gchandle_rva()...");
    m_gchandle_rva = g_mem::gchandle_rva();
    g_dlog("[run] gchandle_rva=0x%llX", m_gchandle_rva);
    std::printf("[sha] gchandle_rva=0x%llX\n", m_gchandle_rva);

    g_dlog("[run] resolve_item_klasses()...");
    __try { resolve_item_klasses(); g_dlog("[run] resolve_item_klasses OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] resolve_item_klasses CRASHED"); std::printf("[sha] item_klasses: ex\n"); }
    g_dlog("[run] resolve_console()...");
    __try { resolve_console();      g_dlog("[run] resolve_console OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] resolve_console CRASHED"); std::printf("[sha] console: ex\n"); }
    g_dlog("[run] resolve_bn()...");
    __try { resolve_bn();           g_dlog("[run] resolve_bn OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] resolve_bn CRASHED"); std::printf("[sha] bn: ex\n"); }
    g_dlog("[run] resolve_cam()...");
    __try { resolve_cam();          g_dlog("[run] resolve_cam OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] resolve_cam CRASHED"); std::printf("[sha] cam: ex\n"); }
    g_dlog("[run] resolve_fov()...");
    __try { resolve_fov();          g_dlog("[run] resolve_fov OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] resolve_fov CRASHED"); std::printf("[sha] fov: ex\n"); }
    g_dlog("[run] resolve_extra_ciphers()...");
    __try { resolve_extra_ciphers(); g_dlog("[run] resolve_extra_ciphers OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] resolve_extra_ciphers CRASHED"); std::printf("[sha] extra_ciphers: ex\n"); }
    g_dlog("[run] resolve_camera_icalls()...");
    __try { resolve_camera_icalls(); g_dlog("[run] resolve_camera_icalls OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] resolve_camera_icalls CRASHED"); std::printf("[sha] camera_icalls: ex\n"); }

    g_dlog("[run] opening report: %s", out_path);
    g_report rep;
    if (!rep.open(out_path))
    {
        std::printf("[sha] open failed: %s\n", out_path);
        return false;
    }

    rep.header();

    __try { write_bn(rep);        } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_cam(rep);       } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_bp(rep);        } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_bce(rep);       } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_entity(rep);    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_eyes(rep);      } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_movement(rep);  } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_item(rep);      } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_itemdef(rep);   } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_container(rep); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_inventory(rep); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_weapon(rep);    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_ciphers(rep);   } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_static_pointers(rep); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_native_camera(rep);   } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_camera_icalls(rep);   } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_tod_sky_instances(rep); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { write_unity_constants(rep); } __except(EXCEPTION_EXECUTE_HANDLER) {}

    // Dump ALL fields for key classes — gives ground truth for every offset
    auto dump_class = [&rep](const char* class_name) {
        auto k = g_rt::klass(class_name);
        if (!k) return;
        rep.section(class_name);
        void* it = nullptr;
        while (auto f = k->fields(&it)) {
            if (!f || (f->flags() & FIELD_ATTRIBUTE_STATIC)) continue;
            const char* fn = f->name();
            if (!fn) continue;
            char key[128];
            std::snprintf(key, sizeof(key), "  %s", fn);
            rep.field(key, f->offset());
        }
        rep.blank();
    };

    const char* dump_classes[] = {
        "BasePlayer", "BaseCombatEntity", "BaseEntity", "BaseNetworkable",
        "PlayerEyes", "PlayerInput", "PlayerModel", "PlayerInventory",
        "Item", "ItemDefinition", "ItemContainer",
        "BaseProjectile", "RecoilProperties", "HeldEntity",
        "BaseMovement", "Model", "MainCamera", "TOD_Sky",
        "Magazine", "WorldItem", "AttackEntity", "FlintStrikeWeapon",
        "ConsoleSystem", "ConVar_Graphics", "EffectNetwork",
        "UI_LoadingScreen", "MixerSnapshotManager",
        "BaseVehicle", "BaseNpc", "BuildingBlock", "Door",
        // Enhanced coverage
        "SkinnedMultiMesh", "ModelState", "PlayerWalkMovement",
        "TOD_Sky_Static", "TOD_AmbientParameters", "TOD_NightParameters",
        "TOD_CycleParameters", "BaseEntity", "ListComponent_Projectile_c",
        "PlayerModel", "ViewModel", "FlintStrikeWeapon",
        "BaseHelicopter", "CH47Helicopter", "HackableLockedCrate"
    };
    for (auto cn : dump_classes)
        __try { dump_class(cn); } __except(EXCEPTION_EXECUTE_HANDLER) {}

    g_dlog("[run] writing report (rep.close)...");
    rep.close();
    g_dlog("[run] report closed OK");

    g_dlog("[run] g_mat::append()...");
    __try { g_mat::append(out_path); g_dlog("[run] g_mat::append OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] g_mat::append CRASHED"); std::printf("[sha] mat: ex\n"); }

    g_dlog("[run] g_mesh::dump()...");
    __try { g_mesh::dump("C:\\rust_mesh.tri"); g_dlog("[run] g_mesh::dump OK"); } __except(EXCEPTION_EXECUTE_HANDLER) { g_dlog("[run] g_mesh::dump CRASHED"); std::printf("[sha] mesh: ex\n"); }

    g_dlog("[run] done -> %s", out_path);
    std::printf("[sha] done -> %s\n", out_path);
    return true;
}
