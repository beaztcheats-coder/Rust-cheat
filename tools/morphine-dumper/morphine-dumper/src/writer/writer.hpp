#pragma once

#include "../decryption/decryption.hpp"

#include <cstdio>
#include <cstdint>
#include <string>

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
	void write_decryption_unresolved( const char* name );

	void write_fov_unresolved( );
	void write_decrypt_fov( std::uint64_t fn_rva, const c_decryption::constants_t& c, bool source_is_encrypt );
	void write_encrypt_fov( std::uint64_t fn_rva, const c_decryption::constants_t& c, bool source_is_encrypt );

private:
	std::FILE* m_file = nullptr;
	std::uint32_t m_auto_fields = 0;
	std::uint32_t m_auto_probed_fields = 0;
	std::uint32_t m_pending_fields = 0;
	std::uint32_t m_nonzero_fields = 0;
	bool m_current_scope_is_struct = true;
	static std::string fmt_value( std::uint64_t v );
};
