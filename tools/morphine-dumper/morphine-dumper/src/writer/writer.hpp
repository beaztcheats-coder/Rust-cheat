#pragma once

#include "../decryption/decryption.hpp"

#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

class c_writer
{
public:
	bool open( const char* path );
	void close( );
	bool is_open( ) const { return m_file != nullptr; }

	void write_header( std::uint64_t game_base, std::uint32_t image_size );
	void write_global( const char* name, std::uint64_t value );
	void write_build( const std::string& build_id );

	void struct_begin( const char* name );
	void struct_field( const char* name, std::uint64_t value );
	void struct_field_u32( const char* name, std::uint64_t value );
	void struct_field_probed( const char* name, std::uint64_t value );
	void struct_field_pending( const char* name );
	void struct_field( const char* name, std::uint64_t value, const char* trailing );
	void struct_end( );

	void blank_line( );
	void write_generation_summary( );

	void write_decryption_fn(
		const char* name,
		std::uint64_t fn_rva,
		const c_decryption::constants_t& c,
		std::uint64_t hv_offset );
	void write_direct_decryption_fn(
		const char* name,
		std::uint64_t fn_rva,
		const c_decryption::constants_t& c );
	void write_encrypt_fn(
		const char* name,
		std::uint64_t fn_rva,
		const c_decryption::constants_t& c,
		std::uint64_t hv_offset );
	void write_direct_encrypt_fn(
		const char* name,
		std::uint64_t fn_rva,
		const c_decryption::constants_t& c );
	void write_decryption_unresolved( const char* name );

	void write_fov_unresolved( );
	void write_decrypt_fov( std::uint64_t fn_rva, const c_decryption::constants_t& c, bool source_is_encrypt );
	void write_encrypt_fov( std::uint64_t fn_rva, const c_decryption::constants_t& c, bool source_is_encrypt );

	void flush_json( const char* path, const char* build_id, std::uint64_t game_base, std::uint32_t image_size );
	void add_json_class_field( const char* class_name, const char* field_name, std::uint64_t offset );

private:
	std::FILE* m_file = nullptr;
	std::uint32_t m_auto_fields = 0;
	std::uint32_t m_auto_probed_fields = 0;
	std::uint32_t m_pending_fields = 0;
	std::uint32_t m_nonzero_fields = 0;
	bool m_current_scope_is_struct = true;
	static std::string fmt_value( std::uint64_t v );

	// JSON data is heap-allocated to avoid C2712 (SEH + object unwinding conflict)
	struct json_field_t { std::string ns; std::string name; std::uint64_t value; };
	struct json_op_t { int type; std::uint32_t value; };
	struct json_fn_t {
		std::string name;
		std::uint64_t fn_rva;
		std::vector<json_op_t> ops;
		std::string input_style;
		std::string return_style;
		std::uint64_t read_offset;
		bool is_encrypt;
		bool is_fov;
	};
	struct json_data_t {
		std::vector<json_field_t> fields;
		std::vector<json_fn_t> fns;
		std::string current_ns;
	};
	json_data_t* m_json = nullptr;

	static const char* op_type_name( int type );
	static const char* op_inverse_name( int type );
};
