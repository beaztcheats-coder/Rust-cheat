#pragma once

#include "../runtime/runtime.hpp"
#include "../scanner/scanner.hpp"
#include "../decryption/decryption.hpp"
#include "../verification/verification.hpp"
#include "../materials/materials.hpp"
#include "../writer/writer.hpp"

#include <string>
#include <vector>

class c_core
{
public:
	bool run( const char* output_path, bool dump_materials = false );

	struct model_bone_indices_t
	{
		std::int32_t pelvis = -1;
		std::int32_t l_hip = -1;
		std::int32_t l_knee = -1;
		std::int32_t l_foot = -1;
		std::int32_t r_hip = -1;
		std::int32_t r_knee = -1;
		std::int32_t r_foot = -1;
		std::int32_t spine2 = -1;
		std::int32_t spine4 = -1;
		std::int32_t l_clavicle = -1;
		std::int32_t l_upperarm = -1;
		std::int32_t l_forearm = -1;
		std::int32_t l_hand = -1;
		std::int32_t r_clavicle = -1;
		std::int32_t r_upperarm = -1;
		std::int32_t r_forearm = -1;
		std::int32_t r_hand = -1;
		std::int32_t neck = -1;
		std::int32_t head = -1;
		std::int32_t count = 0;
	};

private:
	struct bn_chain_t
	{
		std::uint64_t base_networkable_rva = 0;
		std::uint32_t static_fields        = 0xB8;
		std::uint32_t wrapper_class_ptr    = 0;
		std::uint32_t parent_static_fields = 0;
		std::uint32_t entities             = 0;
		std::uint64_t hv_offset            = 0x18;
		std::uintptr_t entity_list_wrapper_addr = 0;
		std::uintptr_t decrypt_0_addr      = 0;
		std::uintptr_t decrypt_1_addr      = 0;
		c_decryption::constants_t decrypt_0{ };
		c_decryption::constants_t decrypt_1{ };
	};

	struct camera_chain_t
	{
		std::uint64_t main_camera_c = 0;
		std::uint32_t camera_static = 0xB8;
		std::uint32_t camera_object = 0;
		std::uint32_t entity        = 0x10;
		std::uint32_t position      = 0;
		std::uint32_t view_matrix   = 0;
		std::uint32_t world_to_camera_matrix = 0;
		std::uint32_t projection_matrix = 0;
		std::uint32_t projection_layout = 0;
		std::uint32_t field_of_view = 0;
		std::uint32_t aspect        = 0;
		std::uint32_t near_clip     = 0;
		std::uint32_t far_clip      = 0;
		std::uint32_t culling_mask  = 0;
	};

	struct player_inventory_offsets_t
	{
		std::uint64_t main = 0;
		std::uint64_t belt = 0;
		std::uint64_t wear = 0;
	};

	struct item_offsets_t
	{
		std::uint64_t info = 0;
		std::uint64_t uid = 0;
		std::uint64_t amount = 0;
		std::uint64_t held_entity = 0;
	};

	bool resolve_bn_chain( );
	bool resolve_camera_chain( );
	bool resolve_console_system( );
	void resolve_cl_active_item_decryption( );
	void resolve_player_inventory_decryption( );
	void resolve_player_eyes_decryption( );
	void resolve_fov_encryption( );
	void resolve_build_id( );
	void resolve_model_bones( );
	item_offsets_t resolve_item_offsets( );

	void verify_bn_chain( );
	void verify_camera_chain( );
	void verify_all( );

	void resolve_item_classes( );

	il2cpp::il2cpp_class_t* m_item_class           = nullptr;
	il2cpp::il2cpp_class_t* m_item_id_class        = nullptr;
	il2cpp::il2cpp_class_t* m_item_container_class = nullptr;
	il2cpp::il2cpp_class_t* m_console_command_class = nullptr;

	void write_il2cpphandle( c_writer& w );
	void write_basenetworkable( c_writer& w );
	void write_camera( c_writer& w );
	void write_base_player( c_writer& w );
	void write_base_combat_entity( c_writer& w );
	void write_base_entity( c_writer& w );
	void write_item_container( c_writer& w );
	void write_itemdefinition( c_writer& w );
	void write_base_projectile( c_writer& w );
	void write_recoil_properties( c_writer& w );
	void write_flint_strike_weapon( c_writer& w );
	void write_world_item( c_writer& w );
	void write_item_icon( c_writer& w );
	void write_decryptions( c_writer& w );

	std::uint64_t                  m_gc_handles_rva = 0;
	bn_chain_t                     m_bn{ };
	camera_chain_t                 m_camera{ };
	player_inventory_offsets_t     m_player_inventory{ };
	item_offsets_t                 m_item_offsets{ };
	model_bone_indices_t           m_model_bones{ };
	std::vector<std::string>       m_model_bone_names{ };
	std::uintptr_t                 m_cl_active_item_fn_addr = 0;
	std::uint64_t                  m_cl_active_item_offset = 0;
	c_decryption::constants_t      m_cl_active_item_constants{ };
	bool                           m_cl_active_item_plain = false;
	std::uintptr_t                 m_player_inventory_fn_addr = 0;
	c_decryption::constants_t      m_player_inventory_constants{ };
	std::uintptr_t                 m_player_eyes_fn_addr = 0;
	c_decryption::constants_t      m_player_eyes_constants{ };
	std::uintptr_t                 m_fov_fn_addr = 0;
	c_decryption::constants_t      m_fov_constants{ };
	bool                           m_fov_constants_are_encrypt = false;
	bool                           m_bn_verified = false;
	bool                           m_camera_verified = false;
	std::string                    m_build_id{ };
};
