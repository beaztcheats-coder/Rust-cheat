#include "writer.hpp"

#include <ctime>
#include <cstring>

const char* c_writer::op_type_name( int type )
{
	switch ( type )
	{
	case c_decryption::op_t::op_add: return "add";
	case c_decryption::op_t::op_xor: return "xor";
	case c_decryption::op_t::op_rol: return "rol";
	case c_decryption::op_t::op_sub: return "sub";
	default: return "unknown";
	}
}

const char* c_writer::op_inverse_name( int type )
{
	switch ( type )
	{
	case c_decryption::op_t::op_add: return "sub";
	case c_decryption::op_t::op_sub: return "add";
	case c_decryption::op_t::op_xor: return "xor";
	case c_decryption::op_t::op_rol: return "ror";
	default: return "unknown";
	}
}

bool c_writer::open( const char* path )
{
	m_file = std::fopen( path, "w" );
	if ( m_file && !m_json )
		m_json = new json_data_t( );
	return m_file != nullptr;
}

void c_writer::close( )
{
	if ( m_file )
	{
		std::fclose( m_file );
		m_file = nullptr;
	}
	if ( m_json )
	{
		delete m_json;
		m_json = nullptr;
	}
}

std::string c_writer::fmt_value( std::uint64_t v )
{
	char buf[ 32 ];
	if ( v >= 0x1000 )
		std::snprintf( buf, sizeof( buf ), "0x%llX", v );
	else
		std::snprintf( buf, sizeof( buf ), "0x%llx", v );
	return buf;
}

void c_writer::write_header( std::uint64_t game_base, std::uint32_t image_size )
{
	if ( !m_file ) return;

	std::time_t now = std::time( nullptr ) + ( 3 * 60 * 60 );
	std::tm utc_plus_3{ };
	gmtime_s( &utc_plus_3, &now );

	char timestamp[ 64 ]{ };
	std::strftime( timestamp, sizeof( timestamp ), "%Y-%m-%d %I:%M:%S %p", &utc_plus_3 );

	std::fprintf( m_file, "// validict on discord\n" );
	std::fprintf( m_file, "// game_base @ runtime: 0x%llX  (image size 0x%X)\n", game_base, image_size );
	std::fprintf( m_file, "// Dump generated on: %s UTC+3\n", timestamp );
	std::fprintf( m_file, "\n" );
	std::fprintf( m_file, "#include <cstdint>\n" );
	std::fprintf( m_file, "#include <string>\n" );
	std::fprintf( m_file, "\n" );
}

void c_writer::write_global( const char* name, std::uint64_t value )
{
	if ( !m_file ) return;
	m_json->fields.push_back( { "globals", name, value } );
	std::fprintf( m_file, "inline static constexpr uintptr_t %s = %s;\n",
		name, fmt_value( value ).c_str( ) );
}

void c_writer::write_build( const std::string& build_id )
{
	if ( !m_file ) return;
	std::fprintf( m_file, "inline std::string Build = \"%s\";\n", build_id.c_str( ) );
}

void c_writer::struct_begin( const char* name )
{
	if ( !m_file ) return;
	m_json->current_ns = name;
	m_current_scope_is_struct =
		!std::strcmp( name, "il2cpp_api" ) ||
		!std::strcmp( name, "gc_handles" ) ||
		!std::strcmp( name, "klass_rvas" );
	std::fprintf( m_file, "%s %s\n{\n", m_current_scope_is_struct ? "struct" : "namespace", name );
}

void c_writer::struct_field( const char* name, std::uint64_t value )
{
	if ( !m_file ) return;
	m_json->fields.push_back( { m_json->current_ns, name, value } );
	++m_auto_fields;
	if ( value )
		++m_nonzero_fields;
	else
		++m_pending_fields;
	if ( m_current_scope_is_struct )
		std::fprintf( m_file, "\tinline static constexpr uintptr_t %s = %s;\n",
			name, fmt_value( value ).c_str( ) );
	else
		std::fprintf( m_file, "\tconstexpr std::uintptr_t %s = %s;\n",
			name, fmt_value( value ).c_str( ) );
}

void c_writer::struct_field_u32( const char* name, std::uint64_t value )
{
	if ( !m_file ) return;
	m_json->fields.push_back( { m_json->current_ns, name, value } );
	++m_auto_fields;
	if ( value )
		++m_nonzero_fields;
	else
		++m_pending_fields;
	if ( m_current_scope_is_struct )
		std::fprintf( m_file, "\tinline static constexpr std::uint32_t %s = %s;\n",
			name, fmt_value( value ).c_str( ) );
	else
		std::fprintf( m_file, "\tconstexpr std::uint32_t %s = %s;\n",
			name, fmt_value( value ).c_str( ) );
}

void c_writer::struct_field_probed( const char* name, std::uint64_t value )
{
	if ( !m_file ) return;
	m_json->fields.push_back( { m_json->current_ns, name, value } );
	++m_auto_probed_fields;
	if ( value )
		++m_nonzero_fields;
	if ( m_current_scope_is_struct )
		std::fprintf( m_file, "\tinline static constexpr uintptr_t %s = %s;\n",
			name, fmt_value( value ).c_str( ) );
	else
		std::fprintf( m_file, "\tconstexpr std::uintptr_t %s = %s;\n",
			name, fmt_value( value ).c_str( ) );
}

void c_writer::struct_field_pending( const char* name )
{
	if ( !m_file ) return;
	m_json->fields.push_back( { m_json->current_ns, name, 0 } );
	++m_pending_fields;
	if ( m_current_scope_is_struct )
		std::fprintf( m_file, "\tinline static constexpr uintptr_t %s = 0x0;\n", name );
	else
		std::fprintf( m_file, "\tconstexpr std::uintptr_t %s = 0x0;\n", name );
}

void c_writer::struct_field( const char* name, std::uint64_t value, const char* trailing )
{
	if ( !m_file ) return;
	m_json->fields.push_back( { m_json->current_ns, name, value } );
	++m_auto_fields;
	if ( value )
		++m_nonzero_fields;
	else
		++m_pending_fields;
	if ( m_current_scope_is_struct )
		std::fprintf( m_file, "\tinline static constexpr uintptr_t %s = %s; //%s\n",
			name, fmt_value( value ).c_str( ), trailing );
	else
		std::fprintf( m_file, "\tconstexpr std::uintptr_t %s = %s;\n",
			name, fmt_value( value ).c_str( ) );
}

void c_writer::struct_end( )
{
	if ( !m_file ) return;
	std::fprintf( m_file, "%s\n", m_current_scope_is_struct ? "};" : "}" );
}

void c_writer::blank_line( )
{
	if ( !m_file ) return;
	std::fputc( '\n', m_file );
}

void c_writer::write_generation_summary( )
{
	if ( !m_file ) return;
	std::fprintf( m_file, "\n// ---- generation summary (per-tag provenance) ----\n" );
	std::fprintf( m_file, "// AUTO          : %u\n", m_auto_fields );
	std::fprintf( m_file, "// AUTO_PROBED   : %u\n", m_auto_probed_fields );
	std::fprintf( m_file, "// BRIDGE_RAW    : 0    (manual bridge - should fall to 0)\n" );
	std::fprintf( m_file, "// PENDING_B     : 0\n" );
	std::fprintf( m_file, "// PENDING_BONE  : %u\n", m_pending_fields );
	std::fprintf( m_file, "// TOTAL non-zero: %u\n", m_nonzero_fields );
	std::fprintf( m_file, "// AUTO offsets:    %u\n", m_nonzero_fields );
	std::fprintf( m_file, "// PENDING offsets: %u\n", m_pending_fields );
}

void c_writer::write_decryption_fn(
	const char* name,
	std::uint64_t fn_rva,
	const c_decryption::constants_t& c,
	std::uint64_t hv_offset )
{
	if ( !m_file ) return;

	json_fn_t f; f.name = name; f.fn_rva = fn_rva;
	for ( int i = 0; i < c.op_count; i++ ) f.ops.push_back( { c.ops[ i ].type, c.ops[ i ].value } );
	f.input_style = "pointer"; f.return_style = "handle"; f.read_offset = hv_offset;
	f.is_encrypt = false; f.is_fov = false; m_json->fns.push_back( f );

	std::fprintf( m_file, "// fn_rva = 0x%llX\n", fn_rva );
	std::fprintf( m_file, "uintptr_t decryption::%s(uint64_t a1)\n{\n", name );
	std::fprintf( m_file, "\tstd::uintptr_t rax = driver.read<std::uintptr_t>(a1 + 0x%llX);\n", hv_offset );
	std::fprintf( m_file, "\tstd::uint32_t* rdx = (std::uint32_t*)&rax;\n" );
	std::fprintf( m_file, "\tstd::uint32_t r8d = 0x2;\n" );
	std::fprintf( m_file, "\tstd::uint32_t eax, ecx;\n" );
	std::fprintf( m_file, "\tdo {\n" );
	std::fprintf( m_file, "\t\tecx = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\teax = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\trdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);\n" );

	for ( int i = 0; i < c.op_count; i++ )
	{
		const auto& op = c.ops[ i ];
		switch ( op.type )
		{
		case c_decryption::op_t::op_add:
			std::fprintf( m_file, "\t\tecx = ecx + 0x%X;\n", op.value );
			break;
		case c_decryption::op_t::op_sub:
			std::fprintf( m_file, "\t\tecx = ecx - 0x%X;\n", op.value );
			break;
		case c_decryption::op_t::op_xor:
			std::fprintf( m_file, "\t\tecx = ecx ^ 0x%X;\n", op.value );
			break;
		case c_decryption::op_t::op_rol:
			std::fprintf( m_file, "\t\teax = ecx;\n" );
			std::fprintf( m_file, "\t\tecx = ecx << 0x%X;\n", op.value );
			std::fprintf( m_file, "\t\teax = eax >> 0x%X;\n", 32 - op.value );
			std::fprintf( m_file, "\t\tecx = ecx | eax;\n" );
			break;
		}
	}

	std::fprintf( m_file, "\t\t*((std::uint32_t*)rdx - 1) = ecx;\n" );
	std::fprintf( m_file, "\t\t--r8d;\n" );
	std::fprintf( m_file, "\t} while (r8d);\n" );
	std::fprintf( m_file, "\treturn il2cpp_get_handle(rax);\n" );
	std::fprintf( m_file, "}\n\n" );
}

static void write_u32_ops( std::FILE* file, const char* var, const c_decryption::constants_t& c, bool inverse )
{
	int begin = inverse ? c.op_count - 1 : 0;
	int end = inverse ? -1 : c.op_count;
	int step = inverse ? -1 : 1;

	for ( int i = begin; i != end; i += step )
	{
		auto type = c.ops[ i ].type;
		auto value = c.ops[ i ].value;

		if ( inverse )
		{
			if ( type == c_decryption::op_t::op_add )
				type = c_decryption::op_t::op_sub;
			else if ( type == c_decryption::op_t::op_sub )
				type = c_decryption::op_t::op_add;
		}

		switch ( type )
		{
		case c_decryption::op_t::op_add:
			std::fprintf( file, "\t%s += 0x%X;\n", var, value );
			break;
		case c_decryption::op_t::op_sub:
			std::fprintf( file, "\t%s -= 0x%X;\n", var, value );
			break;
		case c_decryption::op_t::op_xor:
			std::fprintf( file, "\t%s ^= 0x%X;\n", var, value );
			break;
		case c_decryption::op_t::op_rol:
			if ( inverse )
				std::fprintf( file, "\t%s = (%s >> 0x%X) | (%s << 0x%X);\n",
					var, var, value, var, 32 - value );
			else
				std::fprintf( file, "\t%s = (%s << 0x%X) | (%s >> 0x%X);\n",
					var, var, value, var, 32 - value );
			break;
		}
	}
}

void c_writer::write_direct_decryption_fn(
	const char* name,
	std::uint64_t fn_rva,
	const c_decryption::constants_t& c )
{
	if ( !m_file ) return;

	json_fn_t f; f.name = name; f.fn_rva = fn_rva;
	for ( int i = 0; i < c.op_count; i++ ) f.ops.push_back( { c.ops[ i ].type, c.ops[ i ].value } );
	f.input_style = "raw"; f.return_style = "raw"; f.read_offset = 0;
	f.is_encrypt = false; f.is_fov = false; m_json->fns.push_back( f );

	std::fprintf( m_file, "// fn_rva = 0x%llX\n", fn_rva );
	std::fprintf( m_file, "uintptr_t decryption::%s(uint64_t a1)\n{\n", name );
	std::fprintf( m_file, "\tstd::uint32_t* rdx = (std::uint32_t*)&a1;\n" );
	std::fprintf( m_file, "\tstd::uint32_t r9d = 0x2;\n" );
	std::fprintf( m_file, "\tstd::uint32_t eax, edx;\n" );
	std::fprintf( m_file, "\tdo {\n" );
	std::fprintf( m_file, "\t\tedx = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\teax = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\trdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);\n" );
	for ( int i = 0; i < c.op_count; i++ )
	{
		const auto& op = c.ops[ i ];
		switch ( op.type )
		{
		case c_decryption::op_t::op_add:
			std::fprintf( m_file, "\t\tedx = edx + 0x%X;\n", op.value );
			break;
		case c_decryption::op_t::op_sub:
			std::fprintf( m_file, "\t\tedx = edx - 0x%X;\n", op.value );
			break;
		case c_decryption::op_t::op_xor:
			std::fprintf( m_file, "\t\tedx = edx ^ 0x%X;\n", op.value );
			break;
		case c_decryption::op_t::op_rol:
			std::fprintf( m_file, "\t\tedx = (edx << 0x%X) | (edx >> 0x%X);\n",
				op.value, 32 - op.value );
			break;
		}
	}
	std::fprintf( m_file, "\t\t*((std::uint32_t*)rdx - 1) = edx;\n" );
	std::fprintf( m_file, "\t\t--r9d;\n" );
	std::fprintf( m_file, "\t} while (r9d);\n" );
	std::fprintf( m_file, "\treturn a1;\n" );
	std::fprintf( m_file, "}\n\n" );
}

void c_writer::write_encrypt_fn(
	const char* name,
	std::uint64_t fn_rva,
	const c_decryption::constants_t& c,
	std::uint64_t hv_offset )
{
	if ( !m_file ) return;

	json_fn_t f; f.name = name; f.fn_rva = fn_rva;
	for ( int i = 0; i < c.op_count; i++ ) f.ops.push_back( { c.ops[ i ].type, c.ops[ i ].value } );
	f.input_style = "raw"; f.return_style = "raw"; f.read_offset = hv_offset;
	f.is_encrypt = true; f.is_fov = false; m_json->fns.push_back( f );

	std::fprintf( m_file, "// fn_rva = 0x%llX (encrypt, auto-generated inverse)\n", fn_rva );
	std::fprintf( m_file, "uintptr_t encryption::%s(uint64_t a1)\n{\n", name );
	std::fprintf( m_file, "\tstd::uintptr_t rax = a1;\n" );
	std::fprintf( m_file, "\tstd::uint32_t* rdx = (std::uint32_t*)&rax;\n" );
	std::fprintf( m_file, "\tstd::uint32_t r8d = 0x2;\n" );
	std::fprintf( m_file, "\tstd::uint32_t eax, ecx;\n" );
	std::fprintf( m_file, "\tdo {\n" );
	std::fprintf( m_file, "\t\tecx = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\teax = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\trdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);\n" );

	write_u32_ops( m_file, "ecx", c, true );

	std::fprintf( m_file, "\t\t*((std::uint32_t*)rdx - 1) = ecx;\n" );
	std::fprintf( m_file, "\t\t--r8d;\n" );
	std::fprintf( m_file, "\t} while (r8d);\n" );
	std::fprintf( m_file, "\tdriver.write<std::uintptr_t>(a1 + 0x%llX, rax);\n", hv_offset );
	std::fprintf( m_file, "\treturn rax;\n" );
	std::fprintf( m_file, "}\n\n" );
}

void c_writer::write_direct_encrypt_fn(
	const char* name,
	std::uint64_t fn_rva,
	const c_decryption::constants_t& c )
{
	if ( !m_file ) return;

	json_fn_t f; f.name = name; f.fn_rva = fn_rva;
	for ( int i = 0; i < c.op_count; i++ ) f.ops.push_back( { c.ops[ i ].type, c.ops[ i ].value } );
	f.input_style = "raw"; f.return_style = "raw"; f.read_offset = 0;
	f.is_encrypt = true; f.is_fov = false; m_json->fns.push_back( f );

	std::fprintf( m_file, "// fn_rva = 0x%llX (encrypt, auto-generated inverse)\n", fn_rva );
	std::fprintf( m_file, "uintptr_t encryption::%s(uint64_t a1)\n{\n", name );
	std::fprintf( m_file, "\tstd::uint32_t* rdx = (std::uint32_t*)&a1;\n" );
	std::fprintf( m_file, "\tstd::uint32_t r9d = 0x2;\n" );
	std::fprintf( m_file, "\tstd::uint32_t eax, edx;\n" );
	std::fprintf( m_file, "\tdo {\n" );
	std::fprintf( m_file, "\t\tedx = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\teax = *(std::uint32_t*)(rdx);\n" );
	std::fprintf( m_file, "\t\trdx = (std::uint32_t*)((std::uint8_t*)rdx + 0x4);\n" );

	write_u32_ops( m_file, "edx", c, true );

	std::fprintf( m_file, "\t\t*((std::uint32_t*)rdx - 1) = edx;\n" );
	std::fprintf( m_file, "\t\t--r9d;\n" );
	std::fprintf( m_file, "\t} while (r9d);\n" );
	std::fprintf( m_file, "\treturn a1;\n" );
	std::fprintf( m_file, "}\n\n" );
}

void c_writer::write_decryption_unresolved( const char* name )
{
	if ( !m_file ) return;
	std::fprintf( m_file, "// %s decryption unresolved\n", name );
}

void c_writer::write_fov_unresolved( )
{
	if ( !m_file ) return;

	std::fprintf( m_file, "// fov decryption/encryption unresolved\n" );
}

void c_writer::write_decrypt_fov( std::uint64_t fn_rva, const c_decryption::constants_t& c, bool source_is_encrypt )
{
	if ( !m_file ) return;

	json_fn_t f; f.name = "decrypt_fov"; f.fn_rva = fn_rva;
	for ( int i = 0; i < c.op_count; i++ ) f.ops.push_back( { c.ops[ i ].type, c.ops[ i ].value } );
	f.input_style = "raw"; f.return_style = "raw"; f.read_offset = 0;
	f.is_encrypt = false; f.is_fov = true; m_json->fns.push_back( f );

	std::fprintf( m_file, "// fn_rva = 0x%llX\n", fn_rva );
	std::fprintf( m_file, "inline uint32_t decryption::decrypt_fov(uint32_t val) {\n" );
	write_u32_ops( m_file, "val", c, source_is_encrypt );
	std::fprintf( m_file, "\treturn val;\n" );
	std::fprintf( m_file, "}\n\n" );
}

void c_writer::write_encrypt_fov( std::uint64_t fn_rva, const c_decryption::constants_t& c, bool source_is_encrypt )
{
	if ( !m_file ) return;

	json_fn_t f; f.name = "encrypt_fov"; f.fn_rva = fn_rva;
	for ( int i = 0; i < c.op_count; i++ ) f.ops.push_back( { c.ops[ i ].type, c.ops[ i ].value } );
	f.input_style = "raw"; f.return_style = "raw"; f.read_offset = 0;
	f.is_encrypt = true; f.is_fov = true; m_json->fns.push_back( f );

	std::fprintf( m_file, "// fn_rva = 0x%llX (encrypt)\n", fn_rva );
	std::fprintf( m_file, "inline uint32_t decryption::encrypt_fov(uint32_t val) {\n" );
	write_u32_ops( m_file, "val", c, !source_is_encrypt );
	std::fprintf( m_file, "\treturn val;\n" );
	std::fprintf( m_file, "}\n" );
}

void c_writer::flush_json( const char* path, const char* build_id, std::uint64_t game_base, std::uint32_t image_size )
{
	std::FILE* f = nullptr;
	fopen_s( &f, path, "w" );
	if ( !f ) return;

	std::fprintf( f, "{\n" );
	std::fprintf( f, "  \"build\": \"%s\",\n", build_id );
	std::fprintf( f, "  \"game_base\": \"0x%llX\",\n", ( unsigned long long )game_base );
	std::fprintf( f, "  \"image_size\": \"0x%X\",\n", image_size );

	// Group fields by namespace
	std::fprintf( f, "  \"sections\": {\n" );
	{
		bool first_ns = true;
		std::string prev_ns;
		bool in_ns = false;

		for ( const auto& fld : m_json->fields )
		{
			if ( fld.ns != prev_ns )
			{
				if ( in_ns )
				{
					std::fprintf( f, "\n    },\n" );
				}
				if ( !first_ns && !in_ns )
					std::fprintf( f, ",\n" );

				std::fprintf( f, "    \"%s\": {\n", fld.ns.c_str( ) );
				in_ns = true;
				first_ns = false;
				prev_ns = fld.ns;
			}
			else
			{
				std::fprintf( f, ",\n" );
			}
			std::fprintf( f, "      \"%s\": \"0x%llX\"", fld.name.c_str( ), ( unsigned long long )fld.value );
		}
		if ( in_ns )
			std::fprintf( f, "\n    }\n" );
		std::fprintf( f, "  },\n" );
	}

	// Decrypt functions
	std::fprintf( f, "  \"decrypt_functions\": {\n" );
	{
		bool first = true;
		for ( const auto& fn : m_json->fns )
		{
			if ( fn.is_encrypt ) continue;
			if ( !first ) std::fprintf( f, ",\n" );
			first = false;
			std::fprintf( f, "    \"%s\": {\n", fn.name.c_str( ) );
			std::fprintf( f, "      \"fn_rva\": \"0x%llX\",\n", ( unsigned long long )fn.fn_rva );
			std::fprintf( f, "      \"ops\": [" );
			for ( int i = 0; i < (int)fn.ops.size( ); i++ )
			{
				if ( i > 0 ) std::fprintf( f, ", " );
				std::fprintf( f, "{\"op\": \"%s\", \"value\": \"0x%X\"}",
					op_type_name( fn.ops[ i ].type ), fn.ops[ i ].value );
			}
			std::fprintf( f, "],\n" );
			std::fprintf( f, "      \"input_style\": \"%s\",\n", fn.input_style.c_str( ) );
			std::fprintf( f, "      \"return_style\": \"%s\",\n", fn.return_style.c_str( ) );
			if ( fn.read_offset > 0 )
				std::fprintf( f, "      \"read_offset\": \"0x%llX\",\n", ( unsigned long long )fn.read_offset );
			else
				std::fprintf( f, "      \"read_offset\": null,\n" );
			std::fprintf( f, "      \"confidence\": \"auto\",\n" );
			std::fprintf( f, "      \"is_fov\": %s\n", fn.is_fov ? "true" : "false" );
			std::fprintf( f, "    }" );
		}
		std::fprintf( f, "\n  },\n" );
	}

	// Encrypt functions (auto-generated inverse of decrypt)
	std::fprintf( f, "  \"encrypt_functions\": {\n" );
	{
		bool first = true;
		for ( const auto& fn : m_json->fns )
		{
			if ( !fn.is_encrypt ) continue;
			if ( !first ) std::fprintf( f, ",\n" );
			first = false;
			std::fprintf( f, "    \"%s\": {\n", fn.name.c_str( ) );
			std::fprintf( f, "      \"fn_rva\": \"0x%llX\",\n", ( unsigned long long )fn.fn_rva );
			// Encrypt = inverse ops: reverse order, ADD->SUB, SUB->ADD, XOR->XOR, ROL->ROR
			std::fprintf( f, "      \"ops\": [" );
			for ( int i = (int)fn.ops.size( ) - 1; i >= 0; i-- )
			{
				if ( i < (int)fn.ops.size( ) - 1 ) std::fprintf( f, ", " );
				std::fprintf( f, "{\"op\": \"%s\", \"value\": \"0x%X\"}",
					op_inverse_name( fn.ops[ i ].type ), fn.ops[ i ].value );
			}
			std::fprintf( f, "],\n" );
			std::fprintf( f, "      \"confidence\": \"auto-generated\",\n" );
			std::fprintf( f, "      \"is_fov\": %s\n", fn.is_fov ? "true" : "false" );
			std::fprintf( f, "    }" );
		}
		std::fprintf( f, "\n  }\n" );
	}

	std::fprintf( f, "}\n" );
	fclose( f );
}

void c_writer::add_json_class_field( const char* class_name, const char* field_name, std::uint64_t offset )
{
	if ( m_json )
		m_json->fields.push_back( { class_name, field_name, offset } );
}
