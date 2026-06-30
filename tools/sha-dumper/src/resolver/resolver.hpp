#pragma once

#include "../glue/glue.hpp"

void g_dlog(const char* fmt, ...);
#include "../mem/mem.hpp"
#include "../cipher/cipher.hpp"
#include "../report/report.hpp"
#include "../mat/mat.hpp"
#include "../mesh/mesh.hpp"

class g_resolver
{
public:
    bool run(const char* out_path);

private:
    struct bn_t
    {
        uint64_t        slot_rva      = 0;
        uint32_t        wrapper_off   = 0;
        uint32_t        parent_off    = 0;
        uint32_t        ents_off      = 0;
        uint64_t        hv_off        = 0x18;
        uintptr_t       dec0_addr     = 0;
        uintptr_t       dec1_addr     = 0;
        cipher_t::chain_t dec0{};
        cipher_t::chain_t dec1{};
    };

    struct cam_t
    {
        uint64_t rva     = 0;
        uint32_t obj_off = 0;
    };

    struct cam_icall_t
    {
        uint32_t field_of_view      = 0;
        uint32_t view_matrix        = 0;
        uint32_t projection_matrix  = 0;
        uint32_t culling_mask       = 0;
        uint32_t world_position     = 0;
        bool     resolved           = false;
    };

    struct cam_icall_debug_t
    {
        const char* source[6]    = {"none","none","none","none","none","none"};
        uint64_t    fn_addr[6]   = {};
        uint8_t     first_bytes[6][32] = {};
    };

    struct fov_t
    {
        uintptr_t         fn_addr = 0;
        cipher_t::chain_t chain{};
    };

    struct extra_cipher_t
    {
        uintptr_t         fn_addr = 0;
        cipher_t::chain_t chain{};
        const char*        tag     = nullptr;
    };

    struct bp_t
    {
        uint64_t cl_active_item   = 0;
        uint64_t player_eyes      = 0;
        uint64_t player_inventory = 0;
        uint64_t current_team     = 0;
        uint64_t base_movement    = 0;
        uint64_t player_model     = 0;
        uint64_t player_flags     = 0;
        uint64_t display_name     = 0;
        uint64_t player_input     = 0;
        uint64_t metabolism       = 0;
        uint64_t model_state      = 0;
        uint64_t local_rva        = 0;
    };

    struct bce_t
    {
        uint64_t lifestate  = 0;
        uint64_t model      = 0;
        uint64_t health     = 0;
        uint64_t max_health = 0;
    };

    struct entity_t
    {
        uint64_t flags     = 0;
        uint64_t net       = 0;
        uint64_t prefab_id = 0;
    };

    struct eyes_t
    {
        uint64_t body_rotation = 0;
        uint64_t eye_offset    = 0;
    };

    struct movement_t
    {
        uint64_t velocity = 0;
        uint64_t grounded = 0;
    };

    struct item_t
    {
        uint64_t info      = 0;
        uint64_t amount    = 0;
        uint64_t uid       = 0;
        uint64_t skin      = 0;
        uint64_t condition = 0;
        uint64_t max_cond  = 0;
        uint64_t position  = 0;
        uint64_t parent    = 0;
    };

    struct itemdef_t
    {
        uint64_t shortname = 0;
        uint64_t itemid    = 0;
        uint64_t category  = 0;
        uint64_t stackable = 0;
    };

    struct container_t
    {
        uint64_t item_list = 0;
        uint64_t capacity  = 0;
        uint64_t uid       = 0;
        uint64_t flags     = 0;
    };

    struct inventory_t
    {
        uint64_t belt = 0;
        uint64_t wear = 0;
    };

    struct weapon_t
    {
        uint64_t owner_item_uid = 0;
        uint64_t primary_mag    = 0;
        uint64_t recoil         = 0;
        uint64_t mag_contents   = 0;
        uint64_t mag_capacity   = 0;
    };

    bn_t        m_bn{};
    cam_t       m_cam{};
    cam_icall_t m_cam_icall{};
    cam_icall_debug_t m_cam_icall_debug{};
    fov_t       m_fov{};
    std::vector<extra_cipher_t> m_extra_ciphers{};
    bp_t        m_bp{};
    bce_t       m_bce{};
    entity_t    m_ent{};
    eyes_t      m_eyes{};
    movement_t  m_move{};
    item_t      m_item{};
    itemdef_t   m_idef{};
    container_t m_cont{};
    inventory_t m_inv{};
    weapon_t    m_weapon{};

    il2cpp::il2cpp_class_t* m_item_klass      = nullptr;
    il2cpp::il2cpp_class_t* m_item_id_klass   = nullptr;
    il2cpp::il2cpp_class_t* m_container_klass = nullptr;
    il2cpp::il2cpp_class_t* m_cmd_klass       = nullptr;

    uint64_t m_gchandle_rva = 0;

    void resolve_bn();
    void resolve_cam();
    void resolve_console();
    void resolve_fov();
    void resolve_extra_ciphers();
    void resolve_item_klasses();
    void resolve_camera_icalls();

    void write_bn      (g_report& r);
    void write_cam     (g_report& r);
    void write_bp      (g_report& r);
    void write_bce     (g_report& r);
    void write_entity  (g_report& r);
    void write_eyes    (g_report& r);
    void write_movement(g_report& r);
    void write_item    (g_report& r);
    void write_itemdef (g_report& r);
    void write_container(g_report& r);
    void write_inventory(g_report& r);
    void write_weapon  (g_report& r);
    void write_ciphers (g_report& r);
    void write_static_pointers(g_report& r);
    void write_native_camera(g_report& r);
    void write_camera_icalls(g_report& r);
    void write_tod_sky_instances(g_report& r);
    void write_unity_constants(g_report& r);

    static uint64_t foff(il2cpp::field_info_t* f)
    {
        return f ? f->offset() : 0;
    }

    static uint64_t foff(il2cpp::field_info_t* f, il2cpp::il2cpp_class_t* k)
    {
        return f ? f->offset() : 0;
    }
};
