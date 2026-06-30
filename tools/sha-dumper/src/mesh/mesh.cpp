#include "mesh.hpp"
#include "../glue/glue.hpp"

#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#pragma comment(lib, "shell32.lib")
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <fstream>

using find_all_fn = void*(*)(void*);
using get_obj_fn  = void*(*)(void*);
using get_vec3_fn = void(*)(gfx::vec3_t*, void*);
using get_quat_fn = void(*)(gfx::quat_t*, void*);

static find_all_fn s_find_all = nullptr;

struct Tri { gfx::vec3_t v0, v1, v2; };

struct AABB {
    gfx::vec3_t mn{1e30f, 1e30f, 1e30f};
    gfx::vec3_t mx{-1e30f, -1e30f, -1e30f};
    void expand(const gfx::vec3_t& p) {
        if (p.x < mn.x) mn.x = p.x;
        if (p.y < mn.y) mn.y = p.y;
        if (p.z < mn.z) mn.z = p.z;
        if (p.x > mx.x) mx.x = p.x;
        if (p.y > mx.y) mx.y = p.y;
        if (p.z > mx.z) mx.z = p.z;
    }
};

static gfx::vec3_t v3_add(gfx::vec3_t a, gfx::vec3_t b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
static gfx::vec3_t v3_mul(gfx::vec3_t a, float s) { return {a.x*s, a.y*s, a.z*s}; }
static gfx::vec3_t v3_cross(gfx::vec3_t a, gfx::vec3_t b) { return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x}; }

static gfx::vec3_t rotate_vec3(gfx::vec3_t v, gfx::quat_t q) {
    gfx::vec3_t t = v3_mul(v3_cross({q.x, q.y, q.z}, v), 2.0f);
    return v3_add(v3_add(v, v3_mul(t, q.w)), v3_cross({q.x, q.y, q.z}, t));
}

static gfx::vec3_t transform_point(gfx::vec3_t local, gfx::vec3_t pos, gfx::quat_t rot, gfx::vec3_t scale) {
    gfx::vec3_t scaled;
    scaled.x = local.x * scale.x; scaled.y = local.y * scale.y; scaled.z = local.z * scale.z;
    gfx::vec3_t rotated = rotate_vec3(scaled, rot);
    return v3_add(rotated, pos);
}

static char g_logPath[MAX_PATH] = "C:\\rust_mesh_dump.log";

static void mesh_log(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::printf("%s\n", buf);
    auto fp = std::fopen(g_logPath, "a");
    if (fp) { std::fprintf(fp, "%s\n", buf); std::fclose(fp); }
}

static void init_log_path() {
    char desktop[MAX_PATH]{};
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktop)))
        GetEnvironmentVariableA("USERPROFILE", desktop, MAX_PATH);
    std::snprintf(g_logPath, MAX_PATH, "%s\\rust_mesh_dump.log", desktop);
}

static int array_count(void* arr) {
    if (!arr || IsBadReadPtr(arr, 0x24)) return 0;
    return *(int32_t*)((uint8_t*)arr + 0x18);
}

static void* find_all_type(il2cpp::il2cpp_class_t* k) {
    if (!k) return nullptr;
    void* arr = nullptr;
    __try {
        auto t = k->type();
        if (!t) return nullptr;
        auto obj = il2cpp::type_get_object(t);
        if (!obj) return nullptr;
        if (!s_find_all || IsBadReadPtr((void*)s_find_all, 8)) return nullptr;
        arr = s_find_all(obj);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        mesh_log("[mesh] find_all_type exception");
        return nullptr;
    }
    return arr;
}

static bool dump_mesh_colliders(std::vector<Tri>& tris) {
    mesh_log("[mesh] dump_mesh_colliders: resolving classes...");

    il2cpp::il2cpp_class_t* mc_k = nullptr;
    il2cpp::il2cpp_class_t* mesh_k = nullptr;
    il2cpp::il2cpp_class_t* comp_k = nullptr;
    il2cpp::il2cpp_class_t* trans_k = nullptr;

    __try { mc_k = il2cpp::get_class_by_name("MeshCollider", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_class MeshCollider CRASHED"); return false; }
    if (!mc_k) { mesh_log("[mesh] MeshCollider class not found"); return false; }
    mesh_log("[mesh] MeshCollider class=%p", (void*)mc_k);

    __try { mesh_k = il2cpp::get_class_by_name("Mesh", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_class Mesh CRASHED"); return false; }
    if (!mesh_k) { mesh_log("[mesh] Mesh class not found"); return false; }

    __try { comp_k = il2cpp::get_class_by_name("Component", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_class Component CRASHED"); return false; }
    __try { trans_k = il2cpp::get_class_by_name("Transform", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_class Transform CRASHED"); return false; }
    if (!comp_k || !trans_k) { mesh_log("[mesh] Component/Transform class not found (comp=%p trans=%p)", (void*)comp_k, (void*)trans_k); return false; }

    mesh_log("[mesh] resolving methods...");

    il2cpp::method_info_t* sm_m = nullptr;
    il2cpp::method_info_t* tr_m = nullptr;
    il2cpp::method_info_t* pos_m = nullptr;
    il2cpp::method_info_t* rot_m = nullptr;
    il2cpp::method_info_t* scl_m = nullptr;
    il2cpp::method_info_t* vt_m = nullptr;
    il2cpp::method_info_t* tri_m = nullptr;

    __try { sm_m = il2cpp::get_method_by_name(mc_k, "get_sharedMesh", 0); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method sharedMesh CRASHED"); }
    __try { tr_m = il2cpp::get_method_by_name(comp_k, "get_transform", 0); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method transform CRASHED"); }
    __try { pos_m = il2cpp::get_method_by_name(trans_k, "get_position", 0); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method position CRASHED"); }
    __try { rot_m = il2cpp::get_method_by_name(trans_k, "get_rotation", 0); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method rotation CRASHED"); }
    __try { scl_m = il2cpp::get_method_by_name(trans_k, "get_localScale", 0); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method localScale CRASHED"); }
    __try { vt_m = il2cpp::get_method_by_name(mesh_k, "get_vertices", 0); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method vertices CRASHED"); }
    __try { tri_m = il2cpp::get_method_by_name(mesh_k, "get_triangles", 0); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method triangles CRASHED"); }

    if (!sm_m || !tr_m || !pos_m || !rot_m || !scl_m || !vt_m || !tri_m) {
        mesh_log("[mesh] method resolution failed (sm=%p tr=%p pos=%p rot=%p scl=%p vt=%p tri=%p)",
            (void*)sm_m, (void*)tr_m, (void*)pos_m, (void*)rot_m, (void*)scl_m, (void*)vt_m, (void*)tri_m);
        return false;
    }
    mesh_log("[mesh] all methods resolved OK");

    auto get_shared_mesh = (get_obj_fn)sm_m->get_fn_ptr<void*>();
    auto get_transform = (get_obj_fn)tr_m->get_fn_ptr<void*>();
    auto get_pos = (get_vec3_fn)pos_m->get_fn_ptr<void*>();
    auto get_rot = (get_quat_fn)rot_m->get_fn_ptr<void*>();
    auto get_scl = (get_vec3_fn)scl_m->get_fn_ptr<void*>();
    auto get_verts = (get_obj_fn)vt_m->get_fn_ptr<void*>();
    auto get_tris = (get_obj_fn)tri_m->get_fn_ptr<void*>();

    mesh_log("[mesh] calling FindObjectsOfTypeAll(MeshCollider)...");
    void* arr = find_all_type(mc_k);
    int count = array_count(arr);
    mesh_log("[mesh] MeshCollider array=%p count=%d", arr, count);
    if (count <= 0 || count > 500000) {
        if (count == 0) mesh_log("[mesh] no MeshColliders — are you in-game with world loaded?");
        return false;
    }

    uint64_t* elems = (uint64_t*)((uint8_t*)arr + 0x20);
    int meshCount = 0;
    int skipCount = 0;

    for (int i = 0; i < count; i++) {
        auto obj = (void*)elems[i];
        if (!obj || IsBadReadPtr(obj, 0x20)) { skipCount++; continue; }

        void* mesh = nullptr;
        __try { mesh = get_shared_mesh(obj); } __except(EXCEPTION_EXECUTE_HANDLER) { skipCount++; continue; }
        if (!mesh || IsBadReadPtr(mesh, 0x20)) { skipCount++; continue; }

        void* trans = nullptr;
        __try { trans = get_transform(obj); } __except(EXCEPTION_EXECUTE_HANDLER) { skipCount++; continue; }
        if (!trans || IsBadReadPtr(trans, 0x20)) { skipCount++; continue; }

        gfx::vec3_t pos{0,0,0}, scl{1,1,1};
        gfx::quat_t rot{0,0,0,1};
        __try { get_pos(&pos, trans); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        __try { get_rot(&rot, trans); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        __try { get_scl(&scl, trans); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        void* varr = nullptr;
        __try { varr = get_verts(mesh); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        void* iarr = nullptr;
        __try { iarr = get_tris(mesh); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        if (!varr || !iarr) { skipCount++; continue; }
        if (IsBadReadPtr(varr, 0x24) || IsBadReadPtr(iarr, 0x24)) { skipCount++; continue; }

        int vcount = *(int32_t*)((uint8_t*)varr + 0x18);
        int icount = *(int32_t*)((uint8_t*)iarr + 0x18);
        if (vcount <= 0 || icount < 3 || vcount > 5000000 || icount > 15000000) { skipCount++; continue; }

        if (IsBadReadPtr((gfx::vec3_t*)((uint8_t*)varr + 0x20), vcount * sizeof(gfx::vec3_t))) { skipCount++; continue; }
        if (IsBadReadPtr((int32_t*)((uint8_t*)iarr + 0x20), icount * sizeof(int32_t))) { skipCount++; continue; }

        gfx::vec3_t* verts = (gfx::vec3_t*)((uint8_t*)varr + 0x20);
        int32_t* idxs = (int32_t*)((uint8_t*)iarr + 0x20);

        int triBefore = (int)tris.size();
        __try {
            for (int t = 0; t + 2 < icount; t += 3) {
                int i0 = idxs[t], i1 = idxs[t+1], i2 = idxs[t+2];
                if (i0 < 0 || i0 >= vcount || i1 < 0 || i1 >= vcount || i2 < 0 || i2 >= vcount) continue;
                Tri tri;
                tri.v0 = transform_point(verts[i0], pos, rot, scl);
                tri.v1 = transform_point(verts[i1], pos, rot, scl);
                tri.v2 = transform_point(verts[i2], pos, rot, scl);
                tris.push_back(tri);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            tris.resize(triBefore);
            skipCount++;
            continue;
        }
        int newTriCount = (int)tris.size() - triBefore;
        if (newTriCount < 5) {
            tris.resize(triBefore);
            skipCount++;
            continue;
        }
        AABB meshBox;
        for (int t = triBefore; t < (int)tris.size(); ++t) {
            meshBox.expand(tris[t].v0);
            meshBox.expand(tris[t].v1);
            meshBox.expand(tris[t].v2);
        }
        float extX = meshBox.mx.x - meshBox.mn.x;
        float extY = meshBox.mx.y - meshBox.mn.y;
        float extZ = meshBox.mx.z - meshBox.mn.z;
        if (extX < 0.5f && extY < 0.5f && extZ < 0.5f) {
            tris.resize(triBefore);
            skipCount++;
            continue;
        }
        meshCount++;
        if (meshCount % 100 == 0) mesh_log("[mesh] progress: %d colliders processed, %zu tris so far", meshCount, tris.size());
    }

    mesh_log("[mesh] done: %d MeshColliders dumped, %d skipped, %zu triangles total", meshCount, skipCount, tris.size());
    return meshCount > 0;
}

static bool dump_box_colliders(std::vector<Tri>& tris) {
    mesh_log("[mesh] dump_box_colliders: starting...");

    il2cpp::il2cpp_class_t* bc_k = nullptr;
    __try { bc_k = il2cpp::get_class_by_name("BoxCollider", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_class BoxCollider CRASHED"); return false; }
    if (!bc_k) { mesh_log("[mesh] BoxCollider class not found"); return false; }

    il2cpp::il2cpp_class_t* comp_k = nullptr;
    il2cpp::il2cpp_class_t* trans_k = nullptr;
    __try { comp_k = il2cpp::get_class_by_name("Component", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { trans_k = il2cpp::get_class_by_name("Transform", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    if (!comp_k || !trans_k) { mesh_log("[mesh] BoxCollider: Component/Transform not found"); return false; }

    il2cpp::method_info_t* tr_m = nullptr;
    il2cpp::method_info_t* pos_m = nullptr;
    il2cpp::method_info_t* rot_m = nullptr;
    il2cpp::method_info_t* scl_m = nullptr;
    __try { tr_m = il2cpp::get_method_by_name(comp_k, "get_transform", 0); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { pos_m = il2cpp::get_method_by_name(trans_k, "get_position", 0); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { rot_m = il2cpp::get_method_by_name(trans_k, "get_rotation", 0); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { scl_m = il2cpp::get_method_by_name(trans_k, "get_localScale", 0); } __except(EXCEPTION_EXECUTE_HANDLER) {}

    il2cpp::method_info_t* center_m = nullptr;
    il2cpp::method_info_t* size_m = nullptr;
    __try { center_m = il2cpp::get_method_by_name(bc_k, "get_center", 0); } __except(EXCEPTION_EXECUTE_HANDLER) {}
    __try { size_m = il2cpp::get_method_by_name(bc_k, "get_size", 0); } __except(EXCEPTION_EXECUTE_HANDLER) {}

    // Diagnostic: dump ALL method names if resolution failed
    if (!center_m || !size_m) {
        mesh_log("[mesh] BoxCollider method resolution failed (get_center=%p get_size=%p) — dumping all methods:",
            (void*)center_m, (void*)size_m);
        il2cpp::il2cpp_class_t* cur = bc_k;
        int depth = 0;
        while (cur && depth < 10) {
            const char* cn = cur->name();
            mesh_log("[mesh]   [depth=%d] class='%s'", depth, cn ? cn : "(null)");
            void* miter = nullptr;
            int methodCount = 0;
            while (auto* method = cur->methods(&miter)) {
                const char* mn = method->name();
                mesh_log("[mesh]     method: name='%s'", mn ? mn : "(null)");
                methodCount++;
            }
            mesh_log("[mesh]     -> %d methods in this class", methodCount);
            cur = cur->parent();
            depth++;
        }
    }

    if (!tr_m || !pos_m || !rot_m || !scl_m || !center_m || !size_m) {
        mesh_log("[mesh] BoxCollider resolution failed (tr=%p pos=%p rot=%p scl=%p center=%p size=%p) — see method dump above",
            (void*)tr_m, (void*)pos_m, (void*)rot_m, (void*)scl_m, (void*)center_m, (void*)size_m);
        return false;
    }

    auto get_transform = (get_obj_fn)tr_m->get_fn_ptr<void*>();
    auto get_pos = (get_vec3_fn)pos_m->get_fn_ptr<void*>();
    auto get_rot = (get_quat_fn)rot_m->get_fn_ptr<void*>();
    auto get_scl = (get_vec3_fn)scl_m->get_fn_ptr<void*>();
    auto get_center = (get_vec3_fn)center_m->get_fn_ptr<void*>();
    auto get_size = (get_vec3_fn)size_m->get_fn_ptr<void*>();
    mesh_log("[mesh] BoxCollider methods resolved (get_center=%p get_size=%p)", (void*)get_center, (void*)get_size);

    void* arr = find_all_type(bc_k);
    int count = array_count(arr);
    mesh_log("[mesh] BoxCollider array=%p count=%d", arr, count);
    if (count <= 0 || count > 500000) return false;

    uint64_t* elems = (uint64_t*)((uint8_t*)arr + 0x20);
    int boxCount = 0;

    for (int i = 0; i < count; i++) {
        auto obj = (void*)elems[i];
        if (!obj || IsBadReadPtr(obj, 0x40)) continue;

        gfx::vec3_t center{0,0,0}, size{0,0,0};
        __try {
            get_center(&center, obj);
            get_size(&size, obj);
        } __except(EXCEPTION_EXECUTE_HANDLER) { continue; }

        if (size.x <= 0 || size.y <= 0 || size.z <= 0) continue;
        if (size.x > 1000 || size.y > 1000 || size.z > 1000) continue;

        void* trans = nullptr;
        __try { trans = get_transform(obj); } __except(EXCEPTION_EXECUTE_HANDLER) { continue; }
        if (!trans || IsBadReadPtr(trans, 0x20)) continue;

        gfx::vec3_t pos{0,0,0}, scl{1,1,1};
        gfx::quat_t rot{0,0,0,1};
        __try { get_pos(&pos, trans); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        __try { get_rot(&rot, trans); } __except(EXCEPTION_EXECUTE_HANDLER) {}
        __try { get_scl(&scl, trans); } __except(EXCEPTION_EXECUTE_HANDLER) {}

        float hx = size.x * 0.5f, hy = size.y * 0.5f, hz = size.z * 0.5f;
        gfx::vec3_t corners[8] = {
            {center.x-hx, center.y-hy, center.z-hz},
            {center.x+hx, center.y-hy, center.z-hz},
            {center.x+hx, center.y+hy, center.z-hz},
            {center.x-hx, center.y+hy, center.z-hz},
            {center.x-hx, center.y-hy, center.z+hz},
            {center.x+hx, center.y-hy, center.z+hz},
            {center.x+hx, center.y+hy, center.z+hz},
            {center.x-hx, center.y+hy, center.z+hz}
        };

        for (auto& c : corners)
            c = transform_point(c, pos, rot, scl);

        int faces[12][3] = {
            {0,1,2},{0,2,3}, {5,4,7},{5,7,6},
            {4,0,3},{4,3,7}, {1,5,6},{1,6,2},
            {3,2,6},{3,6,7}, {4,5,1},{4,1,0}
        };

        for (int f = 0; f < 12; f++) {
            Tri tri;
            tri.v0 = corners[faces[f][0]];
            tri.v1 = corners[faces[f][1]];
            tri.v2 = corners[faces[f][2]];
            tris.push_back(tri);
        }
        boxCount++;
    }

    mesh_log("[mesh] done: %d BoxColliders dumped", boxCount);
    return boxCount > 0;
}

static bool init_find_all() {
    mesh_log("[mesh] init_find_all: resolving FindObjectsOfTypeAll...");
    if (!s_find_all) {
        il2cpp::il2cpp_class_t* res = nullptr;
        __try { res = il2cpp::get_class_by_name("Resources", "UnityEngine"); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_class Resources CRASHED"); return false; }
        if (res) {
            mesh_log("[mesh] Resources class=%p, getting method...", (void*)res);
            il2cpp::method_info_t* m = nullptr;
            __try { m = il2cpp::get_method_by_name(res, "FindObjectsOfTypeAll", 1); } __except(EXCEPTION_EXECUTE_HANDLER) { mesh_log("[mesh] get_method FindObjectsOfTypeAll CRASHED"); }
            if (m) {
                s_find_all = (find_all_fn)m->get_fn_ptr<void*>();
                mesh_log("[mesh] FindObjectsOfTypeAll fn ptr=%p", (void*)s_find_all);
            }
        }
    }
    if (!s_find_all) {
        mesh_log("[mesh] trying resolve_icall fallback...");
        __try {
            s_find_all = (find_all_fn)il2cpp::resolve_icall("UnityEngine.Resources::FindObjectsOfTypeAll(System.Type)");
            mesh_log("[mesh] resolve_icall result=%p", (void*)s_find_all);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            mesh_log("[mesh] resolve_icall CRASHED");
        }
    }
    bool ok = s_find_all != nullptr;
    mesh_log("[mesh] init_find_all result=%d", ok);
    return ok;
}

bool g_mesh::dump(const char* path) {
    init_log_path();
    mesh_log("========== mesh dump starting ==========");

    bool fa = init_find_all();
    if (!fa) { mesh_log("[mesh] ABORT: FindObjectsOfTypeAll not resolved"); return false; }

    std::vector<Tri> tris;
    tris.reserve(500000);

    dump_mesh_colliders(tris);
    dump_box_colliders(tris);

    if (tris.empty()) {
        mesh_log("[mesh] ABORT: no triangles collected — were you in-game (world loaded)?");
        return false;
    }

    mesh_log("[mesh] writing %zu triangles to %s...", tris.size(), path);
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        mesh_log("[mesh] cannot open %s for writing — trying Desktop fallback", path);
        char desktop[MAX_PATH]{};
        if (FAILED(SHGetFolderPathA(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktop)))
            GetEnvironmentVariableA("USERPROFILE", desktop, MAX_PATH);
        char fallback[MAX_PATH]{};
        std::snprintf(fallback, MAX_PATH, "%s\\rust_mesh.tri", desktop);
        out.open(fallback, std::ios::binary);
        if (!out.is_open()) {
            mesh_log("[mesh] ABORT: cannot open %s either", fallback);
            return false;
        }
        path = fallback;
        mesh_log("[mesh] writing to Desktop fallback: %s", path);
    }
    out.write(reinterpret_cast<const char*>(tris.data()), (std::streamsize)(tris.size() * sizeof(Tri)));
    out.close();

    mesh_log("[mesh] SUCCESS: wrote %zu triangles (%zu bytes) to %s", tris.size(), tris.size() * sizeof(Tri), path);
    return true;
}
