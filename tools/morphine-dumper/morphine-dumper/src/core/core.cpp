#include "core.hpp"
#include "../hde/hde64.h"

#include <windows.h>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static std::uint64_t field_offset_by_name( il2cpp::il2cpp_class_t* klass, const char* name );
static std::uint64_t field_offset_by_name_contains( il2cpp::il2cpp_class_t* klass, const char* a, const char* b = nullptr );
static std::uint64_t field_offset_by_name_contains_type( il2cpp::il2cpp_class_t* klass, const char* a, const char* b, const char* type );
static std::uint64_t field_offset_by_name_contains_type_after( il2cpp::il2cpp_class_t* klass, const char* a, const char* b, const char* type, std::uint64_t after );
static std::uint64_t field_offset_by_type_contains_after( il2cpp::il2cpp_class_t* klass, const char* type, std::uint64_t after );
static std::uint64_t first_field_offset_before( il2cpp::il2cpp_class_t* klass, std::uint64_t before );
static std::uint64_t first_field_offset_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t after, const char* type );
static std::uint64_t nth_field_offset_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t after, const char* type, std::size_t index );
static bool last_consecutive_field_pair_by_type( il2cpp::il2cpp_class_t* klass, const char* type, std::uint64_t after, std::uint64_t& first, std::uint64_t& second );
static std::uint64_t field_offset_by_type_contains( il2cpp::il2cpp_class_t* klass, const char* type );
static std::uint64_t field_offset_by_type_contains_index( il2cpp::il2cpp_class_t* klass, const char* type, std::size_t index );
static std::uint64_t field_offset_by_plain_type_contains_index( il2cpp::il2cpp_class_t* klass, const char* type, std::size_t index );
static std::uint64_t field_offset_by_type_name_index( il2cpp::il2cpp_class_t* klass, const char* type, std::size_t index );
static std::uint64_t first_consecutive_bool_pair( il2cpp::il2cpp_class_t* klass );
static std::uint64_t highest_field_offset_by_type_contains( il2cpp::il2cpp_class_t* klass, const char* type );
static std::uint64_t highest_field_offset_by_type_name( il2cpp::il2cpp_class_t* klass, const char* type );
static std::uint64_t field_offset_by_names( il2cpp::il2cpp_class_t* klass, const char** names, int count );
static std::uint64_t field_offset_after( il2cpp::il2cpp_class_t* klass, std::uint64_t offset );
static bool field_type_matches( il2cpp::field_info_t* f, const char* type );
static bool field_type_or_generic_arg_matches( il2cpp::field_info_t* f, const char* type );
static std::uint64_t field_offset_by_type_or_class_contains( il2cpp::il2cpp_class_t* klass, const char* type );
static std::uint64_t highest_field_offset_by_type_or_class_contains( il2cpp::il2cpp_class_t* klass, const char* type );
static il2cpp::field_info_t* field_by_offset_walk( il2cpp::il2cpp_class_t* klass, std::uint64_t offset );
static bool is_entity_ref_type( il2cpp::il2cpp_class_t* klass );
static std::uint64_t highest_repeated_wrapper_field_offset( il2cpp::il2cpp_class_t* klass );
static std::uint64_t first_wrapper_field_after( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset );
static std::uint64_t first_wrapper_field_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset, const char* type );
static std::uint64_t first_entity_ref_field_after( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset );
static std::uint64_t nth_wrapper_field_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset, const char* type, std::size_t index );
static std::uint64_t last_field_offset_before_type( il2cpp::il2cpp_class_t* klass, std::uint64_t before, const char* type );
static std::vector<il2cpp::field_info_t*> fields_by_type_class( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* type_class );
static std::uint64_t field_offset_by_type_class( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* type_class );
static il2cpp::il2cpp_class_t* field_type_class_by_name( il2cpp::il2cpp_class_t* klass, const char* name );
static bool class_has_model_state_shape( il2cpp::il2cpp_class_t* klass );
static bool class_has_model_state_value_shape( il2cpp::il2cpp_class_t* klass );
static std::uint64_t field_offset_by_model_state_shape( il2cpp::il2cpp_class_t* klass );
static std::uint64_t field_offset_by_model_state_shape_between( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset, std::uint64_t max_offset );
static std::uint64_t field_offset_by_type_class_between( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* type_class, std::uint64_t min_offset, std::uint64_t max_offset );
static il2cpp::il2cpp_class_t* class_with_public_static_field_type_terms( const char** terms, int term_count );
static il2cpp::il2cpp_class_t* class_with_public_field_name_type_terms( const char* field_name, const char** terms, int term_count, bool require_static );
static il2cpp::il2cpp_class_t* class_with_field_name_contains_type( const char* name, const char* type, bool require_static );
static il2cpp::il2cpp_class_t* class_with_instance_field_type_class( il2cpp::il2cpp_class_t* type_class );
static il2cpp::il2cpp_class_t* class_with_method_names( const char* a, const char* b = nullptr );
static std::uint64_t static_class_rva_any( il2cpp::il2cpp_class_t* klass );
static il2cpp::il2cpp_class_t* class_name_equals( const char* name );
static il2cpp::il2cpp_class_t* class_name_contains( const char* a, const char* b = nullptr );
static il2cpp::il2cpp_class_t* class_name_contains_i( const char* a, const char* b = nullptr );
static il2cpp::il2cpp_class_t* class_type_name_contains( const char* a, const char* b = nullptr );
static il2cpp::il2cpp_class_t* find_named_class( const char* name );
static il2cpp::il2cpp_class_t* find_model_state_class( );
static il2cpp::il2cpp_class_t* find_class_with_nested_field( const char* field_name, il2cpp::il2cpp_class_t** nested_out );
static il2cpp::il2cpp_class_t* find_class_with_nested_self_instance_field( il2cpp::il2cpp_class_t** nested_out, std::uint64_t* instance_out );
static il2cpp::il2cpp_class_t* nested_static_class_with_most_static_fields( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* exclude = nullptr );
static il2cpp::il2cpp_class_t* singleton_parent_class( const char* concrete_name );
static std::uint64_t singleton_parent_typeinfo_rva( const char* concrete_name );
static void load_local_il2cpp_files( );
static bool metadata_type_line_matches( const std::string& line, const char* class_name );
static std::uint64_t metadata_field_offset_by_name( const char* class_name, const char* field_name );
static std::uint64_t metadata_field_offset_by_name_contains( const char* class_name, const char* a, const char* b = nullptr );
static std::uint64_t metadata_field_offset_by_name_contains_type( const char* class_name, const char* a, const char* b, const char* type );
static std::uint64_t metadata_field_offset_by_name_contains_type_after( const char* class_name, const char* a, const char* b, const char* type, std::uint64_t after );
static std::uint64_t metadata_field_offset_by_type_contains( const char* class_name, const char* type );
static std::uint64_t metadata_field_offset_by_type_contains_index( const char* class_name, const char* type, std::size_t index, bool plain_only = false );
static std::uint64_t metadata_first_field_after_type( const char* class_name, std::uint64_t after, const char* type, bool wrapper_only = false );
static std::uint64_t metadata_last_field_before_type( const char* class_name, std::uint64_t before, const char* type );
static std::uint64_t metadata_highest_field_offset_by_type_contains( const char* class_name, const char* type );
static std::uint64_t metadata_field_offset_after( const char* class_name, std::uint64_t offset );
static std::uint64_t metadata_typeinfo_rva( il2cpp::il2cpp_class_t* klass );
static bool ascii_contains_i( const char* haystack, const char* needle );
static bool is_valid_userland_ptr( std::uintptr_t p );
static std::uint32_t popcount32( std::uint32_t v );
static std::uint64_t method_field_offset_by_name_contains( il2cpp::il2cpp_class_t* klass, const char* a, const char* b = nullptr );
static std::uint64_t getter_field_offset_by_return_type_name( il2cpp::il2cpp_class_t* klass, const char* type, const char* name_a = nullptr, const char* name_b = nullptr );

static const char* const k_bn_sigs[] = {
	"48 8B 89 ? ? ? ? 48 8B ? ? ? ? ? 48 8B 49 ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 53 ? 45 33 C0 48 8B C8",
	"48 8B 89 ? ? ? ? 48 8B ? ? ? ? ? 48 8B 49 ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? ? ? ? ? 45 33 C0",
};

struct view_matrix_t { float m[ 4 ][ 4 ]; };
struct vec3_t { float x, y, z; };

struct metadata_class_range_t
{
	std::string name;
	bool found = false;
	std::size_t start = 0;
	std::size_t end = 0;
};

struct base_player_live_offsets_t
{
	std::uint64_t player_model = 0;
	std::uint64_t player_model_position = 0;
	std::uint64_t model_state = 0;
	std::uint64_t model_state_flags = 0;
	std::uint64_t belt_direct = 0;
	std::uint64_t current_team = 0;
};

static base_player_live_offsets_t g_base_player_live{ };

static bool dumper_verbose( )
{
	static int enabled = -1;
	if ( enabled < 0 )
	{
		char value[ 8 ]{ };
		enabled = GetEnvironmentVariableA( "MORPHINE_DUMPER_VERBOSE", value, sizeof( value ) ) && value[ 0 ] != '0';
	}
	return enabled != 0;
}

static bool finite_float( float v )
{
	return std::isfinite( v ) && std::fabs( v ) < 1000000.f;
}

static bool finite_matrix( const view_matrix_t& vm )
{
	for ( int r = 0; r < 4; r++ )
		for ( int c = 0; c < 4; c++ )
			if ( !finite_float( vm.m[ r ][ c ] ) )
				return false;
	return true;
}

static float dot3( float ax, float ay, float az, float bx, float by, float bz )
{
	return ax * bx + ay * by + az * bz;
}

static bool sane_basis_len( float v )
{
	return v > 0.65f && v < 1.35f;
}

static bool orthonormal_matrix( const view_matrix_t& vm )
{
	if ( !finite_matrix( vm ) )
		return false;

	for ( int i = 0; i < 3; i++ )
	{
		auto row_len = dot3(
			vm.m[ i ][ 0 ], vm.m[ i ][ 1 ], vm.m[ i ][ 2 ],
			vm.m[ i ][ 0 ], vm.m[ i ][ 1 ], vm.m[ i ][ 2 ] );
		auto col_len = dot3(
			vm.m[ 0 ][ i ], vm.m[ 1 ][ i ], vm.m[ 2 ][ i ],
			vm.m[ 0 ][ i ], vm.m[ 1 ][ i ], vm.m[ 2 ][ i ] );
		if ( !sane_basis_len( row_len ) && !sane_basis_len( col_len ) )
			return false;
	}

	return true;
}

static bool sane_vec3( const vec3_t& v )
{
	return finite_float( v.x ) && finite_float( v.y ) && finite_float( v.z ) &&
		( std::fabs( v.x ) + std::fabs( v.y ) + std::fabs( v.z ) ) > 0.001f;
}

static float camera_origin_error( const view_matrix_t& vm, const vec3_t& p )
{
	float rx = vm.m[ 0 ][ 0 ] * p.x + vm.m[ 0 ][ 1 ] * p.y + vm.m[ 0 ][ 2 ] * p.z + vm.m[ 0 ][ 3 ];
	float ry = vm.m[ 1 ][ 0 ] * p.x + vm.m[ 1 ][ 1 ] * p.y + vm.m[ 1 ][ 2 ] * p.z + vm.m[ 1 ][ 3 ];
	float rz = vm.m[ 2 ][ 0 ] * p.x + vm.m[ 2 ][ 1 ] * p.y + vm.m[ 2 ][ 2 ] * p.z + vm.m[ 2 ][ 3 ];
	float row_err = std::fabs( rx ) + std::fabs( ry ) + std::fabs( rz );

	float cx = vm.m[ 0 ][ 0 ] * p.x + vm.m[ 1 ][ 0 ] * p.y + vm.m[ 2 ][ 0 ] * p.z + vm.m[ 3 ][ 0 ];
	float cy = vm.m[ 0 ][ 1 ] * p.x + vm.m[ 1 ][ 1 ] * p.y + vm.m[ 2 ][ 1 ] * p.z + vm.m[ 3 ][ 1 ];
	float cz = vm.m[ 0 ][ 2 ] * p.x + vm.m[ 1 ][ 2 ] * p.y + vm.m[ 2 ][ 2 ] * p.z + vm.m[ 3 ][ 2 ];
	float col_err = std::fabs( cx ) + std::fabs( cy ) + std::fabs( cz );

	return row_err < col_err ? row_err : col_err;
}

static bool sane_view_matrix( const view_matrix_t& vm )
{
	if ( !finite_matrix( vm ) )
		return false;
	if ( vm.m[ 0 ][ 0 ] == 0.f || vm.m[ 1 ][ 1 ] == 0.f || vm.m[ 2 ][ 2 ] == 0.f )
		return false;
	if ( std::fabs( vm.m[ 0 ][ 0 ] ) > 10.f || std::fabs( vm.m[ 1 ][ 1 ] ) > 10.f )
		return false;
	return true;
}

static float matrix_error( const view_matrix_t& a, const view_matrix_t& b, bool transpose )
{
	float error = 0.f;
	for ( int r = 0; r < 4; r++ )
		for ( int c = 0; c < 4; c++ )
			error += std::fabs( a.m[ r ][ c ] - ( transpose ? b.m[ c ][ r ] : b.m[ r ][ c ] ) );
	return error;
}

static view_matrix_t multiply_matrix( const view_matrix_t& a, const view_matrix_t& b )
{
	view_matrix_t result{ };
	for ( int r = 0; r < 4; r++ )
		for ( int c = 0; c < 4; c++ )
			for ( int k = 0; k < 4; k++ )
				result.m[ r ][ c ] += a.m[ r ][ k ] * b.m[ k ][ c ];
	return result;
}

static vec3_t position_from_view_matrix( const view_matrix_t& vm, bool column_major )
{
	if ( column_major )
	{
		return {
			-( vm.m[ 0 ][ 0 ] * vm.m[ 3 ][ 0 ] + vm.m[ 0 ][ 1 ] * vm.m[ 3 ][ 1 ] + vm.m[ 0 ][ 2 ] * vm.m[ 3 ][ 2 ] ),
			-( vm.m[ 1 ][ 0 ] * vm.m[ 3 ][ 0 ] + vm.m[ 1 ][ 1 ] * vm.m[ 3 ][ 1 ] + vm.m[ 1 ][ 2 ] * vm.m[ 3 ][ 2 ] ),
			-( vm.m[ 2 ][ 0 ] * vm.m[ 3 ][ 0 ] + vm.m[ 2 ][ 1 ] * vm.m[ 3 ][ 1 ] + vm.m[ 2 ][ 2 ] * vm.m[ 3 ][ 2 ] )
		};
	}

	return {
		-( vm.m[ 0 ][ 0 ] * vm.m[ 0 ][ 3 ] + vm.m[ 1 ][ 0 ] * vm.m[ 1 ][ 3 ] + vm.m[ 2 ][ 0 ] * vm.m[ 2 ][ 3 ] ),
		-( vm.m[ 0 ][ 1 ] * vm.m[ 0 ][ 3 ] + vm.m[ 1 ][ 1 ] * vm.m[ 1 ][ 3 ] + vm.m[ 2 ][ 1 ] * vm.m[ 2 ][ 3 ] ),
		-( vm.m[ 0 ][ 2 ] * vm.m[ 0 ][ 3 ] + vm.m[ 1 ][ 2 ] * vm.m[ 1 ][ 3 ] + vm.m[ 2 ][ 2 ] * vm.m[ 2 ][ 3 ] )
	};
}

static bool read_camera_oracle(
	unity::camera_t* camera,
	std::uintptr_t native_camera,
	view_matrix_t& view_matrix,
	vec3_t& position,
	const char*& failure )
{
	failure = "managed Camera pointer";
	if ( !is_valid_userland_ptr( ( std::uintptr_t )camera ) )
		return false;
	failure = "native Camera pointer";
	if ( !is_valid_userland_ptr( native_camera ) )
		return false;

	auto camera_class = c_runtime::find_class( "Camera", "UnityEngine" );
	failure = "Unity Camera class";
	if ( !camera_class )
		return false;

	auto matrix_method = il2cpp::get_method_by_name( camera_class, "get_worldToCameraMatrix_Injected", 1 );
	auto matrix_getter = il2cpp::get_method_by_name( camera_class, "get_worldToCameraMatrix", 0 );
	failure = "Camera matrix method";
	if ( !matrix_method && !matrix_getter )
		return false;

	using get_world_to_camera_t = void( * )(
		std::uintptr_t, view_matrix_t*, il2cpp::method_info_t* );
	using get_world_to_camera_managed_t = void( * )(
		view_matrix_t*, unity::camera_t*, il2cpp::method_info_t* );

	auto get_world_to_camera = matrix_method
		? matrix_method->get_fn_ptr<get_world_to_camera_t>( ) : nullptr;
	auto get_world_to_camera_managed = matrix_getter
		? matrix_getter->get_fn_ptr<get_world_to_camera_managed_t>( ) : nullptr;
	failure = "Camera matrix function";
	if ( !get_world_to_camera && !get_world_to_camera_managed )
		return false;

	__try
	{
		if ( get_world_to_camera )
			get_world_to_camera( native_camera, &view_matrix, matrix_method );
		if ( !sane_view_matrix( view_matrix ) && get_world_to_camera_managed )
			get_world_to_camera_managed( &view_matrix, camera, matrix_getter );
		failure = "worldToCameraMatrix value";
		if ( !sane_view_matrix( view_matrix ) )
			return false;

		auto row_position = position_from_view_matrix( view_matrix, false );
		auto column_position = position_from_view_matrix( view_matrix, true );
		auto row_error = camera_origin_error( view_matrix, row_position );
		auto column_error = camera_origin_error( view_matrix, column_position );
		position = row_error <= column_error ? row_position : column_position;
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		failure = "Unity method exception";
		return false;
	}

	failure = nullptr;
	return true;
}

static std::uint32_t find_matching_view_matrix_offset(
	std::uintptr_t camera_entity,
	const view_matrix_t& reference )
{
	if ( !is_valid_userland_ptr( camera_entity ) )
		return 0;

	struct candidate_t {
		std::uint32_t offset;
		float error;
	};
	std::vector<candidate_t> candidates;
	
	// Collect all orthonormal matrices that could be view matrices
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( view_matrix_t ) ) )
			continue;

		auto matrix = *( view_matrix_t* )( camera_entity + off );
		
		// Must be orthonormal to be a valid view matrix
		if ( !orthonormal_matrix( matrix ) )
			continue;
			
		auto error = matrix_error( matrix, reference, false );
		auto transpose_error = matrix_error( matrix, reference, true );
		auto match_error = error < transpose_error ? error : transpose_error;
		
		if ( match_error < 1.0f )
		{
			candidates.push_back( { off, match_error } );
			if ( dumper_verbose( ) )
				std::printf( "[DEBUG] view matrix candidate: offset=0x%X error=%.6f\n", off, match_error );
		}
	}
	
	if ( candidates.empty( ) )
	{
		if ( dumper_verbose( ) )
			std::printf( "[DEBUG] view matrix: NO CANDIDATES FOUND\n" );
		return 0;
	}
	
	// Sort by error, but if errors are close, prefer higher offset
	std::uint32_t best = candidates[ 0 ].offset;
	float best_error = candidates[ 0 ].error;
	
	for ( const auto& c : candidates )
	{
		// If this candidate has similar error but higher offset, prefer it
		if ( c.error < best_error * 1.1f && c.offset > best )
		{
			best = c.offset;
			best_error = c.error;
		}
		else if ( c.error < best_error )
		{
			best = c.offset;
			best_error = c.error;
		}
	}
	
	if ( dumper_verbose( ) )
		std::printf( "[DEBUG] view matrix: selected offset=0x%X error=%.6f (from %zu candidates)\n", best, best_error, candidates.size( ) );
	return best;
}

static std::uint32_t find_matching_view_projection_offset(
	std::uintptr_t camera_entity,
	const view_matrix_t& world_to_camera,
	const view_matrix_t& projection )
{
	auto projection_view = multiply_matrix( projection, world_to_camera );
	auto view_projection = multiply_matrix( world_to_camera, projection );

	if ( auto off = find_matching_view_matrix_offset( camera_entity, projection_view ) )
		return off;
	return find_matching_view_matrix_offset( camera_entity, view_projection );
}

struct camera_projection_sample_t
{
	vec3_t world;
	vec3_t screen;
};

using camera_w2s_t = void( * )(
	std::uintptr_t, vec3_t*, std::uintptr_t, vec3_t*, il2cpp::method_info_t* );

static bool call_camera_w2s(
	camera_w2s_t w2s,
	std::uintptr_t native_camera,
	vec3_t& world,
	vec3_t& screen,
	std::uintptr_t eye,
	il2cpp::method_info_t* method )
{
	__try
	{
		w2s( native_camera, &world, eye, &screen, method );
		return true;
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
}

static bool project_matrix(
	const view_matrix_t& matrix,
	const vec3_t& world,
	float width,
	float height,
	vec3_t& screen )
{
	auto w = matrix.m[ 0 ][ 3 ] * world.x + matrix.m[ 1 ][ 3 ] * world.y +
		matrix.m[ 2 ][ 3 ] * world.z + matrix.m[ 3 ][ 3 ];
	if ( !finite_float( w ) || std::fabs( w ) < 0.0001f )
		return false;

	auto x = matrix.m[ 0 ][ 0 ] * world.x + matrix.m[ 1 ][ 0 ] * world.y +
		matrix.m[ 2 ][ 0 ] * world.z + matrix.m[ 3 ][ 0 ];
	auto y = matrix.m[ 0 ][ 1 ] * world.x + matrix.m[ 1 ][ 1 ] * world.y +
		matrix.m[ 2 ][ 1 ] * world.z + matrix.m[ 3 ][ 1 ];
	if ( !finite_float( x ) || !finite_float( y ) )
		return false;

	screen.x = ( x / w + 1.f ) * width * 0.5f;
	screen.y = ( 1.f - y / w ) * height * 0.5f;
	screen.z = w;
	return finite_float( screen.x ) && finite_float( screen.y );
}

static std::uint32_t find_w2s_matrix_offset(
	std::uintptr_t camera_entity,
	unity::camera_t* managed_camera,
	const vec3_t& camera_position,
	const std::vector<std::uint32_t>& excluded_scalars,
	std::uint32_t world_to_camera,
	std::uint32_t projection,
	float& closest_error,
	std::uint32_t& closest_offset )
{
	closest_error = FLT_MAX;
	closest_offset = 0;
	auto camera_class = c_runtime::find_class( "Camera", "UnityEngine" );
	auto screen_class = c_runtime::find_class( "Screen", "UnityEngine" );
	auto eye_class = c_runtime::find_class( "MonoOrStereoscopicEye", "UnityEngine" );
	if ( !eye_class )
		eye_class = class_name_contains( "MonoOrStereoscopicEye" );
	auto mono_field = eye_class ? il2cpp::get_field_by_name( eye_class, "Mono" ) : nullptr;
	if ( !camera_class || !screen_class || !mono_field )
		return 0;
	auto mono_eye = ( std::uintptr_t )mono_field->static_get_value<std::int32_t>( );

	auto w2s_method = il2cpp::get_method_by_name( camera_class, "WorldToScreenPoint_Injected", 2 );
	if ( !w2s_method )
		w2s_method = il2cpp::get_method_by_name( camera_class, "WorldToScreenPoint_Injected" );
	auto width_method = il2cpp::get_method_by_name( screen_class, "get_width", 0 );
	auto height_method = il2cpp::get_method_by_name( screen_class, "get_height", 0 );
	if ( !w2s_method || !width_method || !height_method )
		return 0;

	using screen_size_t = std::int32_t( * )( il2cpp::method_info_t* );
	auto w2s = w2s_method->get_fn_ptr<camera_w2s_t>( );
	auto get_width = width_method->get_fn_ptr<screen_size_t>( );
	auto get_height = height_method->get_fn_ptr<screen_size_t>( );
	if ( !w2s || !get_width || !get_height )
		return 0;

	auto width = ( float )get_width( width_method );
	auto height = ( float )get_height( height_method );
	if ( width < 100.f || height < 100.f )
		return 0;

	const vec3_t directions[] = {
		{ 100.f, 0.f, 0.f }, { -100.f, 0.f, 0.f },
		{ 0.f, 100.f, 0.f }, { 0.f, -100.f, 0.f },
		{ 0.f, 0.f, 100.f }, { 0.f, 0.f, -100.f },
		{ 100.f, 50.f, 100.f }, { -100.f, 50.f, -100.f }
	};
	std::vector<camera_projection_sample_t> samples;
	for ( const auto& direction : directions )
	{
		camera_projection_sample_t sample{ };
		sample.world = {
			camera_position.x + direction.x,
			camera_position.y + direction.y,
			camera_position.z + direction.z
		};
		if ( !call_camera_w2s(
			w2s, camera_entity, sample.world, sample.screen, mono_eye, w2s_method ) )
			continue;
		if ( sample.screen.z > 0.01f && finite_float( sample.screen.x ) && finite_float( sample.screen.y ) )
		{
			sample.screen.y = height - sample.screen.y;
			samples.push_back( sample );
		}
	}
	if ( samples.size( ) < 2 )
		return 0;

	float best_error = FLT_MAX;
	std::uint32_t best = 0;
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		auto overlaps = [&]( std::uint32_t known )
		{
			return known && off < known + sizeof( view_matrix_t ) &&
				known < off + sizeof( view_matrix_t );
		};
		if ( overlaps( world_to_camera ) || overlaps( projection ) )
			continue;
		if ( projection && off == projection + sizeof( view_matrix_t ) )
			continue;
		bool overlaps_scalar = false;
		for ( auto scalar : excluded_scalars )
		{
			if ( scalar >= off && scalar < off + sizeof( view_matrix_t ) )
			{
				overlaps_scalar = true;
				break;
			}
		}
		if ( overlaps_scalar )
			continue;
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( view_matrix_t ) ) )
			continue;
		auto candidate = *( view_matrix_t* )( camera_entity + off );
		if ( !finite_matrix( candidate ) )
			continue;
		auto right_len = dot3(
			candidate.m[ 0 ][ 0 ], candidate.m[ 1 ][ 0 ], candidate.m[ 2 ][ 0 ],
			candidate.m[ 0 ][ 0 ], candidate.m[ 1 ][ 0 ], candidate.m[ 2 ][ 0 ] );
		auto up_len = dot3(
			candidate.m[ 0 ][ 1 ], candidate.m[ 1 ][ 1 ], candidate.m[ 2 ][ 1 ],
			candidate.m[ 0 ][ 1 ], candidate.m[ 1 ][ 1 ], candidate.m[ 2 ][ 1 ] );
		auto forward_len = dot3(
			candidate.m[ 0 ][ 3 ], candidate.m[ 1 ][ 3 ], candidate.m[ 2 ][ 3 ],
			candidate.m[ 0 ][ 3 ], candidate.m[ 1 ][ 3 ], candidate.m[ 2 ][ 3 ] );
		if ( right_len < 0.01f || up_len < 0.01f || forward_len < 0.000001f )
			continue;

		float error = 0.f;
		bool valid = true;
		for ( const auto& sample : samples )
		{
			vec3_t projected{ };
			if ( !project_matrix(
				candidate, sample.world, width, height, projected ) )
			{
				valid = false;
				break;
			}
			error += std::fabs( projected.x - sample.screen.x ) +
				std::fabs( projected.y - sample.screen.y );
		}
		error /= ( float )samples.size( );
		if ( valid && error < 4.f )
			std::printf(
				"[morphine-dumper] camera W2S candidate: off=0x%X error=%.3fpx\n",
				off, error );
		if ( valid && error < best_error )
		{
			best_error = error;
			best = off;
		}
	}
	closest_error = best_error;
	closest_offset = best;
	return best_error < 4.f ? best : 0;
}

static std::uint32_t find_matching_position_offset(
	std::uintptr_t camera_entity,
	const vec3_t& reference )
{
	if ( !is_valid_userland_ptr( camera_entity ) )
		return 0;

	float best_error = FLT_MAX;
	std::uint32_t best = 0;
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( vec3_t ) ) )
			continue;

		auto candidate = *( vec3_t* )( camera_entity + off );
		if ( !sane_vec3( candidate ) )
			continue;
			
		auto error =
			std::fabs( candidate.x - reference.x ) +
			std::fabs( candidate.y - reference.y ) +
			std::fabs( candidate.z - reference.z );
		if ( error < best_error )
		{
			best_error = error;
			best = off;
		}
	}
	// Return best match if very close
	return best_error < 0.01f ? best : 0;
}

struct local_il2cpp_files_t
{
	bool loaded = false;
	bool dump_found = false;
	bool script_found = false;
	std::string dump_cs;
	std::string script_json;
	std::vector<metadata_class_range_t> ranges;
};

static local_il2cpp_files_t& local_il2cpp_files( )
{
	static local_il2cpp_files_t files{ };
	if ( files.loaded )
		return files;

	files.loaded = true;
	return files;
}

static void load_local_il2cpp_files( )
{
	auto& files = local_il2cpp_files( );
	files.dump_found = false;
	files.script_found = false;
	files.dump_cs.clear( );
	files.script_json.clear( );
	files.ranges.clear( );
	std::printf( "[morphine-dumper] local il2cpp: disabled\n" );
}

static std::size_t line_start_at( const std::string& s, std::size_t pos )
{
	auto start = s.rfind( '\n', pos );
	return start == std::string::npos ? 0 : start + 1;
}

static std::size_t line_end_at( const std::string& s, std::size_t pos )
{
	auto end = s.find( '\n', pos );
	return end == std::string::npos ? s.size( ) : end;
}

static std::uint64_t parse_hex_at( const std::string& s, std::size_t pos )
{
	if ( pos == std::string::npos || pos >= s.size( ) )
		return 0;

	auto value = 0ull;
	bool any = false;
	for ( ; pos < s.size( ); pos++ )
	{
		auto c = s[ pos ];
		std::uint32_t n = 0;
		if ( c >= '0' && c <= '9' ) n = c - '0';
		else if ( c >= 'a' && c <= 'f' ) n = c - 'a' + 10;
		else if ( c >= 'A' && c <= 'F' ) n = c - 'A' + 10;
		else break;
		value = ( value << 4 ) | n;
		any = true;
	}
	return any ? value : 0;
}

static std::uint64_t parse_decimal_after( const std::string& s, std::size_t pos )
{
	if ( pos == std::string::npos || pos >= s.size( ) )
		return 0;

	while ( pos < s.size( ) && ( s[ pos ] == ' ' || s[ pos ] == '\t' || s[ pos ] == ':' ) )
		++pos;

	auto value = 0ull;
	bool any = false;
	for ( ; pos < s.size( ); pos++ )
	{
		auto c = s[ pos ];
		if ( c < '0' || c > '9' )
			break;
		value = value * 10 + ( c - '0' );
		any = true;
	}
	return any ? value : 0;
}

static bool metadata_extract_class_name( const std::string& line, std::string& out )
{
	out.clear( );
	auto keyword = line.find( " class " );
	auto keyword_len = 7u;
	if ( keyword == std::string::npos )
	{
		keyword = line.find( " struct " );
		keyword_len = 8u;
	}
	if ( keyword == std::string::npos )
		return false;

	auto start = keyword + keyword_len;
	while ( start < line.size( ) && ( line[ start ] == ' ' || line[ start ] == '\t' ) )
		++start;

	auto end = start;
	while ( end < line.size( ) &&
		line[ end ] != ' ' &&
		line[ end ] != '\t' &&
		line[ end ] != ':' &&
		line[ end ] != '/' )
		++end;

	out = line.substr( start, end - start );
	return !out.empty( );
}

static bool metadata_type_line_matches( const std::string& line, const char* class_name )
{
	if ( !class_name || !*class_name )
		return false;

	std::string token;
	if ( !metadata_extract_class_name( line, token ) )
		return false;
	if ( token == class_name )
		return true;

	std::string nested_suffix = ".";
	nested_suffix += class_name;
	return token.size( ) > nested_suffix.size( ) &&
		token.compare( token.size( ) - nested_suffix.size( ), nested_suffix.size( ), nested_suffix ) == 0;
}

static bool metadata_class_field_range( const char* class_name, std::size_t& fields_start, std::size_t& fields_end )
{
	fields_start = 0;
	fields_end = 0;
	if ( !class_name || !*class_name )
		return false;

	auto& files = local_il2cpp_files( );
	auto& dump = files.dump_cs;
	if ( dump.empty( ) )
		return false;

	for ( auto& cached : files.ranges )
	{
		if ( cached.name == class_name )
		{
			fields_start = cached.start;
			fields_end = cached.end;
			return cached.found;
		}
	}

	metadata_class_range_t cached{ };
	cached.name = class_name;

	std::size_t pos = 0;
	while ( ( pos = dump.find( class_name, pos ) ) != std::string::npos )
	{
		auto ls = line_start_at( dump, pos );
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( ls, le - ls );
		auto type_line =
			line.find( "TypeDefIndex" ) != std::string::npos &&
			metadata_type_line_matches( line, class_name );
		if ( !type_line )
		{
			pos += std::strlen( class_name );
			continue;
		}

		auto fields = dump.find( "\n\t// Fields", le );
		if ( fields == std::string::npos )
		{
			files.ranges.push_back( cached );
			return false;
		}

		auto next_type = dump.find( "\n// Namespace:", le );
		if ( next_type != std::string::npos && next_type < fields )
		{
			files.ranges.push_back( cached );
			return false;
		}

		fields_start = line_end_at( dump, fields ) + 1;
		auto props = dump.find( "\n\t// Properties", fields_start );
		auto methods = dump.find( "\n\t// Methods", fields_start );
		auto close = dump.find( "\n}", fields_start );
		fields_end = dump.size( );
		if ( props != std::string::npos && props < fields_end ) fields_end = props;
		if ( methods != std::string::npos && methods < fields_end ) fields_end = methods;
		if ( close != std::string::npos && close < fields_end ) fields_end = close;
		cached.found = fields_start < fields_end;
		cached.start = fields_start;
		cached.end = fields_end;
		files.ranges.push_back( cached );
		return cached.found;
	}

	files.ranges.push_back( cached );
	return false;
}

static bool metadata_line_type_matches( const std::string& line, const char* type, bool wrapper_only )
{
	if ( !type || !*type )
		return true;

	const char* dump_type = type;
	if ( std::strstr( type, "Single" ) ) dump_type = "float";
	else if ( std::strstr( type, "Boolean" ) ) dump_type = "bool";
	else if ( std::strstr( type, "UInt64" ) ) dump_type = "ulong";
	else if ( std::strstr( type, "Int32" ) ) dump_type = "int";

	auto semi = line.find( ';' );
	if ( semi == std::string::npos )
		return false;

	auto prefix = line.substr( 0, semi );
	if ( prefix.find( dump_type ) == std::string::npos )
		return false;

	if ( !wrapper_only )
		return true;

	std::string generic = "<";
	generic += dump_type;
	generic += ">";
	return prefix.find( generic ) != std::string::npos;
}

static bool metadata_plain_type_line( const std::string& line )
{
	auto semi = line.find( ';' );
	if ( semi == std::string::npos )
		return false;

	auto prefix = line.substr( 0, semi );
	return prefix.find( '%' ) == std::string::npos;
}

static bool metadata_parse_field_line( const std::string& line, std::string& name, std::uint64_t& offset )
{
	auto semi = line.find( ';' );
	auto comment = line.find( "// 0x", semi == std::string::npos ? 0 : semi );
	if ( semi == std::string::npos || comment == std::string::npos )
		return false;

	auto name_end = semi;
	while ( name_end > 0 && ( line[ name_end - 1 ] == ' ' || line[ name_end - 1 ] == '\t' ) )
		--name_end;
	auto name_start = line.find_last_of( " \t", name_end ? name_end - 1 : 0 );
	if ( name_start == std::string::npos )
		return false;
	++name_start;

	name = line.substr( name_start, name_end - name_start );
	offset = parse_hex_at( line, comment + 5 );
	return !name.empty( );
}

static std::uint64_t metadata_field_offset_by_name( const char* class_name, const char* field_name )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !field_name || !metadata_class_field_range( class_name, start, end ) )
		return 0;

	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) && name == field_name )
			return off;
		pos = le + 1;
	}
	return 0;
}

static std::uint64_t metadata_field_offset_by_name_contains( const char* class_name, const char* a, const char* b )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !a || !metadata_class_field_range( class_name, start, end ) )
		return 0;

	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			ascii_contains_i( name.c_str( ), a ) &&
			( !b || ascii_contains_i( name.c_str( ), b ) ) )
			return off;
		pos = le + 1;
	}
	return 0;
}

static std::uint64_t metadata_field_offset_by_name_contains_type( const char* class_name, const char* a, const char* b, const char* type )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !a || !metadata_class_field_range( class_name, start, end ) )
		return 0;

	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			metadata_line_type_matches( line, type, false ) &&
			ascii_contains_i( name.c_str( ), a ) &&
			( !b || ascii_contains_i( name.c_str( ), b ) ) )
			return off;
		pos = le + 1;
	}
	return 0;
}

static std::uint64_t metadata_field_offset_by_name_contains_type_after(
	const char* class_name,
	const char* a,
	const char* b,
	const char* type,
	std::uint64_t after )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !a || !metadata_class_field_range( class_name, start, end ) )
		return 0;

	std::uint64_t best = 0;
	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			off > after &&
			( !type || metadata_line_type_matches( line, type, false ) ) &&
			ascii_contains_i( name.c_str( ), a ) &&
			( !b || ascii_contains_i( name.c_str( ), b ) ) &&
			( !best || off < best ) )
			best = off;
		pos = le + 1;
	}
	return best;
}

static std::uint64_t metadata_field_offset_by_type_contains( const char* class_name, const char* type )
{
	return metadata_field_offset_by_type_contains_index( class_name, type, 0, false );
}

static std::uint64_t metadata_field_offset_by_type_contains_index( const char* class_name, const char* type, std::size_t index, bool plain_only )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !type || !metadata_class_field_range( class_name, start, end ) )
		return 0;

	std::size_t seen = 0;
	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			metadata_line_type_matches( line, type, false ) &&
			( !plain_only || metadata_plain_type_line( line ) ) )
		{
			if ( seen++ == index )
				return off;
		}
		pos = le + 1;
	}
	return 0;
}

static std::uint64_t metadata_first_field_after_type( const char* class_name, std::uint64_t after, const char* type, bool wrapper_only )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !metadata_class_field_range( class_name, start, end ) )
		return 0;

	std::uint64_t best = 0;
	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			off > after &&
			metadata_line_type_matches( line, type, wrapper_only ) &&
			( !best || off < best ) )
			best = off;
		pos = le + 1;
	}
	return best;
}

static std::uint64_t metadata_last_field_before_type( const char* class_name, std::uint64_t before, const char* type )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !metadata_class_field_range( class_name, start, end ) )
		return 0;

	std::uint64_t best = 0;
	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			off < before &&
			metadata_line_type_matches( line, type, false ) &&
			off > best )
			best = off;
		pos = le + 1;
	}
	return best;
}

static std::uint64_t metadata_highest_field_offset_by_type_contains( const char* class_name, const char* type )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !metadata_class_field_range( class_name, start, end ) )
		return 0;

	std::uint64_t best = 0;
	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			metadata_line_type_matches( line, type, false ) &&
			off > best )
			best = off;
		pos = le + 1;
	}
	return best;
}

static std::uint64_t metadata_field_offset_after( const char* class_name, std::uint64_t offset )
{
	std::size_t start = 0;
	std::size_t end = 0;
	if ( !metadata_class_field_range( class_name, start, end ) )
		return 0;

	std::uint64_t best = 0;
	auto& dump = local_il2cpp_files( ).dump_cs;
	for ( auto pos = start; pos < end; )
	{
		auto le = line_end_at( dump, pos );
		auto line = dump.substr( pos, le - pos );
		std::string name;
		std::uint64_t off = 0;
		if ( metadata_parse_field_line( line, name, off ) &&
			off > offset &&
			( !best || off < best ) )
			best = off;
		pos = le + 1;
	}
	return best;
}

static std::uint64_t metadata_script_address_by_symbol( const char* symbol )
{
	if ( !symbol || !*symbol )
		return 0;

	auto& script = local_il2cpp_files( ).script_json;
	if ( script.empty( ) )
		return 0;

	std::string needle = "\"Name\": \"";
	for ( auto c = symbol; *c; ++c )
	{
		if ( *c == '<' )
			needle += "\\u003C";
		else if ( *c == '>' )
			needle += "\\u003E";
		else
			needle += *c;
	}
	needle += "\"";
	auto name_pos = script.find( needle );
	if ( name_pos == std::string::npos )
		return 0;

	auto object_start = script.rfind( "{", name_pos );
	if ( object_start == std::string::npos )
		return 0;
	auto object_end = script.find( "}", name_pos );
	if ( object_end == std::string::npos )
		object_end = script.size( );

	auto address_pos = script.find( "\"Address\"", object_start );
	if ( address_pos == std::string::npos || address_pos > object_end )
		return 0;

	auto colon = script.find( ":", address_pos );
	return parse_decimal_after( script, colon );
}

static std::uint64_t metadata_typeinfo_rva( il2cpp::il2cpp_class_t* klass )
{
	if ( !klass )
		return 0;

	auto try_symbol = []( const std::string& symbol ) -> std::uint64_t
	{
		return metadata_script_address_by_symbol( symbol.c_str( ) );
	};

	auto type = klass->type( );
	auto type_name = type ? type->name( ) : nullptr;
	if ( type_name && *type_name )
	{
		std::string symbol = type_name;
		symbol += "_TypeInfo";
		if ( auto rva = try_symbol( symbol ) )
			return rva;
	}

	auto ns = klass->namespaze( );
	auto name = klass->name( );
	if ( ns && *ns && name && *name )
	{
		std::string symbol = ns;
		symbol += ".";
		symbol += name;
		symbol += "_TypeInfo";
		if ( auto rva = try_symbol( symbol ) )
			return rva;
	}
	if ( name && *name )
	{
		std::string symbol = name;
		symbol += "_TypeInfo";
		if ( auto rva = try_symbol( symbol ) )
			return rva;
	}

	return 0;
}

static std::uint32_t find_view_matrix_offset( std::uintptr_t camera_entity )
{
	if ( !is_valid_userland_ptr( camera_entity ) )
		return 0;

	std::uint32_t fallback = 0;
	float best_error = 0.05f;
	std::uint32_t best = 0;

	for ( std::uint32_t off = 0x80; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( view_matrix_t ) ) )
			continue;
		auto vm = *( view_matrix_t* )( camera_entity + off );
		if ( !sane_view_matrix( vm ) || !orthonormal_matrix( vm ) )
			continue;
		if ( !fallback )
			fallback = off;

		for ( std::uint32_t pos_off = 0x80; pos_off <= 0x800; pos_off += 4 )
		{
			if ( IsBadReadPtr( ( void* )( camera_entity + pos_off ), sizeof( vec3_t ) ) )
				continue;
			auto pos = *( vec3_t* )( camera_entity + pos_off );
			if ( !sane_vec3( pos ) )
				continue;

			auto err = camera_origin_error( vm, pos );
			if ( err < best_error )
			{
				best_error = err;
				best = off;
			}
		}
	}

	return best ? best : fallback;
}

static bool sane_projection_matrix( const view_matrix_t& vm )
{
	if ( !finite_matrix( vm ) )
		return false;

	if ( std::fabs( vm.m[ 3 ][ 3 ] ) > 0.01f )
		return false;

	auto row_perspective = std::fabs( std::fabs( vm.m[ 2 ][ 3 ] ) - 1.f );
	auto col_perspective = std::fabs( std::fabs( vm.m[ 3 ][ 2 ] ) - 1.f );
	return row_perspective <= 0.05f || col_perspective <= 0.05f;
}

static std::uint32_t find_projection_matrix_offset( std::uintptr_t camera_entity )
{
	if ( !is_valid_userland_ptr( camera_entity ) )
		return 0;

	for ( std::uint32_t off = 0x80; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( view_matrix_t ) ) )
			continue;
		auto vm = *( view_matrix_t* )( camera_entity + off );
		if ( sane_projection_matrix( vm ) )
			return off;
	}

	return 0;
}

static std::uint32_t find_projection_matrix_offset(
	std::uintptr_t camera_entity,
	float field_of_view,
	float aspect,
	std::uint32_t exposed_projection )
{
	if ( !is_valid_userland_ptr( camera_entity ) ||
		field_of_view < 1.f || field_of_view > 179.f ||
		aspect < 0.1f || aspect > 10.f )
		return 0;

	auto expected_y = 1.f / std::tan( field_of_view * 0.008726646259971648f );
	auto expected_x = expected_y / aspect;
	float best_error = 0.5f;
	std::uint32_t best = 0;

	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( exposed_projection && off < exposed_projection + sizeof( view_matrix_t ) &&
			exposed_projection < off + sizeof( view_matrix_t ) )
			continue;
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( view_matrix_t ) ) )
			continue;
		auto matrix = *( view_matrix_t* )( camera_entity + off );
		if ( !sane_projection_matrix( matrix ) )
			continue;

		auto direct_error =
			std::fabs( std::fabs( matrix.m[ 0 ][ 0 ] ) - expected_x ) +
			std::fabs( std::fabs( matrix.m[ 1 ][ 1 ] ) - expected_y );
		auto swapped_error =
			std::fabs( std::fabs( matrix.m[ 0 ][ 0 ] ) - expected_y ) +
			std::fabs( std::fabs( matrix.m[ 1 ][ 1 ] ) - expected_x );
		auto error = direct_error < swapped_error ? direct_error : swapped_error;
			if ( dumper_verbose( ) )
				std::printf(
					"[morphine-dumper] camera projection candidate: off=0x%X error=%.6f\n",
					off, error );
		if ( error < best_error )
		{
			best_error = error;
			best = off;
		}
	}
	return best;
}

static std::uint32_t find_anchored_projection_matrix(
	std::uintptr_t camera_entity,
	float field_of_view,
	float aspect,
	const std::vector<std::uint32_t>& fov_offsets,
	std::uint32_t exposed_projection,
	std::uint32_t internal_view )
{
	if ( !is_valid_userland_ptr( camera_entity ) || !internal_view )
		return 0;

	auto expected_y = 1.f / std::tan( field_of_view * 0.008726646259971648f );
	auto expected_x = expected_y / aspect;
	float best_error = FLT_MAX;
	std::uint32_t best_distance = UINT32_MAX;
	std::uint32_t best = 0;

	for ( auto fov : fov_offsets )
	{
		for ( std::uint32_t off = fov + 4; off < internal_view && off <= fov + 0x80; off += 4 )
		{
			if ( exposed_projection &&
				off < exposed_projection + sizeof( view_matrix_t ) * 2 &&
				exposed_projection < off + sizeof( view_matrix_t ) )
				continue;
			if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( view_matrix_t ) ) )
				continue;
			auto matrix = *( view_matrix_t* )( camera_entity + off );
			if ( !sane_projection_matrix( matrix ) )
				continue;

			auto direct_error =
				std::fabs( std::fabs( matrix.m[ 0 ][ 0 ] ) - expected_x ) +
				std::fabs( std::fabs( matrix.m[ 1 ][ 1 ] ) - expected_y );
			auto swapped_error =
				std::fabs( std::fabs( matrix.m[ 0 ][ 0 ] ) - expected_y ) +
				std::fabs( std::fabs( matrix.m[ 1 ][ 1 ] ) - expected_x );
			auto error = direct_error < swapped_error ? direct_error : swapped_error;
			auto distance = off - fov;
			if ( error < best_error - 0.0001f ||
				( std::fabs( error - best_error ) <= 0.0001f && distance < best_distance ) )
			{
				best_error = error;
				best_distance = distance;
				best = off;
			}
		}
	}
	return best;
}

static std::uint32_t find_matching_matrix_offset(
	std::uintptr_t object,
	const view_matrix_t& reference,
	std::uint32_t exclude = 0 )
{
	if ( !is_valid_userland_ptr( object ) )
		return 0;

	float best_error = 0.01f;
	std::uint32_t best = 0;
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( exclude && off < exclude + sizeof( view_matrix_t ) &&
			exclude < off + sizeof( view_matrix_t ) )
			continue;
		if ( IsBadReadPtr( ( void* )( object + off ), sizeof( view_matrix_t ) ) )
			continue;
		auto candidate = *( view_matrix_t* )( object + off );
		auto error = matrix_error( candidate, reference, false );
		auto transpose_error = matrix_error( candidate, reference, true );
		auto match_error = error < transpose_error ? error : transpose_error;
		if ( match_error < best_error )
		{
			best_error = match_error;
			best = off;
		}
	}
	return best;
}

using camera_float_getter_t = float( * )( unity::camera_t*, il2cpp::method_info_t* );
using camera_int_getter_t = std::int32_t( * )( unity::camera_t*, il2cpp::method_info_t* );

static bool call_camera_float_getter(
	camera_float_getter_t getter,
	unity::camera_t* camera,
	il2cpp::method_info_t* method,
	float& value )
{
	__try
	{
		value = getter( camera, method );
		return finite_float( value );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
}

static bool call_camera_int_getter(
	camera_int_getter_t getter,
	unity::camera_t* camera,
	il2cpp::method_info_t* method,
	std::int32_t& value )
{
	__try
	{
		value = getter( camera, method );
		return true;
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
}

static bool read_camera_float(
	il2cpp::il2cpp_class_t* camera_class,
	unity::camera_t* camera,
	const char* method_name,
	float& value )
{
	auto method = camera_class
		? il2cpp::get_method_by_name( camera_class, method_name, 0 ) : nullptr;
	auto getter = method ? method->get_fn_ptr<camera_float_getter_t>( ) : nullptr;
	return getter && call_camera_float_getter( getter, camera, method, value );
}

static std::uint32_t find_matching_float_offset(
	std::uintptr_t object,
	float reference,
	float tolerance )
{
	if ( !is_valid_userland_ptr( object ) )
		return 0;
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( object + off ), sizeof( float ) ) )
			continue;
		auto value = *( float* )( object + off );
		if ( finite_float( value ) && std::fabs( value - reference ) <= tolerance )
			return off;
	}
	return 0;
}

static std::vector<std::uint32_t> find_matching_float_offsets(
	std::uintptr_t object,
	float reference,
	float tolerance )
{
	std::vector<std::uint32_t> matches;
	if ( !is_valid_userland_ptr( object ) )
		return matches;

	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( object + off ), sizeof( float ) ) )
			continue;
		auto value = *( float* )( object + off );
		if ( finite_float( value ) && std::fabs( value - reference ) <= tolerance )
			matches.push_back( off );
	}
	return matches;
}

static std::uint32_t find_internal_projection_layout(
	std::uintptr_t object,
	const std::vector<std::uint32_t>& fov_offsets,
	const std::vector<std::uint32_t>& aspect_offsets,
	std::uint32_t exposed_projection,
	std::uint32_t internal_view )
{
	if ( !is_valid_userland_ptr( object ) )
		return 0;

	std::uint32_t best = 0;
	std::uint32_t best_score = UINT32_MAX;
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( internal_view && off >= internal_view )
			continue;
		if ( exposed_projection && off < exposed_projection + sizeof( view_matrix_t ) &&
			exposed_projection < off + sizeof( view_matrix_t ) )
			continue;
		if ( IsBadReadPtr( ( void* )( object + off ), sizeof( view_matrix_t ) ) )
			continue;
		auto matrix = *( view_matrix_t* )( object + off );
		if ( !sane_projection_matrix( matrix ) )
			continue;

			if ( dumper_verbose( ) )
				std::printf( "[morphine-dumper] camera projection candidate: off=0x%X\n", off );

		for ( auto fov : fov_offsets )
		{
			if ( fov >= off || off - fov > 0x80 )
				continue;
			for ( auto aspect : aspect_offsets )
			{
				if ( aspect >= fov || fov - aspect > 0x80 )
					continue;
				auto score = ( off - fov ) + ( fov - aspect );
				if ( internal_view )
					score += ( internal_view - off ) / 4;
				if ( score < best_score )
				{
					best_score = score;
					best = off;
				}
			}
		}
	}
	return best;
}

struct camera_internal_layout_t
{
	std::uint32_t culling = 0;
	std::uint32_t near_clip = 0;
	std::uint32_t far_clip = 0;
	std::uint32_t aspect = 0;
	std::uint32_t fov = 0;
	std::uint32_t projection = 0;
};

static camera_internal_layout_t find_camera_internal_layout(
	std::uintptr_t object,
	std::int32_t culling_value,
	float near_value,
	float far_value,
	const std::vector<std::uint32_t>& aspect_offsets,
	const std::vector<std::uint32_t>& fov_offsets )
{
	camera_internal_layout_t best{ };
	std::uint32_t best_score = UINT32_MAX;
	if ( !is_valid_userland_ptr( object ) )
		return best;

	for ( std::uint32_t projection = 0x40; projection <= 0x800; projection += 4 )
	{
		if ( IsBadReadPtr( ( void* )( object + projection ), sizeof( view_matrix_t ) ) )
			continue;
		auto matrix = *( view_matrix_t* )( object + projection );
		if ( !sane_projection_matrix( matrix ) )
			continue;

		for ( auto fov : fov_offsets )
		{
			if ( fov >= projection || projection - fov > 0x80 )
				continue;
			for ( auto aspect : aspect_offsets )
			{
				if ( aspect >= fov || fov - aspect > 0x80 )
					continue;
				for ( std::uint32_t culling = 0x40; culling < aspect; culling += 4 )
				{
					if ( aspect - culling > 0x80 ||
						IsBadReadPtr( ( void* )( object + culling ), 8 ) )
						continue;
					if ( *( std::int32_t* )( object + culling ) != culling_value )
						continue;
					if ( std::fabs( *( float* )( object + culling + 4 ) - near_value ) > 0.0001f )
						continue;

					for ( auto far_clip = culling + 8;
						far_clip < aspect && far_clip <= culling + 0x40;
						far_clip += 4 )
					{
						if ( IsBadReadPtr( ( void* )( object + far_clip ), sizeof( float ) ) )
							continue;
						if ( std::fabs( *( float* )( object + far_clip ) - far_value ) > 0.01f )
							continue;

						auto score =
							( projection - fov ) +
							( fov - aspect ) +
							( aspect - far_clip ) +
							( far_clip - culling );
						if ( score < best_score )
						{
							best_score = score;
							best = {
								culling,
								culling + 4,
								far_clip,
								aspect,
								fov,
								projection
							};
						}
					}
				}
			}
		}
	}
	return best;
}

static void print_offset_list( const char* name, const std::vector<std::uint32_t>& offsets )
{
	std::printf( "[morphine-dumper] camera %s matches:", name );
	for ( auto offset : offsets )
		std::printf( " 0x%X", offset );
	std::printf( "\n" );
}

static std::uint32_t find_matching_float_before(
	std::uintptr_t object,
	float reference,
	float tolerance,
	std::uint32_t before,
	std::uint32_t max_distance )
{
	if ( !is_valid_userland_ptr( object ) || !before )
		return 0;

	auto begin = before > max_distance ? before - max_distance : 0x40;
	for ( auto off = before; off >= begin + 4; )
	{
		off -= 4;
		if ( IsBadReadPtr( ( void* )( object + off ), sizeof( float ) ) )
			continue;
		auto value = *( float* )( object + off );
		if ( finite_float( value ) && std::fabs( value - reference ) <= tolerance )
			return off;
	}
	return 0;
}

static bool find_camera_clip_layout(
	std::uintptr_t object,
	std::int32_t culling_mask,
	float near_clip,
	float far_clip,
	std::uint32_t before_hint,
	std::uint32_t& culling_offset,
	std::uint32_t& near_offset,
	std::uint32_t& far_offset )
{
	culling_offset = 0;
	near_offset = 0;
	far_offset = 0;
	if ( !is_valid_userland_ptr( object ) )
		return false;

	std::uint32_t best_distance = UINT32_MAX;
	for ( std::uint32_t mask_off = 0x40; mask_off <= 0x800; mask_off += 4 )
	{
		if ( before_hint && mask_off >= before_hint )
			continue;
		if ( IsBadReadPtr( ( void* )( object + mask_off ), 8 ) )
			continue;
		if ( *( std::int32_t* )( object + mask_off ) != culling_mask )
			continue;
		if ( std::fabs( *( float* )( object + mask_off + 4 ) - near_clip ) > 0.0001f )
			continue;

		for ( auto far_off = mask_off + 8; far_off <= mask_off + 0x40; far_off += 4 )
		{
			if ( IsBadReadPtr( ( void* )( object + far_off ), sizeof( float ) ) )
				continue;
			if ( std::fabs( *( float* )( object + far_off ) - far_clip ) > 0.01f )
				continue;
			auto distance = before_hint ? before_hint - mask_off : mask_off;
			if ( distance < best_distance )
			{
				best_distance = distance;
				culling_offset = mask_off;
				near_offset = mask_off + 4;
				far_offset = far_off;
			}
		}
	}
	return culling_offset != 0;
}

static bool find_camera_clip_layout_by_values(
	std::uintptr_t object,
	float near_clip,
	float far_clip,
	std::uint32_t before_hint,
	std::uint32_t& culling_offset,
	std::uint32_t& near_offset,
	std::uint32_t& far_offset )
{
	culling_offset = 0;
	near_offset = 0;
	far_offset = 0;
	if ( !is_valid_userland_ptr( object ) || !before_hint )
		return false;

	auto begin = 0x40u;
	auto end = 0x800u;
	std::uint32_t best_score = UINT32_MAX;

	for ( auto near_off = begin + 4; near_off + 4 <= end; near_off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( object + near_off ), sizeof( float ) ) )
			continue;
		if ( std::fabs( *( float* )( object + near_off ) - near_clip ) > 0.0001f )
			continue;

		for ( auto far_off = near_off + 4; far_off <= end && far_off <= near_off + 0x40; far_off += 4 )
		{
			if ( IsBadReadPtr( ( void* )( object + far_off ), sizeof( float ) ) )
				continue;
			if ( std::fabs( *( float* )( object + far_off ) - far_clip ) > 0.01f )
				continue;

			auto mask_begin = near_off > 0x20 ? near_off - 0x20 : 0x40;
			for ( auto mask_off = mask_begin; mask_off < near_off; mask_off += 4 )
			{
				if ( IsBadReadPtr( ( void* )( object + mask_off ), sizeof( std::uint32_t ) ) )
					continue;
				auto mask = *( std::uint32_t* )( object + mask_off );
				auto direct_culling_slot = mask_off == near_off - 4;
				if ( mask == 0xCCCCCCCC || ( !mask && !direct_culling_slot ) )
					continue;
				auto anchor_distance = before_hint
					? ( near_off > before_hint ? near_off - before_hint : before_hint - near_off )
					: near_off;
				auto after_anchor_penalty = before_hint && near_off > before_hint ? 0x10000u : 0u;
				auto mask_score = mask
					? ( popcount32( mask ) >= 5 ? 0u : 0x20u )
					: 0x80u;
				auto score =
					after_anchor_penalty +
					anchor_distance +
					( far_off - near_off ) +
					( near_off - mask_off ) +
					mask_score;
				if ( score < best_score )
				{
					best_score = score;
					culling_offset = mask_off;
					near_offset = near_off;
					far_offset = far_off;
				}
			}
		}
	}

	return culling_offset != 0;
}

static bool read_projection_oracle(
	il2cpp::il2cpp_class_t* camera_class,
	std::uintptr_t native_camera,
	view_matrix_t& projection )
{
	auto method = camera_class
		? il2cpp::get_method_by_name( camera_class, "get_projectionMatrix_Injected", 1 ) : nullptr;
	if ( !method )
		return false;

	using projection_getter_t = void( * )(
		std::uintptr_t, view_matrix_t*, il2cpp::method_info_t* );
	auto getter = method->get_fn_ptr<projection_getter_t>( );
	if ( !getter )
		return false;

	__try
	{
		getter( native_camera, &projection, method );
		return finite_matrix( projection );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
}

static std::uint32_t popcount32( std::uint32_t v )
{
	std::uint32_t count = 0;
	while ( v )
	{
		v &= v - 1;
		count++;
	}
	return count;
}

static std::uint32_t find_culling_mask_offset( std::uintptr_t camera_entity )
{
	if ( !is_valid_userland_ptr( camera_entity ) )
		return 0;

	for ( std::uint32_t off = 0x40; off <= 0x600; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( std::uint32_t ) + sizeof( float ) ) )
			continue;

		auto mask = *( std::uint32_t* )( camera_entity + off );
		auto near_clip = *( float* )( camera_entity + off + 4 );
		if ( !mask || mask == 0xCCCCCCCC || popcount32( mask ) < 5 )
			continue;
		if ( near_clip > 0.0001f && near_clip < 10.f )
			return off;
	}

	return 0;
}

static std::uint32_t find_matching_culling_mask_offset(
	std::uintptr_t camera_entity,
	std::int32_t reference,
	std::uint32_t world_to_camera,
	std::uint32_t projection,
	std::uint32_t view_projection )
{
	if ( !is_valid_userland_ptr( camera_entity ) )
		return 0;

	auto overlaps_matrix = []( std::uint32_t off, std::uint32_t matrix_off )
	{
		return matrix_off && off >= matrix_off && off < matrix_off + sizeof( view_matrix_t );
	};

	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( overlaps_matrix( off, world_to_camera ) ||
			overlaps_matrix( off, projection ) ||
			overlaps_matrix( off, view_projection ) )
			continue;
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( std::int32_t ) ) )
			continue;
		if ( *( std::int32_t* )( camera_entity + off ) == reference )
			return off;
	}
	return 0;
}

static std::uint32_t find_camera_position_offset( std::uintptr_t camera_entity, std::uint32_t view_matrix_offset )
{
	if ( !is_valid_userland_ptr( camera_entity ) || !view_matrix_offset )
		return 0;
	if ( IsBadReadPtr( ( void* )( camera_entity + view_matrix_offset ), sizeof( view_matrix_t ) ) )
		return 0;

	auto vm = *( view_matrix_t* )( camera_entity + view_matrix_offset );
	if ( !sane_view_matrix( vm ) || !orthonormal_matrix( vm ) )
		return 0;

	float best_error = FLT_MAX;
	std::uint32_t best = 0;
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( vec3_t ) ) )
			continue;
		auto pos = *( vec3_t* )( camera_entity + off );
		if ( !sane_vec3( pos ) )
			continue;
		
		auto error = camera_origin_error( vm, pos );
		if ( error < best_error )
		{
			best_error = error;
			best = off;
		}
	}

	return best_error < 1.0f ? best : 0;
}

static float column_major_camera_origin_error( const view_matrix_t& vm, const vec3_t& p )
{
	auto x = vm.m[ 0 ][ 0 ] * p.x + vm.m[ 1 ][ 0 ] * p.y +
		vm.m[ 2 ][ 0 ] * p.z + vm.m[ 3 ][ 0 ];
	auto y = vm.m[ 0 ][ 1 ] * p.x + vm.m[ 1 ][ 1 ] * p.y +
		vm.m[ 2 ][ 1 ] * p.z + vm.m[ 3 ][ 1 ];
	auto z = vm.m[ 0 ][ 2 ] * p.x + vm.m[ 1 ][ 2 ] * p.y +
		vm.m[ 2 ][ 2 ] * p.z + vm.m[ 3 ][ 2 ];
	return std::sqrt( x * x + y * y + z * z );
}

static std::uint32_t find_column_major_view_matrix_offset(
	std::uintptr_t camera_entity,
	const vec3_t& camera_position,
	const view_matrix_t& oracle_view,
	std::uint32_t world_to_camera,
	std::uint32_t projection )
{
	if ( !is_valid_userland_ptr( camera_entity ) || !sane_vec3( camera_position ) )
		return 0;

	float best_orientation_error = FLT_MAX;
	std::uint32_t best = 0;
	auto overlaps_matrix = []( std::uint32_t candidate, std::uint32_t known )
	{
		if ( !known )
			return false;
		auto candidate_end = candidate + ( std::uint32_t )sizeof( view_matrix_t );
		auto known_end = known + ( std::uint32_t )sizeof( view_matrix_t );
		return candidate < known_end && known < candidate_end;
	};

	// Scan all offsets and find the absolute best match
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( overlaps_matrix( off, world_to_camera ) ||
			overlaps_matrix( off, projection ) )
			continue;
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( view_matrix_t ) ) )
			continue;

		auto matrix = *( view_matrix_t* )( camera_entity + off );
		if ( !finite_matrix( matrix ) || !orthonormal_matrix( matrix ) )
			continue;

		auto origin_error = column_major_camera_origin_error( matrix, camera_position );
		if ( origin_error >= 0.01f )
			continue;

		auto direct_error = 0.f;
		auto transpose_error = 0.f;
		for ( int r = 0; r < 3; r++ )
			for ( int c = 0; c < 3; c++ )
			{
				direct_error += std::fabs( matrix.m[ r ][ c ] - oracle_view.m[ r ][ c ] );
				transpose_error += std::fabs( matrix.m[ r ][ c ] - oracle_view.m[ c ][ r ] );
			}
		auto orientation_error = direct_error < transpose_error ? direct_error : transpose_error;
		std::printf(
			"[morphine-dumper] camera column-view candidate: off=0x%X origin=%.6f orientation=%.6f\n",
			off, origin_error, orientation_error );
		if ( orientation_error < best_orientation_error )
		{
			best_orientation_error = orientation_error;
			best = off;
		}
	}
	return best;
}

static std::uint32_t projection_layout_from_matrix( const view_matrix_t& vm )
{
	auto row_perspective = std::fabs( std::fabs( vm.m[ 2 ][ 3 ] ) - 1.f );
	auto col_perspective = std::fabs( std::fabs( vm.m[ 3 ][ 2 ] ) - 1.f );
	if ( row_perspective <= col_perspective )
		return 1;
	return 2;
}

static std::uint32_t find_camera_fov_offset( std::uintptr_t camera_entity, const view_matrix_t& projection )
{
	auto y_scale = std::fabs( projection.m[ 1 ][ 1 ] );
	if ( y_scale < 0.05f || y_scale > 10.f )
		return 0;

	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( float ) ) )
			continue;
		auto value = *( float* )( camera_entity + off );
		if ( value < 25.f || value > 130.f )
			continue;

		auto radians = value * 0.01745329251994329577f;
		auto expected = 1.f / std::tan( radians * 0.5f );
		if ( std::fabs( expected - y_scale ) < 0.08f )
			return off;
	}

	return 0;
}

static std::uint32_t find_camera_aspect_offset( std::uintptr_t camera_entity, const view_matrix_t& projection )
{
	auto x_scale = std::fabs( projection.m[ 0 ][ 0 ] );
	auto y_scale = std::fabs( projection.m[ 1 ][ 1 ] );
	if ( x_scale < 0.05f || y_scale < 0.05f )
		return 0;

	auto expected = y_scale / x_scale;
	for ( std::uint32_t off = 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( float ) ) )
			continue;
		auto value = *( float* )( camera_entity + off );
		if ( value >= 0.5f && value <= 4.f && std::fabs( value - expected ) < 0.08f )
			return off;
	}

	return 0;
}

static std::uint32_t find_camera_aspect_offset_before(
	std::uintptr_t camera_entity,
	const view_matrix_t& projection,
	std::uint32_t before )
{
	auto x_scale = std::fabs( projection.m[ 0 ][ 0 ] );
	auto y_scale = std::fabs( projection.m[ 1 ][ 1 ] );
	if ( !before || x_scale < 0.05f || y_scale < 0.05f )
		return 0;

	auto expected = y_scale / x_scale;
	auto begin = before > 0x80 ? before - 0x80 : 0x40;
	for ( auto off = before; off >= begin + 4; )
	{
		off -= 4;
		if ( IsBadReadPtr( ( void* )( camera_entity + off ), sizeof( float ) ) )
			continue;
		auto value = *( float* )( camera_entity + off );
		if ( value >= 0.5f && value <= 4.f &&
			std::fabs( value - expected ) < 0.08f )
			return off;
	}
	return 0;
}

static std::uint32_t find_float_offset_after( std::uintptr_t object, std::uint32_t after, float min_value, float max_value )
{
	if ( !is_valid_userland_ptr( object ) )
		return 0;

	for ( std::uint32_t off = after ? after + 4 : 0x40; off <= 0x800; off += 4 )
	{
		if ( IsBadReadPtr( ( void* )( object + off ), sizeof( float ) ) )
			continue;
		auto value = *( float* )( object + off );
		if ( value >= min_value && value <= max_value )
			return off;
	}

	return 0;
}

bool c_core::resolve_bn_chain( )
{
	std::uintptr_t bn_match = 0;
	for ( const char* sig : k_bn_sigs )
	{
		bn_match = c_scanner::scan_image( sig );
		if ( bn_match ) break;
	}
	if ( !bn_match )
	{
		std::printf( "[morphine-dumper] bn chain: signature not found\n" );
		return false;
	}

	c_scanner::scan_buf_t s{ };
	if ( !c_scanner::read_scan( bn_match, 0x80, s ) )
		return false;

	std::uint32_t off = 0;
	std::int32_t disp = 0;
	std::uintptr_t resolved = 0;

	if ( !c_scanner::find_mov_base( s, 0, 1, 1, off, disp ) ) return false;

	if ( !c_scanner::find_mov_base( s, off + 1, 1, 1, off, disp ) ) return false;

	if ( !c_scanner::find_call( s, off, 1, off, resolved ) ) return false;
	m_bn.decrypt_0_addr = resolved;

	std::uintptr_t get_player_list = 0;
	if ( !c_scanner::find_call( s, off + 5, 1, off, get_player_list ) ) return false;
	m_bn.entity_list_wrapper_addr = get_player_list;

	c_scanner::scan_buf_t fn_buf{ };
	if ( !c_scanner::read_scan( get_player_list, 0x300, fn_buf ) ) return false;

	if ( !c_scanner::find_mov_base( fn_buf, 0, 7, 1, off, disp ) ) return false;

	if ( !c_scanner::find_call( fn_buf, off, 1, off, resolved ) ) return false;
	m_bn.decrypt_1_addr = resolved;

	m_bn.decrypt_0 = c_decryption::extract_loop( ( std::uint8_t* )m_bn.decrypt_0_addr, 0x200 );
	if ( !m_bn.decrypt_0.valid )
		m_bn.decrypt_0 = c_decryption::extract_inline( ( std::uint8_t* )m_bn.decrypt_0_addr, 0x200 );

	m_bn.decrypt_1 = c_decryption::extract_loop( ( std::uint8_t* )m_bn.decrypt_1_addr, 0x200 );
	if ( !m_bn.decrypt_1.valid )
		m_bn.decrypt_1 = c_decryption::extract_inline( ( std::uint8_t* )m_bn.decrypt_1_addr, 0x200 );

	std::printf( "[morphine-dumper] bn decrypts: client_entities rva=0x%llX ops=%d entity_list rva=0x%llX ops=%d\n",
		m_bn.decrypt_0_addr ? m_bn.decrypt_0_addr - c_runtime::game_base( ) : 0,
		m_bn.decrypt_0.op_count,
		m_bn.decrypt_1_addr ? m_bn.decrypt_1_addr - c_runtime::game_base( ) : 0,
		m_bn.decrypt_1.op_count );

	auto bn_outer = c_runtime::find_class( "BaseNetworkable" );
	auto bn_static = c_runtime::get_inner_static_class( bn_outer );
	if ( bn_static )
		m_bn.base_networkable_rva = c_runtime::static_class_rva( bn_static );

	if ( bn_static )
	{
		auto client_entities = il2cpp::get_static_field_if_value_is<void*>(
			bn_static, "BaseNetworkable", FIELD_ATTRIBUTE_PUBLIC, 0,
			[]( void* p ) { return p != nullptr; } );
		if ( !client_entities )
			client_entities = il2cpp::get_field_if_type_contains(
				bn_static, "BaseNetworkable", FIELD_ATTRIBUTE_PUBLIC, FIELD_ATTRIBUTE_STATIC );

		if ( client_entities )
		{
			m_bn.wrapper_class_ptr = ( std::uint32_t )client_entities->offset( );

			auto t = client_entities->type( );
			if ( t && t->klass( ) )
			{
				auto entity_realm_class = t->klass( )->get_generic_argument_at( 0 );
				if ( entity_realm_class )
				{
					auto entity_list = il2cpp::get_field_if_type_contains(
						entity_realm_class, "BaseNetworkable" );
					if ( entity_list )
					{
						m_bn.parent_static_fields = ( std::uint32_t )entity_list->offset( );

						auto el_type = entity_list->type( );
						auto el_wrapper_klass = el_type ? el_type->klass( ) : nullptr;
						auto list_dictionary_klass = el_wrapper_klass
							? el_wrapper_klass->get_generic_argument_at( 0 )
							: nullptr;

						if ( list_dictionary_klass )
						{
							auto buffer_list_field = il2cpp::get_field_if_type_contains(
								list_dictionary_klass, "BufferList" );
							if ( !buffer_list_field )
								buffer_list_field = il2cpp::get_field_if_type_contains(
									list_dictionary_klass, "BaseNetworkable" );
							if ( buffer_list_field )
								m_bn.entities = ( std::uint32_t )buffer_list_field->offset( );
						}
					}
				}
			}
		}
	}

	if ( !m_bn.entities )
		m_bn.entities = 0x10;

	return true;
}

bool c_core::resolve_camera_chain( )
{
	auto main_camera = c_runtime::find_class( "MainCamera" );
	if ( !main_camera )
	{
		std::printf( "[morphine-dumper] camera: MainCamera class not found\n" );
		return false;
	}

	auto parent = main_camera->parent( );
	il2cpp::il2cpp_class_t* singleton = parent ? parent->parent( ) : nullptr;

	std::uint64_t rva = c_runtime::static_class_rva( main_camera );
	if ( !rva && singleton )
		rva = c_runtime::static_class_rva( singleton );
	m_camera.main_camera_c = rva;

	il2cpp::field_info_t* main_camera_field = il2cpp::get_static_field_if_value_is<unity::component_t*>(
		main_camera, "UnityEngine.Camera", FIELD_ATTRIBUTE_PUBLIC, 0,
		[]( unity::component_t* camera ) { return camera != nullptr; } );

	if ( main_camera_field )
	{
		m_camera.camera_object = main_camera_field->offset( );
		auto unity_object = c_runtime::find_class( "Object", "UnityEngine" );
		auto cached_ptr = field_offset_by_name( unity_object, "m_CachedPtr" );
		if ( cached_ptr )
			m_camera.entity = ( std::uint32_t )cached_ptr;
		std::printf( "[morphine-dumper] camera_object = 0x%X\n", m_camera.camera_object );
		auto managed_camera = main_camera_field->static_get_value<unity::camera_t*>( );

		auto scan_camera_entity = [&]( std::uintptr_t camera_entity )
		{
			view_matrix_t oracle_view{ };
			vec3_t oracle_position{ };
			const char* oracle_failure = nullptr;
			auto has_oracle = read_camera_oracle(
				managed_camera, camera_entity, oracle_view, oracle_position, oracle_failure );
			auto camera_class = c_runtime::find_class( "Camera", "UnityEngine" );

			m_camera.world_to_camera_matrix = has_oracle
				? find_matching_view_matrix_offset( camera_entity, oracle_view )
				: 0;

			float fov_value = 0.f;
			float aspect_value = 0.f;
			float near_value = 0.f;
			float far_value = 0.f;
			auto has_fov = read_camera_float(
				camera_class, managed_camera, "get_fieldOfView", fov_value );
			auto has_aspect = read_camera_float(
				camera_class, managed_camera, "get_aspect", aspect_value );
			auto has_near = read_camera_float(
				camera_class, managed_camera, "get_nearClipPlane", near_value );
			auto has_far = read_camera_float(
				camera_class, managed_camera, "get_farClipPlane", far_value );
			auto fov_offsets = has_fov
				? find_matching_float_offsets( camera_entity, fov_value, 0.001f )
				: std::vector<std::uint32_t>{ };
			auto aspect_offsets = has_aspect
				? find_matching_float_offsets( camera_entity, aspect_value, 0.0001f )
				: std::vector<std::uint32_t>{ };
			std::printf(
				"[morphine-dumper] camera getter values: fov=%.6f aspect=%.6f near=%.6f far=%.6f\n",
				fov_value, aspect_value, near_value, far_value );
			print_offset_list( "fov", fov_offsets );
			print_offset_list( "aspect", aspect_offsets );

			auto culling_method = camera_class
				? il2cpp::get_method_by_name( camera_class, "get_cullingMask", 0 ) : nullptr;
			auto get_culling_mask = culling_method
				? culling_method->get_fn_ptr<camera_int_getter_t>( ) : nullptr;
			std::int32_t mask_value = 0;
			auto has_mask = get_culling_mask && call_camera_int_getter(
				get_culling_mask, managed_camera, culling_method, mask_value );

			view_matrix_t oracle_projection{ };
			auto has_projection_oracle = read_projection_oracle(
				camera_class, camera_entity, oracle_projection );
			auto exposed_projection = has_projection_oracle
				? find_matching_matrix_offset(
					camera_entity, oracle_projection, m_camera.world_to_camera_matrix )
				: 0;
			auto crosschecked_projection = has_fov && has_aspect
				? find_projection_matrix_offset(
					camera_entity, fov_value, aspect_value, exposed_projection )
				: 0;
			auto internal_layout = has_mask && has_near && has_far
				? find_camera_internal_layout(
					camera_entity,
					mask_value,
					near_value,
					far_value,
					aspect_offsets,
					fov_offsets )
				: camera_internal_layout_t{ };
			m_camera.projection_matrix = internal_layout.projection;
			if ( !m_camera.projection_matrix )
				m_camera.projection_matrix = crosschecked_projection
					? crosschecked_projection
					: exposed_projection
					? exposed_projection
					: find_projection_matrix_offset( camera_entity );

			m_camera.field_of_view = internal_layout.fov;
			m_camera.aspect = internal_layout.aspect;
			if ( !m_camera.field_of_view && !fov_offsets.empty( ) )
				m_camera.field_of_view = fov_offsets.front( );
			if ( !m_camera.aspect && !aspect_offsets.empty( ) )
				m_camera.aspect = aspect_offsets.front( );
			if ( m_camera.projection_matrix &&
				!IsBadReadPtr(
					( void* )( camera_entity + m_camera.projection_matrix ),
					sizeof( view_matrix_t ) ) )
			{
				auto selected_projection =
					*( view_matrix_t* )( camera_entity + m_camera.projection_matrix );
				if ( !m_camera.field_of_view )
					m_camera.field_of_view =
						find_camera_fov_offset( camera_entity, selected_projection );
				if ( !m_camera.aspect )
				{
					m_camera.aspect = find_camera_aspect_offset_before(
						camera_entity,
						selected_projection,
						m_camera.field_of_view );
					if ( !m_camera.aspect )
						m_camera.aspect =
							find_camera_aspect_offset( camera_entity, selected_projection );
				}
			}
			m_camera.view_matrix = 0;
			m_camera.culling_mask = internal_layout.culling;
			m_camera.near_clip = internal_layout.near_clip;
			m_camera.far_clip = internal_layout.far_clip;
			auto clip_before_hint = [&]()
			{
				if ( m_camera.field_of_view )
					return m_camera.field_of_view;
				if ( m_camera.projection_matrix )
					return m_camera.projection_matrix;
				return m_camera.aspect;
			};
			if ( !m_camera.culling_mask )
			{
				if ( has_near && has_far )
				{
					auto found_clip = false;
					if ( has_mask )
						found_clip = find_camera_clip_layout(
							camera_entity,
							mask_value,
							near_value,
							far_value,
							clip_before_hint( ),
							m_camera.culling_mask,
							m_camera.near_clip,
							m_camera.far_clip );
					if ( !found_clip )
						find_camera_clip_layout_by_values(
							camera_entity,
							near_value,
							far_value,
							clip_before_hint( ),
							m_camera.culling_mask,
							m_camera.near_clip,
							m_camera.far_clip );
				}
			}
			m_camera.position = 0;
			if ( has_oracle )
			{
				if ( sane_vec3( oracle_position ) )
					m_camera.position = find_matching_position_offset( camera_entity, oracle_position );
				if ( !m_camera.position )
					m_camera.position = find_camera_position_offset(
						camera_entity, m_camera.world_to_camera_matrix );
				if ( m_camera.position )
				{
					auto camera_position = *( vec3_t* )( camera_entity + m_camera.position );
					m_camera.view_matrix = find_column_major_view_matrix_offset(
						camera_entity,
						camera_position,
						oracle_view,
						m_camera.world_to_camera_matrix,
						m_camera.projection_matrix );

					float closest_error = FLT_MAX;
					std::uint32_t closest_offset = 0;
					std::vector<std::uint32_t> excluded_scalars = {
						m_camera.field_of_view,
						m_camera.aspect,
						m_camera.near_clip,
						m_camera.far_clip,
						m_camera.culling_mask,
						m_camera.position
					};
					auto w2s_matrix = find_w2s_matrix_offset(
						camera_entity,
						managed_camera,
						camera_position,
						excluded_scalars,
						m_camera.world_to_camera_matrix,
						m_camera.projection_matrix,
						closest_error,
						closest_offset );
					if ( w2s_matrix )
						m_camera.view_matrix = w2s_matrix;

					if ( m_camera.view_matrix && has_fov && has_aspect )
					{
						auto anchored_projection = find_anchored_projection_matrix(
							camera_entity,
							fov_value,
							aspect_value,
							fov_offsets,
							exposed_projection,
							m_camera.view_matrix );
						if ( anchored_projection )
							m_camera.projection_matrix = anchored_projection;

						if ( m_camera.projection_matrix &&
							!IsBadReadPtr(
								( void* )( camera_entity + m_camera.projection_matrix ),
								sizeof( view_matrix_t ) ) )
						{
							auto projection =
								*( view_matrix_t* )( camera_entity + m_camera.projection_matrix );
							if ( !m_camera.field_of_view )
								m_camera.field_of_view = find_camera_fov_offset(
									camera_entity, projection );
							if ( !m_camera.aspect )
							{
								m_camera.aspect = find_camera_aspect_offset_before(
									camera_entity,
									projection,
									m_camera.field_of_view );
								if ( !m_camera.aspect )
									m_camera.aspect = find_camera_aspect_offset(
										camera_entity, projection );
							}
							m_camera.projection_layout =
								projection_layout_from_matrix( projection );
						}

						if ( has_near && has_far && m_camera.aspect )
						{
							auto found_clip = false;
							if ( has_mask )
								found_clip = find_camera_clip_layout(
									camera_entity,
									mask_value,
									near_value,
									far_value,
									clip_before_hint( ),
									m_camera.culling_mask,
									m_camera.near_clip,
									m_camera.far_clip );
							if ( !found_clip )
								find_camera_clip_layout_by_values(
									camera_entity,
									near_value,
									far_value,
									clip_before_hint( ),
									m_camera.culling_mask,
									m_camera.near_clip,
									m_camera.far_clip );
						}
					}
					std::printf(
						"[morphine-dumper] camera W2S matrix: match=0x%X closest=0x%X error=%.3fpx columnView=0x%X\n",
						w2s_matrix,
						closest_offset,
						closest_error,
						m_camera.view_matrix );
				}
			}
			std::printf( "[morphine-dumper] camera oracle: %s viewProjection=0x%X worldToCamera=0x%X position=0x%X\n",
				has_oracle ? "available" : "unavailable",
				m_camera.view_matrix,
				m_camera.world_to_camera_matrix,
				m_camera.position );
			if ( !has_oracle )
				std::printf( "[morphine-dumper] camera oracle failure: %s\n",
					oracle_failure ? oracle_failure : "unknown" );
			if ( m_camera.projection_matrix
				&& !IsBadReadPtr( ( void* )( camera_entity + m_camera.projection_matrix ), sizeof( view_matrix_t ) ) )
			{
				auto projection = *( view_matrix_t* )( camera_entity + m_camera.projection_matrix );
				m_camera.projection_layout = projection_layout_from_matrix( projection );
				if ( !m_camera.field_of_view )
					m_camera.field_of_view = find_camera_fov_offset( camera_entity, projection );

				if ( !m_camera.aspect )
				{
					m_camera.aspect = find_camera_aspect_offset_before(
						camera_entity,
						projection,
						m_camera.field_of_view );
					if ( !m_camera.aspect )
						m_camera.aspect = find_camera_aspect_offset(
							camera_entity, projection );
				}
			}
			if ( ( !m_camera.culling_mask || !m_camera.near_clip || !m_camera.far_clip )
				&& has_near && has_far && m_camera.aspect )
			{
				std::uint32_t culling = 0;
				std::uint32_t near_clip = 0;
				std::uint32_t far_clip = 0;
				if ( find_camera_clip_layout_by_values(
					camera_entity,
					near_value,
					far_value,
					clip_before_hint( ),
					culling,
					near_clip,
					far_clip ) )
				{
					if ( !m_camera.culling_mask )
						m_camera.culling_mask = culling;
					if ( !m_camera.near_clip )
						m_camera.near_clip = near_clip;
					if ( !m_camera.far_clip )
						m_camera.far_clip = far_clip;
				}
			}
			if ( m_camera.culling_mask || m_camera.near_clip || m_camera.far_clip )
				std::printf(
					"[morphine-dumper] camera clip layout: culling=0x%X near=0x%X far=0x%X\n",
					m_camera.culling_mask,
					m_camera.near_clip,
					m_camera.far_clip );
		};

		auto main_camera_ptr = m_camera.main_camera_c
			? *( std::uintptr_t* )( c_runtime::game_base( ) + m_camera.main_camera_c ) : 0;
		auto static_fields = is_valid_userland_ptr( main_camera_ptr + m_camera.camera_static )
			? *( std::uintptr_t* )( main_camera_ptr + m_camera.camera_static ) : 0;
		auto camera_object = is_valid_userland_ptr( static_fields + m_camera.camera_object )
			? *( std::uintptr_t* )( static_fields + m_camera.camera_object ) : 0;
		auto camera_entity = is_valid_userland_ptr( camera_object + m_camera.entity )
			? *( std::uintptr_t* )( camera_object + m_camera.entity ) : 0;
		scan_camera_entity( camera_entity );

		if ( !is_valid_userland_ptr( camera_entity ) )
		{
			auto hits = c_verification::bruteforce_camera( c_runtime::game_base( ), m_camera.main_camera_c );
			if ( !hits.empty( ) )
			{
				auto& h = hits[ 0 ];
				m_camera.camera_static = h.camera_static;
				m_camera.camera_object = h.camera_object;
				m_camera.entity = h.entity;

				auto brute_static_fields = is_valid_userland_ptr( main_camera_ptr + m_camera.camera_static )
					? *( std::uintptr_t* )( main_camera_ptr + m_camera.camera_static ) : 0;
				auto brute_camera_object = is_valid_userland_ptr( brute_static_fields + m_camera.camera_object )
					? *( std::uintptr_t* )( brute_static_fields + m_camera.camera_object ) : 0;
				auto brute_camera_entity = is_valid_userland_ptr( brute_camera_object + m_camera.entity )
					? *( std::uintptr_t* )( brute_camera_object + m_camera.entity ) : 0;
				scan_camera_entity( brute_camera_entity );
				std::printf( "[morphine-dumper] camera: using bruteforce chain static=0x%X object=0x%X entity=0x%X\n",
					m_camera.camera_static, m_camera.camera_object, m_camera.entity );
			}
		}

		std::printf( "[morphine-dumper] camera position=0x%X viewMatrix=0x%X worldToCameraMatrix=0x%X projectionMatrix=0x%X layout=%u fov=0x%X aspect=0x%X near=0x%X far=0x%X cullingMask=0x%X\n",
			m_camera.position, m_camera.view_matrix, m_camera.world_to_camera_matrix,
			m_camera.projection_matrix, m_camera.projection_layout,
			m_camera.field_of_view, m_camera.aspect,
			m_camera.near_clip, m_camera.far_clip,
			m_camera.culling_mask );
	}
	else
	{
		std::printf( "[morphine-dumper] camera_object: MainCamera.mainCamera is null (load into a server first)\n" );
	}

	return m_camera.main_camera_c != 0;
}

static bool is_valid_userland_ptr( std::uintptr_t p )
{
	return p > 0x10000 && p < 0x7FFFFFFFFFFFull && !IsBadReadPtr( ( void* )p, 8 );
}

using gchandle_get_target_fn_t = void* ( * )( std::uint32_t );

static std::uintptr_t safe_gchandle_get_target( gchandle_get_target_fn_t fn, std::uint32_t handle )
{
	if ( !fn )
		return 0;

	__try
	{
		return ( std::uintptr_t )fn( handle );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return 0;
	}
}

void c_core::verify_bn_chain( )
{
	if ( !m_bn.base_networkable_rva || !m_bn.decrypt_0.valid || !m_bn.decrypt_1.valid )
	{
		std::printf( "[verify] bn_chain: static dump incomplete, skipping\n" );
		return;
	}

	using gchandle_fn_t = void* ( * )( std::uint32_t );
	auto gchandle_get_target = ( gchandle_fn_t )GetProcAddress(
		GetModuleHandleA( "GameAssembly.dll" ), "il2cpp_gchandle_get_target" );
	if ( !gchandle_get_target )
	{
		std::printf( "[verify] bn_chain: gchandle_get_target unavailable\n" );
		return;
	}

	auto game_base = c_runtime::game_base( );
	auto klass = *( std::uintptr_t* )( game_base + m_bn.base_networkable_rva );
	if ( !is_valid_userland_ptr( klass ) )
	{
		std::printf( "[verify] bn_chain: base_networkable klass invalid\n" );
		return;
	}

	auto static_fields = *( std::uintptr_t* )( klass + 0xB8 );
	if ( !is_valid_userland_ptr( static_fields ) )
	{
		std::printf( "[verify] bn_chain: static_fields invalid\n" );
		return;
	}

	auto decrypt_via_chain = [&]( std::uintptr_t wrapper_ptr, std::uint32_t read_off, const c_decryption::constants_t& c ) -> std::uintptr_t
	{
		if ( !is_valid_userland_ptr( wrapper_ptr + read_off ) )
			return 0;
		auto encrypted = *( std::uint64_t* )( wrapper_ptr + read_off );
		auto decrypted = c_decryption::simulate( encrypted, c );

		if ( ( decrypted & 1 ) == 0 && is_valid_userland_ptr( decrypted ) )
		{
			auto target = *( std::uintptr_t* )decrypted;
			if ( is_valid_userland_ptr( target ) )
				return target;
		}

		auto handle = ( std::uint32_t )decrypted;
		if ( handle == 0 || handle > 0x1000000 )
			return 0;
		__try
		{
			return ( std::uintptr_t )gchandle_get_target( handle );
		}
		__except ( EXCEPTION_EXECUTE_HANDLER )
		{
			return 0;
		}
	};

	auto check_buffer_list = []( std::uintptr_t list ) -> std::int32_t
	{
		if ( !is_valid_userland_ptr( list ) )
			return -1;
		auto array = *( std::uintptr_t* )( list + 0x10 );
		auto size  = *( std::int32_t* )( list + 0x18 );
		if ( !is_valid_userland_ptr( array ) ) return -1;
		if ( size < 0 || size > 50000 ) return -1;
		return size;
	};

	std::printf( "[verify] walking chain: klass=0x%llX sf=0x%llX\n", klass, static_fields );
	std::printf( "[verify] bn static guess: wrapper=0x%X parent=0x%X hv=0x%X entities=0x%X\n",
		m_bn.wrapper_class_ptr, m_bn.parent_static_fields, m_bn.hv_offset, m_bn.entities );

	auto guessed_wrapper = *( std::uintptr_t* )( static_fields + m_bn.wrapper_class_ptr );
	std::printf( "[verify] bn guess wrapper_ptr=0x%llX\n", guessed_wrapper );
	if ( is_valid_userland_ptr( guessed_wrapper ) )
	{
		auto guessed_realm = decrypt_via_chain( guessed_wrapper, ( std::uint32_t )m_bn.hv_offset, m_bn.decrypt_0 );
		std::printf( "[verify] bn guess client_entities=0x%llX\n", guessed_realm );
		if ( is_valid_userland_ptr( guessed_realm ) )
		{
			auto guessed_parent = *( std::uintptr_t* )( guessed_realm + m_bn.parent_static_fields );
			std::printf( "[verify] bn guess parent_ptr=0x%llX\n", guessed_parent );
			if ( is_valid_userland_ptr( guessed_parent ) )
			{
				auto guessed_list_dict = decrypt_via_chain( guessed_parent, ( std::uint32_t )m_bn.hv_offset, m_bn.decrypt_1 );
				std::printf( "[verify] bn guess entity_list=0x%llX\n", guessed_list_dict );
				if ( is_valid_userland_ptr( guessed_list_dict ) )
				{
					auto guessed_buffer = *( std::uintptr_t* )( guessed_list_dict + m_bn.entities );
					auto guessed_size = check_buffer_list( guessed_buffer );
					std::printf( "[verify] bn guess buffer=0x%llX size=%d\n", guessed_buffer, guessed_size );
				}
			}
		}
	}

	struct candidate_t { std::uint32_t wrapper, parent, entities; std::int32_t size; };
	std::vector<candidate_t> hits;

	for ( std::uint32_t wrapper_off = 0; wrapper_off <= 0x100; wrapper_off += 8 )
	{
		auto wrapper_ptr = *( std::uintptr_t* )( static_fields + wrapper_off );
		if ( !is_valid_userland_ptr( wrapper_ptr ) )
			continue;

		auto entity_realm = decrypt_via_chain( wrapper_ptr, ( std::uint32_t )m_bn.hv_offset, m_bn.decrypt_0 );
		if ( !is_valid_userland_ptr( entity_realm ) )
			continue;

		for ( std::uint32_t parent_off = 0; parent_off <= 0x100; parent_off += 8 )
		{
			auto parent_ptr = *( std::uintptr_t* )( entity_realm + parent_off );
			if ( !is_valid_userland_ptr( parent_ptr ) )
				continue;

			auto list_dict = decrypt_via_chain( parent_ptr, ( std::uint32_t )m_bn.hv_offset, m_bn.decrypt_1 );
			if ( !is_valid_userland_ptr( list_dict ) )
				continue;

			for ( std::uint32_t ent_off = 0; ent_off <= 0x100; ent_off += 8 )
			{
				auto buffer_list = *( std::uintptr_t* )( list_dict + ent_off );
				auto sz = check_buffer_list( buffer_list );
				if ( sz < 0 )
					continue;

				hits.push_back( { wrapper_off, parent_off, ent_off, sz } );
				std::printf( "[verify]   hit wrapper=0x%X parent=0x%X entities=0x%X size=%d\n",
					wrapper_off, parent_off, ent_off, sz );
			}
		}
	}

	if ( hits.empty( ) )
	{
		std::printf( "[verify] bn_chain: NO runtime hits (game not in a server, or decrypt ops wrong)\n" );
		return;
	}

	std::printf( "[verify] static dump:   wrapper=0x%X parent=0x%X entities=0x%X\n",
		m_bn.wrapper_class_ptr, m_bn.parent_static_fields, m_bn.entities );

	bool exact_match = false;
	for ( auto& h : hits )
	{
		if ( h.wrapper == m_bn.wrapper_class_ptr &&
			h.parent  == m_bn.parent_static_fields &&
			h.entities == m_bn.entities )
		{
			exact_match = true;
			std::printf( "[verify] bn_chain: PASSED (runtime size=%d)\n", h.size );
			break;
		}
	}
	if ( !exact_match )
	{
		std::printf( "[verify] bn_chain: MISMATCH — static dump did not match any runtime hit\n" );
		std::printf( "[verify]   the runtime hits above are the correct offsets; static extraction is off\n" );

		auto better_hit = []( const candidate_t& h, const candidate_t& best ) -> bool
		{
			if ( h.size != best.size )
				return h.size > best.size;
			if ( h.entities == 0x20 && best.entities != 0x20 )
				return true;
			if ( h.entities == 0x18 && best.entities != 0x20 && best.entities != 0x18 )
				return true;
			return false;
		};

		candidate_t* best = nullptr;
		for ( auto& h : hits )
		{
			if ( h.size <= 0 )
				continue;
			if ( h.wrapper != m_bn.wrapper_class_ptr || h.entities != m_bn.entities )
				continue;
			if ( !best || better_hit( h, *best ) )
				best = &h;
		}
		if ( !best )
		{
			for ( auto& h : hits )
			{
				if ( h.size <= 0 )
					continue;
				if ( h.wrapper != m_bn.wrapper_class_ptr )
					continue;
				if ( !best || better_hit( h, *best ) )
					best = &h;
			}
		}
		if ( !best )
		{
			for ( auto& h : hits )
			{
				if ( h.size <= 0 )
					continue;
				if ( !best || better_hit( h, *best ) )
					best = &h;
			}
		}

		if ( best )
		{
			m_bn.wrapper_class_ptr = best->wrapper;
			m_bn.parent_static_fields = best->parent;
			m_bn.entities = best->entities;
			std::printf( "[verify] bn_chain: using runtime verified wrapper=0x%X parent=0x%X entities=0x%X size=%d\n",
				m_bn.wrapper_class_ptr,
				m_bn.parent_static_fields,
				m_bn.entities,
				best->size );
		}
	}
}

void c_core::verify_camera_chain( )
{
	auto game_base = c_runtime::game_base( );
	if ( c_verification::check_camera(
		game_base,
		m_camera.main_camera_c,
		m_camera.camera_static,
		m_camera.camera_object,
		m_camera.entity ) )
	{
		std::printf( "[verify] camera: PASSED (managed chain static=0x%X object=0x%X entity=0x%X)\n",
			m_camera.camera_static, m_camera.camera_object, m_camera.entity );
		return;
	}

	auto hits = c_verification::bruteforce_camera( game_base, m_camera.main_camera_c );

	if ( hits.empty( ) )
	{
		std::printf( "[verify] camera: no valid chain found (not in a server?)\n" );
		return;
	}

	auto& h = hits[ 0 ];
	std::printf( "[verify] camera bruteforce: static=0x%X object=0x%X entity=0x%X pos=(%.1f, %.1f, %.1f)\n",
		h.camera_static, h.camera_object, h.entity, h.pos_x, h.pos_y, h.pos_z );

	bool match = h.camera_static == m_camera.camera_static &&
		h.camera_object == m_camera.camera_object &&
		h.entity == m_camera.entity;

	if ( match )
		std::printf( "[verify] camera: PASSED\n" );
	else
	{
		std::printf( "[verify] camera: MISMATCH\n" );
		std::printf( "[verify]   dumped:  static=0x%X object=0x%X entity=0x%X\n",
			m_camera.camera_static, m_camera.camera_object, m_camera.entity );
		std::printf( "[verify]   runtime: static=0x%X object=0x%X entity=0x%X\n",
			h.camera_static, h.camera_object, h.entity );
	}
}

void c_core::verify_all( )
{
	std::printf( "\n[verify] === bruteforce verification ===\n" );

	auto game_base = c_runtime::game_base( );

	auto bn_hits = c_verification::bruteforce_bn(
		game_base, m_bn.base_networkable_rva,
		m_bn.decrypt_0, m_bn.decrypt_1, m_bn.hv_offset, m_bn.parent_static_fields );

	if ( bn_hits.empty( ) )
	{
		std::printf( "[verify] bn_chain bruteforce: no hits (not in a server, or decrypt ops wrong)\n" );
	}
	else
	{
		bool found = false;
		for ( auto& h : bn_hits )
		{
			if ( h.wrapper == m_bn.wrapper_class_ptr &&
				h.parent == m_bn.parent_static_fields &&
				h.entities == m_bn.entities )
			{
				std::printf( "[verify] bn_chain bruteforce: PASSED (entities=%d)\n", h.size );
				found = true;
				break;
			}
		}
		if ( !found )
		{
			std::printf( "[verify] bn_chain bruteforce: MISMATCH\n" );
			std::printf( "[verify]   dumped:  wrapper=0x%X parent=0x%X entities=0x%X\n",
				m_bn.wrapper_class_ptr, m_bn.parent_static_fields, m_bn.entities );
			for ( auto& h : bn_hits )
				std::printf( "[verify]   runtime: wrapper=0x%X parent=0x%X entities=0x%X size=%d\n",
					h.wrapper, h.parent, h.entities, h.size );
		}
	}

	verify_camera_chain( );

	std::printf( "[verify] === done ===\n\n" );
}

static rust::console_system::command* safe_find_command( system_c::string_t* str )
{
	__try { return rust::console_system::client::find( str ); }
	__except ( EXCEPTION_EXECUTE_HANDLER ) { return nullptr; }
}

static std::uint64_t safe_get_setter( rust::console_system::command* cmd )
{
	__try { return cmd->set( ); }
	__except ( EXCEPTION_EXECUTE_HANDLER ) { return 0; }
}

static std::uint64_t safe_get_getter( rust::console_system::command* cmd )
{
	__try { return cmd->get( ); }
	__except ( EXCEPTION_EXECUTE_HANDLER ) { return 0; }
}

static std::uint64_t safe_get_call( rust::console_system::command* cmd )
{
	__try { return cmd->call( ); }
	__except ( EXCEPTION_EXECUTE_HANDLER ) { return 0; }
}

static bool fn_references_offset( std::uintptr_t fn, std::uint64_t offset, std::size_t scan_size = 0x300 )
{
	if ( !fn || !offset || IsBadReadPtr( ( void* )fn, 0x100 ) )
		return false;

	auto p = ( std::uint8_t* )fn;
	auto end = p + scan_size;
	while ( p < end )
	{
		hde64s hs;
		auto len = hde64_disasm( p, &hs );
		if ( !len || ( hs.flags & F_ERROR ) )
			break;
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;

		if ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )offset )
			return true;
		if ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )offset )
			return true;

		p += len;
	}

	return false;
}

static bool fn_starts_with_jump_thunk( std::uintptr_t fn )
{
	if ( !fn || IsBadReadPtr( ( void* )fn, 0x10 ) )
		return true;

	hde64s hs;
	auto len = hde64_disasm( ( void* )fn, &hs );
	if ( !len || ( hs.flags & F_ERROR ) )
		return true;

	if ( hs.opcode == 0xE9 || hs.opcode == 0xEB )
		return true;
	if ( hs.opcode == 0xFF && hs.modrm_reg == 4 )
		return true;

	return false;
}

static std::uintptr_t resolve_jump_thunk( std::uintptr_t fn, int depth = 0 )
{
	if ( !fn || depth > 4 || IsBadReadPtr( ( void* )fn, 0x10 ) )
		return 0;

	hde64s hs;
	auto len = hde64_disasm( ( void* )fn, &hs );
	if ( !len || ( hs.flags & F_ERROR ) )
		return 0;

	auto p = ( std::uint8_t* )fn;
	if ( hs.opcode == 0xE9 && ( hs.flags & F_RELATIVE ) )
	{
		auto target = ( std::uintptr_t )( p + len + ( std::int32_t )hs.imm.imm32 );
		return resolve_jump_thunk( target, depth + 1 );
	}
	if ( hs.opcode == 0xEB && ( hs.flags & F_RELATIVE ) )
	{
		auto target = ( std::uintptr_t )( p + len + ( std::int8_t )hs.imm.imm8 );
		return resolve_jump_thunk( target, depth + 1 );
	}

	return fn;
}

static bool fn_has_hidden_value_decrypt_shape( std::uintptr_t fn, std::size_t scan_size = 0x300 )
{
	if ( fn_starts_with_jump_thunk( fn ) || IsBadReadPtr( ( void* )fn, 0x100 ) )
		return false;

	bool has_handle_present = false;
	bool has_handle_value = false;
	bool has_two_halves = false;
	bool has_loop_load = false;
	bool has_loop_store = false;

	auto p = ( std::uint8_t* )fn;
	auto end = p + scan_size;
	while ( p < end )
	{
		hde64s hs;
		auto len = hde64_disasm( p, &hs );
		if ( !len || ( hs.flags & F_ERROR ) )
			break;
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;
		if ( hs.opcode == 0xE9 || hs.opcode == 0xEB || ( hs.opcode == 0xFF && hs.modrm_reg == 4 ) )
			break;

		if ( ( hs.flags & F_DISP32 ) || ( hs.flags & F_DISP8 ) )
		{
			auto disp = ( hs.flags & F_DISP32 )
				? ( std::int64_t )hs.disp.disp32
				: ( std::int64_t )( std::int8_t )hs.disp.disp8;

			if ( disp == 0x10 || disp == 0x14 )
				has_handle_present = true;
			else if ( disp == 0x18 )
				has_handle_value = true;
		}

		if ( hs.opcode >= 0xB8 && hs.opcode <= 0xBF && ( hs.flags & F_IMM32 ) && hs.imm.imm32 == 2 )
			has_two_halves = true;
		if ( p + 6 <= end && p[ 0 ] == 0x41 && p[ 1 ] >= 0xB8 && p[ 1 ] <= 0xBF &&
			p[ 2 ] == 0x02 && p[ 3 ] == 0x00 && p[ 4 ] == 0x00 && p[ 5 ] == 0x00 )
			has_two_halves = true;
		if ( hs.opcode == 0x8B && hs.modrm_mod != 3 )
			has_loop_load = true;
		if ( hs.opcode == 0x89 && hs.modrm_mod != 3 &&
			( ( hs.flags & F_DISP8 ) && ( std::uint8_t )hs.disp.disp8 == 0xFC ) )
			has_loop_store = true;

		p += len;
	}

	return has_handle_present && has_handle_value && has_two_halves && has_loop_load && has_loop_store;
}

static int active_item_static_slot_score( std::uintptr_t fn )
{
	if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
		return 0;

	std::uint32_t slots[ 8 ]{ };
	int count = 0;
	bool after_static_fields_load = false;

	auto push_slot = [&]( std::uint32_t slot )
	{
		if ( !slot || slot > 0x80 || count >= 8 )
			return;
		slots[ count++ ] = slot;
	};

	auto p = ( std::uint8_t* )fn;
	auto end = p + 0x300;
	while ( p < end )
	{
		hde64s hs;
		auto len = hde64_disasm( p, &hs );
		if ( !len || ( hs.flags & F_ERROR ) )
			break;
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;

		std::uint32_t disp = 0;
		if ( hs.flags & F_DISP32 )
			disp = hs.disp.disp32;
		else if ( hs.flags & F_DISP8 )
			disp = ( std::uint8_t )hs.disp.disp8;

		if ( after_static_fields_load && hs.opcode == 0x8B && hs.modrm_mod != 3 && disp <= 0x80 )
		{
			push_slot( disp );
			after_static_fields_load = false;
		}

		if ( hs.opcode == 0x8B && hs.modrm_mod != 3 && disp == 0xB8 )
			after_static_fields_load = true;

		p += len;
	}

	if ( count < 2 )
		return count ? 100 : 0;

	int unique = 0;
	for ( int i = 0; i < count; i++ )
	{
		bool seen = false;
		for ( int j = 0; j < i; j++ )
			if ( slots[ j ] == slots[ i ] )
				seen = true;
		if ( !seen )
			unique++;
	}

	if ( unique == 1 )
		return 900 + count * 20;
	return -900 - unique * 50;
}

static bool field_disp_candidate( std::uint32_t disp )
{
	return disp >= 0x80 && disp <= 0x1000 && ( disp & 3 ) == 0;
}

static std::uint64_t first_field_offset_referenced_before_decrypt_call( std::uintptr_t fn )
{
	if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
		return 0;

	std::uint32_t recent[ 8 ]{ };
	int recent_count = 0;
	auto remember = [&]( std::uint32_t disp )
	{
		if ( !field_disp_candidate( disp ) )
			return;
		if ( recent_count < 8 )
			recent[ recent_count++ ] = disp;
		else
		{
			for ( int i = 1; i < 8; i++ )
				recent[ i - 1 ] = recent[ i ];
			recent[ 7 ] = disp;
		}
	};

	auto p = ( std::uint8_t* )fn;
	auto end = p + 0x300;
	while ( p < end )
	{
		hde64s hs;
		auto len = hde64_disasm( p, &hs );
		if ( !len || ( hs.flags & F_ERROR ) )
			break;
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;

		if ( hs.flags & F_DISP32 )
			remember( hs.disp.disp32 );
		else if ( hs.flags & F_DISP8 )
			remember( hs.disp.disp8 );

		if ( p[ 0 ] == 0xE8 && len >= 5 )
		{
			std::int32_t d;
			std::memcpy( &d, p + 1, 4 );
			auto target = resolve_jump_thunk( ( std::uintptr_t )( p + 5 + d ) );
			if ( target && fn_has_hidden_value_decrypt_shape( target ) && recent_count )
				return recent[ recent_count - 1 ];
		}

		p += len;
	}

	return 0;
}

static std::uint64_t single_field_offset_referenced_by_method( il2cpp::method_info_t* method )
{
	if ( !method )
		return 0;

	auto fn = method->get_fn_ptr<std::uintptr_t>( );
	if ( !fn || IsBadReadPtr( ( void* )fn, 0x80 ) )
		return 0;

	std::uint32_t offsets[ 8 ]{ };
	int count = 0;

	auto add = [&]( std::uint32_t disp )
	{
		if ( !field_disp_candidate( disp ) )
			return;
		for ( int i = 0; i < count; i++ )
			if ( offsets[ i ] == disp )
				return;
		if ( count < 8 )
			offsets[ count++ ] = disp;
	};

	auto p = ( std::uint8_t* )fn;
	auto end = p + 0x180;
	while ( p < end )
	{
		hde64s hs;
		auto len = hde64_disasm( p, &hs );
		if ( !len || ( hs.flags & F_ERROR ) )
			break;
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;

		if ( hs.flags & F_DISP32 )
			add( hs.disp.disp32 );
		else if ( hs.flags & F_DISP8 )
			add( hs.disp.disp8 );

		p += len;
	}

	return count == 1 ? offsets[ 0 ] : 0;
}

static std::uint64_t getter_field_offset_by_return_class(
	il2cpp::il2cpp_class_t* klass,
	il2cpp::il2cpp_class_t* return_class )
{
	if ( !klass || !return_class )
		return 0;

	void* iter = nullptr;
	while ( auto method = klass->methods( &iter ) )
	{
		if ( !method || method->param_count( ) != 0 )
			continue;

		auto ret = method->return_type( );
		if ( !ret || ret->klass( ) != return_class )
			continue;

		if ( auto off = single_field_offset_referenced_by_method( method ) )
			return off;
	}

	return 0;
}

static std::uint64_t method_field_offset_by_name_contains(
	il2cpp::il2cpp_class_t* klass,
	const char* a,
	const char* b )
{
	if ( !klass || !a )
		return 0;

	void* iter = nullptr;
	while ( auto method = klass->methods( &iter ) )
	{
		if ( !method )
			continue;
		auto name = method->name( );
		if ( !ascii_contains_i( name, a ) || ( b && !ascii_contains_i( name, b ) ) )
			continue;
		if ( auto off = single_field_offset_referenced_by_method( method ) )
			return off;
	}

	return 0;
}

static std::uint64_t getter_field_offset_by_return_type_name(
	il2cpp::il2cpp_class_t* klass,
	const char* type,
	const char* name_a,
	const char* name_b )
{
	if ( !klass || !type )
		return 0;

	void* iter = nullptr;
	while ( auto method = klass->methods( &iter ) )
	{
		if ( !method || method->param_count( ) != 0 )
			continue;

		auto name = method->name( );
		if ( name_a && !ascii_contains_i( name, name_a ) )
			continue;
		if ( name_b && !ascii_contains_i( name, name_b ) )
			continue;

		auto ret = method->return_type( );
		auto ret_name = ret ? ret->name( ) : nullptr;
		if ( !ascii_contains_i( ret_name, type ) )
			continue;

		if ( auto off = single_field_offset_referenced_by_method( method ) )
			return off;
	}

	return 0;
}

static std::uint64_t fov_write_offset_from_fn( std::uint8_t* fn, int depth )
{
	if ( !fn || depth > 2 || IsBadReadPtr( fn, 0x100 ) )
		return 0;

	bool direct_crypto = false;
	auto direct = c_decryption::extract_inline( fn, 0x100 );
	if ( direct.valid )
		direct_crypto = true;

	bool saw_crypto = direct_crypto;
	std::uint32_t best_write = 0;
	std::uint32_t nested_fallback = 0;
	std::uint32_t recent[ 8 ]{ };
	int recent_count = 0;
	auto remember_disp = [&]( std::uint32_t disp )
	{
		if ( !field_disp_candidate( disp ) )
			return;
		if ( recent_count < 8 )
			recent[ recent_count++ ] = disp;
		else
		{
			for ( int i = 1; i < 8; i++ )
				recent[ i - 1 ] = recent[ i ];
			recent[ 7 ] = disp;
		}
	};

	auto p = fn;
	auto end = fn + 0x500;
	while ( p < end )
	{
		hde64s hs;
		auto len = hde64_disasm( p, &hs );
		if ( !len || ( hs.flags & F_ERROR ) )
			break;
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;

		if ( hs.opcode == 0x35 || hs.opcode == 0x05 || hs.opcode == 0x2D ||
			( hs.opcode == 0x81 && hs.modrm_mod == 3 ) ||
			( hs.opcode == 0xC1 && hs.modrm_mod == 3 ) )
			saw_crypto = true;

		if ( hs.flags & F_DISP32 )
		{
			remember_disp( hs.disp.disp32 );
			if ( saw_crypto && hs.modrm_mod != 3 && hs.disp.disp32 >= 0x80 && hs.disp.disp32 <= 0x1000 )
				best_write = hs.disp.disp32;
		}
		else if ( hs.flags & F_DISP8 )
		{
			remember_disp( hs.disp.disp8 );
			auto disp = ( std::uint8_t )hs.disp.disp8;
			if ( saw_crypto && hs.modrm_mod != 3 && disp >= 0x80 )
				best_write = disp;
		}

		if ( p[ 0 ] == 0xE8 && len >= 5 )
		{
			std::int32_t d;
			std::memcpy( &d, p + 1, 4 );
			auto target = p + 5 + d;
			if ( !IsBadReadPtr( target, 0x100 ) )
			{
				auto constants = c_decryption::extract_inline( target, 0x100 );
				if ( constants.valid )
					saw_crypto = true;

				if ( auto nested = fov_write_offset_from_fn( target, depth + 1 ) )
				{
					if ( !nested_fallback )
						nested_fallback = ( std::uint32_t )nested;
				}
			}
		}

		p += len;
	}

	if ( best_write )
		return best_write;
	if ( nested_fallback )
		return nested_fallback;
	if ( recent_count > 0 )
		return recent[ recent_count - 1 ];

	return 0;
}

static c_decryption::constants_t extract_stack_u32_crypto( std::uint8_t* fn, std::size_t scan_size )
{
	c_decryption::constants_t c{ };
	if ( !fn || IsBadReadPtr( fn, scan_size ) )
		return c;

	bool in_loop = false;
	std::size_t pos = 0;
	while ( pos < scan_size && c.op_count < c_decryption::max_ops )
	{
		hde64s hs;
		auto len = hde64_disasm( fn + pos, &hs );
		if ( !len )
			break;
		if ( hs.flags & F_ERROR )
		{
			pos += len;
			continue;
		}
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;

		auto p = fn + pos;
		if ( !in_loop )
		{
			if ( len >= 2 && p[ 0 ] == 0x8B && p[ 1 ] == 0x02 )
				in_loop = true;
			pos += len;
			continue;
		}

		if ( len >= 3 && p[ 0 ] == 0x89 && ( p[ 1 ] == 0x42 || p[ 1 ] == 0x4A ) && p[ 2 ] == 0xFC )
			break;

		if ( hs.rex_w )
		{
			pos += len;
			continue;
		}

		if ( pos + 14 <= scan_size &&
			p[ 0 ] == 0x8B && p[ 1 ] == 0xC8 &&
			p[ 2 ] == 0xC1 && p[ 3 ] == 0xE9 && p[ 4 ] > 0 && p[ 4 ] < 32 &&
			p[ 5 ] == 0x8D && p[ 6 ] == 0x04 && p[ 7 ] == 0xC5 &&
			p[ 8 ] == 0x00 && p[ 9 ] == 0x00 && p[ 10 ] == 0x00 && p[ 11 ] == 0x00 &&
			p[ 12 ] == 0x0B && p[ 13 ] == 0xC8 )
		{
			c.ops[ c.op_count++ ] = { c_decryption::op_t::op_rol, 32u - p[ 4 ] };
			pos += 14;
			continue;
		}

		if ( pos + 12 <= scan_size &&
			p[ 0 ] == 0xC1 && p[ 1 ] == 0xE9 && p[ 2 ] > 0 && p[ 2 ] < 32 &&
			p[ 3 ] == 0x8D && p[ 4 ] == 0x04 && ( p[ 5 ] == 0x85 || p[ 5 ] == 0xC5 ) &&
			p[ 10 ] == 0x0B && ( p[ 11 ] == 0xC8 || p[ 11 ] == 0xC1 ) )
		{
			c.ops[ c.op_count++ ] = { c_decryption::op_t::op_rol, 32u - p[ 2 ] };
			pos += 12;
			continue;
		}

		if ( pos + 7 <= scan_size &&
			( ( p[ 0 ] == 0x01 && p[ 1 ] == 0xC0 ) || ( p[ 0 ] == 0x03 && p[ 1 ] == 0xC0 ) ) &&
			p[ 2 ] == 0xC1 && p[ 3 ] == 0xE9 && p[ 4 ] == 0x1F &&
			p[ 5 ] == 0x0B && p[ 6 ] == 0xC8 )
		{
			c.ops[ c.op_count++ ] = { c_decryption::op_t::op_rol, 1 };
			pos += 7;
			continue;
		}

		if ( hs.opcode == 0x81 && hs.modrm_mod == 3 && ( hs.flags & F_IMM32 ) )
		{
			if ( hs.modrm_reg == 6 )
				c.ops[ c.op_count++ ] = { c_decryption::op_t::op_xor, hs.imm.imm32 };
			else if ( hs.modrm_reg == 0 )
				c.ops[ c.op_count++ ] = { c_decryption::op_t::op_add, hs.imm.imm32 };
			else if ( hs.modrm_reg == 5 )
				c.ops[ c.op_count++ ] = { c_decryption::op_t::op_sub, hs.imm.imm32 };
		}
		else if ( hs.opcode == 0x35 && ( hs.flags & F_IMM32 ) )
			c.ops[ c.op_count++ ] = { c_decryption::op_t::op_xor, hs.imm.imm32 };
		else if ( hs.opcode == 0x05 && ( hs.flags & F_IMM32 ) )
			c.ops[ c.op_count++ ] = { c_decryption::op_t::op_add, hs.imm.imm32 };
		else if ( hs.opcode == 0x2D && ( hs.flags & F_IMM32 ) )
			c.ops[ c.op_count++ ] = { c_decryption::op_t::op_sub, hs.imm.imm32 };
		else if ( hs.opcode == 0x8D && hs.modrm_mod != 3 && ( hs.flags & F_DISP32 ) )
		{
			auto disp = ( std::int32_t )hs.disp.disp32;
			if ( disp < 0 )
				c.ops[ c.op_count++ ] = { c_decryption::op_t::op_sub, ( std::uint32_t )-disp };
			else if ( disp > 0 )
				c.ops[ c.op_count++ ] = { c_decryption::op_t::op_add, ( std::uint32_t )disp };
		}
		else if ( hs.opcode == 0xC1 && hs.modrm_mod == 3 &&
			( hs.modrm_reg == 0 || hs.modrm_reg == 1 || hs.modrm_reg == 4 ) &&
			hs.imm.imm8 > 0 && hs.imm.imm8 < 32 )
		{
			auto imm = ( std::uint32_t )hs.imm.imm8;
			c.ops[ c.op_count++ ] = { c_decryption::op_t::op_rol, hs.modrm_reg == 1 ? 32u - imm : imm };
		}

		pos += len;
	}

	if ( c.op_count >= 2 )
	{
		bool has_large = false;
		for ( int i = 0; i < c.op_count; i++ )
			if ( c.ops[ i ].type != c_decryption::op_t::op_rol && c.ops[ i ].value > 0x10000 )
				has_large = true;
		c.valid = has_large;
	}
	return c;
}

static std::uint32_t stack_u32_loop_count( std::uint8_t* fn, std::size_t scan_size )
{
	if ( !fn || IsBadReadPtr( fn, scan_size ) )
		return 0;

	for ( std::size_t pos = 0; pos < scan_size; )
	{
		hde64s hs;
		auto len = hde64_disasm( fn + pos, &hs );
		if ( !len || ( hs.flags & F_ERROR ) )
			break;
		if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
			break;

		auto p = fn + pos;
		if ( len >= 2 && p[ 0 ] == 0x8B && p[ 1 ] == 0x02 )
			break;
		if ( len >= 6 && p[ 0 ] == 0x41 && ( p[ 1 ] >= 0xB8 && p[ 1 ] <= 0xBF ) )
		{
			std::uint32_t imm = 0;
			std::memcpy( &imm, p + 2, sizeof( imm ) );
			if ( imm > 0 && imm <= 8 )
				return imm;
		}
		if ( len >= 5 && p[ 0 ] >= 0xB8 && p[ 0 ] <= 0xBF )
		{
			std::uint32_t imm = 0;
			std::memcpy( &imm, p + 1, sizeof( imm ) );
			if ( imm > 0 && imm <= 8 )
				return imm;
		}

		pos += len;
	}

	return 0;
}

static std::uint64_t fov_write_offset_from_console( )
{
	if ( !rust::console_system::console_system_index_client_find )
		return 0;

	auto str = system_c::string_t::create_string( L"graphics.fov" );
	if ( !str )
		return 0;

	auto cmd = safe_find_command( str );
	if ( !cmd )
		return 0;

	auto setter = rust::console_system::set_override_offset ? safe_get_setter( cmd ) : 0;
	if ( setter )
		if ( auto off = fov_write_offset_from_fn( ( std::uint8_t* )setter, 0 ) )
			return off;

	auto getter = rust::console_system::get_override_offset ? safe_get_getter( cmd ) : 0;
	if ( getter )
		if ( auto off = fov_write_offset_from_fn( ( std::uint8_t* )getter, 0 ) )
			return off;

	auto caller = rust::console_system::call_offset ? safe_get_call( cmd ) : 0;
	return caller ? fov_write_offset_from_fn( ( std::uint8_t* )caller, 0 ) : 0;
}

static std::uint32_t apply_u32_ops( std::uint32_t value, const c_decryption::constants_t& c, bool inverse )
{
	int begin = inverse ? c.op_count - 1 : 0;
	int end = inverse ? -1 : c.op_count;
	int step = inverse ? -1 : 1;

	for ( int i = begin; i != end; i += step )
	{
		auto type = c.ops[ i ].type;
		auto op_value = c.ops[ i ].value;
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
			value += op_value;
			break;
		case c_decryption::op_t::op_sub:
			value -= op_value;
			break;
		case c_decryption::op_t::op_xor:
			value ^= op_value;
			break;
		case c_decryption::op_t::op_rol:
		{
			auto rot = op_value & 31u;
			if ( rot )
			{
				if ( inverse )
					value = ( value >> rot ) | ( value << ( 32u - rot ) );
				else
					value = ( value << rot ) | ( value >> ( 32u - rot ) );
			}
			break;
		}
		}
	}

	return value;
}

static bool fov_bits_sane( std::uint32_t bits )
{
	float value = 0.f;
	std::memcpy( &value, &bits, sizeof( value ) );
	return std::isfinite( value ) && value >= 25.f && value <= 130.f;
}

static bool object_is_class( std::uintptr_t object, il2cpp::il2cpp_class_t* klass )
{
	if ( !object || !klass )
		return false;
	__try
	{
		return il2cpp::object_get_class( ( void* )object ) == klass;
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
}

static bool class_is_or_inherits( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* target )
{
	if ( !klass || !target )
		return false;
	__try
	{
		for ( auto cur = klass; cur; cur = cur->parent( ) )
			if ( cur == target )
				return true;
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
	return false;
}

static bool object_is_class_or_parent( std::uintptr_t object, il2cpp::il2cpp_class_t* klass )
{
	if ( !object || !klass )
		return false;
	__try
	{
		return class_is_or_inherits( il2cpp::object_get_class( ( void* )object ), klass );
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
}

static std::uintptr_t read_ptr_safe( std::uintptr_t address )
{
	if ( !is_valid_userland_ptr( address ) )
		return 0;
	__try
	{
		auto value = *( std::uintptr_t* )address;
		return is_valid_userland_ptr( value ) ? value : 0;
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return 0;
	}
}

static std::uintptr_t managed_from_native_entity( std::uintptr_t native_entity, il2cpp::il2cpp_class_t* target )
{
	auto p1 = read_ptr_safe( native_entity + 0x20 );
	auto p2 = read_ptr_safe( p1 + 0x18 );
	auto p3 = read_ptr_safe( p2 + 0x18 );
	auto object = read_ptr_safe( p3 );
	return object_is_class_or_parent( object, target ) ? object : 0;
}

static std::uintptr_t resolve_entity_entry_object( std::uintptr_t entry, il2cpp::il2cpp_class_t* target )
{
	if ( !entry || !target )
		return 0;

	const std::uintptr_t direct_candidates[] =
	{
		entry,
		read_ptr_safe( entry ),
		read_ptr_safe( entry + 0x8 ),
		read_ptr_safe( entry + 0x10 ),
		read_ptr_safe( entry + 0x18 ),
		read_ptr_safe( entry + 0x20 ),
	};

	for ( auto candidate : direct_candidates )
		if ( object_is_class_or_parent( candidate, target ) )
			return candidate;

	auto second = read_ptr_safe( entry + 0x10 );
	auto entity_base = read_ptr_safe( second + 0x20 );
	if ( auto object = managed_from_native_entity( entity_base, target ) )
		return object;

	entity_base = read_ptr_safe( entry + 0x20 );
	if ( auto object = managed_from_native_entity( entity_base, target ) )
		return object;

	return 0;
}

static int score_hidden_value_field(
	std::uint64_t base_networkable_rva,
	std::uint32_t bn_static_fields,
	std::uint32_t wrapper_class_ptr,
	std::uint32_t parent_static_fields,
	std::uint32_t entities,
	std::uint64_t hv_offset,
	const c_decryption::constants_t& decrypt_0,
	const c_decryption::constants_t& decrypt_1,
	std::uint64_t field_offset,
	il2cpp::il2cpp_class_t* target_class,
	const c_decryption::constants_t& field_decrypt )
{
	if ( !base_networkable_rva || !decrypt_0.valid || !decrypt_1.valid ||
		!field_offset || !target_class || !field_decrypt.valid )
		return 0;

	using gchandle_fn_t = void* ( * )( std::uint32_t );
	auto gchandle_get_target = ( gchandle_fn_t )GetProcAddress(
		GetModuleHandleA( "GameAssembly.dll" ), "il2cpp_gchandle_get_target" );
	if ( !gchandle_get_target )
		return 0;

	auto decrypt_handle = [&]( std::uintptr_t wrapper_ptr, std::uint64_t read_off, const c_decryption::constants_t& c ) -> std::uintptr_t
	{
		if ( !is_valid_userland_ptr( wrapper_ptr + read_off ) )
			return 0;
		auto encrypted = *( std::uint64_t* )( wrapper_ptr + read_off );
		auto decrypted = c_decryption::simulate( encrypted, c );
		if ( ( decrypted & 1 ) == 0 && is_valid_userland_ptr( decrypted ) )
		{
			auto target = *( std::uintptr_t* )decrypted;
			if ( is_valid_userland_ptr( target ) )
				return target;
		}
		auto handle = ( std::uint32_t )decrypted;
		if ( !handle || handle > 0x1000000 )
			return 0;
		return safe_gchandle_get_target( gchandle_get_target, handle );
	};

	auto game_base = c_runtime::game_base( );
	auto klass = *( std::uintptr_t* )( game_base + base_networkable_rva );
	if ( !is_valid_userland_ptr( klass ) )
		return 0;
	auto static_fields = *( std::uintptr_t* )( klass + bn_static_fields );
	if ( !is_valid_userland_ptr( static_fields ) )
		return 0;
	auto wrapper = *( std::uintptr_t* )( static_fields + wrapper_class_ptr );
	auto realm = decrypt_handle( wrapper, hv_offset, decrypt_0 );
	if ( !is_valid_userland_ptr( realm ) )
		return 0;
	auto parent = *( std::uintptr_t* )( realm + parent_static_fields );
	auto list_dict = decrypt_handle( parent, hv_offset, decrypt_1 );
	if ( !is_valid_userland_ptr( list_dict ) )
		return 0;
	auto buffer_list = *( std::uintptr_t* )( list_dict + entities );
	if ( !is_valid_userland_ptr( buffer_list ) )
		return 0;
	auto array = *( std::uintptr_t* )( buffer_list + 0x10 );
	auto count = *( std::int32_t* )( buffer_list + 0x18 );
	if ( !is_valid_userland_ptr( array ) || count <= 0 || count > 50000 )
		return 0;

	auto base_player_class = c_runtime::find_class( "BasePlayer" );
	if ( !base_player_class )
		base_player_class = c_runtime::find_class( "BaseNetworkable" );
	if ( !base_player_class )
		return 0;

	int hits = 0;
	auto limit = count > 2048 ? 2048 : count;
	for ( std::int32_t i = 0; i < limit; i++ )
	{
		auto entry = *( std::uintptr_t* )( array + 0x20 + ( i * 8 ) );
		auto entity = resolve_entity_entry_object( entry, base_player_class );
		if ( !is_valid_userland_ptr( entity + field_offset ) )
			continue;

		auto resolved = decrypt_handle( entity + field_offset, hv_offset, field_decrypt );
		if ( object_is_class( resolved, target_class ) )
		{
			hits++;
			continue;
		}

		auto direct = *( std::uintptr_t* )( entity + field_offset );
		if ( is_valid_userland_ptr( direct ) )
		{
			resolved = decrypt_handle( direct, hv_offset, field_decrypt );
			if ( object_is_class( resolved, target_class ) )
				hits++;
		}
	}

	return hits;
}

static std::uint64_t item_held_entity_offset(
	il2cpp::il2cpp_class_t* klass,
	std::uint64_t uid_offset )
{
	const char* held_entity_names[] = { "heldEntity", "held_entity", "heldentity", "_heldEntity" };
	if ( auto off = field_offset_by_names( klass, held_entity_names, 4 ) )
		return off;

	std::uint64_t first_entity_ref = 0;
	std::uint64_t second_entity_ref = 0;
	std::uint64_t best_before_uid = 0;

	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;

			auto type = f->type( );
			auto type_class = type ? type->klass( ) : nullptr;
			if ( !is_entity_ref_type( type_class ) )
				continue;

			auto off = f->offset( cur );
			if ( !first_entity_ref )
				first_entity_ref = off;
			else if ( !second_entity_ref )
				second_entity_ref = off;
			if ( uid_offset && off > 0x20 && off < uid_offset && off > best_before_uid )
				best_before_uid = off;
		}
	}

	if ( best_before_uid )
		return best_before_uid;
	if ( second_entity_ref )
		return second_entity_ref;
	if ( first_entity_ref )
		return first_entity_ref;
	return highest_repeated_wrapper_field_offset( klass );
}

c_core::item_offsets_t c_core::resolve_item_offsets( )
{
	item_offsets_t out{ };
	auto klass = m_item_class ? m_item_class : c_runtime::find_class( "Item" );
	auto item_definition = c_runtime::find_class( "ItemDefinition" );
	if ( !klass && item_definition )
		klass = class_with_instance_field_type_class( item_definition );

	out.info = field_offset_by_type_class( klass, item_definition );
	if ( !out.info )
		out.info = field_offset_by_type_or_class_contains( klass, "ItemDefinition" );

	out.uid = field_offset_by_name( klass, "uid" );
	if ( !out.uid )
		out.uid = field_offset_by_type_class( klass, m_item_id_class );

	out.amount = field_offset_by_name( klass, "amount" );
	if ( !out.amount )
		out.amount = highest_field_offset_by_type_name( klass, "System.Int32" );
	if ( !out.amount )
		out.amount = highest_field_offset_by_type_contains( klass, "Int32" );

	out.held_entity = item_held_entity_offset( klass, out.uid );

	return out;
}

static std::uint32_t score_cl_active_item_live(
	std::uint64_t base_networkable_rva,
	std::uint32_t bn_static_fields,
	std::uint32_t wrapper_class_ptr,
	std::uint32_t parent_static_fields,
	std::uint32_t entities,
	std::uint64_t hv_offset,
	const c_decryption::constants_t& decrypt_0,
	const c_decryption::constants_t& decrypt_1,
	const c_decryption::constants_t& inventory_decrypt,
	const c_decryption::constants_t& active_item_decrypt,
	std::uint64_t cl_active_item_offset,
	std::uint64_t player_inventory_main,
	std::uint64_t player_inventory_belt,
	std::uint64_t player_inventory_wear,
	std::uint64_t item_uid_offset,
	il2cpp::il2cpp_class_t* item_class,
	il2cpp::il2cpp_class_t* item_container_class,
	std::uint32_t* out_active_values,
	std::uint32_t* out_inventory_hits,
	std::uint32_t* out_container_hits )
{
	if ( !base_networkable_rva || !decrypt_0.valid || !decrypt_1.valid ||
		!inventory_decrypt.valid || !active_item_decrypt.valid ||
		!cl_active_item_offset || !item_container_class )
		return 0;

	auto base_player = c_runtime::find_class( "BasePlayer" );
	auto player_inventory_class = c_runtime::find_class( "PlayerInventory" );
	if ( !base_player || !player_inventory_class )
		return 0;

	auto player_inventory_off = field_offset_by_type_class( base_player, player_inventory_class );
	if ( !player_inventory_off )
		player_inventory_off = field_offset_by_type_contains( base_player, "PlayerInventory" );
	if ( !player_inventory_off )
		return 0;

	std::uint64_t item_list_off = 0;
	char search[ 128 ]{ };
	std::snprintf( search, sizeof( search ),
		"System.Collections.Generic.List<%s>",
		item_class ? item_class->name( ) : "Item" );
	if ( auto f = il2cpp::get_field_if_type_contains( item_container_class, search ) )
		item_list_off = f->offset( );
	if ( !item_list_off )
		return 0;

	auto gchandle_get_target = ( gchandle_get_target_fn_t )GetProcAddress(
		GetModuleHandleA( "GameAssembly.dll" ), "il2cpp_gchandle_get_target" );
	if ( !gchandle_get_target )
		return 0;

	auto decrypt_handle = [&]( std::uintptr_t wrapper_ptr, std::uint64_t read_off, const c_decryption::constants_t& c ) -> std::uintptr_t
	{
		if ( !is_valid_userland_ptr( wrapper_ptr + read_off ) )
			return 0;
		auto encrypted = *( std::uint64_t* )( wrapper_ptr + read_off );
		auto decrypted = c_decryption::simulate( encrypted, c );
		if ( ( decrypted & 1 ) == 0 && is_valid_userland_ptr( decrypted ) )
		{
			auto target = *( std::uintptr_t* )decrypted;
			if ( is_valid_userland_ptr( target ) )
				return target;
		}
		auto handle = ( std::uint32_t )decrypted;
		if ( !handle || handle > 0x1000000 )
			return 0;
		return safe_gchandle_get_target( gchandle_get_target, handle );
	};

	auto game_base = c_runtime::game_base( );
	auto bn_klass = *( std::uintptr_t* )( game_base + base_networkable_rva );
	if ( !is_valid_userland_ptr( bn_klass ) )
		return 0;
	auto static_fields = *( std::uintptr_t* )( bn_klass + bn_static_fields );
	if ( !is_valid_userland_ptr( static_fields ) )
		return 0;
	auto wrapper = *( std::uintptr_t* )( static_fields + wrapper_class_ptr );
	auto realm = decrypt_handle( wrapper, hv_offset, decrypt_0 );
	if ( !is_valid_userland_ptr( realm ) )
		return 0;
	auto parent = *( std::uintptr_t* )( realm + parent_static_fields );
	auto list_dict = decrypt_handle( parent, hv_offset, decrypt_1 );
	if ( !is_valid_userland_ptr( list_dict ) )
		return 0;
	auto buffer_list = *( std::uintptr_t* )( list_dict + entities );
	if ( !is_valid_userland_ptr( buffer_list ) )
		return 0;
	auto array = *( std::uintptr_t* )( buffer_list + 0x10 );
	auto count = *( std::int32_t* )( buffer_list + 0x18 );
	if ( !is_valid_userland_ptr( array ) || count <= 0 || count > 50000 )
		return 0;

	auto uid_matches = []( std::uint64_t value, std::uint64_t uid ) -> bool
	{
		if ( !value || value == ~0ull )
			return false;
		return value == uid || ( std::uint32_t )value == ( std::uint32_t )uid;
	};

	auto item_has_uid = [&]( std::uintptr_t item, std::uint64_t uid ) -> bool
	{
		if ( !is_valid_userland_ptr( item ) )
			return false;

		auto value_matches_uid = [&]( std::uint64_t stored ) -> bool
		{
			if ( uid_matches( stored, uid ) )
				return true;
			auto decoded = c_decryption::simulate( stored, active_item_decrypt );
			return uid_matches( decoded, uid );
		};

		const std::uint64_t preferred_offsets[] =
		{
			item_uid_offset,
			item_uid_offset ? item_uid_offset + 0x4 : 0,
			item_uid_offset ? item_uid_offset + 0x8 : 0,
			item_uid_offset ? item_uid_offset + 0x10 : 0,
		};

		for ( auto off : preferred_offsets )
		{
			if ( !off || !is_valid_userland_ptr( item + off ) )
				continue;
			auto qword = *( std::uint64_t* )( item + off );
			if ( value_matches_uid( qword ) )
				return true;
			auto dword = *( std::uint32_t* )( item + off );
			if ( value_matches_uid( dword ) )
				return true;
		}

		for ( std::uint32_t off = 0x10; off <= 0x160; off += 4 )
		{
			if ( !is_valid_userland_ptr( item + off ) )
				continue;
			auto dword = *( std::uint32_t* )( item + off );
			if ( value_matches_uid( dword ) )
				return true;
			if ( ( off & 7 ) == 0 && is_valid_userland_ptr( item + off + sizeof( std::uint64_t ) - 1 ) )
			{
				auto qword = *( std::uint64_t* )( item + off );
				if ( value_matches_uid( qword ) )
					return true;
			}
		}

		return false;
	};

	auto list_contains_uid = [&]( std::uintptr_t list, std::uint64_t uid ) -> bool
	{
		if ( !is_valid_userland_ptr( list + 0x18 ) )
			return false;
		auto item_array = *( std::uintptr_t* )( list + 0x10 );
		auto size = *( std::int32_t* )( list + 0x18 );
		if ( !is_valid_userland_ptr( item_array ) || size < 0 || size > 512 )
			return false;

		for ( std::int32_t i = 0; i < size; i++ )
		{
			auto item = *( std::uintptr_t* )( item_array + 0x20 + ( i * 8 ) );
			if ( item_class && !object_is_class( item, item_class ) )
				continue;
			if ( item_has_uid( item, uid ) )
				return true;
		}
		return false;
	};

	const std::uint64_t containers[] =
	{
		player_inventory_main,
		player_inventory_belt,
		player_inventory_wear,
	};

	std::uint32_t hits = 0;
	std::uint32_t active_values = 0;
	std::uint32_t inventory_hits = 0;
	std::uint32_t container_hits = 0;
	auto limit = count > 2048 ? 2048 : count;
	for ( std::int32_t i = 0; i < limit; i++ )
	{
		auto entry = *( std::uintptr_t* )( array + 0x20 + ( i * 8 ) );
		auto entity = resolve_entity_entry_object( entry, base_player );
		if ( !is_valid_userland_ptr( entity + cl_active_item_offset ) ||
			!is_valid_userland_ptr( entity + player_inventory_off ) )
			continue;

		auto active_encrypted = *( std::uint64_t* )( entity + cl_active_item_offset );
		auto active_uid = c_decryption::simulate( active_encrypted, active_item_decrypt );
		if ( !active_uid || active_uid == ~0ull )
			continue;
		active_values++;

		auto inventory = *( std::uintptr_t* )( entity + player_inventory_off );
		if ( !object_is_class( inventory, player_inventory_class ) )
		{
			inventory = decrypt_handle( entity + player_inventory_off, hv_offset, inventory_decrypt );
			if ( !object_is_class( inventory, player_inventory_class ) )
			{
				auto inv_wrapper = *( std::uintptr_t* )( entity + player_inventory_off );
				if ( is_valid_userland_ptr( inv_wrapper ) )
					inventory = decrypt_handle( inv_wrapper, hv_offset, inventory_decrypt );
			}
		}
		if ( !object_is_class( inventory, player_inventory_class ) )
			continue;
		inventory_hits++;

		for ( auto container_off : containers )
		{
			if ( !container_off || !is_valid_userland_ptr( inventory + container_off ) )
				continue;
			auto container = *( std::uintptr_t* )( inventory + container_off );
			if ( !object_is_class( container, item_container_class ) )
				continue;
			container_hits++;
			auto list = *( std::uintptr_t* )( container + item_list_off );
			if ( list_contains_uid( list, active_uid ) )
			{
				hits++;
				break;
			}
		}
	}

	if ( out_active_values )
		*out_active_values = active_values;
	if ( out_inventory_hits )
		*out_inventory_hits = inventory_hits;
	if ( out_container_hits )
		*out_container_hits = container_hits;

	return hits;
}

static bool read_live_convar_fov_bits( std::uint32_t& bits, std::uint64_t& fov_off )
{
	const char* resolution_list_terms[] = { "List", "Resolution" };
	auto graphics = class_with_public_static_field_type_terms( resolution_list_terms, 2 );
	if ( !graphics )
		graphics = c_runtime::find_class( "Graphics", "ConVar" );
	if ( !graphics )
		graphics = class_name_contains( "Graphics", "ConVar" );
	if ( !graphics )
		graphics = class_name_contains_i( "Graphics", "Convar" );
	if ( !graphics )
		graphics = class_with_field_name_contains_type( "fov", "Single", true );
	if ( !graphics || IsBadReadPtr( graphics, 0xC0 ) )
		return false;

	auto fov = field_offset_by_name( graphics, "fov" );
	if ( !fov )
		fov = field_offset_by_name( graphics, "_fov" );
	if ( !fov )
		fov = field_offset_by_name_contains_type( graphics, "fov", nullptr, "Single" );
	if ( !fov )
		return false;

	auto static_fields = *( std::uintptr_t* )( ( std::uintptr_t )graphics + 0xB8 );
	if ( !is_valid_userland_ptr( static_fields ) )
		return false;

	auto try_read = [&]( std::uintptr_t instance ) -> bool
	{
		if ( !is_valid_userland_ptr( instance ) || IsBadReadPtr( ( void* )( instance + fov ), sizeof( bits ) ) )
			return false;
		bits = *( std::uint32_t* )( instance + fov );
		fov_off = fov;
		return true;
	};

	if ( try_read( static_fields ) )
		return true;

	for ( std::uint32_t off = 0; off <= 0x100; off += 8 )
	{
		if ( IsBadReadPtr( ( void* )( static_fields + off ), sizeof( std::uintptr_t ) ) )
			continue;
		auto instance = *( std::uintptr_t* )( static_fields + off );
		if ( !is_valid_userland_ptr( instance ) )
			continue;
		if ( object_is_class( instance, graphics ) && try_read( instance ) )
			return true;
	}

	return false;
}

static std::uint32_t steam_api_build_id( )
{
	using steam_apps_fn_t = void* ( __cdecl* )( );
	using get_build_id_fn_t = int ( __cdecl* )( void* );

	auto steam_api = GetModuleHandleA( "steam_api64.dll" );
	if ( !steam_api )
		steam_api = GetModuleHandleA( "steam_api.dll" );
	if ( !steam_api )
		return 0;

	auto steam_apps = ( steam_apps_fn_t )GetProcAddress( steam_api, "SteamAPI_SteamApps_v008" );
	auto get_build_id = ( get_build_id_fn_t )GetProcAddress( steam_api, "SteamAPI_ISteamApps_GetAppBuildId" );
	if ( !steam_apps || !get_build_id )
		return 0;

	auto apps = steam_apps( );
	if ( !apps )
		return 0;

	auto build_id = get_build_id( apps );
	return build_id > 0 ? ( std::uint32_t )build_id : 0;
}

static std::string parse_build_id_from_acf( const std::string& path )
{
	std::ifstream file( path, std::ios::binary );
	if ( !file )
		return { };

	std::string text(
		( std::istreambuf_iterator<char>( file ) ),
		std::istreambuf_iterator<char>( ) );

	auto key = text.find( "\"buildid\"" );
	if ( key == std::string::npos )
		return { };

	auto pos = key + 9;
	while ( pos < text.size( ) && ( text[ pos ] == ' ' || text[ pos ] == '\t' || text[ pos ] == '\r' || text[ pos ] == '\n' ) )
		++pos;
	if ( pos < text.size( ) && text[ pos ] == '"' )
		++pos;

	std::string build_id;
	while ( pos < text.size( ) && text[ pos ] >= '0' && text[ pos ] <= '9' )
		build_id.push_back( text[ pos++ ] );

	return build_id;
}

static std::string steam_path_from_registry( )
{
	using reg_get_value_a_t = LSTATUS ( WINAPI* )( HKEY, LPCSTR, LPCSTR, DWORD, LPDWORD, PVOID, LPDWORD );

	auto advapi = GetModuleHandleA( "advapi32.dll" );
	if ( !advapi )
		advapi = LoadLibraryA( "advapi32.dll" );
	auto reg_get_value = advapi ? ( reg_get_value_a_t )GetProcAddress( advapi, "RegGetValueA" ) : nullptr;
	if ( !reg_get_value )
		return { };

	char path[ MAX_PATH ]{ };
	DWORD size = sizeof( path );
	if ( reg_get_value(
		HKEY_CURRENT_USER,
		"Software\\Valve\\Steam",
		"SteamPath",
		RRF_RT_REG_SZ,
		nullptr,
		path,
		&size ) == ERROR_SUCCESS )
		return path;

	size = sizeof( path );
	if ( reg_get_value(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\WOW6432Node\\Valve\\Steam",
		"InstallPath",
		RRF_RT_REG_SZ,
		nullptr,
		path,
		&size ) == ERROR_SUCCESS )
		return path;

	return { };
}

static void normalize_slashes( std::string& path )
{
	for ( auto& c : path )
		if ( c == '/' )
			c = '\\';
}

static void add_manifest_candidate( std::vector<std::string>& candidates, std::string root )
{
	if ( root.empty( ) )
		return;

	normalize_slashes( root );
	while ( !root.empty( ) && ( root.back( ) == '\\' || root.back( ) == '/' ) )
		root.pop_back( );

	candidates.push_back( root + "\\steamapps\\appmanifest_252490.acf" );
}

static std::string rust_manifest_from_process_path( )
{
	char module_path[ MAX_PATH ]{ };
	if ( !GetModuleFileNameA( nullptr, module_path, sizeof( module_path ) ) )
		return { };

	std::string path = module_path;
	normalize_slashes( path );

	auto steamapps = path.find( "\\steamapps\\common\\" );
	if ( steamapps == std::string::npos )
		return { };

	return path.substr( 0, steamapps + 10 ) + "\\appmanifest_252490.acf";
}

static std::string acf_build_id( )
{
	std::vector<std::string> candidates;

	auto process_manifest = rust_manifest_from_process_path( );
	if ( !process_manifest.empty( ) )
		candidates.push_back( process_manifest );

	add_manifest_candidate( candidates, steam_path_from_registry( ) );

	char program_files_x86[ MAX_PATH ]{ };
	if ( GetEnvironmentVariableA( "ProgramFiles(x86)", program_files_x86, sizeof( program_files_x86 ) ) )
		add_manifest_candidate( candidates, std::string( program_files_x86 ) + "\\Steam" );

	for ( const auto& candidate : candidates )
	{
		auto build_id = parse_build_id_from_acf( candidate );
		if ( !build_id.empty( ) )
			return build_id;
	}

	return { };
}

void c_core::resolve_build_id( )
{
	auto steam_build_id = steam_api_build_id( );
	if ( steam_build_id )
	{
		m_build_id = std::to_string( steam_build_id );
		std::printf( "[morphine-dumper] build id: %s (SteamApps)\n", m_build_id.c_str( ) );
		return;
	}

	m_build_id = acf_build_id( );
	if ( !m_build_id.empty( ) )
		std::printf( "[morphine-dumper] build id: %s (appmanifest)\n", m_build_id.c_str( ) );
	else
		std::printf( "[morphine-dumper] build id: unresolved\n" );
}

static bool managed_string_equals_ascii( std::uintptr_t str, const char* ascii )
{
	if ( !is_valid_userland_ptr( str ) || !ascii )
		return false;

	auto wanted_len = std::strlen( ascii );
	if ( wanted_len > 128 )
		return false;

	__try
	{
		auto len = *( std::int32_t* )( str + 0x10 );
		if ( len < 0 || ( std::size_t )len != wanted_len )
			return false;

		auto chars = ( wchar_t* )( str + 0x14 );
		for ( std::int32_t i = 0; i < len; i++ )
			if ( chars[ i ] != ( wchar_t )ascii[ i ] )
				return false;
		return true;
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
}

static std::int32_t bone_index_from_names( std::uintptr_t bone_names, const char* name )
{
	if ( !is_valid_userland_ptr( bone_names ) || !name )
		return -1;

	__try
	{
		auto count = *( std::int32_t* )( bone_names + 0x18 );
		if ( count <= 0 || count > 256 )
			return -1;

		auto data = bone_names + 0x20;
		for ( std::int32_t i = 0; i < count; i++ )
		{
			auto str = *( std::uintptr_t* )( data + ( i * 8 ) );
			if ( managed_string_equals_ascii( str, name ) )
				return i;
		}
	}
	__except ( EXCEPTION_EXECUTE_HANDLER )
	{
		return -1;
	}

	return -1;
}

void c_core::resolve_model_bones( )
{
	if ( !m_bn.base_networkable_rva || !m_bn.decrypt_0.valid || !m_bn.decrypt_1.valid )
		return;

	auto base_entity = c_runtime::find_class( "BaseEntity" );
	auto model_off = field_offset_by_name( base_entity, "model" );
	auto model_class = field_type_class_by_name( base_entity, "model" );
	if ( !model_class )
		model_class = c_runtime::find_class( "Model" );
	auto bone_names_off = field_offset_by_name( model_class, "boneNames" );
	if ( !model_off || !bone_names_off )
		return;

	using gchandle_fn_t = void* ( * )( std::uint32_t );
	auto gchandle_get_target = ( gchandle_fn_t )GetProcAddress(
		GetModuleHandleA( "GameAssembly.dll" ), "il2cpp_gchandle_get_target" );
	if ( !gchandle_get_target )
		return;

	auto decrypt_handle = [&]( std::uintptr_t wrapper_ptr, std::uint64_t read_off, const c_decryption::constants_t& c ) -> std::uintptr_t
	{
		if ( !is_valid_userland_ptr( wrapper_ptr + read_off ) )
			return 0;
		auto encrypted = *( std::uint64_t* )( wrapper_ptr + read_off );
		auto decrypted = c_decryption::simulate( encrypted, c );

		if ( ( decrypted & 1 ) == 0 && is_valid_userland_ptr( decrypted ) )
		{
			auto target = *( std::uintptr_t* )decrypted;
			if ( is_valid_userland_ptr( target ) )
				return target;
		}

		auto handle = ( std::uint32_t )decrypted;
		if ( !handle || handle > 0x1000000 )
			return 0;
		__try { return ( std::uintptr_t )gchandle_get_target( handle ); }
		__except ( EXCEPTION_EXECUTE_HANDLER ) { return 0; }
	};

	auto game_base = c_runtime::game_base( );
	auto klass = *( std::uintptr_t* )( game_base + m_bn.base_networkable_rva );
	if ( !is_valid_userland_ptr( klass ) )
		return;
	auto static_fields = *( std::uintptr_t* )( klass + m_bn.static_fields );
	if ( !is_valid_userland_ptr( static_fields ) )
		return;
	auto wrapper = *( std::uintptr_t* )( static_fields + m_bn.wrapper_class_ptr );
	auto realm = decrypt_handle( wrapper, m_bn.hv_offset, m_bn.decrypt_0 );
	if ( !is_valid_userland_ptr( realm ) )
		return;
	auto parent = *( std::uintptr_t* )( realm + m_bn.parent_static_fields );
	auto list_dict = decrypt_handle( parent, m_bn.hv_offset, m_bn.decrypt_1 );
	if ( !is_valid_userland_ptr( list_dict ) )
		return;
	auto buffer_list = *( std::uintptr_t* )( list_dict + m_bn.entities );
	if ( !is_valid_userland_ptr( buffer_list ) )
		return;
	auto array = *( std::uintptr_t* )( buffer_list + 0x10 );
	auto count = *( std::int32_t* )( buffer_list + 0x18 );
	if ( !is_valid_userland_ptr( array ) || count <= 0 || count > 50000 )
		return;

	auto entity_class = c_runtime::find_class( "BaseEntity" );
	if ( !entity_class )
		entity_class = c_runtime::find_class( "BaseNetworkable" );
	if ( !entity_class )
		return;

	auto set_idx = []( std::int32_t& dst, std::int32_t idx )
	{
		if ( dst < 0 && idx >= 0 )
			dst = idx;
	};

	auto limit = count > 1024 ? 1024 : count;
	for ( std::int32_t i = 0; i < limit; i++ )
	{
		auto entry = *( std::uintptr_t* )( array + 0x20 + ( i * 8 ) );
		auto entity = resolve_entity_entry_object( entry, entity_class );
		if ( !is_valid_userland_ptr( entity + model_off ) )
			continue;
		auto model = *( std::uintptr_t* )( entity + model_off );
		if ( !is_valid_userland_ptr( model + bone_names_off ) )
			continue;
		auto bone_names = *( std::uintptr_t* )( model + bone_names_off );
		if ( !is_valid_userland_ptr( bone_names ) )
			continue;

		auto bone_count = *( std::int32_t* )( bone_names + 0x18 );
		if ( bone_count > 0 && bone_count < 256 )
			m_model_bones.count = bone_count;

		set_idx( m_model_bones.pelvis, bone_index_from_names( bone_names, "pelvis" ) );
		set_idx( m_model_bones.l_hip, bone_index_from_names( bone_names, "l_hip" ) );
		set_idx( m_model_bones.l_knee, bone_index_from_names( bone_names, "l_knee" ) );
		set_idx( m_model_bones.l_foot, bone_index_from_names( bone_names, "l_foot" ) );
		set_idx( m_model_bones.r_hip, bone_index_from_names( bone_names, "r_hip" ) );
		set_idx( m_model_bones.r_knee, bone_index_from_names( bone_names, "r_knee" ) );
		set_idx( m_model_bones.r_foot, bone_index_from_names( bone_names, "r_foot" ) );
		set_idx( m_model_bones.spine2, bone_index_from_names( bone_names, "spine2" ) );
		set_idx( m_model_bones.spine4, bone_index_from_names( bone_names, "spine4" ) );
		set_idx( m_model_bones.l_clavicle, bone_index_from_names( bone_names, "l_clavicle" ) );
		set_idx( m_model_bones.l_upperarm, bone_index_from_names( bone_names, "l_upperarm" ) );
		set_idx( m_model_bones.l_forearm, bone_index_from_names( bone_names, "l_forearm" ) );
		set_idx( m_model_bones.l_hand, bone_index_from_names( bone_names, "l_hand" ) );
		set_idx( m_model_bones.r_clavicle, bone_index_from_names( bone_names, "r_clavicle" ) );
		set_idx( m_model_bones.r_upperarm, bone_index_from_names( bone_names, "r_upperarm" ) );
		set_idx( m_model_bones.r_forearm, bone_index_from_names( bone_names, "r_forearm" ) );
		set_idx( m_model_bones.r_hand, bone_index_from_names( bone_names, "r_hand" ) );
		set_idx( m_model_bones.neck, bone_index_from_names( bone_names, "neck" ) );
		set_idx( m_model_bones.head, bone_index_from_names( bone_names, "head" ) );

		if ( m_model_bones.pelvis >= 0 && m_model_bones.head >= 0 && m_model_bones.r_hand >= 0 )
			break;
	}

	std::printf( "[morphine-dumper] model bones: count=%d pelvis=%d head=%d\n",
		m_model_bones.count, m_model_bones.pelvis, m_model_bones.head );
}

static void resolve_base_player_live_offsets(
	std::uint64_t base_networkable_rva,
	std::uint32_t bn_static_fields,
	std::uint32_t wrapper_class_ptr,
	std::uint32_t parent_static_fields,
	std::uint32_t entities,
	std::uint64_t hv_offset,
	const c_decryption::constants_t& decrypt_0,
	const c_decryption::constants_t& decrypt_1,
	il2cpp::il2cpp_class_t* item_class,
	il2cpp::il2cpp_class_t* item_container_class )
{
	g_base_player_live = { };

	if ( !base_networkable_rva || !decrypt_0.valid || !decrypt_1.valid )
		return;

	auto base_player = c_runtime::find_class( "BasePlayer" );
	auto player_model_klass = c_runtime::find_class( "PlayerModel" );
	if ( !player_model_klass )
		player_model_klass = class_name_contains( "PlayerModel" );
	if ( !base_player || !player_model_klass )
		return;

	auto player_model_off = field_offset_by_type_class( base_player, player_model_klass );
	if ( !player_model_off )
		player_model_off = field_offset_by_type_contains( base_player, "PlayerModel" );
	if ( !player_model_off )
		return;

	auto model_state_klass = c_runtime::find_class( "ModelState" );
	if ( !model_state_klass )
		model_state_klass = class_name_contains( "ModelState" );
	if ( !model_state_klass )
		model_state_klass = find_model_state_class( );
	if ( auto model_state_shape_off = field_offset_by_model_state_shape( base_player ) )
		g_base_player_live.model_state = model_state_shape_off;

	std::uint64_t item_list_off = 0;
	std::uint64_t item_flags_off = 0;
	if ( item_container_class )
	{
		char search[ 128 ]{ };
		std::snprintf( search, sizeof( search ),
			"System.Collections.Generic.List<%s>",
			item_class ? item_class->name( ) : "Item" );
		if ( auto f = il2cpp::get_field_if_type_contains( item_container_class, search ) )
			item_list_off = f->offset( );
		char flag_search[ 128 ]{ };
		std::snprintf( flag_search, sizeof( flag_search ), "%s.Flag", item_container_class->name( ) );
		if ( auto f = il2cpp::get_field_if_type_contains( item_container_class, flag_search, FIELD_ATTRIBUTE_PUBLIC ) )
			item_flags_off = f->offset( );
		if ( !item_flags_off )
			item_flags_off = field_offset_by_name( item_container_class, "flags" );
		if ( !item_flags_off )
			item_flags_off = field_offset_by_name_contains( item_container_class, "flag" );
		if ( !item_flags_off )
			item_flags_off = field_offset_by_type_or_class_contains( item_container_class, "Flag" );
	}

	struct hit_t
	{
		std::uint64_t off;
		std::uint64_t aux;
		std::uintptr_t klass;
		int hits;
	};

	std::vector<hit_t> belt_hits;
	std::vector<hit_t> player_model_hits;
	std::vector<hit_t> player_model_position_hits;
	std::vector<hit_t> model_hits;
	std::vector<hit_t> team_hits;
	auto add_hit = []( std::vector<hit_t>& hits, std::uint64_t off, std::uint64_t aux, std::uintptr_t klass )
	{
		for ( auto& h : hits )
		{
			if ( h.off == off && h.aux == aux && h.klass == klass )
			{
				h.hits++;
				return;
			}
		}
		hits.push_back( { off, aux, klass, 1 } );
	};

	auto root_bone_off = field_offset_by_name( player_model_klass, "rootBone" );
	auto bone_transforms_off = field_offset_by_name( player_model_klass, "boneTransforms" );
	if ( !bone_transforms_off )
		bone_transforms_off = field_offset_by_type_contains( player_model_klass, "Transform[]" );
	auto player_model_skinned_multi_mesh = field_offset_by_type_contains( player_model_klass, "SkinnedMultiMesh" );
	auto read_transform_position = []( std::uintptr_t transform, vec3_t& out ) -> bool
	{
		auto native = read_ptr_safe( transform + 0x10 );
		if ( !is_valid_userland_ptr( native + 0x90 ) )
			return false;
		out = *( vec3_t* )( native + 0x90 );
		return finite_float( out.x ) && finite_float( out.y ) && finite_float( out.z );
	};
	auto read_player_model_root_position = [&]( std::uintptr_t player_model, vec3_t& out ) -> bool
	{
		if ( root_bone_off && is_valid_userland_ptr( player_model + root_bone_off ) )
			if ( read_transform_position( *( std::uintptr_t* )( player_model + root_bone_off ), out ) )
				return true;
		if ( bone_transforms_off && is_valid_userland_ptr( player_model + bone_transforms_off ) )
		{
			auto array = *( std::uintptr_t* )( player_model + bone_transforms_off );
			if ( is_valid_userland_ptr( array + 0x20 ) )
				return read_transform_position( *( std::uintptr_t* )( array + 0x20 ), out );
		}
		return false;
	};
	auto near_vec3 = []( const vec3_t& a, const vec3_t& b ) -> bool
	{
		auto dx = a.x - b.x;
		auto dy = a.y - b.y;
		auto dz = a.z - b.z;
		return ( dx * dx + dy * dy + dz * dz ) <= ( 25.f * 25.f );
	};

	auto plausible_list = []( std::uintptr_t list ) -> bool
	{
		if ( !is_valid_userland_ptr( list + 0x18 ) )
			return false;
		auto array = *( std::uintptr_t* )( list + 0x10 );
		auto size = *( std::int32_t* )( list + 0x18 );
		return size >= 0 && size <= 1000 && ( size == 0 || is_valid_userland_ptr( array ) );
	};

	auto item_container_flags = [&]( std::uintptr_t container, std::uint32_t& flags ) -> bool
	{
		if ( !item_list_off || !item_flags_off || !is_valid_userland_ptr( container + item_list_off ) )
			return false;
		if ( item_container_class )
		{
			auto object_klass = *( std::uintptr_t* )container;
			if ( object_klass != ( std::uintptr_t )item_container_class )
				return false;
		}
		auto list = *( std::uintptr_t* )( container + item_list_off );
		if ( !plausible_list( list ) )
			return false;
		flags = *( std::uint32_t* )( container + item_flags_off );
		return ( flags & 0xFFFF0000u ) == 0;
	};

	auto field_allows_item_container = [&]( std::uint64_t off ) -> bool
	{
		auto field = field_by_offset_walk( base_player, off );
		auto type = field ? field->type( ) : nullptr;
		auto type_name = type ? type->name( ) : nullptr;
		if ( ascii_contains_i( type_name, "ItemContainer" ) )
			return true;
		return item_container_class && ascii_contains_i( type_name, item_container_class->name( ) );
	};

	auto plausible_model_state_flags = []( std::uint32_t flags ) -> bool
	{
		return ( flags & 0xFFF00000u ) == 0;
	};

	auto plausible_team_id = []( std::uint64_t value ) -> bool
	{
		if ( value >= 76561190000000000ull && value <= 76561210000000000ull )
			return false;
		return value == 0 || value < 0x100000000ull;
	};

	using gchandle_fn_t = void* ( * )( std::uint32_t );
	auto gchandle_get_target = ( gchandle_fn_t )GetProcAddress(
		GetModuleHandleA( "GameAssembly.dll" ), "il2cpp_gchandle_get_target" );
	if ( !gchandle_get_target )
		return;

	auto decrypt_handle = [&]( std::uintptr_t wrapper_ptr, std::uint64_t read_off, const c_decryption::constants_t& c ) -> std::uintptr_t
	{
		if ( !is_valid_userland_ptr( wrapper_ptr + read_off ) )
			return 0;
		auto encrypted = *( std::uint64_t* )( wrapper_ptr + read_off );
		auto decrypted = c_decryption::simulate( encrypted, c );

		if ( ( decrypted & 1 ) == 0 && is_valid_userland_ptr( decrypted ) )
		{
			auto target = *( std::uintptr_t* )decrypted;
			if ( is_valid_userland_ptr( target ) )
				return target;
		}

		auto handle = ( std::uint32_t )decrypted;
		if ( !handle || handle > 0x1000000 )
			return 0;
		return safe_gchandle_get_target( gchandle_get_target, handle );
	};

	auto game_base = c_runtime::game_base( );
	auto klass = *( std::uintptr_t* )( game_base + base_networkable_rva );
	if ( !is_valid_userland_ptr( klass ) )
		return;
	auto static_fields = *( std::uintptr_t* )( klass + bn_static_fields );
	if ( !is_valid_userland_ptr( static_fields ) )
		return;
	auto wrapper = *( std::uintptr_t* )( static_fields + wrapper_class_ptr );
	auto realm = decrypt_handle( wrapper, hv_offset, decrypt_0 );
	if ( !is_valid_userland_ptr( realm ) )
		return;
	auto parent = *( std::uintptr_t* )( realm + parent_static_fields );
	auto list_dict = decrypt_handle( parent, hv_offset, decrypt_1 );
	if ( !is_valid_userland_ptr( list_dict ) )
		return;
	auto buffer_list = *( std::uintptr_t* )( list_dict + entities );
	if ( !is_valid_userland_ptr( buffer_list ) )
		return;
	auto array = *( std::uintptr_t* )( buffer_list + 0x10 );
	auto count = *( std::int32_t* )( buffer_list + 0x18 );
	if ( !is_valid_userland_ptr( array ) || count <= 0 || count > 50000 )
		return;

	int players = 0;
	int resolved_entries = 0;
	auto limit = count > 2048 ? 2048 : count;
	for ( std::int32_t i = 0; i < limit; i++ )
	{
		auto entry = *( std::uintptr_t* )( array + 0x20 + ( i * 8 ) );
		auto entity = resolve_entity_entry_object( entry, base_player );
		if ( entity )
			resolved_entries++;
		if ( !is_valid_userland_ptr( entity + player_model_off ) )
			continue;
		auto player_model = *( std::uintptr_t* )( entity + player_model_off );
		if ( !is_valid_userland_ptr( player_model ) )
			continue;

		players++;

		vec3_t model_root_pos{ };
		if ( read_player_model_root_position( player_model, model_root_pos ) )
		{
			auto start = player_model_skinned_multi_mesh ? player_model_skinned_multi_mesh + 4 : 0x180ull;
			for ( std::uint64_t off = start; off <= 0x700; off += 4 )
			{
				if ( !is_valid_userland_ptr( player_model + off + sizeof( vec3_t ) ) )
					continue;
				auto value = *( vec3_t* )( player_model + off );
				if ( !finite_float( value.x ) || !finite_float( value.y ) || !finite_float( value.z ) )
					continue;
				if ( near_vec3( value, model_root_pos ) )
					add_hit( player_model_position_hits, off, 0, 0 );
			}
		}

		for ( std::uint64_t off = 0x20; off <= 0x800; off += 8 )
		{
			if ( !is_valid_userland_ptr( entity + off ) )
				continue;
			auto value = *( std::uint64_t* )( entity + off );
			if ( !plausible_team_id( value ) )
				continue;

			auto field = field_by_offset_walk( base_player, off );
			auto type = field ? field->type( ) : nullptr;
			auto type_name = type ? type->name( ) : nullptr;
			auto field_name = field ? field->name( ) : nullptr;
			if ( ascii_contains_i( field_name, "user" ) ||
				ascii_contains_i( field_name, "steam" ) )
				continue;
			if ( type_name &&
				!ascii_contains_i( type_name, "UInt64" ) &&
				!ascii_contains_i( type_name, "ulong" ) &&
				!ascii_contains_i( type_name, "System.UInt64" ) )
				continue;

			add_hit( team_hits, off, value ? 1 : 0, 0 );
		}

		for ( std::uint64_t off = 0x20; off <= 0x800; off += 8 )
		{
			if ( !is_valid_userland_ptr( entity + off ) )
				continue;
			auto ptr = *( std::uintptr_t* )( entity + off );
			if ( !is_valid_userland_ptr( ptr ) )
				continue;

			if ( object_is_class( ptr, player_model_klass ) )
				add_hit( player_model_hits, off, 0, ( std::uintptr_t )player_model_klass );

			if ( item_list_off && item_flags_off )
			{
				std::uint32_t flags = 0;
				if ( item_container_flags( ptr, flags ) )
				{
					if ( flags & 4u )
						add_hit( belt_hits, off, 0, 0 );
				}
				else
				{
					if ( !field_allows_item_container( off ) )
						continue;
					const std::uint64_t inner_offsets[] = { 0, 0x8, 0x10, 0x18, 0x20, 0x28, 0x30 };
					for ( auto inner : inner_offsets )
					{
						if ( !is_valid_userland_ptr( ptr + inner ) )
							continue;
						auto inner_ptr = *( std::uintptr_t* )( ptr + inner );
						if ( item_container_flags( inner_ptr, flags ) && ( flags & 4u ) )
						{
							add_hit( belt_hits, off, inner, 0 );
							break;
						}
					}
				}
			}

			auto object_klass = *( std::uintptr_t* )ptr;
			if ( !is_valid_userland_ptr( object_klass ) )
				continue;
			if ( model_state_klass && ( std::uintptr_t )model_state_klass != object_klass )
				continue;
			for ( std::uint64_t flags_off = 0x10; flags_off <= 0x80; flags_off += 4 )
			{
				if ( !is_valid_userland_ptr( ptr + flags_off ) )
					continue;
				auto flags = *( std::uint32_t* )( ptr + flags_off );
				if ( plausible_model_state_flags( flags ) )
				{
					add_hit( model_hits, off, flags_off, object_klass );
					break;
				}
			}
		}
	}

	auto choose_hit = [&]( const std::vector<hit_t>& hits, bool require_aux_zero ) -> hit_t
	{
		hit_t best{ };
		for ( const auto& h : hits )
		{
			if ( require_aux_zero && h.aux )
				continue;
			if ( h.hits > best.hits )
				best = h;
		}
		return best;
	};
	auto choose_team_hit = [&]( const std::vector<hit_t>& hits ) -> hit_t
	{
		hit_t best{ };
		for ( const auto& h : hits )
		{
			if ( !h.aux )
				continue;
			if ( h.hits > best.hits )
				best = h;
		}
		if ( best.hits )
			return best;
		return choose_hit( hits, false );
	};

	auto belt = choose_hit( belt_hits, false );
	auto live_player_model = choose_hit( player_model_hits, true );
	auto live_player_model_position = choose_hit( player_model_position_hits, true );
	auto model = choose_hit( model_hits, false );
	auto team = choose_team_hit( team_hits );
	auto live_min_hits = players <= 2 ? 1 : 2;
	if ( players && live_player_model.hits >= live_min_hits )
		g_base_player_live.player_model = live_player_model.off;
	if ( players && live_player_model_position.hits >= live_min_hits )
		g_base_player_live.player_model_position = live_player_model_position.off;
	if ( players && belt.hits >= live_min_hits )
		g_base_player_live.belt_direct = belt.off;
	if ( players && model.hits >= live_min_hits )
	{
		g_base_player_live.model_state = model.off;
		g_base_player_live.model_state_flags = model.aux;
	}
	if ( players && team.hits >= live_min_hits )
		g_base_player_live.current_team = team.off;

	std::printf(
		"[morphine-dumper] base_player live: players=%d resolved=%d playerModel=0x%llX hits=%d playerModelPosition=0x%llX hits=%d modelState=0x%llX hits=%d flags=0x%llX belt=0x%llX hits=%d inner=0x%llX currentTeam=0x%llX hits=%d nonzero=%d\n",
		players,
		resolved_entries,
		g_base_player_live.player_model,
		live_player_model.hits,
		g_base_player_live.player_model_position,
		live_player_model_position.hits,
		g_base_player_live.model_state,
		model.hits,
		model.aux,
		g_base_player_live.belt_direct,
		belt.hits,
		belt.aux,
		g_base_player_live.current_team,
		team.hits,
		(int)team.aux );
}

void c_core::resolve_cl_active_item_decryption( )
{
	auto klass = c_runtime::find_class( "BasePlayer" );
	if ( !klass )
	{
		std::printf( "[morphine-dumper] cl_active_item: BasePlayer class not found\n" );
		return;
	}

	std::vector<il2cpp::method_info_t*> methods;
	auto push_method = [&]( il2cpp::method_info_t* method )
	{
		if ( !method )
			return;
		for ( auto existing : methods )
			if ( existing == method )
				return;
		methods.push_back( method );
	};

	void* iter = nullptr;
	while ( auto method = klass->methods( &iter ) )
	{
		auto name = method->name( );
		if ( !name )
			continue;

		if ( std::strstr( name, "ActiveItem" ) ||
			std::strstr( name, "activeItem" ) ||
			std::strstr( name, "clActiveItem" ) ||
			std::strstr( name, "cl_active_item" ) )
			push_method( method );
	}

	if ( m_item_id_class )
	{
		iter = nullptr;
		while ( auto method = klass->methods( &iter ) )
		{
			auto method_name = method->name( );
			if ( method->param_count( ) == 0 )
			{
				auto ret = method->return_type( );
				auto ret_class = ret ? ret->klass( ) : nullptr;
				if ( ret_class && std::strcmp( ret_class->name( ), m_item_id_class->name( ) ) == 0 )
					push_method( method );
			}

			if ( !method_name || ( !std::strstr( method_name, "ActiveItem" )
				&& !std::strstr( method_name, "activeItem" )
				&& !std::strstr( method_name, "clActiveItem" )
				&& !std::strstr( method_name, "cl_active_item" ) ) )
				continue;

			for ( std::uint32_t i = 0; i < method->param_count( ); i++ )
			{
				auto param = method->get_param( i );
				auto param_class = param ? param->klass( ) : nullptr;
				if ( param_class && std::strcmp( param_class->name( ), m_item_id_class->name( ) ) == 0 )
				{
					push_method( method );
					break;
				}
			}
		}
	}

	struct candidate_t
	{
		std::uintptr_t fn_addr;
		c_decryption::constants_t constants;
		std::uint64_t field_offset;
		int score;
		std::uint32_t live_hits;
		std::uint32_t active_values;
		std::uint32_t inventory_hits;
		std::uint32_t container_hits;
		bool hidden_value_shape;
		int static_slot_score;
		bool nested_call_candidate;
		std::uint32_t loop_count;
	};
	std::vector<candidate_t> candidates;
	struct pending_candidate_t
	{
		std::uintptr_t addr;
		int score;
	};
	std::vector<pending_candidate_t> pending_candidates;
	std::vector<std::uintptr_t> nested_scan_sources;
	auto cl_active_item_field = m_item_id_class
		? il2cpp::get_field_if_type_contains( klass, m_item_id_class->name( ) )
		: nullptr;
	auto cl_active_item_offset = cl_active_item_field ? cl_active_item_field->offset( klass ) : 0;

	if ( !cl_active_item_offset )
	{
		iter = nullptr;
		while ( auto method = klass->methods( &iter ) )
		{
			auto name = method->name( );
			auto ret = method->return_type( );
			auto ret_class = ret ? ret->klass( ) : nullptr;
			if ( m_item_id_class && ret_class != m_item_id_class )
				continue;

			auto fn = method->get_fn_ptr<std::uintptr_t>( );
			auto inferred = first_field_offset_referenced_before_decrypt_call( fn );
			if ( inferred )
			{
				cl_active_item_offset = inferred;
				push_method( method );
				std::printf( "[morphine-dumper] cl_active_item: inferred field offset 0x%llX from method %s\n",
					cl_active_item_offset, name ? name : "<unnamed>" );
				break;
			}
		}
	}

	if ( cl_active_item_offset )
	{
		iter = nullptr;
		while ( auto method = klass->methods( &iter ) )
		{
			auto fn = method->get_fn_ptr<std::uintptr_t>( );
			if ( fn_references_offset( fn, cl_active_item_offset ) )
				push_method( method );
		}
	}

	std::vector<std::uint64_t> cl_active_item_offsets;
	auto push_cl_active_item_offset = [&]( std::uint64_t offset )
	{
		if ( !offset || offset > 0x2000 )
			return;
		for ( auto existing : cl_active_item_offsets )
			if ( existing == offset )
				return;
		cl_active_item_offsets.push_back( offset );
	};

	push_cl_active_item_offset( cl_active_item_offset );
	if ( m_item_id_class )
	{
		void* field_iter = nullptr;
		while ( auto field = klass->fields( &field_iter ) )
		{
			if ( field->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto type = field->type( );
			auto field_class = type ? type->klass( ) : nullptr;
			if ( field_class == m_item_id_class ||
				( field_class && field_class->name( ) && m_item_id_class->name( ) &&
					std::strcmp( field_class->name( ), m_item_id_class->name( ) ) == 0 ) )
				push_cl_active_item_offset( field->offset( klass ) );
		}
	}

	auto queue_nested_calls = [&]( std::uintptr_t addr, int score )
	{
		for ( auto existing : nested_scan_sources )
			if ( existing == addr )
				return;
		nested_scan_sources.push_back( addr );

		auto p = ( std::uint8_t* )addr;
		auto end = p + 0x900;
		while ( p < end && !IsBadReadPtr( p, 0x10 ) )
		{
			hde64s hs;
			auto len = hde64_disasm( p, &hs );
			if ( !len || ( hs.flags & F_ERROR ) )
				break;
			if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
				break;

			if ( p[ 0 ] == 0xE8 && len >= 5 )
			{
				std::int32_t d;
				std::memcpy( &d, p + 1, 4 );
				auto target = resolve_jump_thunk( ( std::uintptr_t )( p + 5 + d ) );
				if ( target && target != addr )
					pending_candidates.push_back( { target, score } );
			}

			p += len;
		}
	};

	auto add_candidate = [&]( std::uintptr_t addr, int score )
	{
		addr = resolve_jump_thunk( addr );
		if ( !addr || IsBadReadPtr( ( void* )addr, 0x100 ) )
			return;
		if ( fn_starts_with_jump_thunk( addr ) )
			return;
		auto nested_call_candidate = score >= 900;
		auto hidden_value_shape = fn_has_hidden_value_decrypt_shape( addr );
		if ( !hidden_value_shape && score < 120 )
			return;
		auto static_slot_score = active_item_static_slot_score( addr );

		auto constants = extract_stack_u32_crypto( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_loop( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_inline( ( std::uint8_t* )addr, 0x500 );

		if ( !constants.valid )
			return;
		auto loop_count = stack_u32_loop_count( ( std::uint8_t* )addr, 0x100 );
		if ( nested_call_candidate && loop_count && loop_count != 2 )
			return;
		if ( constants.op_count < 3 || constants.op_count > 6 )
			return;
		int rol_count = 0;
		int add_sub_count = 0;
		int xor_count = 0;
		for ( int i = 0; i < constants.op_count; i++ )
		{
			if ( constants.ops[ i ].type == c_decryption::op_t::op_rol )
				rol_count++;
			else if ( constants.ops[ i ].type == c_decryption::op_t::op_add ||
				constants.ops[ i ].type == c_decryption::op_t::op_sub )
				add_sub_count++;
			else if ( constants.ops[ i ].type == c_decryption::op_t::op_xor )
				xor_count++;
		}
		if ( rol_count < 1 || add_sub_count + xor_count < 2 )
			return;
		if ( m_bn.decrypt_0.valid && c_decryption::ops_match( constants, m_bn.decrypt_0 ) )
			return;
		if ( m_bn.decrypt_1.valid && c_decryption::ops_match( constants, m_bn.decrypt_1 ) )
			return;
		if ( m_player_inventory_constants.valid && c_decryption::ops_match( constants, m_player_inventory_constants ) )
			return;
		if ( m_player_eyes_constants.valid && c_decryption::ops_match( constants, m_player_eyes_constants ) )
			return;

		if ( !nested_call_candidate )
			queue_nested_calls( addr, score + 1000 );

		for ( auto test_offset : cl_active_item_offsets )
		{
			std::uint32_t active_values = 0;
			std::uint32_t inventory_hits = 0;
			std::uint32_t container_hits = 0;
			auto live_hits = score_cl_active_item_live(
				m_bn.base_networkable_rva,
				m_bn.static_fields,
				m_bn.wrapper_class_ptr,
				m_bn.parent_static_fields,
				m_bn.entities,
				m_bn.hv_offset,
				m_bn.decrypt_0,
				m_bn.decrypt_1,
				m_player_inventory_constants,
				constants,
				test_offset,
				m_player_inventory.main,
				m_player_inventory.belt,
				m_player_inventory.wear,
				m_item_offsets.uid,
				m_item_class,
				m_item_container_class,
				&active_values,
				&inventory_hits,
				&container_hits );

			auto candidate_score = score;
			if ( test_offset == cl_active_item_offset )
				candidate_score += 20;
			if ( nested_call_candidate && loop_count == 2 )
				candidate_score += 750;
			candidate_score += static_slot_score;
			if ( live_hits )
				candidate_score += ( int )live_hits * 10000;
			else if ( active_values && inventory_hits && container_hits && !hidden_value_shape && static_slot_score > 0 )
				candidate_score += 5000;
			else
				std::printf(
					"[morphine-dumper] cl_active_item candidate: rva=0x%llX field=0x%llX ops=%d active=%u inventories=%u containers=%u slot_score=%d loop=%u\n",
					addr - c_runtime::game_base( ),
					test_offset,
					constants.op_count,
					active_values,
					inventory_hits,
					container_hits,
					static_slot_score,
					loop_count );

			for ( auto& existing : candidates )
			{
				if ( existing.fn_addr == addr &&
					existing.field_offset == test_offset &&
					c_decryption::ops_match( existing.constants, constants ) )
				{
					if ( live_hits > existing.live_hits )
						existing.live_hits = live_hits;
					if ( active_values > existing.active_values )
						existing.active_values = active_values;
					if ( inventory_hits > existing.inventory_hits )
						existing.inventory_hits = inventory_hits;
					if ( container_hits > existing.container_hits )
						existing.container_hits = container_hits;
					if ( static_slot_score > existing.static_slot_score )
						existing.static_slot_score = static_slot_score;
					if ( nested_call_candidate )
						existing.nested_call_candidate = true;
					if ( loop_count > existing.loop_count )
						existing.loop_count = loop_count;
					if ( candidate_score > existing.score )
						existing.score = candidate_score;
					return;
				}
			}

			candidates.push_back( { addr, constants, test_offset, candidate_score, live_hits,
				active_values, inventory_hits, container_hits, hidden_value_shape, static_slot_score,
				nested_call_candidate, loop_count } );
		}
	};

	for ( auto method : methods )
	{
		auto name = method->name( );
		auto fn = method->get_fn_ptr<std::uintptr_t>( );
		int base_score = 0;
		if ( name && ( std::strstr( name, "ActiveItem" ) ||
			std::strstr( name, "activeItem" ) ||
			std::strstr( name, "clActiveItem" ) ||
			std::strstr( name, "cl_active_item" ) ) )
			base_score += 80;
		auto ret = method->return_type( );
		auto ret_class = ret ? ret->klass( ) : nullptr;
		if ( m_item_id_class && ret_class == m_item_id_class )
			base_score += 40;
		if ( fn_references_offset( fn, cl_active_item_offset ) )
			base_score += 120;
		add_candidate( fn, base_score );

		if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
			continue;

		auto p = ( std::uint8_t* )fn;
		auto end = p + 0x200;
		bool saw_target_offset = false;
		while ( p < end )
		{
			hde64s hs;
			auto len = hde64_disasm( p, &hs );
			if ( !len || ( hs.flags & F_ERROR ) )
				break;
			if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
				break;

			if ( cl_active_item_offset && ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )cl_active_item_offset ) )
				saw_target_offset = true;
			if ( cl_active_item_offset && ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )cl_active_item_offset ) )
				saw_target_offset = true;

			if ( p[ 0 ] == 0xE8 && len >= 5 )
			{
				std::int32_t d;
				std::memcpy( &d, p + 1, 4 );
				add_candidate( ( std::uintptr_t )( p + 5 + d ), base_score + ( saw_target_offset ? 160 : 0 ) );
			}

			p += len;
		}
	}

	for ( std::size_t i = 0; i < pending_candidates.size( ) && i < 512; i++ )
		add_candidate( pending_candidates[ i ].addr, pending_candidates[ i ].score );

	if ( candidates.empty( ) )
	{
		std::printf( "[morphine-dumper] cl_active_item: no decryption candidates found\n" );
		return;
	}

	auto candidate_rank = []( const candidate_t& candidate ) -> int
	{
		if ( candidate.live_hits )
			return 3;
		if ( candidate.active_values && candidate.inventory_hits && candidate.container_hits &&
			!candidate.hidden_value_shape && candidate.nested_call_candidate &&
			candidate.static_slot_score == 0 && ( !candidate.loop_count || candidate.loop_count == 2 ) )
			return 2;
		return 0;
	};

	auto* best = &candidates.front( );
	for ( auto& candidate : candidates )
	{
		auto candidate_rank_value = candidate_rank( candidate );
		auto best_rank_value = candidate_rank( *best );
		if ( candidate_rank_value > best_rank_value ||
			( candidate_rank_value == best_rank_value && candidate.live_hits > best->live_hits ) ||
			( candidate_rank_value == best_rank_value && candidate.live_hits == best->live_hits && candidate.score > best->score ) ||
			( candidate.score == best->score && best->constants.was_unrolled && !candidate.constants.was_unrolled ) )
			best = &candidate;
	}

	auto best_rank_value = candidate_rank( *best );
	if ( best_rank_value < 2 )
	{
		std::printf( "[morphine-dumper] cl_active_item: no runtime-verified candidate found (%zu candidates)\n",
			candidates.size( ) );
		return;
	}

	m_cl_active_item_fn_addr = best->fn_addr;
	m_cl_active_item_offset = best->field_offset;
	m_cl_active_item_constants = best->constants;

	// VALIDATION: cl_active_item decrypt should start with SUB in recent Rust builds.
	// The dumper sometimes finds a DECOY function due to IL2CPP obfuscation.
	// If the first op is NOT SUB, override with known-good constants.
	// When the game updates: run CLA_TRYALL diagnostic in the cheat to verify.
	if ( m_cl_active_item_constants.valid && m_cl_active_item_constants.op_count >= 3 &&
		m_cl_active_item_constants.ops[ 0 ].type != c_decryption::op_t::op_sub )
	{
		std::printf( "[morphine-dumper] cl_active_item: WARNING - dumper found decoy (first op=%s, expected SUB)\n",
			c_decryption::op_name( m_cl_active_item_constants.ops[ 0 ].type ) );
		std::printf( "[morphine-dumper] cl_active_item: overriding with known-good constants (SUB/ROL2/XOR/ADD)\n" );
		m_cl_active_item_constants.ops[ 0 ] = { c_decryption::op_t::op_sub, 0x1D2981D5 };
		m_cl_active_item_constants.ops[ 1 ] = { c_decryption::op_t::op_rol, 0x2 };
		m_cl_active_item_constants.ops[ 2 ] = { c_decryption::op_t::op_xor, 0x8DA4E5D3 };
		m_cl_active_item_constants.ops[ 3 ] = { c_decryption::op_t::op_add, 0x6189597E };
		m_cl_active_item_constants.op_count = 4;
	}

	std::printf( "[morphine-dumper] cl_active_item: extracted %d ops from rva 0x%llX field=0x%llX (%zu candidates)\n",
		m_cl_active_item_constants.op_count,
		m_cl_active_item_fn_addr - c_runtime::game_base( ),
		m_cl_active_item_offset,
		candidates.size( ) );
	std::printf( "[morphine-dumper] cl_active_item: live_hits=%u score=%d\n",
		best->live_hits,
		best->score );
	if ( !best->live_hits )
		std::printf( "[morphine-dumper] cl_active_item: observed active=%u inventories=%u containers=%u nested=%d loop=%u (not uid-verified)\n",
			best->active_values,
			best->inventory_hits,
			best->container_hits,
			best->nested_call_candidate ? 1 : 0,
			best->loop_count );
}

void c_core::resolve_player_inventory_decryption( )
{
	auto base_player = c_runtime::find_class( "BasePlayer" );
	auto inventory_class = c_runtime::find_class( "PlayerInventory" );
	if ( !base_player || !inventory_class )
	{
		std::printf( "[morphine-dumper] player_inventory decrypt: missing class (bp=%p inv=%p)\n",
			base_player, inventory_class );
		return;
	}

	std::vector<il2cpp::method_info_t*> methods;
	auto player_inventory_field = il2cpp::get_field_if_type_contains( base_player, "PlayerInventory" );
	auto player_inventory_offset = player_inventory_field ? player_inventory_field->offset( base_player ) : 0;

	auto push_method = [&]( il2cpp::method_info_t* method )
	{
		if ( !method )
			return;
		for ( auto existing : methods )
			if ( existing == method )
				return;
		methods.push_back( method );
	};

	void* iter = nullptr;
	while ( auto method = base_player->methods( &iter ) )
	{
		auto name = method->name( );
		auto ret = method->return_type( );
		auto ret_class = ret ? ret->klass( ) : nullptr;

		if ( ret_class == inventory_class )
		{
			push_method( method );
			continue;
		}

		for ( std::uint32_t i = 0; i < method->param_count( ); i++ )
		{
			auto param = method->get_param( i );
			auto param_class = param ? param->klass( ) : nullptr;
			if ( param_class == inventory_class )
			{
				push_method( method );
				break;
			}
		}

		if ( name && ( std::strstr( name, "Inventory" ) ||
			std::strstr( name, "inventory" ) ||
			std::strstr( name, "PlayerInventory" ) ) )
			push_method( method );
	}

	if ( player_inventory_offset )
	{
		iter = nullptr;
		while ( auto method = base_player->methods( &iter ) )
		{
			auto fn = method->get_fn_ptr<std::uintptr_t>( );
			if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
				continue;

			auto p = ( std::uint8_t* )fn;
			auto end = p + 0x300;
			while ( p < end )
			{
				hde64s hs;
				auto len = hde64_disasm( p, &hs );
				if ( !len || ( hs.flags & F_ERROR ) )
					break;
				if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
					break;

				if ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )player_inventory_offset )
				{
					push_method( method );
					break;
				}
				if ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )player_inventory_offset )
				{
					push_method( method );
					break;
				}

				p += len;
			}
		}
	}

	struct candidate_t
	{
		std::uintptr_t fn_addr;
		c_decryption::constants_t constants;
		int score;
		int live_hits;
	};
	std::vector<candidate_t> candidates;

	auto add_candidate = [&]( std::uintptr_t addr, int score )
	{
		addr = resolve_jump_thunk( addr );
		if ( !addr || IsBadReadPtr( ( void* )addr, 0x100 ) )
			return;
		if ( !fn_has_hidden_value_decrypt_shape( addr ) )
			return;

		auto constants = extract_stack_u32_crypto( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_loop( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_inline( ( std::uint8_t* )addr, 0x500 );

		if ( !constants.valid )
			return;
		if ( m_bn.decrypt_0.valid && c_decryption::ops_match( constants, m_bn.decrypt_0 ) )
			return;
		if ( m_bn.decrypt_1.valid && c_decryption::ops_match( constants, m_bn.decrypt_1 ) )
			return;
		if ( m_cl_active_item_constants.valid && c_decryption::ops_match( constants, m_cl_active_item_constants ) )
			return;

		auto live_hits = score_hidden_value_field(
			m_bn.base_networkable_rva,
			m_bn.static_fields,
			m_bn.wrapper_class_ptr,
			m_bn.parent_static_fields,
			m_bn.entities,
			m_bn.hv_offset,
			m_bn.decrypt_0,
			m_bn.decrypt_1,
			player_inventory_offset,
			inventory_class,
			constants );
		if ( live_hits )
			score += 20 + ( live_hits > 100 ? 100 : live_hits );

		for ( auto& existing : candidates )
		{
			if ( existing.fn_addr == addr || c_decryption::ops_match( existing.constants, constants ) )
			{
				if ( live_hits > existing.live_hits || ( live_hits == existing.live_hits && score > existing.score ) )
				{
					existing.score = score;
					existing.live_hits = live_hits;
				}
				return;
			}
		}

		candidates.push_back( { addr, constants, score, live_hits } );
	};

	for ( auto method : methods )
	{
		auto name = method->name( );
		auto fn = method->get_fn_ptr<std::uintptr_t>( );
		int base_score = 0;
		if ( name && ( std::strstr( name, "Inventory" ) ||
			std::strstr( name, "inventory" ) ||
			std::strstr( name, "PlayerInventory" ) ) )
			base_score += 80;
		auto ret = method->return_type( );
		auto ret_class = ret ? ret->klass( ) : nullptr;
		if ( ret_class == inventory_class )
			base_score += 40;
		for ( std::uint32_t i = 0; i < method->param_count( ); i++ )
		{
			auto param = method->get_param( i );
			auto param_class = param ? param->klass( ) : nullptr;
			if ( param_class == inventory_class )
			{
				base_score += 30;
				break;
			}
		}
		if ( fn_references_offset( fn, player_inventory_offset ) )
			base_score += 500;
		add_candidate( fn, base_score );

		if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
			continue;

		auto p = ( std::uint8_t* )fn;
		auto end = p + 0x500;
		int target_offset_window = 0;
		while ( p < end )
		{
			hde64s hs;
			auto len = hde64_disasm( p, &hs );
			if ( !len || ( hs.flags & F_ERROR ) )
				break;
			if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
				break;

			if ( player_inventory_offset && ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )player_inventory_offset ) )
				target_offset_window = 6;
			if ( player_inventory_offset && ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )player_inventory_offset ) )
				target_offset_window = 6;

			if ( p[ 0 ] == 0xE8 && len >= 5 )
			{
				std::int32_t d;
				std::memcpy( &d, p + 1, 4 );
				add_candidate( ( std::uintptr_t )( p + 5 + d ), base_score + ( target_offset_window > 0 ? 1000 : 0 ) );
				target_offset_window = 0;
			}

			if ( target_offset_window > 0 )
				target_offset_window--;
			p += len;
		}
	}

	if ( player_inventory_field )
	{
		auto field_type = player_inventory_field->type( );
		auto field_klass = field_type ? field_type->klass( ) : nullptr;
		if ( field_klass )
		{
			void* hv_iter = nullptr;
			while ( auto method = field_klass->methods( &hv_iter ) )
			{
				auto fn = method->get_fn_ptr<std::uintptr_t>( );
				add_candidate( fn, 700 );

				if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
					continue;
				auto p = ( std::uint8_t* )fn;
				auto end = p + 0x300;
				while ( p < end )
				{
					hde64s hs;
					auto len = hde64_disasm( p, &hs );
					if ( !len || ( hs.flags & F_ERROR ) )
						break;
					if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
						break;

					if ( p[ 0 ] == 0xE8 && len >= 5 )
					{
						std::int32_t d;
						std::memcpy( &d, p + 1, 4 );
						add_candidate( ( std::uintptr_t )( p + 5 + d ), 650 );
					}
					p += len;
				}
			}
		}
	}

	auto has_live_candidate = [&] ( ) -> bool
	{
		for ( const auto& candidate : candidates )
			if ( candidate.live_hits > 0 )
				return true;
		return false;
	};

	if ( candidates.empty( ) || !has_live_candidate( ) )
	{
		iter = nullptr;
		while ( auto method = base_player->methods( &iter ) )
		{
			auto fn = method->get_fn_ptr<std::uintptr_t>( );
			if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
				continue;

			auto p = ( std::uint8_t* )fn;
			auto end = p + 0x700;
			int target_offset_window = fn_references_offset( fn, player_inventory_offset ) ? 4 : 0;
			while ( p < end )
			{
				hde64s hs;
				auto len = hde64_disasm( p, &hs );
				if ( !len || ( hs.flags & F_ERROR ) )
					break;
				if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
					break;

				if ( player_inventory_offset && ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )player_inventory_offset ) )
					target_offset_window = 8;
				if ( player_inventory_offset && ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )player_inventory_offset ) )
					target_offset_window = 8;

				if ( p[ 0 ] == 0xE8 && len >= 5 )
				{
					std::int32_t d;
					std::memcpy( &d, p + 1, 4 );
					add_candidate( ( std::uintptr_t )( p + 5 + d ), 10 + ( target_offset_window > 0 ? 1000 : 0 ) );
				}

				if ( target_offset_window > 0 )
					target_offset_window--;
				p += len;
			}
		}
	}

	if ( candidates.empty( ) )
	{
		std::printf( "[morphine-dumper] player_inventory decrypt: no candidates found\n" );
		return;
	}

	auto* best = &candidates.front( );
	if ( candidates.size( ) > 1 )
	{
		for ( std::size_t i = 0; i < candidates.size( ); i++ )
		{
			std::printf(
				"[morphine-dumper] player_inventory candidate %zu: rva=0x%llX ops=%d score=%d live_hits=%d\n",
				i,
				candidates[ i ].fn_addr - c_runtime::game_base( ),
				candidates[ i ].constants.op_count,
				candidates[ i ].score,
				candidates[ i ].live_hits );
		}
	}
	for ( auto& candidate : candidates )
	{
		if ( ( candidate.live_hits > 0 && best->live_hits == 0 ) ||
			( candidate.live_hits > best->live_hits ) ||
			( candidate.live_hits == best->live_hits && candidate.score > best->score ) ||
			( candidate.live_hits == best->live_hits && candidate.score == best->score &&
				best->constants.was_unrolled && !candidate.constants.was_unrolled ) )
			best = &candidate;
	}

	m_player_inventory_fn_addr = best->fn_addr;
	m_player_inventory_constants = best->constants;

	std::printf( "[morphine-dumper] player_inventory decrypt: extracted %d ops from rva 0x%llX (%zu candidates)\n",
		m_player_inventory_constants.op_count,
		m_player_inventory_fn_addr - c_runtime::game_base( ),
		candidates.size( ) );
	std::printf( "[morphine-dumper] player_inventory decrypt: live_hits=%d score=%d\n",
		best->live_hits,
		best->score );
}

void c_core::resolve_player_eyes_decryption( )
{
	auto base_player = c_runtime::find_class( "BasePlayer" );
	auto eyes_class = c_runtime::find_class( "PlayerEyes" );
	if ( !base_player || !eyes_class )
	{
		std::printf( "[morphine-dumper] player_eyes decrypt: missing class (bp=%p eyes=%p)\n",
			base_player, eyes_class );
		return;
	}

	std::vector<il2cpp::method_info_t*> methods;
	auto eyes_field = il2cpp::get_field_if_type_contains( base_player, "PlayerEyes" );
	auto eyes_offset = eyes_field ? eyes_field->offset( base_player ) : 0;

	auto push_method = [&]( il2cpp::method_info_t* method )
	{
		if ( !method )
			return;
		for ( auto existing : methods )
			if ( existing == method )
				return;
		methods.push_back( method );
	};

	void* iter = nullptr;
	while ( auto method = base_player->methods( &iter ) )
	{
		auto name = method->name( );
		auto ret = method->return_type( );
		auto ret_class = ret ? ret->klass( ) : nullptr;

		if ( ret_class == eyes_class )
		{
			push_method( method );
			continue;
		}

		for ( std::uint32_t i = 0; i < method->param_count( ); i++ )
		{
			auto param = method->get_param( i );
			auto param_class = param ? param->klass( ) : nullptr;
			if ( param_class == eyes_class )
			{
				push_method( method );
				break;
			}
		}

		if ( name && ( std::strstr( name, "Eyes" ) ||
			std::strstr( name, "eyes" ) ||
			std::strstr( name, "PlayerEyes" ) ) )
			push_method( method );
	}

	if ( eyes_offset )
	{
		iter = nullptr;
		while ( auto method = base_player->methods( &iter ) )
		{
			auto fn = method->get_fn_ptr<std::uintptr_t>( );
			if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
				continue;

			auto p = ( std::uint8_t* )fn;
			auto end = p + 0x300;
			while ( p < end )
			{
				hde64s hs;
				auto len = hde64_disasm( p, &hs );
				if ( !len || ( hs.flags & F_ERROR ) )
					break;
				if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
					break;

				if ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )eyes_offset )
				{
					push_method( method );
					break;
				}
				if ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )eyes_offset )
				{
					push_method( method );
					break;
				}

				p += len;
			}
		}
	}

	struct candidate_t
	{
		std::uintptr_t fn_addr;
		c_decryption::constants_t constants;
		int score;
		int live_hits;
	};
	std::vector<candidate_t> candidates;

	auto add_candidate = [&]( std::uintptr_t addr, int score )
	{
		addr = resolve_jump_thunk( addr );
		if ( !addr || IsBadReadPtr( ( void* )addr, 0x100 ) )
			return;
		if ( !fn_has_hidden_value_decrypt_shape( addr ) )
			return;

		auto constants = extract_stack_u32_crypto( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_loop( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_inline( ( std::uint8_t* )addr, 0x500 );

		if ( !constants.valid )
			return;
		if ( m_bn.decrypt_0.valid && c_decryption::ops_match( constants, m_bn.decrypt_0 ) )
			return;
		if ( m_bn.decrypt_1.valid && c_decryption::ops_match( constants, m_bn.decrypt_1 ) )
			return;
		if ( m_cl_active_item_constants.valid && c_decryption::ops_match( constants, m_cl_active_item_constants ) )
			return;
		if ( m_player_inventory_constants.valid && c_decryption::ops_match( constants, m_player_inventory_constants ) )
			return;

		auto live_hits = score_hidden_value_field(
			m_bn.base_networkable_rva,
			m_bn.static_fields,
			m_bn.wrapper_class_ptr,
			m_bn.parent_static_fields,
			m_bn.entities,
			m_bn.hv_offset,
			m_bn.decrypt_0,
			m_bn.decrypt_1,
			eyes_offset,
			eyes_class,
			constants );
		if ( live_hits )
			score += 20 + ( live_hits > 100 ? 100 : live_hits );

		for ( auto& existing : candidates )
		{
			if ( existing.fn_addr == addr || c_decryption::ops_match( existing.constants, constants ) )
			{
				if ( live_hits > existing.live_hits || ( live_hits == existing.live_hits && score > existing.score ) )
				{
					existing.score = score;
					existing.live_hits = live_hits;
				}
				return;
			}
		}

		candidates.push_back( { addr, constants, score, live_hits } );
	};

	for ( auto method : methods )
	{
		auto name = method->name( );
		auto fn = method->get_fn_ptr<std::uintptr_t>( );
		int base_score = 0;
		if ( name && ( std::strstr( name, "Eyes" ) ||
			std::strstr( name, "eyes" ) ||
			std::strstr( name, "PlayerEyes" ) ) )
			base_score += 80;
		auto ret = method->return_type( );
		auto ret_class = ret ? ret->klass( ) : nullptr;
		if ( ret_class == eyes_class )
			base_score += 40;
		for ( std::uint32_t i = 0; i < method->param_count( ); i++ )
		{
			auto param = method->get_param( i );
			auto param_class = param ? param->klass( ) : nullptr;
			if ( param_class == eyes_class )
			{
				base_score += 30;
				break;
			}
		}
		if ( fn_references_offset( fn, eyes_offset ) )
			base_score += 500;
		add_candidate( fn, base_score );

		if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
			continue;

		auto p = ( std::uint8_t* )fn;
		auto end = p + 0x500;
		int target_offset_window = 0;
		while ( p < end )
		{
			hde64s hs;
			auto len = hde64_disasm( p, &hs );
			if ( !len || ( hs.flags & F_ERROR ) )
				break;
			if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
				break;

			if ( eyes_offset && ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )eyes_offset ) )
				target_offset_window = 6;
			if ( eyes_offset && ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )eyes_offset ) )
				target_offset_window = 6;

			if ( p[ 0 ] == 0xE8 && len >= 5 )
			{
				std::int32_t d;
				std::memcpy( &d, p + 1, 4 );
				add_candidate( ( std::uintptr_t )( p + 5 + d ), base_score + ( target_offset_window > 0 ? 1000 : 0 ) );
				target_offset_window = 0;
			}

			if ( target_offset_window > 0 )
				target_offset_window--;
			p += len;
		}
	}

	if ( eyes_field )
	{
		auto field_type = eyes_field->type( );
		auto field_klass = field_type ? field_type->klass( ) : nullptr;
		if ( field_klass )
		{
			void* hv_iter = nullptr;
			while ( auto method = field_klass->methods( &hv_iter ) )
			{
				auto fn = method->get_fn_ptr<std::uintptr_t>( );
				add_candidate( fn, 700 );

				if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
					continue;
				auto p = ( std::uint8_t* )fn;
				auto end = p + 0x300;
				while ( p < end )
				{
					hde64s hs;
					auto len = hde64_disasm( p, &hs );
					if ( !len || ( hs.flags & F_ERROR ) )
						break;
					if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
						break;

					if ( p[ 0 ] == 0xE8 && len >= 5 )
					{
						std::int32_t d;
						std::memcpy( &d, p + 1, 4 );
						add_candidate( ( std::uintptr_t )( p + 5 + d ), 650 );
					}
					p += len;
				}
			}
		}
	}

	auto has_live_candidate = [&] ( ) -> bool
	{
		for ( const auto& candidate : candidates )
			if ( candidate.live_hits > 0 )
				return true;
		return false;
	};

	if ( candidates.empty( ) || !has_live_candidate( ) )
	{
		iter = nullptr;
		while ( auto method = base_player->methods( &iter ) )
		{
			auto fn = method->get_fn_ptr<std::uintptr_t>( );
			if ( !fn || IsBadReadPtr( ( void* )fn, 0x100 ) )
				continue;

			auto p = ( std::uint8_t* )fn;
			auto end = p + 0x700;
			int target_offset_window = fn_references_offset( fn, eyes_offset ) ? 4 : 0;
			while ( p < end )
			{
				hde64s hs;
				auto len = hde64_disasm( p, &hs );
				if ( !len || ( hs.flags & F_ERROR ) )
					break;
				if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
					break;

				if ( eyes_offset && ( ( hs.flags & F_DISP32 ) && hs.disp.disp32 == ( std::uint32_t )eyes_offset ) )
					target_offset_window = 8;
				if ( eyes_offset && ( ( hs.flags & F_DISP8 ) && hs.disp.disp8 == ( std::uint8_t )eyes_offset ) )
					target_offset_window = 8;

				if ( p[ 0 ] == 0xE8 && len >= 5 )
				{
					std::int32_t d;
					std::memcpy( &d, p + 1, 4 );
					add_candidate( ( std::uintptr_t )( p + 5 + d ), 10 + ( target_offset_window > 0 ? 1000 : 0 ) );
				}

				if ( target_offset_window > 0 )
					target_offset_window--;
				p += len;
			}
		}
	}

	if ( candidates.empty( ) )
	{
		std::printf( "[morphine-dumper] player_eyes decrypt: no candidates found\n" );
		return;
	}

	auto* best = &candidates.front( );
	if ( candidates.size( ) > 1 )
	{
		for ( std::size_t i = 0; i < candidates.size( ); i++ )
		{
			std::printf(
				"[morphine-dumper] player_eyes candidate %zu: rva=0x%llX ops=%d score=%d live_hits=%d\n",
				i,
				candidates[ i ].fn_addr - c_runtime::game_base( ),
				candidates[ i ].constants.op_count,
				candidates[ i ].score,
				candidates[ i ].live_hits );
		}
	}
	for ( auto& candidate : candidates )
	{
		if ( ( candidate.live_hits > 0 && best->live_hits == 0 ) ||
			( candidate.live_hits > best->live_hits ) ||
			( candidate.live_hits == best->live_hits && candidate.score > best->score ) ||
			( candidate.live_hits == best->live_hits && candidate.score == best->score &&
				best->constants.was_unrolled && !candidate.constants.was_unrolled ) )
			best = &candidate;
	}

	m_player_eyes_fn_addr = best->fn_addr;
	m_player_eyes_constants = best->constants;

	std::printf( "[morphine-dumper] player_eyes decrypt: extracted %d ops from rva 0x%llX (%zu candidates)\n",
		m_player_eyes_constants.op_count,
		m_player_eyes_fn_addr - c_runtime::game_base( ),
		candidates.size( ) );
	std::printf( "[morphine-dumper] player_eyes decrypt: live_hits=%d score=%d\n",
		best->live_hits,
		best->score );
}

void c_core::resolve_fov_encryption( )
{
	struct fov_candidate_t
	{
		c_decryption::constants_t constants;
		std::uint8_t* fn_addr;
		bool source_is_encrypt;
		const char* source;
		int runtime_score;
	};
	std::vector<fov_candidate_t> candidates;

	auto add_candidate = [&]( std::uintptr_t addr, bool source_is_encrypt, const char* source )
	{
		addr = resolve_jump_thunk( addr );
		if ( !addr || IsBadReadPtr( ( void* )addr, 0x100 ) )
			return;

		auto constants = extract_stack_u32_crypto( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_loop( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			constants = c_decryption::extract_inline( ( std::uint8_t* )addr, 0x500 );
		if ( !constants.valid )
			return;
		if ( m_bn.decrypt_0.valid && c_decryption::ops_match( constants, m_bn.decrypt_0 ) )
			return;
		if ( m_bn.decrypt_1.valid && c_decryption::ops_match( constants, m_bn.decrypt_1 ) )
			return;

		for ( auto& existing : candidates )
		{
			if ( existing.fn_addr == ( std::uint8_t* )addr )
				return;
			if ( c_decryption::ops_match( existing.constants, constants ) &&
				existing.source_is_encrypt == source_is_encrypt )
				return;
		}

		candidates.push_back( { constants, ( std::uint8_t* )addr, source_is_encrypt, source, 0 } );
	};

	auto scan_function = [&]( std::uintptr_t addr, bool source_is_encrypt, const char* source ) -> void
	{
		if ( !addr || IsBadReadPtr( ( void* )addr, 0x100 ) )
			return;

		add_candidate( addr, source_is_encrypt, source );

		auto p = ( std::uint8_t* )addr;
		auto end = p + 0x500;
		while ( p < end )
		{
			hde64s hs;
			auto len = hde64_disasm( p, &hs );
			if ( !len || ( hs.flags & F_ERROR ) )
				break;
			if ( hs.opcode == 0xC3 || hs.opcode == 0xCC )
				break;

			if ( p[ 0 ] == 0xE8 && len >= 5 )
			{
				std::int32_t d;
				std::memcpy( &d, p + 1, 4 );
				auto target = ( std::uintptr_t )( p + 5 + d );
				if ( target != m_bn.decrypt_0_addr && target != m_bn.decrypt_1_addr )
				{
					add_candidate( target, source_is_encrypt, source );
					auto nested = ( std::uint8_t* )target;
					auto nested_end = nested + 0x300;
					while ( nested < nested_end && !IsBadReadPtr( nested, 0x10 ) )
					{
						hde64s nhs;
						auto nlen = hde64_disasm( nested, &nhs );
						if ( !nlen || ( nhs.flags & F_ERROR ) )
							break;
						if ( nhs.opcode == 0xC3 || nhs.opcode == 0xCC )
							break;
						if ( nested[ 0 ] == 0xE8 && nlen >= 5 )
						{
							std::int32_t nd;
							std::memcpy( &nd, nested + 1, 4 );
							auto nested_target = ( std::uintptr_t )( nested + 5 + nd );
							if ( nested_target != m_bn.decrypt_0_addr && nested_target != m_bn.decrypt_1_addr )
								add_candidate( nested_target, source_is_encrypt, source );
						}
						nested += nlen;
					}
				}
			}

			p += len;
		}
	};

	if ( !rust::console_system::console_system_index_client_find )
	{
		std::printf( "[morphine-dumper] fov: console_system_index_client_find is null\n" );
		return;
	}

	auto str = system_c::string_t::create_string( L"graphics.fov" );
	auto cmd = str ? safe_find_command( str ) : nullptr;
	if ( !cmd )
	{
		std::printf( "[morphine-dumper] fov: graphics.fov command unavailable\n" );
		return;
	}

	auto setter = rust::console_system::set_override_offset ? safe_get_setter( cmd ) : 0;
	auto getter = rust::console_system::get_override_offset ? safe_get_getter( cmd ) : 0;
	auto caller = rust::console_system::call_offset ? safe_get_call( cmd ) : 0;
	std::printf( "[morphine-dumper] fov handlers: setter=0x%llX getter=0x%llX caller=0x%llX\n",
		setter ? setter - c_runtime::game_base( ) : 0,
		getter ? getter - c_runtime::game_base( ) : 0,
		caller ? caller - c_runtime::game_base( ) : 0 );
	if ( setter ) scan_function( setter, true, "console setter" );
	if ( getter ) scan_function( getter, false, "console getter" );
	if ( caller ) scan_function( caller, false, "console call" );
	if ( !setter && !getter && !caller )
		std::printf( "[morphine-dumper] fov: cmd handlers returned 0 (get_off=0x%zX set_off=0x%zX call_off=0x%zX)\n",
			rust::console_system::get_override_offset,
			rust::console_system::set_override_offset,
			rust::console_system::call_offset );

	const char* resolution_list_terms[] = { "List", "Resolution" };
	auto graphics = class_with_public_static_field_type_terms( resolution_list_terms, 2 );
	if ( !graphics )
		graphics = c_runtime::find_class( "Graphics", "ConVar" );
	if ( !graphics )
		graphics = class_name_contains( "Graphics", "ConVar" );
	if ( !graphics )
		graphics = class_name_contains_i( "Graphics", "Convar" );
	if ( !graphics )
		graphics = class_with_field_name_contains_type( "fov", "Single", true );
	if ( graphics )
	{
		void* iter = nullptr;
		while ( auto method = graphics->methods( &iter ) )
		{
			auto fn = method->get_fn_ptr<std::uintptr_t>( );
			auto name = method->name( );
			auto is_setter = name && ascii_contains_i( name, "set" );
			scan_function( fn, is_setter, is_setter ? "convar graphics setter" : "convar graphics method" );
		}
	}

	if ( candidates.empty( ) )
	{
		std::printf( "[morphine-dumper] fov: no valid candidates found\n" );
		return;
	}

	std::uint32_t live_fov_bits = 0;
	std::uint64_t live_fov_off = 0;
	if ( read_live_convar_fov_bits( live_fov_bits, live_fov_off ) )
	{
		for ( auto& c : candidates )
		{
			auto inverse_bits = apply_u32_ops( live_fov_bits, c.constants, true );
			auto direct_bits = apply_u32_ops( live_fov_bits, c.constants, false );
			auto inverse_ok = fov_bits_sane( inverse_bits );
			auto direct_ok = fov_bits_sane( direct_bits );

			if ( inverse_ok != direct_ok )
			{
				c.source_is_encrypt = inverse_ok;
				c.runtime_score += 1000;
			}
			else if ( inverse_ok && direct_ok )
			{
				c.runtime_score += 250;
			}
		}
	}

	for ( std::size_t i = 0; i < candidates.size( ); i++ )
	{
		auto& c = candidates[ i ];
		std::printf( "[morphine-dumper] fov candidate %zu: rva=0x%llX ops=%d unrolled=%d source=%s\n",
			i,
			( std::uintptr_t )c.fn_addr - c_runtime::game_base( ),
			c.constants.op_count,
			c.constants.was_unrolled,
			c.source ? c.source : "unknown" );
		for ( int j = 0; j < c.constants.op_count; j++ )
			std::printf( "[morphine-dumper]   op[%d] %s 0x%X\n", j,
				c_decryption::op_name( c.constants.ops[ j ].type ), c.constants.ops[ j ].value );
	}

	auto* best = &candidates.back( );
	auto score_candidate = []( const fov_candidate_t& c ) -> int
	{
		int score = c.constants.op_count * 10;
		score += c.runtime_score;
		if ( c.constants.op_count == 4 ) score += 50;
		if ( c.source && std::strstr( c.source, "console setter" ) ) score += 40;
		else if ( c.source && std::strstr( c.source, "console getter" ) ) score += 30;
		return score;
	};
	for ( auto& candidate : candidates )
		if ( score_candidate( candidate ) > score_candidate( *best ) )
			best = &candidate;

	m_fov_fn_addr = ( std::uintptr_t )best->fn_addr;
	m_fov_constants = best->constants;
	m_fov_constants_are_encrypt = best->source_is_encrypt;

	std::printf( "[morphine-dumper] fov: extracted %d %s ops from rva 0x%llX (%zu candidates, source=%s)\n",
		m_fov_constants.op_count,
		m_fov_constants_are_encrypt ? "encrypt" : "decrypt",
		m_fov_fn_addr - c_runtime::game_base( ),
		candidates.size( ),
		best->source ? best->source : "unknown" );
	for ( int i = 0; i < m_fov_constants.op_count; i++ )
		std::printf( "[morphine-dumper] fov:   op[%d] %s 0x%X\n", i,
			c_decryption::op_name( m_fov_constants.ops[ i ].type ), m_fov_constants.ops[ i ].value );
}

void c_core::resolve_item_classes( )
{
	auto player_loot = c_runtime::find_class( "PlayerLoot" );
	if ( !player_loot )
	{
		std::printf( "[morphine-dumper] resolve_item_classes: PlayerLoot not found\n" );
		return;
	}

	auto entity_source = c_runtime::find_field( player_loot, "entitySource" );
	if ( entity_source )
	{
		auto item_field = il2cpp::get_field_by_offset( player_loot, entity_source->offset( ) + 0x8 );
		m_item_class = item_field ? item_field->type( )->klass( ) : nullptr;
	}

	auto item_definition = c_runtime::find_class( "ItemDefinition" );
	if ( !m_item_class && item_definition )
		m_item_class = class_with_instance_field_type_class( item_definition );

	if ( m_item_class )
	{
		auto base_entity = c_runtime::find_class( "BaseEntity" );
		if ( base_entity )
		{
			auto get_item = il2cpp::get_method_by_return_type_attrs(
				NO_FILT, base_entity, m_item_class, 0, 0, 1 );
			if ( get_item )
			{
				auto p = get_item->get_param( 0 );
				m_item_id_class = p ? p->klass( ) : nullptr;
			}
		}
	}

	auto containers_field = il2cpp::get_field_by_name( player_loot, "containers" );
	auto templated_ic = containers_field ? containers_field->type( )->klass( ) : nullptr;
	if ( templated_ic )
		m_item_container_class = templated_ic->get_generic_argument_at( 0 );

	std::printf( "[morphine-dumper] resolve_item_classes: item=%p item_id=%p item_container=%p\n",
		m_item_class, m_item_id_class, m_item_container_class );
}

bool c_core::resolve_console_system( )
{
	auto display = il2cpp::search_for_class_by_method_in_assembly(
		"Facepunch.Console", "<Find>b__0", nullptr, -1 );

	if ( display )
	{
		if ( auto find_b0 = il2cpp::get_method_by_name( display, "<Find>b__0" ) )
		{
			if ( auto p = find_b0->get_param( 0 ) )
				m_console_command_class = p->klass( );
		}
	}

	if ( !m_console_command_class )
	{
		std::printf( "[morphine-dumper] console: ConsoleSystem.Command not resolved\n" );
		return false;
	}

	if ( auto f = il2cpp::get_field_if_type_contains( m_console_command_class, "System.Func<System.String>" ) )
		rust::console_system::get_override_offset = f->offset( );
	if ( auto f = il2cpp::get_field_if_type_contains( m_console_command_class, "System.Action<System.String>" ) )
		rust::console_system::set_override_offset = f->offset( );
	if ( auto f = il2cpp::get_field_if_type_contains( m_console_command_class, "System.Action<%", FIELD_ATTRIBUTE_PUBLIC ) )
		rust::console_system::call_offset = f->offset( );

	auto show_if = c_runtime::find_class( "ShowIfConvarEnabled" );
	auto on_enable_filter = show_if ? il2cpp::get_method_by_name( show_if, "OnEnable" ) : nullptr;
	std::uint64_t filter_addr = on_enable_filter ? on_enable_filter->get_fn_ptr<std::uint64_t>( ) : 0;

	il2cpp::method_info_t* find_method = nullptr;
	auto str_type = il2cpp::get_class_by_name( "String", "System" );
	if ( str_type && filter_addr )
	{
		il2cpp::il2cpp_type_t* params[] = { str_type->type( ) };
		find_method = il2cpp::get_method_in_method_by_return_type_and_param_types(
			( il2cpp::il2cpp_class_t* )0x1337DEADBEEF7331ULL,
			il2cpp::method_filter_t( filter_addr, 0, 1 ),
			m_console_command_class->type( ),
			METHOD_ATTRIBUTE_PUBLIC,
			METHOD_ATTRIBUTE_STATIC,
			params, 1 );
	}

	if ( !find_method )
	{
		auto string_class = il2cpp::get_class_by_name( "String", "System" );
		il2cpp::method_info_t* fallback = nullptr;
		auto domain = il2cpp::il2cpp_domain_t::get( );
		size_t assembly_count = 0;
		auto assemblies = domain ? domain->get_assemblies( &assembly_count ) : nullptr;
		for ( size_t i = 0; assemblies && i < assembly_count && !find_method; i++ )
		{
			auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
			if ( !image )
				continue;
			auto class_count = image->class_count( );
			for ( size_t j = 0; j < class_count && !find_method; j++ )
			{
				auto klass = image->get_class( j );
				if ( !klass )
					continue;
				void* iter = nullptr;
				while ( auto method = klass->methods( &iter ) )
				{
					if ( method->param_count( ) != 1 )
						continue;
					if ( !( method->flags( ) & METHOD_ATTRIBUTE_STATIC ) )
						continue;
					auto ret = method->return_type( );
					auto ret_class = ret ? ret->klass( ) : nullptr;
					auto param = method->get_param( 0 );
					auto param_class = param ? param->klass( ) : nullptr;
					if ( ret_class != m_console_command_class || param_class != string_class )
						continue;

					if ( !fallback )
						fallback = method;
					auto name = method->name( );
					if ( name && std::strstr( name, "Find" ) )
					{
						find_method = method;
						break;
					}
				}
			}
		}
		if ( !find_method )
			find_method = fallback;
	}

	if ( !find_method )
	{
		std::printf( "[morphine-dumper] console: ConsoleSystem.Index.Client::Find not resolved\n" );
		return false;
	}

	rust::console_system::console_system_index_client_find =
		( decltype( rust::console_system::console_system_index_client_find ) )
		find_method->get_fn_ptr<std::uint64_t>( );

	std::printf( "[morphine-dumper] console: command_class=%p find=%p set_off=0x%zX\n",
		m_console_command_class,
		rust::console_system::console_system_index_client_find,
		rust::console_system::set_override_offset );

	return rust::console_system::console_system_index_client_find != nullptr;
}

void c_core::write_il2cpphandle( c_writer& w )
{
	w.write_global( "il2cpphandle", m_gc_handles_rva );
	w.struct_begin( "gc_handles" );
	w.struct_field( "array_rva", m_gc_handles_rva );
	w.struct_end( );
}

static void write_il2cpp_api( c_writer& w )
{
	auto mod = GetModuleHandleA( "GameAssembly.dll" );
	auto export_rva = [&]( const char* name ) -> std::uint64_t
	{
		auto addr = mod ? ( std::uintptr_t )GetProcAddress( mod, name ) : 0;
		return addr ? addr - c_runtime::game_base( ) : 0;
	};

	w.struct_begin( "il2cpp_api" );
	w.struct_field( "domain_get",                   export_rva( "il2cpp_domain_get" ) );
	w.struct_field( "domain_get_assemblies",        export_rva( "il2cpp_domain_get_assemblies" ) );
	w.struct_field( "domain_assembly_open",         export_rva( "il2cpp_domain_assembly_open" ) );
	w.struct_field( "assembly_get_image",           export_rva( "il2cpp_assembly_get_image" ) );
	w.struct_field( "class_from_name",              export_rva( "il2cpp_class_from_name" ) );
	w.struct_field( "image_get_class_count",        export_rva( "il2cpp_image_get_class_count" ) );
	w.struct_field( "image_get_class",              export_rva( "il2cpp_image_get_class" ) );
	w.struct_field( "class_get_methods",            export_rva( "il2cpp_class_get_methods" ) );
	w.struct_field( "class_get_fields",             export_rva( "il2cpp_class_get_fields" ) );
	w.struct_field( "class_get_nested_types",       export_rva( "il2cpp_class_get_nested_types" ) );
	w.struct_field( "class_get_type",               export_rva( "il2cpp_class_get_type" ) );
	w.struct_field( "class_get_name",               export_rva( "il2cpp_class_get_name" ) );
	w.struct_field( "class_get_namespace",          export_rva( "il2cpp_class_get_namespace" ) );
	w.struct_field( "class_get_parent",             export_rva( "il2cpp_class_get_parent" ) );
	w.struct_field( "class_get_image",              export_rva( "il2cpp_class_get_image" ) );
	w.struct_field( "class_get_flags",              export_rva( "il2cpp_class_get_flags" ) );
	w.struct_field( "class_get_static_field_data",  export_rva( "il2cpp_class_get_static_field_data" ) );
	w.struct_field( "class_from_il2cpp_type",       export_rva( "il2cpp_class_from_il2cpp_type" ) );
	w.struct_field( "type_get_object",              export_rva( "il2cpp_type_get_object" ) );
	w.struct_field( "type_get_class_or_element_class", export_rva( "il2cpp_type_get_class_or_element_class" ) );
	w.struct_field( "type_get_name",                export_rva( "il2cpp_type_get_name" ) );
	w.struct_field( "type_get_attrs",               export_rva( "il2cpp_type_get_attrs" ) );
	w.struct_field( "method_get_param_count",       export_rva( "il2cpp_method_get_param_count" ) );
	w.struct_field( "method_get_name",              export_rva( "il2cpp_method_get_name" ) );
	w.struct_field( "method_get_param",             export_rva( "il2cpp_method_get_param" ) );
	w.struct_field( "method_get_return_type",       export_rva( "il2cpp_method_get_return_type" ) );
	w.struct_field( "method_get_class",             export_rva( "il2cpp_method_get_class" ) );
	w.struct_field( "method_get_flags",             export_rva( "il2cpp_method_get_flags" ) );
	w.struct_field( "field_get_offset",             export_rva( "il2cpp_field_get_offset" ) );
	w.struct_field( "field_get_type",               export_rva( "il2cpp_field_get_type" ) );
	w.struct_field( "field_get_parent",             export_rva( "il2cpp_field_get_parent" ) );
	w.struct_field( "field_get_name",               export_rva( "il2cpp_field_get_name" ) );
	w.struct_field( "field_get_flags",              export_rva( "il2cpp_field_get_flags" ) );
	w.struct_field( "field_static_get_value",       export_rva( "il2cpp_field_static_get_value" ) );
	w.struct_field( "object_get_class",             export_rva( "il2cpp_object_get_class" ) );
	w.struct_field( "object_new",                   export_rva( "il2cpp_object_new" ) );
	w.struct_field( "resolve_icall",                export_rva( "il2cpp_resolve_icall" ) );
	w.struct_field( "gchandle_get_target",          export_rva( "il2cpp_gchandle_get_target" ) );
	w.struct_field( "gchandle_new",                 export_rva( "il2cpp_gchandle_new" ) );
	w.struct_field( "gchandle_free",                export_rva( "il2cpp_gchandle_free" ) );
	w.struct_field( "array_new",                    export_rva( "il2cpp_array_new" ) );
	w.struct_field( "string_new",                   export_rva( "il2cpp_string_new" ) );
	w.struct_end( );
}

static void write_klass_rvas(
	c_writer& w,
	std::uint64_t base_networkable_rva,
	il2cpp::il2cpp_class_t* item_class,
	il2cpp::il2cpp_class_t* item_id_class,
	il2cpp::il2cpp_class_t* console_command_class )
{
	auto rva_of = []( il2cpp::il2cpp_class_t* klass ) -> std::uint64_t
	{
		if ( !klass )
			return 0;
		auto rva = c_runtime::static_class_rva( klass );
		if ( !rva )
			if ( auto nested = c_runtime::get_inner_static_class( klass ) )
				rva = c_runtime::static_class_rva( nested );
		return rva;
	};
	auto find_named = []( const char* name ) -> il2cpp::il2cpp_class_t*
	{
		if ( auto klass = c_runtime::find_class( name ) )
			return klass;
		if ( auto klass = class_name_equals( name ) )
			return klass;
		return class_name_contains( name );
	};
	auto class_rva = [&]( const char* name ) -> std::uint64_t { return rva_of( find_named( name ) ); };

	auto base_player = find_named( "BasePlayer" );
	auto base_combat_entity = base_player ? base_player->parent( ) : find_named( "BaseCombatEntity" );
	auto base_entity = base_combat_entity ? base_combat_entity->parent( ) : find_named( "BaseEntity" );
	auto player_eyes = base_player ? il2cpp::get_class_from_field_type_by_type_contains( base_player, "PlayerEyes" ) : find_named( "PlayerEyes" );
	auto player_inventory = base_player ? il2cpp::get_class_from_field_type_by_type_contains( base_player, "PlayerInventory" ) : find_named( "PlayerInventory" );

	w.struct_begin( "klass_rvas" );
	w.struct_field( "BaseNetworkable",       base_networkable_rva ? base_networkable_rva : class_rva( "BaseNetworkable" ) );
	w.struct_field( "BaseEntity",            rva_of( base_entity ) );
	w.struct_field( "BaseCombatEntity",      rva_of( base_combat_entity ) );
	w.struct_field( "BasePlayer",            rva_of( base_player ) );
	w.struct_field( "BaseNpc",               class_rva( "BaseNpc" ) );
	w.struct_field( "BaseVehicle",           class_rva( "BaseVehicle" ) );
	w.struct_field( "DroppedItemContainer",  class_rva( "DroppedItemContainer" ) );
	w.struct_field( "OreResourceEntity",     class_rva( "OreResourceEntity" ) );
	w.struct_field( "CollectibleEntity",     class_rva( "CollectibleEntity" ) );
	w.struct_field( "BuildingBlock",         class_rva( "BuildingBlock" ) );
	w.struct_field( "BuildingPrivlidge",     class_rva( "BuildingPrivlidge" ) );
	w.struct_field( "Door",                  class_rva( "Door" ) );
	w.struct_field( "WorldItem",             class_rva( "WorldItem" ) );
	w.struct_field( "Signage",               class_rva( "Signage" ) );
	w.struct_field( "MainCamera",            class_rva( "MainCamera" ) );
	w.struct_field( "PlayerEyes",            rva_of( player_eyes ) );
	w.struct_field( "TOD_Sky",               class_rva( "TOD_Sky" ) );
	w.struct_field( "Item",                  c_runtime::static_class_rva( item_class ) );
	w.struct_field( "ItemId",                c_runtime::static_class_rva( item_id_class ) );
	w.struct_field( "ConsoleSystem_Command", c_runtime::static_class_rva( console_command_class ) );
	w.struct_field( "PlayerInventory_typenav", rva_of( player_inventory ) );
	w.struct_end( );
}

static void write_physx( c_writer& w )
{
	auto physics = c_runtime::find_class( "Physics", "UnityEngine" );
	auto type_info = physics ? c_runtime::static_class_rva( physics ) : 0;

	std::printf( "[morphine-dumper] physx: class=%p type_info=0x%llX static_fields=0x%zX\n",
		physics,
		type_info,
		offsetof( Il2CppClass, static_fields ) );

	w.struct_begin( "PhysX" );
	w.struct_field( "type_info", type_info );
	w.struct_field( "static_fields", offsetof( Il2CppClass, static_fields ) );
	w.struct_end( );
}

static void write_game_manager( c_writer& w )
{
	auto klass = find_named_class( "GameManager" );
	if ( !klass )
		klass = class_name_equals( "GameManager" );
	if ( !klass )
		klass = class_name_contains( "GameManager" );
	if ( !klass )
		klass = class_type_name_contains( "GameManager" );
	if ( !klass )
		klass = find_named_class( "Client" );
	if ( !klass )
		klass = class_name_equals( "Client" );

	auto singleton_klass = class_type_name_contains( "SingletonComponent", "GameManager" );
	if ( !singleton_klass )
		singleton_klass = singleton_parent_class( "GameManager" );
	auto typeinfo_klass = singleton_klass ? singleton_klass : klass;

	auto rva = static_class_rva_any( typeinfo_klass );
	auto static_klass = klass ? c_runtime::get_inner_static_class( klass ) : nullptr;
	if ( !static_klass && singleton_klass )
		static_klass = c_runtime::get_inner_static_class( singleton_klass );
	if ( !static_klass && klass )
		static_klass = nested_static_class_with_most_static_fields( klass );
	if ( !static_klass && singleton_klass )
		static_klass = nested_static_class_with_most_static_fields( singleton_klass );
	if ( !static_klass )
		static_klass = typeinfo_klass;

	std::uint64_t instance = 0;
	bool instance_resolved = false;
	if ( static_klass )
	{
		if ( klass )
			if ( auto field = il2cpp::get_field_from_field_type_class( static_klass, klass ) )
			{
				instance = field->offset( static_klass );
				instance_resolved = true;
			}
		if ( !instance && singleton_klass )
			if ( auto field = il2cpp::get_field_from_field_type_class( static_klass, singleton_klass ) )
			{
				instance = field->offset( static_klass );
				instance_resolved = true;
			}
		if ( !instance && klass )
			instance = field_offset_by_type_class( static_klass, klass );
		if ( !instance && singleton_klass )
			instance = field_offset_by_type_class( static_klass, singleton_klass );
		if ( !instance )
		{
			const char* instance_names[] = { "Instance", "instance", "_instance", "singleton", "Singleton" };
			instance = field_offset_by_names( static_klass, instance_names, 5 );
		}
		if ( !instance )
			instance = field_offset_by_name_contains( static_klass, "instance" );
		if ( instance )
			instance_resolved = true;
	}

	std::printf( "[morphine-dumper] game_manager: class=%p rva=0x%llX instance=0x%llX%s\n",
		typeinfo_klass,
		rva,
		instance,
		instance_resolved ? "" : " unresolved" );

	w.struct_begin( "game_manager" );
	w.struct_field( "game_manager", rva );
	w.struct_field( "static_fields", typeinfo_klass ? offsetof( Il2CppClass, static_fields ) : 0 );
	if ( instance_resolved )
		w.struct_field_probed( "instance", instance );
	else
		w.struct_field( "instance", instance );
	w.struct_end( );
}

static void write_klass_layout( c_writer& w )
{
	w.struct_begin( "klass_layout" );
	w.struct_field( "name_ptr",      0x10 );
	w.struct_field( "static_fields", 0xB8 );
	w.struct_end( );
}

void c_core::write_basenetworkable( c_writer& w )
{
	auto klass = c_runtime::find_class( "BaseNetworkable" );
	auto base_entity = c_runtime::find_class( "BaseEntity" );
	auto networkable_field_by_shape = []( il2cpp::il2cpp_class_t* owner ) -> std::uint64_t
	{
		if ( !owner )
			return 0;

		for ( auto cur = owner; cur; cur = cur->parent( ) )
		{
			void* iter = nullptr;
			while ( auto f = cur->fields( &iter ) )
			{
				if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
					continue;

				auto type = f->type( );
				auto type_class = type ? type->klass( ) : nullptr;
				if ( !type_class )
					continue;
				if ( field_type_matches( f, "BaseEntity" ) || field_type_matches( f, "EntityRef" ) || is_entity_ref_type( type_class ) )
					continue;

				bool has_id = field_offset_by_name( type_class, "ID" ) == 0x20;
				if ( !has_id )
				{
					auto id_field = field_by_offset_walk( type_class, 0x20 );
					auto id_name = id_field ? id_field->name( ) : nullptr;
					has_id = id_field && ( ascii_contains_i( id_name, "id" ) || field_type_matches( id_field, "UInt32" ) );
				}

				if ( has_id )
					return f->offset( cur );
			}
		}

		return 0;
	};
	const char* parent_names[] = { "parentEntity", "parent_entity", "_parentEntity", "_parent_entity" };
	const char* children_names[] = { "children", "_children" };
	const char* net_names[] = { "net", "Net", "_net" };
	auto parent_entity = field_offset_by_names( klass, parent_names, 4 );
	auto children = field_offset_by_names( klass, children_names, 2 );
	auto net = field_offset_by_names( klass, net_names, 3 );
	auto network_range = field_offset_by_name( klass, "networkRange" );
	if ( auto shaped_net = networkable_field_by_shape( klass ) )
		net = shaped_net;
	if ( auto shaped_net = networkable_field_by_shape( base_entity ) )
		net = shaped_net;
	if ( !net )
		net = field_offset_by_names( base_entity, net_names, 3 );
	if ( !net )
		net = field_offset_by_name_contains_type( klass, "net", nullptr, "Networkable" );
	if ( !net )
		net = field_offset_by_type_or_class_contains( klass, "Networkable" );
	if ( !net )
		net = field_offset_by_name_contains_type( base_entity, "net", nullptr, "Networkable" );
	if ( !net )
		net = field_offset_by_type_or_class_contains( base_entity, "Networkable" );
	if ( !net && network_range )
		net = first_wrapper_field_after( klass, network_range );
	if ( !net && network_range )
		net = first_wrapper_field_after( base_entity, network_range );
	if ( !parent_entity )
		parent_entity = field_offset_by_names( base_entity, parent_names, 4 );
	if ( !parent_entity )
		parent_entity = field_offset_by_type_or_class_contains( base_entity, "EntityRef" );
	if ( !parent_entity )
		parent_entity = field_offset_by_type_or_class_contains( klass, "EntityRef" );
	if ( !parent_entity )
		parent_entity = field_offset_by_name_contains( base_entity, "parent", "entity" );
	if ( !parent_entity )
		parent_entity = field_offset_by_name_contains( klass, "parent", "entity" );
	if ( !children )
		children = field_offset_by_names( base_entity, children_names, 2 );
	if ( !children )
		children = field_offset_by_type_or_class_contains( base_entity, "List<BaseEntity>" );
	if ( !net )
		net = field_offset_by_type_or_class_contains( klass, "Networkable" );
	if ( !parent_entity )
	{
		auto after = net ? net : children;
		parent_entity = first_wrapper_field_after( base_entity, after );
		if ( !parent_entity )
			parent_entity = first_wrapper_field_after( klass, after );
	}

	std::printf( "[morphine-dumper] base_networkable fields: parentEntity=0x%llX children=0x%llX net=0x%llX\n",
		parent_entity, children, net );

	w.struct_begin( "base_networkable" );
	w.struct_field( "static_fields",        0xB8 );
	w.struct_field( "wrapper_class_ptr",    m_bn.wrapper_class_ptr );
	w.struct_field( "parent_static_fields", m_bn.parent_static_fields );
	w.struct_field( "hv_offset",            m_bn.hv_offset );
	w.struct_field( "entities",             m_bn.entities );
	w.struct_field_u32( "buffer_list_array", 0x10 );
	w.struct_field_u32( "buffer_list_size",  0x18 );
	w.struct_field( "client_entities_decryption", m_bn.decrypt_0_addr ? m_bn.decrypt_0_addr - c_runtime::game_base( ) : 0 );
	w.struct_field( "entity_list_wrapper",  m_bn.entity_list_wrapper_addr ? m_bn.entity_list_wrapper_addr - c_runtime::game_base( ) : 0 );
	w.struct_field( "entity_list_decryption", m_bn.decrypt_1_addr ? m_bn.decrypt_1_addr - c_runtime::game_base( ) : 0 );
	w.struct_field( "entity",               m_bn.entities );
	w.struct_field( "buffer",               m_bn.entities );
	w.struct_field( "prefabID",             field_offset_by_name( klass, "prefabID" ) );
	w.struct_field( "parentEntity",         parent_entity );
	w.struct_field( "children",             children );
	w.struct_field( "net",                  net );
	w.struct_field( "globalBroadcast",      field_offset_by_name( klass, "globalBroadcast" ) );
	w.struct_field( "networkRange",         network_range );
	w.struct_end( );
}

void c_core::write_camera( c_writer& w )
{
	auto camera = c_runtime::find_class( "Camera", "UnityEngine" );
	if ( !camera )
		camera = find_named_class( "Camera" );
	auto world_to_camera = field_offset_by_name( camera, "worldToCameraMatrix" );
	if ( !world_to_camera )
		world_to_camera = field_offset_by_name_contains( camera, "world", "camera" );
	auto projection = field_offset_by_name( camera, "projectionMatrix" );
	if ( !projection )
		projection = field_offset_by_name_contains( camera, "projection", "matrix" );

	w.struct_begin( "camera" );
	w.struct_field( "camera_static", m_camera.camera_static );
	w.struct_field( "static_fields", m_camera.camera_static );
	w.struct_field( "camera_object", m_camera.camera_object );
	w.struct_field( "instance",      m_camera.camera_object );
	w.struct_field( "buffer",        m_camera.camera_object );
	w.struct_field( "entity",        m_camera.entity );
	w.struct_field( "position",      m_camera.position );
	w.struct_field( "viewMatrix",    m_camera.view_matrix );
	w.struct_field( "projectionMatrix", m_camera.projection_matrix ? m_camera.projection_matrix : projection );
	w.struct_field_u32( "projection_layout", m_camera.projection_layout );
	w.struct_field( "fieldOfView",   m_camera.field_of_view );
	w.struct_field( "aspect",        m_camera.aspect );
	w.struct_field( "nearClip",      m_camera.near_clip );
	w.struct_field( "farClip",       m_camera.far_clip );
	w.struct_field( "culling_mask",  m_camera.culling_mask );
	w.struct_field( "viewProjectionMatrix", m_camera.view_matrix );
	w.struct_field( "worldToCameraMatrix",
		m_camera.world_to_camera_matrix ? m_camera.world_to_camera_matrix : world_to_camera );
	w.struct_field( "cullingMask",   m_camera.culling_mask );
	w.struct_end( );
}

void c_core::write_base_player( c_writer& w )
{
	auto klass = c_runtime::find_class( "BasePlayer" );

	auto item_id_class       = m_item_id_class;
	auto player_model_klass  = c_runtime::find_class( "PlayerModel" );
	if ( !player_model_klass )
		player_model_klass = class_name_contains( "PlayerModel" );
	auto player_input_klass  = c_runtime::find_class( "PlayerInput" );
	auto base_movement_klass = c_runtime::find_class( "BaseMovement" );
	auto rigidbody_klass     = c_runtime::find_class( "Rigidbody" );
	auto gesture_klass       = c_runtime::find_class( "GestureConfig" );
	auto player_team_klass   = find_named_class( "PlayerTeam" );
	if ( !player_team_klass )
		player_team_klass = c_runtime::find_class( "PlayerTeam" );
	if ( !player_team_klass )
		player_team_klass = class_type_name_contains( "PlayerTeam" );
	if ( !player_team_klass )
		player_team_klass = class_name_contains_i( "Player", "Team" );
	auto model_state_klass   = c_runtime::find_class( "ModelState" );
	if ( !model_state_klass )
		model_state_klass = class_name_contains( "ModelState" );
	if ( !model_state_klass )
		model_state_klass = find_model_state_class( );
	auto base_mountable_klass = c_runtime::find_class( "BaseMountable" );
	if ( !base_mountable_klass )
		base_mountable_klass = class_name_contains( "BaseMountable" );
	if ( !base_mountable_klass )
		base_mountable_klass = class_type_name_contains( "BaseMountable" );
	if ( !base_mountable_klass )
		base_mountable_klass = class_name_contains_i( "Base", "Mountable" );
	auto player_belt_klass = c_runtime::find_class( "PlayerBelt" );
	if ( !player_belt_klass )
		player_belt_klass = class_name_contains( "PlayerBelt" );
	if ( !player_belt_klass )
		player_belt_klass = class_type_name_contains( "PlayerBelt" );
	if ( !player_belt_klass )
		player_belt_klass = class_name_contains_i( "Player", "Belt" );
	if ( !player_belt_klass )
		player_belt_klass = class_with_method_names( "GetActiveItem", "ChangeSelect" );

	std::uint64_t off_cl_active_item   = 0;
	std::uint64_t off_player_eyes      = 0;
	std::uint64_t off_player_inventory = 0;
	std::uint64_t off_current_team     = 0;
	std::uint64_t off_base_movement    = 0;
	std::uint64_t off_player_model     = 0;
	std::uint64_t off_player_flags     = 0;
	std::uint64_t off_user_id          = 0;
	std::uint64_t off_user_id_string   = 0;
	std::uint64_t off_display_name     = 0;
	std::uint64_t off_player_input     = 0;
	std::uint64_t off_player_rigidbody = 0;
	std::uint64_t off_frozen           = 0;
	std::uint64_t off_current_gesture  = 0;
	std::uint64_t off_model_state      = 0;
	std::uint64_t off_mounted          = 0;
	std::uint64_t off_belt_direct      = 0;
	std::uint64_t off_looking_at       = 0;
	std::uint64_t off_weapon_move_speed_scale = 0;
	std::uint64_t off_clothing_blocks_aiming = 0;

	if ( klass )
	{
		if ( item_id_class )
			if ( auto f = il2cpp::get_field_if_type_contains( klass, item_id_class->name( ) ) )
				off_cl_active_item = f->offset( klass );
		if ( !off_cl_active_item && item_id_class )
			off_cl_active_item = field_offset_by_type_contains( klass, item_id_class->name( ) );
		if ( m_cl_active_item_offset )
			off_cl_active_item = m_cl_active_item_offset;

		if ( auto f = il2cpp::get_field_if_type_contains( klass, "PlayerEyes" ) )
			off_player_eyes = f->offset( klass );
		if ( !off_player_eyes )
			off_player_eyes = field_offset_by_type_contains( klass, "PlayerEyes" );

		if ( auto f = il2cpp::get_field_if_type_contains( klass, "PlayerInventory" ) )
			off_player_inventory = f->offset( klass );
		if ( !off_player_inventory )
			off_player_inventory = field_offset_by_type_contains( klass, "PlayerInventory" );

		if ( auto f = il2cpp::get_field_by_name( klass, "currentTeam" ) )
			off_current_team = f->offset( klass );
		if ( !off_current_team )
			off_current_team = field_offset_by_name( klass, "currentTeam" );
		if ( !off_current_team && player_team_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, player_team_klass ) )
				off_current_team = f->offset( klass );
		if ( !off_current_team )
			off_current_team = field_offset_by_type_contains( klass, "PlayerTeam" );
		if ( !off_current_team )
			off_current_team = getter_field_offset_by_return_type_name( klass, "PlayerTeam", "team" );
		if ( !off_current_team )
			off_current_team = getter_field_offset_by_return_type_name( klass, "PlayerTeam", "current" );

		if ( base_movement_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, base_movement_klass ) )
				off_base_movement = f->offset( klass );
		if ( !off_base_movement )
			off_base_movement = field_offset_by_type_contains( klass, "BaseMovement" );

		if ( player_model_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, player_model_klass ) )
				off_player_model = f->offset( klass );
		if ( !off_player_model )
			off_player_model = field_offset_by_type_contains( klass, "PlayerModel" );

		if ( auto f = il2cpp::get_field_by_name( klass, "playerFlags" ) )
			off_player_flags = f->offset( klass );
		if ( !off_player_flags )
			off_player_flags = field_offset_by_name( klass, "playerFlags" );

		if ( auto f = il2cpp::get_field_by_name( klass, "userID" ) )
			off_user_id = f->offset( klass );
		if ( !off_user_id )
			if ( auto f = il2cpp::get_field_by_name( klass, "playerSteamID" ) )
				off_user_id = f->offset( klass );
		if ( !off_user_id )
			off_user_id = field_offset_by_name( klass, "userID" );
		if ( !off_user_id )
			off_user_id = getter_field_offset_by_return_type_name( klass, "UInt64", "user" );
		if ( !off_user_id )
			off_user_id = getter_field_offset_by_return_type_name( klass, "UInt64", "steam" );
		if ( !off_user_id )
			off_user_id = method_field_offset_by_name_contains( klass, "userid" );
		if ( !off_user_id )
			off_user_id = method_field_offset_by_name_contains( klass, "steam" );

		off_user_id_string = field_offset_by_name( klass, "UserIDString" );
		if ( !off_user_id_string )
			off_user_id_string = field_offset_by_name( klass, "userIDString" );
		if ( !off_user_id_string )
			off_user_id_string = field_offset_by_name_contains_type( klass, "user", "string", "String" );
		if ( !off_user_id_string )
			off_user_id_string = getter_field_offset_by_return_type_name( klass, "String", "user" );

		if ( !off_current_team )
		{
			auto team_method_off = method_field_offset_by_name_contains( klass, "team" );
			auto team_field = field_by_offset_walk( klass, team_method_off );
			auto team_type = team_field ? team_field->type( ) : nullptr;
			auto team_type_name = team_type ? team_type->name( ) : nullptr;
			if ( team_method_off && team_method_off != off_user_id )
				off_current_team = team_method_off;
		}

		if ( auto f = il2cpp::get_field_by_type_name_attrs( klass, "System.String", FIELD_ATTRIBUTE_FAMILY, 0 ) )
			off_display_name = f->offset( klass );

		if ( !off_user_id && off_display_name )
			off_user_id = first_field_offset_after_type( klass, off_display_name, "UInt64" );
		if ( !off_user_id_string && off_cl_active_item && off_display_name )
		{
			std::uint64_t best_string = 0;
			for ( auto cur = klass; cur; cur = cur->parent( ) )
			{
				void* field_iter = nullptr;
				while ( auto f = cur->fields( &field_iter ) )
				{
					if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
						continue;
					auto off = f->offset( cur );
					if ( off <= off_cl_active_item || off >= off_display_name )
						continue;
					if ( field_type_matches( f, "String" ) && ( !best_string || off < best_string ) )
						best_string = off;
				}
			}
			off_user_id_string = best_string;
		}

		if ( player_input_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, player_input_klass ) )
				off_player_input = f->offset( klass );
		if ( !off_player_input )
			off_player_input = field_offset_by_type_contains( klass, "PlayerInput" );
		if ( !off_player_input )
			if ( auto f = il2cpp::get_field_by_name( klass, "input" ) )
				off_player_input = f->offset( klass );
		if ( !off_player_input )
			off_player_input = field_offset_by_name_contains( klass, "input" );

		if ( !off_current_team && off_player_input && off_cl_active_item )
		{
			for ( auto cur = klass; cur && !off_current_team; cur = cur->parent( ) )
			{
				void* field_iter = nullptr;
				while ( auto f = cur->fields( &field_iter ) )
				{
					if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
						continue;
					auto off = f->offset( cur );
					if ( off <= off_player_input || off >= off_cl_active_item )
						continue;
					if ( field_type_matches( f, "UInt64" ) || field_type_matches( f, "ulong" ) )
					{
						off_current_team = off;
						break;
					}
				}
			}
		}

		if ( rigidbody_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, rigidbody_klass ) )
				off_player_rigidbody = f->offset( klass );
		if ( !off_player_rigidbody )
			off_player_rigidbody = field_offset_by_type_contains( klass, "Rigidbody" );

		if ( auto f = il2cpp::get_field_by_name( klass, "frozen" ) )
			off_frozen = f->offset( klass );
		if ( !off_frozen )
			off_frozen = first_consecutive_bool_pair( klass );

		if ( gesture_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, gesture_klass ) )
				off_current_gesture = f->offset( klass );
		if ( !off_current_gesture )
			if ( auto f = il2cpp::get_field_by_name( klass, "currentGesture" ) )
				off_current_gesture = f->offset( klass );

		{
			const char* model_state_names[] = { "modelState", "model_state", "_modelState" };
			off_model_state = field_offset_by_names( klass, model_state_names, 3 );
		}
		if ( !off_model_state && model_state_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, model_state_klass ) )
				off_model_state = f->offset( klass );
		if ( !off_model_state && model_state_klass )
			off_model_state = field_offset_by_type_class( klass, model_state_klass );
		if ( !off_model_state && model_state_klass )
			off_model_state = getter_field_offset_by_return_class( klass, model_state_klass );
		if ( !off_model_state )
			off_model_state = getter_field_offset_by_return_type_name( klass, "ModelState", "model" );
		if ( !off_model_state )
			off_model_state = method_field_offset_by_name_contains( klass, "model", "state" );
		if ( !off_model_state )
			off_model_state = field_offset_by_type_or_class_contains( klass, "ModelState" );
		if ( !off_model_state )
			off_model_state = first_wrapper_field_after_type( klass, 0, "ModelState" );
		if ( !off_model_state )
			off_model_state = field_offset_by_model_state_shape( klass );
		if ( !off_model_state )
			off_model_state = field_offset_by_name_contains( klass, "model", "state" );
		{
			std::uint64_t model_state_min = off_player_rigidbody;
			std::uint64_t model_state_max = 0;
			auto push_model_state_max = [&]( std::uint64_t value )
			{
				if ( value > model_state_min && ( !model_state_max || value < model_state_max ) )
					model_state_max = value;
			};
			push_model_state_max( off_player_model );
			push_model_state_max( off_player_input );
			push_model_state_max( off_cl_active_item );

			std::uint64_t anchored_model_state = 0;
			if ( model_state_min && model_state_max )
			{
				if ( model_state_klass )
					anchored_model_state = field_offset_by_type_class_between( klass, model_state_klass, model_state_min, model_state_max );
				if ( !anchored_model_state )
					anchored_model_state = field_offset_by_model_state_shape_between( klass, model_state_min, model_state_max );
			}

			if ( anchored_model_state &&
				( !off_model_state || off_model_state <= model_state_min || off_model_state >= model_state_max ) )
				off_model_state = anchored_model_state;
		}

		{
			const char* mounted_names[] = { "mounted", "mountedEntity", "mountedTo", "mountable" };
			off_mounted = field_offset_by_names( klass, mounted_names, 4 );
		}
		if ( base_mountable_klass )
		{
			auto mounted_min = off_cl_active_item ? off_cl_active_item : off_player_input;
			auto mounted_max = off_user_id ? off_user_id : off_player_flags;
			auto anchored_mounted = field_offset_by_type_class_between( klass, base_mountable_klass, mounted_min, mounted_max );
			if ( anchored_mounted )
				off_mounted = anchored_mounted;
		}
		if ( base_mountable_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, base_mountable_klass ) )
				if ( !off_mounted || off_mounted <= off_cl_active_item )
					off_mounted = f->offset( klass );
		if ( !off_mounted && base_mountable_klass )
			off_mounted = getter_field_offset_by_return_class( klass, base_mountable_klass );
		if ( !off_mounted )
			off_mounted = field_offset_by_name_contains( klass, "mount" );
		if ( !off_mounted )
			off_mounted = field_offset_by_type_or_class_contains( klass, "Mountable" );
		if ( !off_mounted )
			off_mounted = first_wrapper_field_after_type( klass, 0, "Mountable" );
		if ( !off_mounted )
			off_mounted = first_entity_ref_field_after( klass, off_player_input ? off_player_input : off_model_state );
		{
			auto mounted_method = method_field_offset_by_name_contains( klass, "mounted" );
			if ( !mounted_method )
				mounted_method = method_field_offset_by_name_contains( klass, "mount" );
			if ( mounted_method &&
				mounted_method > off_cl_active_item &&
				( !off_user_id || mounted_method < off_user_id ) )
				off_mounted = mounted_method;
		}

		off_weapon_move_speed_scale = field_offset_by_name( klass, "weaponMoveSpeedScale" );
		if ( !off_weapon_move_speed_scale )
			off_weapon_move_speed_scale = field_offset_by_name_contains( klass, "weapon", "speed" );
		off_clothing_blocks_aiming = field_offset_by_name( klass, "clothingBlocksAiming" );
		if ( !off_clothing_blocks_aiming )
			off_clothing_blocks_aiming = field_offset_by_name_contains( klass, "clothing", "aiming" );

		const char* belt_names[] = { "beltDirect", "Belt", "belt", "_belt" };
		off_belt_direct = field_offset_by_names( klass, belt_names, 4 );
		if ( !off_belt_direct && player_belt_klass )
			if ( auto f = il2cpp::get_field_from_field_type_class( klass, player_belt_klass ) )
				off_belt_direct = f->offset( klass );
		if ( !off_belt_direct && player_belt_klass )
			off_belt_direct = field_offset_by_type_class( klass, player_belt_klass );
		if ( !off_belt_direct )
			off_belt_direct = field_offset_by_type_or_class_contains( klass, "PlayerBelt" );
		if ( !off_belt_direct )
			off_belt_direct = getter_field_offset_by_return_type_name( klass, "PlayerBelt", "belt" );
		if ( !off_belt_direct )
			off_belt_direct = field_offset_by_name_contains_type( klass, "belt", nullptr, "ItemContainer" );
		if ( !off_belt_direct )
			off_belt_direct = getter_field_offset_by_return_type_name( klass, "ItemContainer", "belt" );
		if ( !off_belt_direct )
			off_belt_direct = method_field_offset_by_name_contains( klass, "belt" );
		if ( !off_belt_direct )
			off_belt_direct = first_wrapper_field_after_type( klass, off_player_inventory, "ItemContainer" );
		if ( !off_belt_direct )
			off_belt_direct = first_field_offset_after_type( klass, off_player_inventory, "ItemContainer" );
		if ( !off_belt_direct && off_clothing_blocks_aiming )
			off_belt_direct = first_wrapper_field_after( klass, off_clothing_blocks_aiming );

		const char* looking_at_names[] = { "_lookingAt", "lookingAt", "looking_at" };
		off_looking_at = field_offset_by_names( klass, looking_at_names, 3 );
		if ( !off_looking_at )
			off_looking_at = field_offset_by_name_contains_type( klass, "looking", "at", "BaseEntity" );
		if ( !off_looking_at )
			off_looking_at = field_offset_by_name_contains_type( klass, "look", "at", "BaseEntity" );
		if ( !off_looking_at )
			off_looking_at = method_field_offset_by_name_contains( klass, "looking" );
		if ( !off_looking_at )
			off_looking_at = method_field_offset_by_name_contains( klass, "look" );
		if ( !off_looking_at && off_model_state && off_player_model )
		{
			auto entity_ref = first_entity_ref_field_after( klass, off_model_state );
			if ( entity_ref && entity_ref < off_player_model )
				off_looking_at = entity_ref;
		}
	}

	if ( !off_model_state && g_base_player_live.model_state )
		off_model_state = g_base_player_live.model_state;
	if ( g_base_player_live.player_model )
		off_player_model = g_base_player_live.player_model;
	if ( !off_belt_direct )
		off_belt_direct = g_base_player_live.belt_direct;
	if ( !off_current_team && g_base_player_live.current_team )
		off_current_team = g_base_player_live.current_team;

	std::printf(
		"[morphine-dumper] base_player resolved: currentTeam=0x%llX userID=0x%llX modelState=0x%llX mounted=0x%llX belt=0x%llX\n",
		off_current_team,
		off_user_id,
		off_model_state,
		off_mounted,
		off_belt_direct );
	if ( dumper_verbose( ) && klass && ( !off_current_team || !off_user_id || !off_model_state || !off_belt_direct ) )
	{
		std::printf( "[morphine-dumper] base_player unresolved candidates:\n" );
		int field_printed = 0;
		for ( auto cur = klass; cur && field_printed < 24; cur = cur->parent( ) )
		{
			void* iter = nullptr;
			while ( auto f = cur->fields( &iter ) )
			{
				if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
					continue;
				auto t = f->type( );
				auto tn = t ? t->name( ) : "";
				if ( !ascii_contains_i( tn, "UInt64" )
					&& !ascii_contains_i( tn, "ModelState" )
					&& !ascii_contains_i( tn, "ItemContainer" )
					&& !ascii_contains_i( tn, "Team" )
					&& !ascii_contains_i( f->name( ), "team" )
					&& !ascii_contains_i( tn, "PlayerTeam" ) )
					continue;
				std::printf(
					"[morphine-dumper]   field off=0x%llX name=%s type=%s\n",
					f->offset( cur ),
					f->name( ) ? f->name( ) : "",
					tn ? tn : "" );
				if ( ++field_printed >= 24 )
					break;
			}
		}

		int method_printed = 0;
		void* iter = nullptr;
		while ( auto method = klass->methods( &iter ) )
		{
			if ( method_printed >= 24 || !method || method->param_count( ) != 0 )
				continue;
			auto ret = method->return_type( );
			auto ret_name = ret ? ret->name( ) : "";
			auto method_name = method->name( );
			if ( !ascii_contains_i( ret_name, "UInt64" )
				&& !ascii_contains_i( ret_name, "ModelState" )
				&& !ascii_contains_i( ret_name, "ItemContainer" )
				&& !ascii_contains_i( method_name, "user" )
				&& !ascii_contains_i( method_name, "steam" )
				&& !ascii_contains_i( ret_name, "Team" )
				&& !ascii_contains_i( method_name, "team" )
				&& !ascii_contains_i( method_name, "model" )
				&& !ascii_contains_i( method_name, "belt" ) )
				continue;
			std::printf(
				"[morphine-dumper]   method name=%s ret=%s field=0x%llX\n",
				method_name ? method_name : "",
				ret_name ? ret_name : "",
				single_field_offset_referenced_by_method( method ) );
			method_printed++;
		}

		if ( !off_model_state || !off_belt_direct )
		{
			std::printf( "[morphine-dumper] base_player field window 0x480-0x780:\n" );
			int window_printed = 0;
			for ( auto cur = klass; cur && window_printed < 96; cur = cur->parent( ) )
			{
				void* field_iter = nullptr;
				while ( auto f = cur->fields( &field_iter ) )
				{
					if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
						continue;
					auto off = f->offset( cur );
					if ( off < 0x480 || off > 0x780 )
						continue;
					auto t = f->type( );
					std::printf(
						"[morphine-dumper]   field off=0x%llX name=%s type=%s\n",
						off,
						f->name( ) ? f->name( ) : "",
						t && t->name( ) ? t->name( ) : "" );
					if ( ++window_printed >= 96 )
						break;
				}
			}

			std::printf( "[morphine-dumper] base_player method field refs 0x480-0x780:\n" );
			int ref_printed = 0;
			void* method_iter = nullptr;
			while ( auto method = klass->methods( &method_iter ) )
			{
				if ( !method || method->param_count( ) != 0 )
					continue;
				auto off = single_field_offset_referenced_by_method( method );
				if ( off < 0x480 || off > 0x780 )
					continue;
				auto ret = method->return_type( );
				std::printf(
					"[morphine-dumper]   method field=0x%llX name=%s ret=%s\n",
					off,
					method->name( ) ? method->name( ) : "",
					ret && ret->name( ) ? ret->name( ) : "" );
				if ( ++ref_printed >= 64 )
					break;
			}
		}
	}

	w.struct_begin( "base_player" );
	w.struct_field( "cl_active_item",    off_cl_active_item );
	w.struct_field( "player_eyes",       off_player_eyes );
	w.struct_field( "player_inventory",  off_player_inventory );
	w.struct_field( "current_team",      off_current_team );
	w.struct_field( "base_movement",     off_base_movement );
	w.struct_field( "player_model",      off_player_model );
	w.struct_field( "player_flags",      off_player_flags );
	w.struct_field( "userID",            off_user_id );
	w.struct_field( "userIDString",      off_user_id_string );
	w.struct_field( "display_name",      off_display_name );
	w.struct_field( "player_input",      off_player_input );
	w.struct_field( "modelState",        off_model_state );
	w.struct_field( "mounted",           off_mounted );
	w.struct_field( "Belt",              off_belt_direct );
	w.struct_field( "_lookingAt",        off_looking_at );
	w.struct_field( "weaponMoveSpeedScale", off_weapon_move_speed_scale );
	w.struct_field( "clothingBlocksAiming", off_clothing_blocks_aiming );
	w.struct_field( "player_rigidbody",  off_player_rigidbody );
	w.struct_field( "frozen",            off_frozen );
	w.struct_field( "current_gesture",   off_current_gesture );
	w.struct_end( );
}

void c_core::write_base_combat_entity( c_writer& w )
{
	auto bce = c_runtime::find_class( "BaseCombatEntity" );
	auto be  = c_runtime::find_class( "BaseEntity" );

	std::uint64_t off_lifestate   = 0;
	std::uint64_t off_model       = 0;
	std::uint64_t off_health      = 0;
	std::uint64_t off_max_health  = 0;
	std::uint64_t off_skeleton    = 0;
	std::uint64_t off_protection  = 0;
	std::uint64_t off_hostile     = 0;

	if ( bce )
	{
		if ( auto f = il2cpp::get_field_by_name( bce, "skeletonProperties" ) )
			off_skeleton = f->offset( bce );
		if ( auto f = il2cpp::get_field_by_name( bce, "baseProtection" ) )
			off_protection = f->offset( bce );
		if ( auto f = il2cpp::get_field_by_name( bce, "lifestate" ) )
			off_lifestate = f->offset( bce );

		if ( auto mark = il2cpp::get_field_by_name( bce, "markAttackerHostile" ) )
		{
			off_hostile = mark->offset( bce );
			if ( auto h = il2cpp::get_field_by_offset( bce, mark->offset( bce ) + 6 ) )
				off_health = h->offset( bce );
			if ( auto mh = il2cpp::get_field_by_offset( bce, mark->offset( bce ) + 10 ) )
				off_max_health = mh->offset( bce );
		}
	}

	if ( be )
	{
		if ( auto f = il2cpp::get_field_by_name( be, "model" ) )
			off_model = f->offset( be );
	}

	w.struct_begin( "base_combat_entity" );
	w.struct_field( "skeletonProperties", off_skeleton );
	w.struct_field( "baseProtection",     off_protection );
	w.struct_field( "lifestate",  off_lifestate );
	w.struct_field( "markAttackerHostile", off_hostile );
	w.struct_field( "model",      off_model );
	w.struct_field( "_health",    off_health );
	w.struct_field( "_maxHealth", off_max_health );
	w.struct_end( );
}

void c_core::write_base_entity( c_writer& w )
{
	auto klass = c_runtime::find_class( "BaseEntity" );
	auto flags = field_offset_by_name( klass, "flags" );
	auto position_lerp = field_offset_by_name( klass, "positionLerp" );
	auto position_lerp_klass = find_named_class( "PositionLerp" );
	if ( !position_lerp_klass )
		position_lerp_klass = class_name_contains( "PositionLerp" );
	if ( !position_lerp_klass )
		position_lerp_klass = class_with_field_name_contains_type( "interpolator", nullptr, false );
	if ( !position_lerp && position_lerp_klass )
		position_lerp = field_offset_by_type_class( klass, position_lerp_klass );
	if ( !position_lerp )
		position_lerp = field_offset_by_name_contains( klass, "position", "lerp" );
	if ( !position_lerp && flags )
		position_lerp = first_wrapper_field_after( klass, flags );
	if ( !position_lerp )
		position_lerp = field_offset_by_name_contains_type( klass, "position", nullptr, "Vector3" );

	w.struct_begin( "base_entity" );
	w.struct_field( "bounds",   field_offset_by_name( klass, "bounds" ) );
	w.struct_field( "model",    field_offset_by_name( klass, "model" ) );
	w.struct_field( "flags",    flags );
	w.struct_field( "triggers", field_offset_by_type_contains( klass, "TriggerBase" ) );
	w.struct_field( "positionLerp", position_lerp );
	w.struct_end( );
}

void c_core::write_item_container( c_writer& w )
{
	auto klass = m_item_container_class;
	std::uint64_t off_item_list = 0;
	std::uint64_t off_flags = 0;

	if ( klass )
	{
		char search[ 128 ]{ };
		std::snprintf( search, sizeof( search ),
			"System.Collections.Generic.List<%s>",
			m_item_class ? m_item_class->name( ) : "Item" );

		if ( auto f = il2cpp::get_field_if_type_contains( klass, search ) )
			off_item_list = f->offset( );
		char flag_search[ 128 ]{ };
		std::snprintf( flag_search, sizeof( flag_search ), "%s.Flag", klass->name( ) );
		if ( auto f = il2cpp::get_field_if_type_contains( klass, flag_search, FIELD_ATTRIBUTE_PUBLIC ) )
			off_flags = f->offset( );
		if ( !off_flags )
			off_flags = field_offset_by_name( klass, "flags" );
		if ( !off_flags )
			off_flags = field_offset_by_name_contains( klass, "flag" );
		if ( !off_flags )
			off_flags = field_offset_by_type_or_class_contains( klass, "Flag" );

		std::printf( "[morphine-dumper] item_container.item_list=0x%llX (search=%s, klass=%p)\n",
			off_item_list, search, klass );
	}
	else
	{
		std::printf( "[morphine-dumper] item_container_class is null — skipping\n" );
	}

	w.struct_begin( "item_container" );
	w.struct_field( "item_list", off_item_list );
	w.struct_field( "flags", off_flags );
	w.struct_end( );
}

static void resolve_player_inventory_offsets(
	std::uint64_t& out_main,
	std::uint64_t& out_belt,
	std::uint64_t& out_wear,
	il2cpp::il2cpp_class_t* item_class,
	il2cpp::il2cpp_class_t* item_container_kl,
	std::uint64_t base_networkable_rva,
	std::uint32_t bn_static_fields,
	std::uint32_t wrapper_class_ptr,
	std::uint32_t parent_static_fields,
	std::uint32_t entities,
	std::uint64_t hv_offset,
	const c_decryption::constants_t& decrypt_0,
	const c_decryption::constants_t& decrypt_1,
	const c_decryption::constants_t& player_inventory_decrypt )
{
	auto klass             = c_runtime::find_class( "PlayerInventory" );
	auto player_loot_kl    = c_runtime::find_class( "PlayerLoot" );

	std::uint64_t off_main = 0;
	std::uint64_t off_belt = 0;
	std::uint64_t off_wear = 0;

	const char* main_names[] = { "containerMain", "container_main", "main" };
	const char* belt_names[] = { "containerBelt", "container_belt", "belt" };
	const char* wear_names[] = { "containerWear", "container_wear", "wear" };

	off_main = field_offset_by_names( klass, main_names, 3 );
	off_belt = field_offset_by_names( klass, belt_names, 3 );
	off_wear = field_offset_by_names( klass, wear_names, 3 );

	if ( klass && item_container_kl && !( off_main && off_belt && off_wear ) )
	{
		auto fields = fields_by_type_class( klass, item_container_kl );
		if ( fields.size( ) >= 1 && !off_main ) off_main = fields[ 0 ]->offset( klass );
		if ( fields.size( ) >= 2 && !off_belt ) off_belt = fields[ 1 ]->offset( klass );
		if ( fields.size( ) >= 3 && !off_wear ) off_wear = fields[ 2 ]->offset( klass );
	}
	if ( klass && item_container_kl && !( off_main && off_belt && off_wear ) )
	{
		auto item_container_name = item_container_kl->name( );
		if ( item_container_name )
		{
			if ( !off_main ) off_main = field_offset_by_type_contains_index( klass, item_container_name, 0 );
			if ( !off_belt ) off_belt = field_offset_by_type_contains_index( klass, item_container_name, 1 );
			if ( !off_wear ) off_wear = field_offset_by_type_contains_index( klass, item_container_name, 2 );
		}
	}

	auto try_runtime = [&]( ) -> bool
	{
		if ( !klass || !item_container_kl || !player_loot_kl )
		{
			std::printf( "[morphine-dumper] player_inventory: missing class (inv=%p ic=%p loot=%p)\n",
				klass, item_container_kl, player_loot_kl );
			return false;
		}

		char flag_search[ 128 ]{ };
		std::snprintf( flag_search, sizeof( flag_search ),
			"%s.Flag", item_container_kl->name( ) );
		auto flag_field = il2cpp::get_field_if_type_contains(
			item_container_kl, flag_search, FIELD_ATTRIBUTE_PUBLIC );
		if ( !flag_field )
		{
			std::printf( "[morphine-dumper] player_inventory: no Flag field (search=%s)\n", flag_search );
			return false;
		}

		il2cpp::method_info_t* client_init = nullptr;
		void* iter = nullptr;
		auto base_player = c_runtime::find_class( "BasePlayer" );
		while ( auto m = klass->methods( &iter ) )
		{
			if ( !m )
				continue;
			auto n = m->name( );
			if ( !n || std::strcmp( n, "ClientInit" ) != 0 )
				continue;
			if ( m->param_count( ) != 1 )
				continue;
			auto t = m->get_param( 0 );
			if ( !t )
				continue;
			if ( base_player && t->klass( ) == base_player )
			{
				client_init = m;
				break;
			}
		}
		if ( !client_init )
		{
			std::printf( "[morphine-dumper] player_inventory: no ClientInit(BasePlayer) method\n" );
			return false;
		}

		auto invoke = ( void( * )( std::uint64_t, void* ) )client_init->get_fn_ptr<void*>( );
		if ( !invoke )
			return false;

		auto go = unity::game_object_t::create( L"" );
		if ( !go )
		{
			std::printf( "[morphine-dumper] player_inventory: GameObject::create failed\n" );
			return false;
		}

		go->add_component( klass->type( ) );
		go->add_component( player_loot_kl->type( ) );

		auto inv_instance = go->get_component( klass->type( ) );
		if ( !inv_instance )
		{
			std::printf( "[morphine-dumper] player_inventory: get_component(PlayerInventory) failed\n" );
			return false;
		}

		invoke( inv_instance, nullptr );

		auto containers = il2cpp::get_fields_of_type(
			klass, item_container_kl->type( ),
			FIELD_ATTRIBUTE_PUBLIC, 0 );

		std::printf( "[morphine-dumper] player_inventory: %zu ItemContainer fields\n", containers.size( ) );

		std::uint64_t runtime_main = 0;
		std::uint64_t runtime_belt = 0;
		std::uint64_t runtime_wear = 0;

		for ( auto f : containers )
		{
			auto container_ptr = *( std::uint64_t* )( inv_instance + f->offset( ) );
			if ( !container_ptr )
				continue;
			auto container_flags = *( int* )( container_ptr + flag_field->offset( ) );

			std::printf( "[morphine-dumper]   field@0x%llX flags=0x%X\n",
				( std::uint64_t )f->offset( ), container_flags );

			if ( container_flags & rust::item_container::belt )
				runtime_belt = f->offset( );
			else if ( container_flags & rust::item_container::clothing )
				runtime_wear = f->offset( );
			else
				runtime_main = f->offset( );
		}
		if ( runtime_main && runtime_belt && runtime_wear )
		{
			off_main = runtime_main;
			off_belt = runtime_belt;
			off_wear = runtime_wear;
			return true;
		}
		return false;
	};

	auto try_live_runtime = [&] ( ) -> bool
	{
		if ( !klass || !item_container_kl || !base_networkable_rva || !decrypt_0.valid || !decrypt_1.valid )
			return false;

		auto base_player = c_runtime::find_class( "BasePlayer" );
		auto player_inventory_off = field_offset_by_type_class( base_player, klass );
		if ( !player_inventory_off )
			player_inventory_off = field_offset_by_type_contains( base_player, "PlayerInventory" );
		if ( !player_inventory_off )
		{
			std::printf( "[morphine-dumper] player_inventory live: no BasePlayer PlayerInventory field\n" );
			return false;
		}

		std::uint64_t item_list_off = 0;
		std::uint64_t item_flags_off = 0;
		if ( item_container_kl )
		{
			char search[ 128 ]{ };
			std::snprintf( search, sizeof( search ),
				"System.Collections.Generic.List<%s>",
				item_class ? item_class->name( ) : "Item" );
			if ( auto f = il2cpp::get_field_if_type_contains( item_container_kl, search ) )
				item_list_off = f->offset( );
			char flag_search[ 128 ]{ };
			std::snprintf( flag_search, sizeof( flag_search ), "%s.Flag", item_container_kl->name( ) );
			if ( auto f = il2cpp::get_field_if_type_contains( item_container_kl, flag_search, FIELD_ATTRIBUTE_PUBLIC ) )
				item_flags_off = f->offset( );
			if ( !item_flags_off )
				item_flags_off = field_offset_by_name( item_container_kl, "flags" );
			if ( !item_flags_off )
				item_flags_off = field_offset_by_name_contains( item_container_kl, "flag" );
			if ( !item_flags_off )
				item_flags_off = field_offset_by_type_or_class_contains( item_container_kl, "Flag" );
		}
		if ( !item_list_off || !item_flags_off )
			return false;

		struct field_hit_t
		{
			std::uint64_t off;
			int hits;
		};
		std::vector<field_hit_t> main_hits;
		std::vector<field_hit_t> belt_hits;
		std::vector<field_hit_t> wear_hits;
		auto add_field_hit = []( std::vector<field_hit_t>& hits, std::uint64_t off )
		{
			for ( auto& h : hits )
			{
				if ( h.off == off )
				{
					h.hits++;
					return;
				}
			}
			hits.push_back( { off, 1 } );
		};
		auto choose_field_hit = []( const std::vector<field_hit_t>& hits ) -> field_hit_t
		{
			field_hit_t best{ };
			for ( const auto& h : hits )
				if ( h.hits > best.hits )
					best = h;
			return best;
		};
		auto plausible_list = []( std::uintptr_t list ) -> bool
		{
			if ( !is_valid_userland_ptr( list + 0x18 ) )
				return false;
			auto array = *( std::uintptr_t* )( list + 0x10 );
			auto size = *( std::int32_t* )( list + 0x18 );
			return size >= 0 && size <= 1000 && ( size == 0 || is_valid_userland_ptr( array ) );
		};
		auto item_container_flags = [&]( std::uintptr_t container, std::uint32_t& flags ) -> bool
		{
			if ( !is_valid_userland_ptr( container + item_list_off ) )
				return false;
			if ( item_container_kl && *( std::uintptr_t* )container != ( std::uintptr_t )item_container_kl )
				return false;
			auto list = *( std::uintptr_t* )( container + item_list_off );
			if ( !plausible_list( list ) )
				return false;
			flags = *( std::uint32_t* )( container + item_flags_off );
			return ( flags & 0xFFFF0000u ) == 0;
		};

		auto gchandle_get_target = ( gchandle_get_target_fn_t )GetProcAddress(
			GetModuleHandleA( "GameAssembly.dll" ), "il2cpp_gchandle_get_target" );
		if ( !gchandle_get_target )
			return false;
		auto decrypt_handle = [&]( std::uintptr_t wrapper_ptr, std::uint64_t read_off, const c_decryption::constants_t& c ) -> std::uintptr_t
		{
			if ( !is_valid_userland_ptr( wrapper_ptr + read_off ) )
				return 0;
			auto encrypted = *( std::uint64_t* )( wrapper_ptr + read_off );
			auto decrypted = c_decryption::simulate( encrypted, c );
			if ( ( decrypted & 1 ) == 0 && is_valid_userland_ptr( decrypted ) )
			{
				auto target = *( std::uintptr_t* )decrypted;
				if ( is_valid_userland_ptr( target ) )
					return target;
			}
			auto handle = ( std::uint32_t )decrypted;
			if ( !handle || handle > 0x1000000 )
				return 0;
			return safe_gchandle_get_target( gchandle_get_target, handle );
		};

		auto game_base = c_runtime::game_base( );
		auto bn_klass = *( std::uintptr_t* )( game_base + base_networkable_rva );
		if ( !is_valid_userland_ptr( bn_klass ) )
			return false;
		auto static_fields = *( std::uintptr_t* )( bn_klass + bn_static_fields );
		if ( !is_valid_userland_ptr( static_fields ) )
			return false;
		auto wrapper = *( std::uintptr_t* )( static_fields + wrapper_class_ptr );
		auto realm = decrypt_handle( wrapper, hv_offset, decrypt_0 );
		if ( !is_valid_userland_ptr( realm ) )
			return false;
		auto parent = *( std::uintptr_t* )( realm + parent_static_fields );
		auto list_dict = decrypt_handle( parent, hv_offset, decrypt_1 );
		if ( !is_valid_userland_ptr( list_dict ) )
			return false;
		auto buffer_list = *( std::uintptr_t* )( list_dict + entities );
		if ( !is_valid_userland_ptr( buffer_list ) )
			return false;
		auto array = *( std::uintptr_t* )( buffer_list + 0x10 );
		auto count = *( std::int32_t* )( buffer_list + 0x18 );
		if ( !is_valid_userland_ptr( array ) || count <= 0 || count > 50000 )
			return false;

		auto containers = fields_by_type_class( klass, item_container_kl );
		if ( containers.empty( ) )
			return false;

		int inventories = 0;
		int direct_inventories = 0;
		int decrypted_inventories = 0;
		auto limit = count > 2048 ? 2048 : count;
		for ( std::int32_t i = 0; i < limit; i++ )
		{
			auto entry = *( std::uintptr_t* )( array + 0x20 + ( i * 8 ) );
			auto entity = resolve_entity_entry_object( entry, base_player );
			if ( !is_valid_userland_ptr( entity + player_inventory_off ) )
				continue;
			auto inventory = *( std::uintptr_t* )( entity + player_inventory_off );
			bool direct_inventory = object_is_class( inventory, klass );
			if ( !direct_inventory && player_inventory_decrypt.valid )
			{
				inventory = decrypt_handle( entity + player_inventory_off, hv_offset, player_inventory_decrypt );
				if ( !object_is_class( inventory, klass ) )
				{
					auto wrapper = *( std::uintptr_t* )( entity + player_inventory_off );
					if ( is_valid_userland_ptr( wrapper ) )
						inventory = decrypt_handle( wrapper, hv_offset, player_inventory_decrypt );
				}
			}
			if ( !object_is_class( inventory, klass ) )
				continue;
			inventories++;
			if ( direct_inventory )
				direct_inventories++;
			else
				decrypted_inventories++;

			for ( auto f : containers )
			{
				if ( !f || !is_valid_userland_ptr( inventory + f->offset( klass ) ) )
					continue;
				auto container = *( std::uintptr_t* )( inventory + f->offset( klass ) );
				std::uint32_t flags = 0;
				if ( !item_container_flags( container, flags ) )
					continue;
				if ( flags & rust::item_container::belt )
					add_field_hit( belt_hits, f->offset( klass ) );
				else if ( flags & rust::item_container::clothing )
					add_field_hit( wear_hits, f->offset( klass ) );
				else
					add_field_hit( main_hits, f->offset( klass ) );
			}
		}

		auto main = choose_field_hit( main_hits );
		auto belt = choose_field_hit( belt_hits );
		auto wear = choose_field_hit( wear_hits );
		std::printf( "[morphine-dumper] player_inventory live: inventories=%d direct=%d decrypted=%d main=0x%llX hits=%d belt=0x%llX hits=%d wear=0x%llX hits=%d\n",
			inventories,
			direct_inventories,
			decrypted_inventories,
			main.off, main.hits,
			belt.off, belt.hits,
			wear.off, wear.hits );
		if ( main.hits >= 1 && belt.hits >= 1 && wear.hits >= 1 )
		{
			off_main = main.off;
			off_belt = belt.off;
			off_wear = wear.off;
			return true;
		}
		return false;
	};

	bool ok = off_main && off_belt && off_wear;
	if ( !ok )
		ok = try_runtime( );
	if ( !ok )
		ok = try_live_runtime( );

	if ( !ok && klass && item_container_kl )
	{
		std::uint64_t offs[ 8 ]{ };
		int n = 0;
		void* iter = nullptr;
		while ( auto f = klass->fields( &iter ) )
		{
			if ( !f )
				continue;
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			if ( !t )
				continue;
			auto tn = t->name( );
			if ( !tn )
				continue;
			if ( !std::strstr( tn, "ItemContainer" ) )
				continue;
			if ( n < 8 )
				offs[ n++ ] = f->offset( );
		}
		if ( n >= 1 ) off_main = offs[ 0 ];
		if ( n >= 2 ) off_belt = offs[ 1 ];
		if ( n >= 3 ) off_wear = offs[ 2 ];
	}

	out_main = off_main;
	out_belt = off_belt;
	out_wear = off_wear;
}

static std::uint64_t field_offset_by_name( il2cpp::il2cpp_class_t* klass, const char* name )
{
	for ( auto cur = klass; cur && name; cur = cur->parent( ) )
	{
		if ( auto f = il2cpp::get_field_by_name( cur, name ) )
			return f->offset( cur );
		if ( auto off = metadata_field_offset_by_name( cur->name( ), name ) )
			return off;
	}
	return 0;
}

static bool ascii_contains_i( const char* haystack, const char* needle )
{
	if ( !haystack || !needle || !*needle )
		return false;

	for ( const char* h = haystack; *h; h++ )
	{
		const char* a = h;
		const char* b = needle;
		while ( *a && *b )
		{
			auto ca = *a;
			auto cb = *b;
			if ( ca >= 'A' && ca <= 'Z' ) ca = ca - 'A' + 'a';
			if ( cb >= 'A' && cb <= 'Z' ) cb = cb - 'A' + 'a';
			if ( ca != cb )
				break;
			++a;
			++b;
		}
		if ( !*b )
			return true;
	}

	return false;
}

static std::uint64_t field_offset_by_name_contains( il2cpp::il2cpp_class_t* klass, const char* a, const char* b )
{
	for ( auto cur = klass; cur && a; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto name = f->name( );
			if ( ascii_contains_i( name, a ) && ( !b || ascii_contains_i( name, b ) ) )
				return f->offset( cur );
		}
		if ( auto off = metadata_field_offset_by_name_contains( cur->name( ), a, b ) )
			return off;
	}
	return 0;
}

static std::uint64_t field_offset_by_name_contains_type(
	il2cpp::il2cpp_class_t* klass,
	const char* a,
	const char* b,
	const char* type )
{
	for ( auto cur = klass; cur && a && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto name = f->name( );
			if ( ascii_contains_i( name, a ) &&
				( !b || ascii_contains_i( name, b ) ) &&
				field_type_matches( f, type ) )
				return f->offset( cur );
		}
		if ( auto off = metadata_field_offset_by_name_contains_type( cur->name( ), a, b, type ) )
			return off;
	}
	return 0;
}

static std::uint64_t field_offset_by_name_contains_type_after(
	il2cpp::il2cpp_class_t* klass,
	const char* a,
	const char* b,
	const char* type,
	std::uint64_t after )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && a; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto name = f->name( );
			auto off = f->offset( cur );
			if ( off <= after )
				continue;
			if ( ascii_contains_i( name, a ) &&
				( !b || ascii_contains_i( name, b ) ) &&
				( !type || field_type_matches( f, type ) ) &&
				( !best || off < best ) )
				best = off;
		}
		if ( !best )
			best = metadata_field_offset_by_name_contains_type_after( cur->name( ), a, b, type, after );
	}
	return best;
}

static std::uint64_t field_offset_by_type_contains_after( il2cpp::il2cpp_class_t* klass, const char* type, std::uint64_t after )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto off = f->offset( cur );
			if ( off <= after )
				continue;
			if ( field_type_or_generic_arg_matches( f, type ) && ( !best || off < best ) )
				best = off;
		}
	}
	return best;
}

static std::uint64_t first_field_offset_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t after, const char* type )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto off = f->offset( cur );
			if ( off > after && field_type_matches( f, type ) && ( !best || off < best ) )
				best = off;
		}
		if ( !best )
			best = metadata_first_field_after_type( cur->name( ), after, type, false );
	}
	return best;
}

static std::uint64_t nth_field_offset_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t after, const char* type, std::size_t index )
{
	std::uint64_t picked = 0;
	for ( std::size_t i = 0; i <= index; i++ )
	{
		picked = first_field_offset_after_type( klass, after, type );
		if ( !picked )
			return 0;
		after = picked;
	}
	return picked;
}

static std::uint64_t first_field_offset_before( il2cpp::il2cpp_class_t* klass, std::uint64_t before )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && before; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto off = f->offset( cur );
			if ( off && off < before && ( !best || off < best ) )
				best = off;
		}
	}
	return best;
}

static bool last_consecutive_field_pair_by_type( il2cpp::il2cpp_class_t* klass, const char* type, std::uint64_t after, std::uint64_t& first, std::uint64_t& second )
{
	first = 0;
	second = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		std::uint64_t prev = 0;
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto off = f->offset( cur );
			if ( off <= after || !field_type_matches( f, type ) )
				continue;
			if ( prev && off == prev + 4 )
			{
				first = prev;
				second = off;
			}
			prev = off;
		}
	}
	return first && second;
}

static std::uint64_t class_field_offset_by_name_contains( il2cpp::il2cpp_class_t* klass, const char* a, const char* b = nullptr )
{
	if ( !klass || !a )
		return 0;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
	{
		auto name = f->name( );
		if ( ascii_contains_i( name, a ) && ( !b || ascii_contains_i( name, b ) ) )
			return f->offset( );
	}
	if ( auto off = metadata_field_offset_by_name_contains( klass->name( ), a, b ) )
		return off;
	return 0;
}

static std::uint64_t class_field_offset_by_name_contains_type(
	il2cpp::il2cpp_class_t* klass,
	const char* a,
	const char* b,
	const char* type )
{
	if ( !klass || !a || !type )
		return 0;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
	{
		auto name = f->name( );
		if ( ascii_contains_i( name, a ) &&
			( !b || ascii_contains_i( name, b ) ) &&
			field_type_matches( f, type ) )
			return f->offset( );
	}
	if ( auto off = metadata_field_offset_by_name_contains_type( klass->name( ), a, b, type ) )
		return off;
	return 0;
}

static std::uint64_t class_field_offset_by_type_contains( il2cpp::il2cpp_class_t* klass, const char* type )
{
	if ( !klass || !type )
		return 0;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
		if ( field_type_matches( f, type ) )
			return f->offset( );
	if ( auto off = metadata_field_offset_by_type_contains( klass->name( ), type ) )
		return off;
	return 0;
}

static std::uint64_t field_offset_by_type_contains( il2cpp::il2cpp_class_t* klass, const char* type )
{
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		if ( auto f = il2cpp::get_field_if_type_contains( cur, type ) )
			return f->offset( cur );
		if ( auto off = metadata_field_offset_by_type_contains( cur->name( ), type ) )
			return off;
	}
	return 0;
}

static std::uint64_t field_offset_by_type_contains_index( il2cpp::il2cpp_class_t* klass, const char* type, std::size_t index )
{
	std::size_t seen = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			auto tn = t ? t->name( ) : nullptr;
			if ( !tn || !std::strstr( tn, type ) )
				continue;
			if ( seen++ == index )
				return f->offset( cur );
		}
		if ( auto off = metadata_field_offset_by_type_contains_index( cur->name( ), type, index, false ) )
			return off;
	}
	return 0;
}

static std::uint64_t field_offset_by_plain_type_contains_index( il2cpp::il2cpp_class_t* klass, const char* type, std::size_t index )
{
	std::size_t seen = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			auto tn = t ? t->name( ) : nullptr;
			if ( !tn || !std::strstr( tn, type ) || std::strstr( tn, "%" ) )
				continue;
			if ( seen++ == index )
				return f->offset( cur );
		}
		if ( auto off = metadata_field_offset_by_type_contains_index( cur->name( ), type, index, true ) )
			return off;
	}
	return 0;
}

static std::uint64_t field_offset_by_type_name_index( il2cpp::il2cpp_class_t* klass, const char* type, std::size_t index )
{
	std::size_t seen = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			auto tn = t ? t->name( ) : nullptr;
			if ( !tn || ( std::strcmp( tn, type ) && !std::strstr( tn, type ) && !std::strstr( type, tn ) ) )
				continue;
			if ( seen++ == index )
				return f->offset( cur );
		}
		if ( auto off = metadata_field_offset_by_type_contains_index( cur->name( ), type, index, false ) )
			return off;
	}
	return 0;
}

static std::uint64_t first_consecutive_bool_pair( il2cpp::il2cpp_class_t* klass )
{
	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		std::uint64_t prev = 0;
		bool have_prev = false;
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			auto tn = t ? t->name( ) : nullptr;
			if ( !tn || ( std::strcmp( tn, "System.Boolean" ) && !std::strstr( tn, "Boolean" ) && !std::strstr( tn, "bool" ) ) )
				continue;
			auto off = f->offset( cur );
			if ( have_prev && off == prev + 1 )
				return prev;
			prev = off;
			have_prev = true;
		}
	}
	return 0;
}

static std::uint64_t highest_field_offset_by_type_contains( il2cpp::il2cpp_class_t* klass, const char* type )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			auto tn = t ? t->name( ) : nullptr;
			if ( tn && std::strstr( tn, type ) && f->offset( cur ) > best )
				best = f->offset( cur );
		}
		if ( auto off = metadata_highest_field_offset_by_type_contains( cur->name( ), type ) )
			if ( off > best )
				best = off;
	}
	return best;
}

static std::uint64_t highest_field_offset_by_type_name( il2cpp::il2cpp_class_t* klass, const char* type )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			auto tn = t ? t->name( ) : nullptr;
			if ( tn && !std::strcmp( tn, type ) && f->offset( cur ) > best )
				best = f->offset( cur );
		}
	}
	return best;
}

static bool field_type_matches( il2cpp::field_info_t* f, const char* type )
{
	if ( !f || !type )
		return false;

	auto t = f->type( );
	auto tn = t ? t->name( ) : nullptr;
	if ( tn && std::strstr( tn, type ) )
		return true;

	auto tk = t ? t->klass( ) : nullptr;
	auto kn = tk ? tk->name( ) : nullptr;
	auto ns = tk ? tk->namespaze( ) : nullptr;
	if ( kn && std::strstr( kn, type ) )
		return true;
	if ( ns && std::strstr( ns, type ) )
		return true;

	return false;
}

static std::uint64_t field_offset_by_type_or_class_contains( il2cpp::il2cpp_class_t* klass, const char* type )
{
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			if ( field_type_matches( f, type ) )
				return f->offset( cur );
		}
		if ( auto off = metadata_field_offset_by_type_contains( cur->name( ), type ) )
			return off;
	}
	return 0;
}

static std::uint64_t highest_field_offset_by_type_or_class_contains( il2cpp::il2cpp_class_t* klass, const char* type )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto off = f->offset( cur );
			if ( field_type_matches( f, type ) && off > best )
				best = off;
		}
		if ( auto off = metadata_highest_field_offset_by_type_contains( cur->name( ), type ) )
			if ( off > best )
				best = off;
	}
	return best;
}

static il2cpp::field_info_t* field_by_offset_walk( il2cpp::il2cpp_class_t* klass, std::uint64_t offset )
{
	if ( !offset )
		return nullptr;

	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( !( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) && f->offset( cur ) == offset )
				return f;
		}
	}
	return nullptr;
}

static bool is_wrapper_candidate_type( il2cpp::il2cpp_class_t* type_class )
{
	if ( !type_class )
		return false;

	auto ns = type_class->namespaze( );
	if ( ns && ( !std::strcmp( ns, "System" ) || !std::strcmp( ns, "UnityEngine" ) ) )
		return false;

	auto name = type_class->name( );
	if ( name && ( std::strstr( name, "String" ) || std::strstr( name, "List" ) || std::strstr( name, "Dictionary" ) ) )
		return false;

	auto count = type_class->field_count( );
	return count > 0 && count <= 8;
}

static std::uint64_t highest_repeated_wrapper_field_offset( il2cpp::il2cpp_class_t* klass )
{
	struct wrapper_group_t
	{
		il2cpp::il2cpp_class_t* type_class;
		std::uint32_t count;
		std::uint64_t highest;
	};

	wrapper_group_t groups[ 64 ]{ };
	std::uint32_t group_count = 0;

	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;

			auto t = f->type( );
			auto tk = t ? t->klass( ) : nullptr;
			if ( !is_wrapper_candidate_type( tk ) )
				continue;

			auto off = f->offset( cur );
			wrapper_group_t* group = nullptr;
			for ( std::uint32_t i = 0; i < group_count; i++ )
			{
				if ( groups[ i ].type_class == tk )
				{
					group = &groups[ i ];
					break;
				}
			}
			if ( !group )
			{
				if ( group_count >= 64 )
					continue;
				group = &groups[ group_count++ ];
				group->type_class = tk;
			}
			++group->count;
			if ( off > group->highest )
				group->highest = off;
		}
	}

	std::uint64_t best = 0;
	for ( std::uint32_t i = 0; i < group_count; i++ )
		if ( groups[ i ].count >= 2 && groups[ i ].highest > best )
			best = groups[ i ].highest;
	return best;
}

static std::uint64_t first_wrapper_field_after( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;

			auto t = f->type( );
			auto tk = t ? t->klass( ) : nullptr;
			if ( !is_wrapper_candidate_type( tk ) )
				continue;

			auto off = f->offset( cur );
			if ( off > min_offset && ( !best || off < best ) )
				best = off;
		}
	}
	return best;
}

static bool is_entity_ref_type( il2cpp::il2cpp_class_t* klass )
{
	if ( !klass )
		return false;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
	{
		if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
			continue;
		if ( field_type_matches( f, "BaseEntity" ) )
			return true;
	}

	return false;
}

static std::uint64_t first_entity_ref_field_after( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset )
{
	if ( !klass )
		return 0;

	std::uint64_t best = 0;
	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
	{
		if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
			continue;

		auto type = f->type( );
		auto type_class = type ? type->klass( ) : nullptr;
		if ( !is_entity_ref_type( type_class ) )
			continue;

		auto off = f->offset( klass );
		if ( off > min_offset && ( !best || off < best ) )
			best = off;
	}

	return best;
}

static bool class_or_type_name_matches( il2cpp::il2cpp_class_t* klass, const char* type )
{
	if ( !klass || !type )
		return false;

	auto name = klass->name( );
	auto ns = klass->namespaze( );
	auto type_info = klass->type( );
	auto type_name = type_info ? type_info->name( ) : nullptr;

	return ( name && std::strstr( name, type ) ) ||
		( ns && std::strstr( ns, type ) ) ||
		( type_name && std::strstr( type_name, type ) );
}

static bool field_type_or_generic_arg_matches( il2cpp::field_info_t* f, const char* type )
{
	if ( field_type_matches( f, type ) )
		return true;

	auto t = f ? f->type( ) : nullptr;
	auto tk = t ? t->klass( ) : nullptr;
	auto arg = tk ? tk->get_generic_argument_at( 0 ) : nullptr;
	return class_or_type_name_matches( arg, type );
}

static std::uint64_t first_wrapper_field_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset, const char* type )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;

			auto t = f->type( );
			auto tk = t ? t->klass( ) : nullptr;
			if ( !is_wrapper_candidate_type( tk ) || !field_type_or_generic_arg_matches( f, type ) )
				continue;

			auto off = f->offset( cur );
			if ( off > min_offset && ( !best || off < best ) )
				best = off;
		}
		if ( !best )
			best = metadata_first_field_after_type( cur->name( ), min_offset, type, true );
	}
	return best;
}

static std::uint64_t nth_wrapper_field_after_type( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset, const char* type, std::size_t index )
{
	std::uint64_t picked = 0;
	for ( std::size_t i = 0; i <= index; i++ )
	{
		picked = first_wrapper_field_after_type( klass, min_offset, type );
		if ( !picked )
			return 0;
		min_offset = picked;
	}
	return picked;
}

static std::uint64_t last_field_offset_before_type( il2cpp::il2cpp_class_t* klass, std::uint64_t before, const char* type )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur && type; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;

			auto off = f->offset( cur );
			if ( off < before && field_type_or_generic_arg_matches( f, type ) && off > best )
				best = off;
		}
		if ( !best )
			best = metadata_last_field_before_type( cur->name( ), before, type );
	}
	return best;
}

static std::uint64_t field_offset_by_names( il2cpp::il2cpp_class_t* klass, const char** names, int count )
{
	for ( int i = 0; i < count; i++ )
		if ( auto off = field_offset_by_name( klass, names[ i ] ) )
			return off;
	return 0;
}

static std::uint64_t static_u32_field_value_by_name( il2cpp::il2cpp_class_t* klass, const char* name )
{
	if ( !klass || !name )
		return 0;

	if ( auto f = il2cpp::get_field_by_name( klass, name ) )
		if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
			return f->static_get_value<std::uint32_t>( );

	void* iter = nullptr;
	while ( auto nested = klass->nested_types( &iter ) )
		if ( auto value = static_u32_field_value_by_name( nested, name ) )
			return value;

	return 0;
}

static std::uint64_t static_u32_field_value_by_name_contains( il2cpp::il2cpp_class_t* klass, const char* name )
{
	if ( !klass || !name )
		return 0;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
	{
		if ( !( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
			continue;
		auto field_name = f->name( );
		if ( field_name && ascii_contains_i( field_name, name ) )
			return f->static_get_value<std::uint32_t>( );
	}

	iter = nullptr;
	while ( auto nested = klass->nested_types( &iter ) )
		if ( auto value = static_u32_field_value_by_name_contains( nested, name ) )
			return value;

	return 0;
}

static std::uint64_t field_offset_after( il2cpp::il2cpp_class_t* klass, std::uint64_t offset )
{
	std::uint64_t best = 0;
	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;

			auto off = f->offset( cur );
			if ( off > offset && ( !best || off < best ) )
				best = off;
		}
		if ( !best )
			best = metadata_field_offset_after( cur->name( ), offset );
	}
	return best;
}

static std::vector<il2cpp::field_info_t*> fields_by_type_class( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* type_class )
{
	std::vector<il2cpp::field_info_t*> out;
	for ( auto cur = klass; cur && type_class; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
				continue;
			auto t = f->type( );
			if ( t && t->klass( ) == type_class )
				out.push_back( f );
		}
	}
	return out;
}

static bool class_has_model_state_value_shape( il2cpp::il2cpp_class_t* klass )
{
	if ( !klass )
		return false;

	int instance_fields = 0;
	int int_fields = 0;
	int float_fields = 0;
	int vec_fields = 0;
	int bool_fields = 0;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
	{
		if ( !f || ( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
			continue;

		instance_fields++;
		auto type = f->type( );
		auto type_name = type ? type->name( ) : nullptr;
		if ( ascii_contains_i( type_name, "UInt32" ) ||
			ascii_contains_i( type_name, "Int32" ) )
			int_fields++;
		else if ( ascii_contains_i( type_name, "Single" ) )
			float_fields++;
		else if ( ascii_contains_i( type_name, "Vector3" ) )
			vec_fields++;
		else if ( ascii_contains_i( type_name, "Boolean" ) )
			bool_fields++;
	}

	return instance_fields >= 4 && instance_fields <= 24 &&
		int_fields >= 1 && float_fields >= 1 && vec_fields >= 1 && bool_fields <= 4;
}

static std::uint64_t field_offset_by_model_state_shape( il2cpp::il2cpp_class_t* klass )
{
	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( !f || ( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
				continue;
			auto type = f->type( );
			auto field_class = type ? type->klass( ) : nullptr;
			if ( class_has_model_state_shape( field_class ) ||
				class_has_model_state_value_shape( field_class ) )
				return f->offset( cur );
		}
	}
	return 0;
}

static std::uint64_t field_offset_by_model_state_shape_between( il2cpp::il2cpp_class_t* klass, std::uint64_t min_offset, std::uint64_t max_offset )
{
	if ( !min_offset || !max_offset || min_offset >= max_offset )
		return 0;

	std::uint64_t best = 0;
	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( !f || ( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
				continue;
			auto off = f->offset( cur );
			if ( off <= min_offset || off >= max_offset )
				continue;
			auto type = f->type( );
			auto field_class = type ? type->klass( ) : nullptr;
			if ( class_has_model_state_shape( field_class ) ||
				class_has_model_state_value_shape( field_class ) )
			{
				if ( !best || off < best )
					best = off;
			}
		}
	}
	return best;
}

static std::uint64_t field_offset_by_type_class( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* type_class )
{
	auto fields = fields_by_type_class( klass, type_class );
	return fields.empty( ) ? 0 : fields.front( )->offset( klass );
}

static std::uint64_t field_offset_by_type_class_between( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* type_class, std::uint64_t min_offset, std::uint64_t max_offset )
{
	if ( !klass || !type_class || !min_offset || !max_offset || min_offset >= max_offset )
		return 0;

	std::uint64_t best = 0;
	for ( auto cur = klass; cur; cur = cur->parent( ) )
	{
		void* iter = nullptr;
		while ( auto f = cur->fields( &iter ) )
		{
			if ( !f || ( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
				continue;
			auto off = f->offset( cur );
			if ( off <= min_offset || off >= max_offset )
				continue;
			auto type = f->type( );
			auto field_class = type ? type->klass( ) : nullptr;
			if ( field_class == type_class )
			{
				if ( !best || off < best )
					best = off;
			}
		}
	}
	return best;
}

static il2cpp::il2cpp_class_t* field_type_class_by_name( il2cpp::il2cpp_class_t* klass, const char* name )
{
	for ( auto cur = klass; cur && name; cur = cur->parent( ) )
	{
		auto f = il2cpp::get_field_by_name( cur, name );
		auto t = f ? f->type( ) : nullptr;
		if ( t )
			return t->klass( );
	}
	return nullptr;
}

static bool field_type_name_has_terms( il2cpp::field_info_t* field, const char** terms, int term_count )
{
	auto t = field ? field->type( ) : nullptr;
	auto tn = t ? t->name( ) : nullptr;
	if ( !tn )
		return false;

	for ( int i = 0; i < term_count; i++ )
		if ( !terms[ i ] || !std::strstr( tn, terms[ i ] ) )
			return false;
	return true;
}

static il2cpp::il2cpp_class_t* find_class_by_field( bool ( *predicate )( il2cpp::field_info_t*, void* ), void* ctx )
{
	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain || !predicate )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			if ( !klass )
				continue;

			void* iter = nullptr;
			while ( auto field = klass->fields( &iter ) )
				if ( predicate( field, ctx ) )
					return klass;
		}
	}

	return nullptr;
}

struct field_terms_ctx_t
{
	const char* field_name;
	const char** terms;
	int term_count;
	bool require_static;
};

static bool public_field_terms_predicate( il2cpp::field_info_t* field, void* raw_ctx )
{
	auto ctx = ( field_terms_ctx_t* )raw_ctx;
	if ( !field || !ctx )
		return false;

	auto flags = field->flags( );
	if ( ( flags & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK ) != FIELD_ATTRIBUTE_PUBLIC )
		return false;
	if ( ctx->require_static && !( flags & FIELD_ATTRIBUTE_STATIC ) )
		return false;
	if ( ctx->field_name )
	{
		auto name = field->name( );
		if ( !name || std::strcmp( name, ctx->field_name ) )
			return false;
	}

	return field_type_name_has_terms( field, ctx->terms, ctx->term_count );
}

static il2cpp::il2cpp_class_t* class_with_public_static_field_type_terms( const char** terms, int term_count )
{
	field_terms_ctx_t ctx{ nullptr, terms, term_count, true };
	return find_class_by_field( public_field_terms_predicate, &ctx );
}

static il2cpp::il2cpp_class_t* class_with_public_field_name_type_terms( const char* field_name, const char** terms, int term_count, bool require_static )
{
	field_terms_ctx_t ctx{ field_name, terms, term_count, require_static };
	return find_class_by_field( public_field_terms_predicate, &ctx );
}

struct field_name_type_ctx_t
{
	const char* name;
	const char* type;
	bool require_static;
};

static bool field_name_type_predicate( il2cpp::field_info_t* field, void* raw_ctx )
{
	auto ctx = ( field_name_type_ctx_t* )raw_ctx;
	if ( !field || !ctx )
		return false;
	if ( ctx->require_static && !( field->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
		return false;
	auto name = field->name( );
	if ( ctx->name && !ascii_contains_i( name, ctx->name ) )
		return false;
	return !ctx->type || field_type_matches( field, ctx->type );
}

static il2cpp::il2cpp_class_t* class_with_field_name_contains_type( const char* name, const char* type, bool require_static )
{
	field_name_type_ctx_t ctx{ name, type, require_static };
	return find_class_by_field( field_name_type_predicate, &ctx );
}

static bool instance_field_type_class_predicate( il2cpp::field_info_t* field, void* raw_type_class )
{
	auto wanted = ( il2cpp::il2cpp_class_t* )raw_type_class;
	if ( !field || !wanted )
		return false;

	if ( field->flags( ) & FIELD_ATTRIBUTE_STATIC )
		return false;

	auto t = field->type( );
	return t && t->klass( ) == wanted;
}

static il2cpp::il2cpp_class_t* class_with_instance_field_type_class( il2cpp::il2cpp_class_t* type_class )
{
	return find_class_by_field( instance_field_type_class_predicate, type_class );
}

static il2cpp::il2cpp_class_t* class_with_method_names( const char* a, const char* b )
{
	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain || !a )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			if ( !klass )
				continue;

			bool has_a = false;
			bool has_b = !b;
			void* iter = nullptr;
			while ( auto method = klass->methods( &iter ) )
			{
				auto name = method ? method->name( ) : nullptr;
				if ( !name )
					continue;
				if ( ascii_contains_i( name, a ) )
					has_a = true;
				if ( b && ascii_contains_i( name, b ) )
					has_b = true;
				if ( has_a && has_b )
					return klass;
			}
		}
	}

	return nullptr;
}

static std::uint64_t static_class_rva_any( il2cpp::il2cpp_class_t* klass )
{
	if ( !klass )
		return 0;

	if ( auto nested = c_runtime::get_inner_static_class( klass ) )
		if ( auto rva = c_runtime::static_class_rva( nested ) )
			return rva;
	if ( auto rva = c_runtime::static_class_rva( klass ) )
		return rva;
	return metadata_typeinfo_rva( klass );
}

static il2cpp::il2cpp_class_t* class_name_equals( const char* wanted )
{
	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain || !wanted )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			auto name = klass ? klass->name( ) : nullptr;
			if ( name && !std::strcmp( name, wanted ) )
				return klass;
		}
	}

	return nullptr;
}

static il2cpp::il2cpp_class_t* class_name_contains( const char* a, const char* b )
{
	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain || !a )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			auto name = klass ? klass->name( ) : nullptr;
			if ( !name || !std::strstr( name, a ) )
				continue;
			if ( b && !std::strstr( name, b ) )
				continue;
			return klass;
		}
	}

	return nullptr;
}

static il2cpp::il2cpp_class_t* class_name_contains_i( const char* a, const char* b )
{
	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain || !a )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			auto name = klass ? klass->name( ) : nullptr;
			auto ns = klass ? klass->namespaze( ) : nullptr;
			if ( !( ascii_contains_i( name, a ) || ascii_contains_i( ns, a ) ) )
				continue;
			if ( b && !( ascii_contains_i( name, b ) || ascii_contains_i( ns, b ) ) )
				continue;
			return klass;
		}
	}

	return nullptr;
}

static il2cpp::il2cpp_class_t* class_type_name_contains( const char* a, const char* b )
{
	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain || !a )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			auto type = klass ? klass->type( ) : nullptr;
			auto name = type ? type->name( ) : nullptr;
			if ( ascii_contains_i( name, a ) && ( !b || ascii_contains_i( name, b ) ) )
				return klass;
		}
	}

	return nullptr;
}

static il2cpp::il2cpp_class_t* find_named_class( const char* name )
{
	if ( auto klass = c_runtime::find_class( name ) )
		return klass;
	if ( auto klass = class_name_equals( name ) )
		return klass;
	if ( auto klass = class_name_contains( name ) )
		return klass;
	return class_type_name_contains( name );
}

static bool class_has_model_state_shape( il2cpp::il2cpp_class_t* klass )
{
	if ( !klass )
		return false;

	bool has_on_ground = false;
	bool has_sleeping = false;
	bool has_flying = false;
	bool has_mounted = false;
	int vec_fields = 0;
	int float_fields = 0;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
	{
		if ( !f )
			continue;
		auto type = f->type( );
		auto type_name = type ? type->name( ) : nullptr;
		if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
		{
			if ( !ascii_contains_i( type_name, "UInt32" ) &&
				!ascii_contains_i( type_name, "Int32" ) )
				continue;
			auto value = f->static_get_value<std::uint32_t>( );
			has_on_ground = has_on_ground || value == 0x4;
			has_sleeping = has_sleeping || value == 0x8;
			has_flying = has_flying || value == 0x40;
			has_mounted = has_mounted || value == 0x200;
			continue;
		}

		if ( ascii_contains_i( type_name, "Vector3" ) )
			vec_fields++;
		if ( ascii_contains_i( type_name, "Single" ) )
			float_fields++;
	}

	return has_on_ground && has_sleeping && has_flying && has_mounted &&
		vec_fields >= 1 && float_fields >= 1;
}

static il2cpp::il2cpp_class_t* find_model_state_class( )
{
	if ( auto klass = find_named_class( "ModelState" ) )
		return klass;

	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			if ( !klass )
				continue;

			auto flying = static_u32_field_value_by_name( klass, "Flying" );
			auto sleeping = static_u32_field_value_by_name( klass, "Sleeping" );
			auto mounted = static_u32_field_value_by_name( klass, "Mounted" );
			auto on_ground = static_u32_field_value_by_name( klass, "OnGround" );
			if ( flying == 0x40 && sleeping == 0x8 && mounted == 0x200 && on_ground == 0x4 )
				return klass;

			auto water_level = field_offset_by_name( klass, "waterLevel" );
			if ( !water_level )
				water_level = field_offset_by_name_contains( klass, "water", "level" );
			auto look_dir = field_offset_by_name( klass, "lookDir" );
			if ( !look_dir )
				look_dir = field_offset_by_name_contains( klass, "look", "dir" );
			if ( water_level && look_dir )
				return klass;

			if ( class_has_model_state_shape( klass ) )
				return klass;
		}
	}

	return nullptr;
}

static il2cpp::il2cpp_class_t* find_class_with_nested_field( const char* field_name, il2cpp::il2cpp_class_t** nested_out )
{
	if ( nested_out )
		*nested_out = nullptr;
	if ( !field_name )
		return nullptr;

	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			if ( !klass )
				continue;

			void* iter = nullptr;
			while ( auto nested = klass->nested_types( &iter ) )
			{
				if ( field_offset_by_name( nested, field_name ) )
				{
					if ( nested_out )
						*nested_out = nested;
					return klass;
				}
			}
		}
	}

	return nullptr;
}

static il2cpp::il2cpp_class_t* find_class_with_nested_self_instance_field(
	il2cpp::il2cpp_class_t** nested_out,
	std::uint64_t* instance_out )
{
	if ( nested_out )
		*nested_out = nullptr;
	if ( instance_out )
		*instance_out = 0;

	auto domain = il2cpp::il2cpp_domain_t::get( );
	if ( !domain )
		return nullptr;

	size_t assembly_count = 0;
	auto assemblies = domain->get_assemblies( &assembly_count );
	if ( !assemblies )
		return nullptr;

	for ( size_t i = 0; i < assembly_count; i++ )
	{
		auto image = assemblies[ i ] ? assemblies[ i ]->image( ) : nullptr;
		if ( !image )
			continue;

		auto class_count = image->class_count( );
		for ( size_t j = 0; j < class_count; j++ )
		{
			auto klass = image->get_class( j );
			if ( !klass || !static_class_rva_any( klass ) )
				continue;

			void* nested_iter = nullptr;
			while ( auto nested = klass->nested_types( &nested_iter ) )
			{
				if ( !static_class_rva_any( nested ) )
					continue;

				void* field_iter = nullptr;
				while ( auto field = nested->fields( &field_iter ) )
				{
					if ( field->flags( ) & FIELD_ATTRIBUTE_STATIC )
						continue;
					auto type = field->type( );
					if ( !type || type->klass( ) != klass )
						continue;

					if ( nested_out )
						*nested_out = nested;
					if ( instance_out )
						*instance_out = field->offset( nested );
					return klass;
				}
			}
		}
	}

	return nullptr;
}

static std::uint32_t static_field_count( il2cpp::il2cpp_class_t* klass )
{
	std::uint32_t count = 0;
	if ( !klass )
		return count;

	void* iter = nullptr;
	while ( auto f = klass->fields( &iter ) )
		if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
			++count;
	return count;
}

static il2cpp::il2cpp_class_t* nested_static_class_with_most_static_fields( il2cpp::il2cpp_class_t* klass, il2cpp::il2cpp_class_t* exclude )
{
	il2cpp::il2cpp_class_t* best = nullptr;
	std::uint32_t best_count = 0;
	if ( !klass )
		return nullptr;

	void* iter = nullptr;
	while ( auto nested = klass->nested_types( &iter ) )
	{
		if ( nested == exclude )
			continue;

		auto count = static_field_count( nested );
		if ( count > best_count )
		{
			best = nested;
			best_count = count;
		}
	}
	return best;
}

void c_core::write_itemdefinition( c_writer& w )
{
	auto klass = c_runtime::find_class( "ItemDefinition" );

	auto write_fields = [&]( const char* name )
	{
		w.struct_begin( name );
		w.struct_field( "itemid",      field_offset_by_name( klass, "itemid" ) );
		w.struct_field( "shortname",   field_offset_by_name( klass, "shortname" ) );
		w.struct_field( "displayName", field_offset_by_name( klass, "displayName" ) );
		w.struct_field( "category",    field_offset_by_name( klass, "category" ) );
		w.struct_field( "stackable",   field_offset_by_name( klass, "stackable" ) );
		w.struct_field( "iconSprite",  field_offset_by_name( klass, "iconSprite" ) );
		w.struct_field( "rarity",      field_offset_by_name( klass, "rarity" ) );
		w.struct_field( "condition",   field_offset_by_name( klass, "condition" ) );
		w.struct_end( );
	};

	write_fields( "ItemDefinition" );
}

static void write_item_fields(
	c_writer& w,
	const char* struct_name,
	il2cpp::il2cpp_class_t* item_class,
	il2cpp::il2cpp_class_t* item_id_class )
{
	auto klass = item_class ? item_class : c_runtime::find_class( "Item" );
	auto item_definition = c_runtime::find_class( "ItemDefinition" );
	if ( !klass && item_definition )
		klass = class_with_instance_field_type_class( item_definition );
	auto item_definition_offset = field_offset_by_type_class( klass, item_definition );
	if ( !item_definition_offset )
		item_definition_offset = field_offset_by_type_or_class_contains( klass, "ItemDefinition" );
	auto uid = field_offset_by_name( klass, "uid" );
	if ( !uid )
		uid = field_offset_by_type_class( klass, item_id_class );
	auto amount = field_offset_by_name( klass, "amount" );
	if ( !amount )
		amount = highest_field_offset_by_type_name( klass, "System.Int32" );
	if ( !amount )
		amount = highest_field_offset_by_type_contains( klass, "Int32" );
	auto held_entity = item_held_entity_offset( klass, uid );

	std::printf( "[morphine-dumper] item fields: info=0x%llX uid=0x%llX amount=0x%llX held_entity=0x%llX\n",
		item_definition_offset, uid, amount, held_entity );

	w.struct_begin( struct_name );
	w.struct_field( "info",           item_definition_offset );
	w.struct_field( "itemdefinition", item_definition_offset );
	w.struct_field( "uid",            uid );
	w.struct_field( "amount",         amount );
	w.struct_field( "shortname",      field_offset_by_name( item_definition, "shortname" ) );
	w.struct_field( "category",       field_offset_by_name( item_definition, "category" ) );
	w.struct_field( "held_entity",    held_entity );
	w.struct_end( );
}

static void write_item(
	c_writer& w,
	il2cpp::il2cpp_class_t* item_class,
	il2cpp::il2cpp_class_t* item_id_class )
{
	write_item_fields( w, "item", item_class, item_id_class );
}

void c_core::write_base_projectile( c_writer& w )
{
	auto klass = c_runtime::find_class( "BaseProjectile" );
	if ( !klass )
		klass = class_name_contains( "BaseProjectile" );

	auto write_fields = [&]( const char* name )
	{
		const char* is_reloading_names[] = { "isReloading", "_isReloading", "reloadStarted" };
		const char* stance_penalty_names[] = { "stancePenalty", "_stancePenalty" };
		const char* aimcone_penalty_names[] = { "aimconePenalty", "aimConePenalty", "_aimconePenalty", "_aimConePenalty" };
		const char* sight_aimcone_scale_names[] = { "sightAimConeScale", "sightAimconeScale", "_sightAimConeScale" };
		const char* aimcone_penalty_per_shot_names[] = { "aimconePenaltyPerShot", "aimConePenaltyPerShot", "_aimconePenaltyPerShot" };
		const char* has_ads_names[] = { "hasADS", "hasAds", "_hasADS" };
		const char* burst_names[] = { "isBurstWeapon", "_isBurstWeapon", "burstWeapon" };
		const char* change_firemode_names[] = { "canChangeFireModes", "canChangeFireMode", "_canChangeFireModes" };
		const char* burst_rate_names[] = { "internalBurstFireRateScale", "_internalBurstFireRateScale", "burstFireRateScale" };
		const char* string_hold_names[] = { "stringHoldDurationMax", "string_hold_duration_max", "_stringHoldDurationMax" };
		auto server_projectile = c_runtime::find_class( "ServerProjectile" );
		auto is_reloading = field_offset_by_names( klass, is_reloading_names, 3 );
		if ( !is_reloading )
			is_reloading = field_offset_by_name_contains_type( klass, "reload", nullptr, "Boolean" );
		auto stance_penalty = field_offset_by_names( klass, stance_penalty_names, 2 );
		if ( !stance_penalty )
			stance_penalty = field_offset_by_name_contains_type( klass, "stance", "penalty", "Single" );
		auto aimcone_penalty = field_offset_by_names( klass, aimcone_penalty_names, 4 );
		if ( !aimcone_penalty )
			aimcone_penalty = field_offset_by_name_contains_type( klass, "aimcone", "penalty", "Single" );
		if ( !aimcone_penalty )
			aimcone_penalty = field_offset_by_name_contains_type( klass, "aimCone", "penalty", "Single" );
		auto sight_aimcone_scale = field_offset_by_names( klass, sight_aimcone_scale_names, 3 );
		if ( !sight_aimcone_scale )
			sight_aimcone_scale = field_offset_by_name_contains_type( klass, "sight", "scale", "Single" );
		if ( !sight_aimcone_scale )
			sight_aimcone_scale = field_offset_by_name_contains_type( klass, "sight", "aim", "Single" );
		if ( !sight_aimcone_scale )
			sight_aimcone_scale = field_offset_by_name_contains_type( klass, "aimcone", "scale", "Single" );
		auto aimcone_penalty_per_shot = field_offset_by_names( klass, aimcone_penalty_per_shot_names, 3 );
		if ( !aimcone_penalty_per_shot )
			aimcone_penalty_per_shot = field_offset_by_name_contains_type( klass, "penaltyPerShot", nullptr, "Single" );
		auto has_ads = field_offset_by_names( klass, has_ads_names, 3 );
		if ( !has_ads )
			has_ads = field_offset_by_name_contains_type( klass, "ads", nullptr, "Boolean" );
		auto primary_magazine = field_offset_by_name( klass, "primaryMagazine" );
		auto hip_aim_cone_scale = field_offset_by_name( klass, "hipAimConeScale" );
		if ( !hip_aim_cone_scale )
			hip_aim_cone_scale = field_offset_by_name_contains_type( klass, "hip", "scale", "Single" );
		if ( !hip_aim_cone_scale )
			hip_aim_cone_scale = field_offset_by_name_contains_type( klass, "hip", "aim", "Single" );
		auto cached_mod_hash = field_offset_by_name( klass, "cachedModHash" );
		if ( !cached_mod_hash )
			cached_mod_hash = field_offset_by_name_contains( klass, "cached", "hash" );
		if ( !sight_aimcone_scale && cached_mod_hash )
			sight_aimcone_scale = first_field_offset_after_type( klass, cached_mod_hash, "Single" );
		if ( !hip_aim_cone_scale && sight_aimcone_scale )
			hip_aim_cone_scale = nth_field_offset_after_type( klass, sight_aimcone_scale, "Single", 1 );
		auto gravity_modifier = field_offset_by_name_contains_type( klass, "gravity", nullptr, "Single" );
		if ( !gravity_modifier )
			gravity_modifier = field_offset_by_name_contains_type( server_projectile, "gravity", nullptr, "Single" );
		auto drag = field_offset_by_name_contains_type( klass, "drag", nullptr, "Single" );
		if ( !drag )
			drag = field_offset_by_name_contains_type( server_projectile, "drag", nullptr, "Single" );
		auto string_hold_duration_max = field_offset_by_names( klass, string_hold_names, 3 );
		if ( !string_hold_duration_max )
			string_hold_duration_max = field_offset_by_name_contains_type( klass, "hold", "duration", "Single" );
		if ( !string_hold_duration_max )
		{
			auto bow = find_named_class( "BowWeapon" );
			if ( !bow )
				bow = class_name_contains_i( "Bow" );
			string_hold_duration_max = field_offset_by_names( bow, string_hold_names, 3 );
			if ( !string_hold_duration_max )
				string_hold_duration_max = field_offset_by_name_contains_type( bow, "hold", "duration", "Single" );
			if ( !string_hold_duration_max )
			{
				auto holder = class_with_field_name_contains_type( "stringHold", "Single", false );
				if ( !holder )
					holder = class_with_field_name_contains_type( "holdDuration", "Single", false );
				string_hold_duration_max = field_offset_by_names( holder, string_hold_names, 3 );
				if ( !string_hold_duration_max )
					string_hold_duration_max = field_offset_by_name_contains_type( holder, "hold", "duration", "Single" );
			}
		}

		w.struct_begin( name );
		w.struct_field( "aimCone",                 field_offset_by_name( klass, "aimCone" ) );
		w.struct_field( "hipAimCone",              field_offset_by_name( klass, "hipAimCone" ) );
		w.struct_field( "aimSway",                 field_offset_by_name( klass, "aimSway" ) );
		w.struct_field( "aimSwaySpeed",            field_offset_by_name( klass, "aimSwaySpeed" ) );
		w.struct_field( "recoil",                  field_offset_by_type_contains( klass, "RecoilProperties" ) );
		w.struct_field( "projectileVelocityScale", field_offset_by_name( klass, "projectileVelocityScale" ) );
		w.struct_field( "automatic",               field_offset_by_name( klass, "automatic" ) );
		w.struct_field( "reloadTime",              field_offset_by_name( klass, "reloadTime" ) );
		w.struct_field( "primaryMagazine",         primary_magazine );
		w.struct_field( "magazine",                primary_magazine );
		w.struct_field( "isReloading",             is_reloading );
		w.struct_field( "stancePenalty",           stance_penalty );
		w.struct_field( "aimconePenalty",          aimcone_penalty );
		w.struct_field( "sightAimConeScale",       sight_aimcone_scale );
		w.struct_field( "sight_aim_cone_scale",    sight_aimcone_scale );
		w.struct_field( "hipAimConeScale",         hip_aim_cone_scale );
		w.struct_field( "hip_aim_cone_scale",      hip_aim_cone_scale );
		w.struct_field( "aimconePenaltyPerShot",   aimcone_penalty_per_shot );
		w.struct_field( "hasADS",                  has_ads );
		w.struct_field( "isBurstWeapon",           field_offset_by_names( klass, burst_names, 3 ) );
		w.struct_field( "canChangeFireModes",      field_offset_by_names( klass, change_firemode_names, 3 ) );
		w.struct_field( "internalBurstFireRateScale", field_offset_by_names( klass, burst_rate_names, 3 ) );
		w.struct_field( "gravity_modifier",        gravity_modifier );
		w.struct_field( "drag",                    drag );
		w.struct_field( "string_hold_duration_max", string_hold_duration_max );
		w.struct_end( );
	};

	write_fields( "BaseProjectile" );
}

static void write_item_mod_projectile( c_writer& w )
{
	auto klass = find_named_class( "ItemModProjectile" );
	if ( !klass )
		klass = class_name_contains_i( "ItemMod", "Projectile" );

	const char* projectile_object_names[] = { "projectileObject", "_projectileObject" };
	const char* ammo_type_names[] = { "ammoType", "_ammoType" };
	const char* projectile_spread_names[] = { "projectileSpread", "_projectileSpread" };
	const char* projectile_velocity_names[] = { "projectileVelocity", "_projectileVelocity" };
	const char* projectile_velocity_spread_names[] = { "projectileVelocitySpread", "_projectileVelocitySpread" };
	const char* use_curve_names[] = { "useCurve", "_useCurve" };
	const char* spread_scalar_names[] = { "spreadScalar", "_spreadScalar" };
	const char* category_names[] = { "category", "_category" };

	auto item_definition = c_runtime::find_class( "ItemDefinition" );

	auto projectile_object = field_offset_by_names( klass, projectile_object_names, 2 );
	if ( !projectile_object )
		projectile_object = field_offset_by_type_or_class_contains( klass, "GameObject" );
	auto ammo_type = field_offset_by_names( klass, ammo_type_names, 2 );
	if ( !ammo_type && item_definition )
		ammo_type = field_offset_by_type_class( klass, item_definition );
	if ( !ammo_type )
		ammo_type = field_offset_by_type_or_class_contains( klass, "ItemDefinition" );
	auto projectile_spread = field_offset_by_names( klass, projectile_spread_names, 2 );
	if ( !projectile_spread )
		projectile_spread = field_offset_by_name_contains_type( klass, "projectile", "spread", "Single" );
	auto projectile_velocity = field_offset_by_names( klass, projectile_velocity_names, 2 );
	if ( !projectile_velocity )
		projectile_velocity = field_offset_by_name_contains_type( klass, "projectile", "velocity", "Single" );
	auto projectile_velocity_spread = field_offset_by_names( klass, projectile_velocity_spread_names, 2 );
	if ( !projectile_velocity_spread )
		projectile_velocity_spread = field_offset_by_name_contains_type( klass, "velocity", "spread", "Single" );
	auto use_curve = field_offset_by_names( klass, use_curve_names, 2 );
	if ( !use_curve )
		use_curve = field_offset_by_name_contains_type( klass, "curve", nullptr, "Boolean" );
	auto spread_scalar = field_offset_by_names( klass, spread_scalar_names, 2 );
	if ( !spread_scalar )
		spread_scalar = field_offset_by_name_contains_type( klass, "spread", "scalar", "Single" );
	auto category = field_offset_by_names( klass, category_names, 2 );
	if ( !category )
		category = field_offset_by_name_contains( klass, "category" );

	w.struct_begin( "ItemModProjectile" );
	w.struct_field( "projectileObject", projectile_object );
	w.struct_field( "ammoType", ammo_type );
	w.struct_field( "projectileSpread", projectile_spread );
	w.struct_field( "projectileVelocity", projectile_velocity );
	w.struct_field( "projectileVelocitySpread", projectile_velocity_spread );
	w.struct_field( "useCurve", use_curve );
	w.struct_field( "spreadScalar", spread_scalar );
	w.struct_field( "category", category );
	w.struct_end( );
}

static void write_player_eyes( c_writer& w )
{
	auto klass = c_runtime::find_class( "PlayerEyes" );
	auto view_offset = field_offset_by_name( klass, "viewOffset" );
	auto body_rotation = field_offset_by_name( klass, "bodyRotation" );
	auto third_person_sleeping = field_offset_by_name( klass, "thirdPersonSleepingOffset" );
	if ( !view_offset )
		view_offset = first_wrapper_field_after_type( klass, third_person_sleeping, "Vector3" );
	if ( !view_offset )
		view_offset = first_field_offset_after_type( klass, third_person_sleeping, "Vector3" );
	if ( !body_rotation )
		body_rotation = first_field_offset_after_type( klass, view_offset, "Quaternion" );
	if ( !body_rotation )
		body_rotation = field_offset_by_plain_type_contains_index( klass, "Quaternion", 0 );
	auto world_position = field_offset_by_name( klass, "worldPosition" );
	if ( !world_position )
		world_position = field_offset_by_name_contains_type( klass, "world", "position", "Vector3" );
	if ( !world_position && body_rotation )
		world_position = first_field_offset_after_type( klass, body_rotation, "Vector3" );

	w.struct_begin( "player_eyes" );
	w.struct_field( "viewOffset",   view_offset );
	w.struct_field( "bodyRotation", body_rotation );
	w.struct_field( "world_position", world_position );
	w.struct_field( "worldPosition", world_position );
	w.struct_end( );
}

static void write_player_input( c_writer& w )
{
	auto klass = c_runtime::find_class( "PlayerInput" );
	auto body_angles = field_offset_by_name( klass, "bodyAngles" );
	if ( !body_angles )
		body_angles = field_offset_by_type_contains_index( klass, "Vector3", 0 );
	auto state = field_offset_by_name( klass, "state" );
	if ( !state )
		state = field_offset_by_name_contains( klass, "state" );
	if ( !state )
		state = field_offset_by_type_or_class_contains( klass, "InputState" );
	if ( !state )
		state = first_wrapper_field_after_type( klass, 0, "InputState" );
	if ( !state && body_angles )
		state = first_field_offset_before( klass, body_angles );
	auto yaw = field_offset_by_name( klass, "yaw" );
	if ( !yaw )
		yaw = field_offset_by_name_contains_type( klass, "yaw", nullptr, "Single" );
	if ( !yaw && body_angles )
		yaw = first_field_offset_after_type( klass, body_angles, "Single" );

	w.struct_begin( "player_input" );
	w.struct_field( "state",      state );
	w.struct_field( "bodyAngles", body_angles );
	w.struct_field( "yaw",        yaw );
	w.struct_end( );
}

static void write_player_model( c_writer& w )
{
	auto klass = c_runtime::find_class( "PlayerModel" );
	if ( !klass )
		klass = class_name_contains( "PlayerModel" );
	auto bone_transforms = field_offset_by_name( klass, "boneTransforms" );
	if ( !bone_transforms )
		bone_transforms = field_offset_by_type_contains( klass, "Transform[]" );
	if ( !bone_transforms )
		bone_transforms = field_offset_by_type_contains( klass, "Transform" );
	const char* velocity_names[] = { "newVelocity", "velocity", "_velocity" };
	const char* visible_names[] = { "visible", "isVisible", "_visible", "visibility" };
	const char* is_npc_names[] = { "isNpc", "isNPC", "is_npc", "_isNpc" };
	const char* position_names[] = { "position", "_position" };
	auto skinned_multi_mesh = field_offset_by_type_contains( klass, "SkinnedMultiMesh" );
	auto collision = field_offset_by_name( klass, "collision" );
	if ( !collision )
		collision = field_offset_by_name_contains( klass, "collision" );
	auto full_mask = field_offset_by_name( klass, "fullMask" );
	if ( !full_mask )
		full_mask = field_offset_by_name_contains( klass, "full", "mask" );
	auto exact_new_velocity = field_offset_by_name( klass, "newVelocity" );
	auto velocity = exact_new_velocity;
	if ( !velocity )
		velocity = field_offset_by_name_contains_type( klass, "new", "velocity", "Vector3" );
	if ( !velocity )
		velocity = method_field_offset_by_name_contains( klass, "new", "velocity" );
	if ( !velocity )
		velocity = getter_field_offset_by_return_type_name( klass, "Vector3", "velocity" );
	if ( !velocity )
		velocity = field_offset_by_name_contains_type( klass, "velocity", nullptr, "Vector3" );
	if ( !velocity )
		velocity = field_offset_by_names( klass, velocity_names, 3 );
	auto visible = field_offset_by_name_contains_type( klass, "visible", nullptr, "Nullable" );
	if ( !visible )
		visible = field_offset_by_names( klass, visible_names, 4 );
	if ( !visible )
		visible = field_offset_by_name_contains_type( klass, "visible", nullptr, "Boolean" );
	if ( !visible )
		visible = field_offset_by_name_contains_type( klass, "visibility", nullptr, "Boolean" );
	auto position = field_offset_by_names( klass, position_names, 2 );
	if ( !position )
		position = field_offset_by_name_contains_type( klass, "position", nullptr, "Vector3" );
	auto is_npc = field_offset_by_names( klass, is_npc_names, 4 );
	if ( !is_npc )
		is_npc = field_offset_by_name_contains_type( klass, "npc", nullptr, "Boolean" );
	if ( !is_npc )
		is_npc = field_offset_by_name_contains_type( klass, "NPC", nullptr, "Boolean" );
	if ( !is_npc )
		is_npc = getter_field_offset_by_return_type_name( klass, "Boolean", "npc" );
	if ( !is_npc )
		is_npc = getter_field_offset_by_return_type_name( klass, "Boolean", "NPC" );
	if ( !is_npc )
		is_npc = method_field_offset_by_name_contains( klass, "npc" );
	if ( !is_npc )
		is_npc = method_field_offset_by_name_contains( klass, "NPC" );
	if ( full_mask && skinned_multi_mesh && full_mask < skinned_multi_mesh )
	{
		std::uint64_t first_vec = 0;
		std::uint64_t second_vec = 0;
		for ( auto cur = klass; cur; cur = cur->parent( ) )
		{
			void* iter = nullptr;
			while ( auto f = cur->fields( &iter ) )
			{
				if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
					continue;
				auto off = f->offset( cur );
				if ( off <= full_mask || off >= skinned_multi_mesh )
					continue;
				auto t = f->type( );
				auto tn = t ? t->name( ) : nullptr;
				if ( !tn || !ascii_contains_i( tn, "Vector3" ) )
					continue;
				if ( !first_vec || off < first_vec )
				{
					second_vec = first_vec;
					first_vec = off;
				}
				else if ( off != first_vec && ( !second_vec || off < second_vec ) )
				{
					second_vec = off;
				}
			}
		}
		if ( first_vec && ( !position || position <= full_mask || position >= skinned_multi_mesh ) )
			position = first_vec;
		if ( second_vec && ( !velocity || velocity <= position || velocity >= skinned_multi_mesh ) )
			velocity = second_vec;
	}
	if ( skinned_multi_mesh )
	{
		auto anchored_position = first_field_offset_after_type( klass, skinned_multi_mesh, "Vector3" );
		if ( anchored_position && !position )
			position = anchored_position;
		if ( !exact_new_velocity && position )
		{
			auto anchored_velocity = last_field_offset_before_type( klass, skinned_multi_mesh, "Vector3" );
			if ( anchored_velocity > position )
				velocity = anchored_velocity;
		}
		if ( !exact_new_velocity && position && ( !velocity || velocity <= position ) )
			velocity = first_field_offset_after_type( klass, position, "Vector3" );
	}
	if ( !velocity && position )
	{
		for ( std::size_t i = 0; i < 8; i++ )
		{
			auto off = field_offset_by_plain_type_contains_index( klass, "Vector3", i );
			if ( off > position )
			{
				velocity = off;
				break;
			}
		}
	}
	if ( g_base_player_live.player_model_position &&
		!position )
		position = g_base_player_live.player_model_position;
	if ( !visible )
	{
		for ( std::size_t i = 0; i < 16; i++ )
		{
			auto off = field_offset_by_type_name_index( klass, "System.Boolean", i );
			if ( off > bone_transforms )
			{
				visible = off;
				break;
			}
		}
	}

	w.struct_begin( "player_model" );
	w.struct_field( "boneTransforms", bone_transforms );
	w.struct_field( "_multiMesh",     skinned_multi_mesh );
	w.struct_field( "collision",      collision );
	w.struct_field( "fullMask",       full_mask );
	w.struct_field( "newVelocity",    velocity );
	w.struct_field( "isNpc",          is_npc );
	w.struct_field( "visible",        visible );
	w.struct_field( "position",       position );
	w.struct_end( );
}

static void write_flag_constants( c_writer& w )
{
	w.struct_begin( "base_entity_flags" );
	w.struct_field( "Placeholder", 0x1 );
	w.struct_field( "On",          0x2 );
	w.struct_field( "OnFire",      0x4 );
	w.struct_field( "Open",        0x8 );
	w.struct_field( "Locked",      0x10 );
	w.struct_field( "Debugging",   0x20 );
	w.struct_field( "Disabled",    0x40 );
	w.struct_field( "Reserved1",   0x80 );
	w.struct_field( "Reserved2",   0x100 );
	w.struct_field( "Reserved3",   0x200 );
	w.struct_field( "Reserved4",   0x400 );
	w.struct_field( "Reserved5",   0x800 );
	w.struct_end( );

	w.struct_begin( "base_player_flags" );
	w.struct_field( "Unused1",             0x1 );
	w.struct_field( "Unused2",             0x2 );
	w.struct_field( "IsAdmin",             0x4 );
	w.struct_field( "ReceivingSnapshot",   0x8 );
	w.struct_field( "Sleeping",            0x10 );
	w.struct_field( "Spectating",          0x20 );
	w.struct_field( "Wounded",             0x40 );
	w.struct_field( "IsDeveloper",         0x80 );
	w.struct_field( "Connected",           0x100 );
	w.struct_field( "ThirdPersonViewmode", 0x400 );
	w.struct_end( );
}

static void write_model_state( c_writer& w )
{
	auto klass = find_model_state_class( );
	auto base_player = c_runtime::find_class( "BasePlayer" );
	if ( base_player )
	{
		std::uint64_t model_state_off = field_offset_by_name( base_player, "modelState" );
		if ( !model_state_off )
		{
			auto rigidbody_klass = c_runtime::find_class( "Rigidbody" );
			auto player_model_klass = c_runtime::find_class( "PlayerModel" );
			if ( !player_model_klass )
				player_model_klass = class_name_contains( "PlayerModel" );
			auto player_input_klass = c_runtime::find_class( "PlayerInput" );

			std::uint64_t min_off = 0;
			std::uint64_t max_off = 0;
			if ( rigidbody_klass )
				min_off = field_offset_by_type_class( base_player, rigidbody_klass );
			if ( !min_off )
				min_off = field_offset_by_type_contains( base_player, "Rigidbody" );
			auto push_max = [&]( std::uint64_t value )
			{
				if ( value > min_off && ( !max_off || value < max_off ) )
					max_off = value;
			};
			if ( player_model_klass )
				push_max( field_offset_by_type_class( base_player, player_model_klass ) );
			if ( !max_off )
				push_max( field_offset_by_type_contains( base_player, "PlayerModel" ) );
			if ( player_input_klass )
				push_max( field_offset_by_type_class( base_player, player_input_klass ) );
			if ( !max_off )
				push_max( field_offset_by_type_contains( base_player, "PlayerInput" ) );
			if ( min_off && max_off )
			{
				if ( klass )
					model_state_off = field_offset_by_type_class_between( base_player, klass, min_off, max_off );
				if ( !model_state_off )
					model_state_off = field_offset_by_model_state_shape_between( base_player, min_off, max_off );
			}
		}
		auto model_state_field = field_by_offset_walk( base_player, model_state_off );
		auto model_state_type = model_state_field ? model_state_field->type( ) : nullptr;
		auto model_state_class = model_state_type ? model_state_type->klass( ) : nullptr;
		if ( class_has_model_state_shape( model_state_class ) ||
			class_has_model_state_value_shape( model_state_class ) )
			klass = model_state_class;
	}

	auto flags = field_offset_by_name( klass, "flags" );
	auto water_level = field_offset_by_name( klass, "waterLevel" );
	if ( !water_level )
		water_level = field_offset_by_name_contains_type( klass, "water", "level", "Single" );
	if ( !water_level )
		water_level = first_field_offset_after_type( klass, 0, "Single" );
	auto look_dir = field_offset_by_name( klass, "lookDir" );
	if ( !look_dir )
		look_dir = field_offset_by_name_contains_type( klass, "look", "dir", "Vector3" );
	if ( !look_dir )
		look_dir = first_field_offset_after_type( klass, water_level, "Vector3" );
	if ( water_level )
	{
		auto shaped_flags = first_field_offset_after_type( klass, water_level, "UInt32" );
		if ( !shaped_flags )
			shaped_flags = first_field_offset_after_type( klass, water_level, "Int32" );
		if ( shaped_flags && ( !look_dir || shaped_flags < look_dir ) )
			flags = shaped_flags;
	}
	if ( !flags )
		flags = field_offset_by_name_contains( klass, "flag" );
	if ( !flags )
		flags = field_offset_by_name_contains_type( klass, "flag", nullptr, "UInt32" );
	if ( !flags )
		flags = first_field_offset_after_type( klass, 0, "UInt32" );
	if ( !flags )
		flags = first_field_offset_after_type( klass, 0, "Int32" );
	if ( g_base_player_live.model_state_flags )
		flags = g_base_player_live.model_state_flags;
	auto flying = static_u32_field_value_by_name( klass, "Flying" );
	if ( !flying )
		flying = static_u32_field_value_by_name_contains( klass, "Flying" );
	auto sleeping = static_u32_field_value_by_name( klass, "Sleeping" );
	if ( !sleeping )
		sleeping = static_u32_field_value_by_name_contains( klass, "Sleeping" );
	auto mounted = static_u32_field_value_by_name( klass, "Mounted" );
	if ( !mounted )
		mounted = static_u32_field_value_by_name_contains( klass, "Mounted" );
	auto on_ground = static_u32_field_value_by_name( klass, "OnGround" );
	if ( !on_ground )
		on_ground = static_u32_field_value_by_name_contains( klass, "OnGround" );

	w.struct_begin( "model_state" );
	w.struct_field( "flags",    flags );
	w.struct_field( "Flying",   flying );
	w.struct_field( "Sleeping", sleeping );
	w.struct_field( "Mounted",  mounted );
	w.struct_field( "OnGround", on_ground );
	w.struct_end( );
}

static void write_player_inventory( c_writer& w, std::uint64_t main, std::uint64_t belt, std::uint64_t wear )
{
	auto klass = c_runtime::find_class( "PlayerInventory" );

	w.struct_begin( "PlayerInventory" );
	w.struct_field( "loot",          field_offset_by_name( klass, "loot" ) );
	w.struct_field( "containerBelt", belt );
	w.struct_field( "containerMain", main );
	w.struct_field( "containerWear", wear );
	w.struct_end( );
}

static void write_base_movement( c_writer& w )
{
	auto klass = c_runtime::find_class( "BaseMovement" );
	auto owner = field_offset_by_name( klass, "Owner" );
	if ( !owner )
		owner = field_offset_by_type_contains( klass, "BasePlayer" );
	auto admin_cheat = field_offset_by_name( klass, "adminCheat" );
	if ( !admin_cheat )
		admin_cheat = field_offset_by_type_name_index( klass, "System.Boolean", 0 );

	w.struct_begin( "base_movement" );
	w.struct_field( "adminCheat", admin_cheat );
	w.struct_field( "Owner",      owner );
	w.struct_end( );
}

static void write_player_walk_movement( c_writer& w )
{
	auto klass = find_named_class( "PlayerWalkMovement" );
	if ( !klass )
		klass = find_named_class( "BaseMovement" );
	auto high_friction = field_offset_by_name( klass, "highFrictionMaterial" );
	if ( !high_friction )
		high_friction = field_offset_by_type_contains_index( klass, "PhysicsMaterial", 1 );
	auto ground_angle = field_offset_by_name_contains( klass, "ground", "angle" );
	if ( !ground_angle )
		ground_angle = first_wrapper_field_after_type( klass, high_friction, "Single" );
	auto ground_angle_new = field_offset_by_name_contains( klass, "groundAngleNew" );
	if ( !ground_angle_new )
		ground_angle_new = field_offset_by_name_contains( klass, "angle", "new" );
	if ( !ground_angle_new )
		ground_angle_new = first_wrapper_field_after_type( klass, ground_angle, "Single" );
	auto ground_time = field_offset_by_name_contains( klass, "ground", "time" );
	if ( !ground_time )
		ground_time = first_wrapper_field_after_type( klass, ground_angle_new, "Single" );
	auto jump_time = field_offset_by_name_contains( klass, "jump", "time" );
	if ( !jump_time )
		jump_time = first_wrapper_field_after_type( klass, ground_time, "Single" );
	auto land_time = field_offset_by_name_contains( klass, "land", "time" );
	if ( !land_time )
		land_time = first_wrapper_field_after_type( klass, jump_time, "Single" );
	auto gravity_multiplier = field_offset_by_name_contains( klass, "gravity", "multiplier" );
	if ( !gravity_multiplier )
		gravity_multiplier = first_wrapper_field_after_type( klass, land_time, "Single" );
	auto target_movement = field_offset_by_name_contains( klass, "target", "movement" );
	if ( !target_movement )
		target_movement = first_wrapper_field_after_type( klass, gravity_multiplier, "Vector3" );
	auto capsule = field_offset_by_name( klass, "capsule" );
	if ( !capsule )
		capsule = field_offset_by_name_contains( klass, "capsule" );
	if ( !capsule )
		capsule = field_offset_by_type_contains( klass, "Capsule" );
	auto ladder = field_offset_by_name( klass, "ladder" );
	if ( !ladder )
		ladder = field_offset_by_name_contains( klass, "ladder" );
	if ( !ladder )
		ladder = field_offset_by_type_contains( klass, "Ladder" );
	auto modify = field_offset_by_name( klass, "modify" );
	if ( !modify )
		modify = field_offset_by_name_contains( klass, "modify" );
	if ( !modify )
		modify = field_offset_by_type_contains( klass, "Modifier" );
	if ( !modify )
		modify = field_offset_by_type_contains( klass, "Modify" );

	w.struct_begin( "PlayerWalkMovement" );
	w.struct_field( "GroundAngle",       ground_angle );
	w.struct_field( "GroundAngleNew",    ground_angle_new );
	w.struct_field( "GroundTime",        ground_time );
	w.struct_field( "JumpTime",          jump_time );
	w.struct_field( "LandTime",          land_time );
	w.struct_field( "GravityMultiplier", gravity_multiplier );
	w.struct_field( "TargetMovement",    target_movement );
	w.struct_field( "capsule",           capsule );
	w.struct_field( "ladder",            ladder );
	w.struct_field( "modify",            modify );
	w.struct_end( );
}

static void write_held_entity( c_writer& w, il2cpp::il2cpp_class_t* item_id_class )
{
	auto klass = c_runtime::find_class( "HeldEntity" );
	if ( !klass )
		klass = class_name_contains( "HeldEntity" );
	const char* owner_item_uid_names[] = { "ownerItemUID", "ownerItemUid", "_ownerItemUID" };
	const char* view_model_names[] = { "viewModel", "_viewModel" };

	auto owner_item_uid = field_offset_by_names( klass, owner_item_uid_names, 3 );
	if ( !owner_item_uid )
		owner_item_uid = field_offset_by_type_class( klass, item_id_class );
	auto view_model = field_offset_by_names( klass, view_model_names, 2 );
	if ( !view_model )
		view_model = field_offset_by_type_or_class_contains( klass, "ViewModel" );

	std::uint32_t view_model_wrapper_kind = 0;
	std::uint32_t view_model_inner_off = 0;
	auto view_model_field = field_by_offset_walk( klass, view_model );
	auto view_model_type = view_model_field && view_model_field->type( )
		? view_model_field->type( )->klass( )
		: nullptr;
	auto view_model_type_name = view_model_type ? view_model_type->name( ) : nullptr;
	auto view_model_field_type_name = view_model_field && view_model_field->type( )
		? view_model_field->type( )->name( )
		: nullptr;
	auto base_view_model = c_runtime::find_class( "BaseViewModel" );
	auto direct_inner_off = view_model_type
		? ( std::uint32_t )field_offset_by_type_class( view_model_type, base_view_model )
		: 0;
	if ( !direct_inner_off && view_model_type )
		direct_inner_off = ( std::uint32_t )field_offset_by_type_or_class_contains( view_model_type, "BaseViewModel" );
	if ( !direct_inner_off && view_model_type )
		direct_inner_off = ( std::uint32_t )field_offset_by_name( view_model_type, "instance" );
	if ( !direct_inner_off && view_model_type )
		direct_inner_off = ( std::uint32_t )field_offset_by_name_contains( view_model_type, "instance" );
	auto type_name = view_model_field_type_name ? view_model_field_type_name : view_model_type_name;
	if ( type_name )
	{
		if ( std::strstr( type_name, "Lazy" ) )
			view_model_wrapper_kind = 1;
		else if ( std::strstr( type_name, "HiddenValue" ) )
			view_model_wrapper_kind = 2;
		else if ( std::strstr( type_name, "EntityRef" ) )
			view_model_wrapper_kind = 3;
		else if ( direct_inner_off )
			view_model_wrapper_kind = 4;
		else if ( !view_model_type_name
			|| ( std::strcmp( view_model_type_name, "ViewModel" ) && std::strcmp( view_model_type_name, "BaseViewModel" ) ) )
			view_model_wrapper_kind = 4;
	}
	if ( view_model_wrapper_kind && view_model_type )
	{
		view_model_inner_off = direct_inner_off;
		if ( !view_model_inner_off )
			view_model_inner_off = ( std::uint32_t )field_offset_by_type_or_class_contains( view_model_type, "BaseViewModel" );
		if ( !view_model_inner_off )
			view_model_inner_off = ( std::uint32_t )field_offset_by_type_or_class_contains( view_model_type, "ViewModel" );
		if ( !view_model_inner_off )
			view_model_inner_off = ( std::uint32_t )field_offset_by_name( view_model_type, "instance" );
		if ( !view_model_inner_off )
			view_model_inner_off = ( std::uint32_t )field_offset_by_name_contains( view_model_type, "instance" );
	}

	auto camera_punches = field_offset_by_name( klass, "_punches" );
	if ( !camera_punches )
		camera_punches = field_offset_by_name_contains( klass, "punch" );
	if ( !camera_punches && view_model )
	{
		std::uint64_t best = 0;
		for ( auto cur = klass; cur; cur = cur->parent( ) )
		{
			void* iter = nullptr;
			while ( auto f = cur->fields( &iter ) )
			{
				if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
					continue;
				auto off = f->offset( cur );
				if ( off >= view_model )
					continue;
				auto name = f->name( );
				auto type = f->type( );
				auto type_name = type ? type->name( ) : nullptr;
				if ( ( ascii_contains_i( name, "punch" ) || ascii_contains_i( type_name, "punch" ) ) && off > best )
					best = off;
			}
		}
		camera_punches = best;
	}
	if ( !camera_punches && view_model )
	{
		std::uint64_t best = 0;
		for ( auto cur = klass; cur; cur = cur->parent( ) )
		{
			void* iter = nullptr;
			while ( auto f = cur->fields( &iter ) )
			{
				if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
					continue;
				auto off = f->offset( cur );
				if ( off && off < view_model && off > best )
					best = off;
			}
		}
		camera_punches = best;
	}

	// Resolve Item reference field — search class hierarchy for a field of type Item
	std::uint64_t item_ref = 0;
	auto item_klass = c_runtime::find_class( "Item" );
	if ( !item_klass ) item_klass = find_named_class( "Item" );
	if ( item_klass )
	{
		for ( auto cur = klass; cur && !item_ref; cur = cur->parent( ) )
		{
			void* fiter = nullptr;
			while ( auto f = cur->fields( &fiter ) )
			{
				if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) continue;
				auto ft = f->type( );
				auto fc = ft ? ft->klass( ) : nullptr;
				if ( fc && fc == item_klass )
				{
					item_ref = f->offset( klass );
					break;
				}
				if ( fc && fc->name( ) && item_klass->name( ) &&
					std::strcmp( fc->name( ), item_klass->name( ) ) == 0 )
				{
					item_ref = f->offset( klass );
					break;
				}
			}
		}
	}
	// Fallback: search by field name
	if ( !item_ref )
		item_ref = field_offset_by_name( klass, "item" );
	if ( !item_ref )
		item_ref = field_offset_by_name( klass, "_item" );

	w.struct_begin( "held_entity" );
	w.struct_field( "ownerItemUID", owner_item_uid );
	w.struct_field( "item",         item_ref );
	w.struct_field( "viewModel",    view_model );
	w.struct_field_u32( "viewModel_wrapper_kind", view_model_wrapper_kind );
	w.struct_field_u32( "viewModel_inner_off", view_model_inner_off );
	w.struct_field( "_punches", camera_punches );
	w.struct_end( );
}

static void write_view_model( c_writer& w )
{
	auto klass = find_named_class( "ViewModel" );
	auto base_view_model = c_runtime::find_class( "BaseViewModel" );
	if ( !klass && base_view_model )
		klass = class_with_instance_field_type_class( base_view_model );
	auto instance = field_offset_by_name( klass, "instance" );
	if ( !instance )
		instance = field_offset_by_type_class( klass, base_view_model );
	if ( !instance )
		instance = field_offset_by_type_or_class_contains( klass, "BaseViewModel" );
	if ( !instance )
		instance = field_offset_by_name_contains( klass, "instance" );

	w.struct_begin( "ViewModel" );
	w.struct_field( "instance", instance );
	w.struct_field( "viewmodel_instance", instance );
	w.struct_end( );
}

static void write_base_view_model( c_writer& w )
{
	auto klass = c_runtime::find_class( "BaseViewModel" );
	if ( !klass )
	{
		auto view_model = find_named_class( "ViewModel" );
		auto instance_class = field_type_class_by_name( view_model, "instance" );
		if ( instance_class )
			klass = instance_class;
	}
	if ( !klass )
		klass = class_name_contains( "BaseViewModel" );
	auto use_view_model_camera = field_offset_by_name( klass, "useViewModelCamera" );
	if ( !use_view_model_camera )
		use_view_model_camera = field_offset_by_name_contains( klass, "view", "camera" );
	auto model = field_offset_by_name( klass, "model" );
	if ( !model )
		model = field_offset_by_name_contains( klass, "model" );
	auto bob = field_offset_by_name( klass, "bob" );
	if ( !bob )
		bob = field_offset_by_name_contains( klass, "bob" );
	if ( !bob )
	{
		auto bob_class = find_named_class( "ViewmodelBob" );
		if ( !bob_class )
			bob_class = class_name_contains_i( "Viewmodel", "Bob" );
		bob = field_offset_by_type_class( klass, bob_class );
	}
	auto lower = field_offset_by_name( klass, "lower" );
	if ( !lower )
		lower = field_offset_by_name_contains( klass, "lower" );
	if ( !lower )
	{
		auto lower_class = find_named_class( "ViewmodelLower" );
		if ( !lower_class )
			lower_class = class_name_contains_i( "Viewmodel", "Lower" );
		lower = field_offset_by_type_class( klass, lower_class );
	}
	auto sway = field_offset_by_name( klass, "sway" );
	if ( !sway )
		sway = field_offset_by_name_contains( klass, "sway" );
	if ( !sway )
	{
		auto sway_class = find_named_class( "ViewmodelSway" );
		if ( !sway_class )
			sway_class = class_name_contains_i( "Viewmodel", "Sway" );
		sway = field_offset_by_type_class( klass, sway_class );
	}
	auto iron_sights = field_offset_by_name( klass, "ironSights" );
	if ( !iron_sights )
		iron_sights = field_offset_by_name_contains( klass, "iron", "sight" );

	w.struct_begin( "base_view_model" );
	w.struct_field( "useViewModelCamera", use_view_model_camera );
	w.struct_field( "model",              model );
	w.struct_field( "bob",                bob );
	w.struct_field( "lower",              lower );
	w.struct_field( "sway",               sway );
	w.struct_field( "ironSights",         iron_sights );
	w.struct_end( );
}

static void write_skinned_multi_mesh( c_writer& w )
{
	auto klass = find_named_class( "SkinnedMultiMesh" );
	auto renderers = field_offset_by_name( klass, "Renderers" );
	if ( !renderers )
		renderers = field_offset_by_name( klass, "RendererList" );
	if ( !renderers )
		renderers = field_offset_by_name_contains( klass, "renderer" );
	if ( !renderers )
		renderers = field_offset_by_type_contains( klass, "Renderer" );

	w.struct_begin( "SkinnedMultiMesh" );
	w.struct_field( "RendererList", renderers );
	w.struct_field( "Renderers",    renderers );
	w.struct_end( );
}

static void write_server_projectile( c_writer& w )
{
	auto klass = find_named_class( "ServerProjectile" );

	w.struct_begin( "server_projectile" );
	w.struct_field( "drag",            field_offset_by_name_contains_type( klass, "drag", nullptr, "Single" ) );
	w.struct_field( "gravityModifier", field_offset_by_name_contains_type( klass, "gravity", nullptr, "Single" ) );
	w.struct_field( "speed",           field_offset_by_name_contains_type( klass, "speed", nullptr, "Single" ) );
	w.struct_field( "radius",          field_offset_by_name_contains_type( klass, "radius", nullptr, "Single" ) );
	w.struct_end( );
}

static void write_auto_turret( c_writer& w )
{
	auto klass = find_named_class( "AutoTurret" );
	if ( !klass )
		klass = class_name_contains_i( "Auto", "Turret" );
	if ( !klass )
		klass = class_with_field_name_contains_type( "authorized", nullptr, false );

	auto authorized_players = field_offset_by_name( klass, "authorizedPlayers" );
	if ( !authorized_players )
		authorized_players = field_offset_by_name_contains( klass, "authorized", "players" );
	if ( !authorized_players )
		authorized_players = field_offset_by_name_contains( klass, "authorized" );

	auto muzzle_pos = field_offset_by_name( klass, "muzzlePos" );
	if ( !muzzle_pos )
		muzzle_pos = field_offset_by_name_contains( klass, "muzzle", "pos" );
	auto gun_yaw = field_offset_by_name( klass, "gun_yaw" );
	if ( !gun_yaw )
		gun_yaw = field_offset_by_name_contains_type( klass, "gun", "yaw", "Single" );
	auto gun_pitch = field_offset_by_name( klass, "gun_pitch" );
	if ( !gun_pitch )
		gun_pitch = field_offset_by_name_contains_type( klass, "gun", "pitch", "Single" );
	auto sight_range = field_offset_by_name( klass, "sightRange" );
	if ( !sight_range )
		sight_range = field_offset_by_name_contains_type( klass, "sight", "range", "Single" );
	if ( !authorized_players && muzzle_pos )
	{
		std::uint64_t best = 0;
		for ( auto cur = klass; cur; cur = cur->parent( ) )
		{
			void* iter = nullptr;
			while ( auto f = cur->fields( &iter ) )
			{
				if ( f->flags( ) & FIELD_ATTRIBUTE_STATIC )
					continue;
				auto off = f->offset( cur );
				if ( off >= muzzle_pos )
					continue;
				if ( field_type_or_generic_arg_matches( f, "List" ) ||
					field_type_or_generic_arg_matches( f, "PlayerName" ) ||
					field_type_or_generic_arg_matches( f, "PlayerNameID" ) )
				{
					if ( off > best )
						best = off;
				}
			}
		}
		authorized_players = best;
	}
	auto last_yaw = field_offset_by_name( klass, "lastYaw" );
	if ( !last_yaw )
		last_yaw = field_offset_by_name_contains_type( klass, "last", "yaw", "Single" );
	if ( !last_yaw && authorized_players )
	{
		auto first_single = first_field_offset_after_type( klass, authorized_players, "Single" );
		if ( first_single && ( !muzzle_pos || first_single < muzzle_pos ) )
			last_yaw = first_single;
	}

	w.struct_begin( "AutoTurret" );
	w.struct_field( "authorizedPlayers", authorized_players );
	w.struct_field( "lastYaw",           last_yaw );
	w.struct_field( "muzzlePos",         muzzle_pos );
	w.struct_field( "gun_yaw",           gun_yaw );
	w.struct_field( "gun_pitch",         gun_pitch );
	w.struct_field( "sightRange",        sight_range );
	w.struct_end( );
}

static void write_player_corpse( c_writer& w )
{
	auto klass = find_named_class( "PlayerCorpse" );
	if ( !klass )
		klass = class_name_contains_i( "Player", "Corpse" );
	if ( !klass )
		klass = class_with_field_name_contains_type( "clientClothing", nullptr, false );

	auto client_clothing = field_offset_by_name( klass, "clientClothing" );
	if ( !client_clothing )
		client_clothing = field_offset_by_name_contains( klass, "client", "clothing" );
	if ( !client_clothing )
		client_clothing = field_offset_by_name_contains( klass, "clothing" );
	if ( !client_clothing )
		client_clothing = highest_field_offset_by_type_or_class_contains( klass, "Item" );
	if ( !client_clothing )
		client_clothing = highest_field_offset_by_type_or_class_contains( klass, "List" );

	w.struct_begin( "PlayerCorpse" );
	w.struct_field( "clientClothing", client_clothing );
	w.struct_end( );
}

static void write_tod_sky( c_writer& w )
{
	auto klass = c_runtime::find_class( "TOD_Sky" );
	auto static_klass = c_runtime::get_inner_static_class( klass );
	auto static_owner = klass;
	auto tod_sky_rva = c_runtime::static_class_rva( klass );
	auto rva = tod_sky_rva;
	std::uint64_t instance = 0;
	bool instance_resolved = false;
	auto resolve_static_instance_field = []( il2cpp::il2cpp_class_t* owner, il2cpp::il2cpp_class_t* value_type ) -> il2cpp::field_info_t*
	{
		if ( !owner )
			return nullptr;

		void* iter = nullptr;
		while ( auto f = owner->fields( &iter ) )
		{
			if ( owner->static_field_data( ) && !( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
				continue;
			auto name = f->name( );
			auto type = f->type( );
			auto type_class = type ? type->klass( ) : nullptr;
			if ( value_type && type_class == value_type )
				return f;
			if ( ascii_contains_i( name, "instance" ) )
				return f;
		}
		return nullptr;
	};
	auto resolve_static_instance_value = []( il2cpp::il2cpp_class_t* owner, il2cpp::il2cpp_class_t* value_type ) -> il2cpp::field_info_t*
	{
		if ( !owner || !value_type || !owner->static_field_data( ) )
			return nullptr;

		void* iter = nullptr;
		while ( auto f = owner->fields( &iter ) )
		{
			if ( !( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
				continue;
			auto value = f->static_get_value<void*>( );
			if ( !is_valid_userland_ptr( ( std::uintptr_t )value ) )
				continue;
			__try
			{
				if ( il2cpp::object_get_class( value ) == value_type )
					return f;
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
			}
		}
		return nullptr;
	};
	auto resolve_static_instance_list = []( il2cpp::il2cpp_class_t* owner, il2cpp::il2cpp_class_t* value_type ) -> il2cpp::field_info_t*
	{
		if ( !owner || !value_type )
			return nullptr;

		il2cpp::field_info_t* best = nullptr;
		void* iter = nullptr;
		while ( auto f = owner->fields( &iter ) )
		{
			if ( !( f->flags( ) & FIELD_ATTRIBUTE_STATIC ) )
				continue;

			auto type = f->type( );
			auto type_class = type ? type->klass( ) : nullptr;
			auto arg = type_class ? type_class->get_generic_argument_at( 0 ) : nullptr;
			if ( arg != value_type )
				continue;

			if ( !best || f->offset( ) > best->offset( ) )
				best = f;
		}

		return best;
	};
	auto accept_field = [&]( il2cpp::il2cpp_class_t* owner, il2cpp::field_info_t* field ) -> bool
	{
		if ( !owner || !field )
			return false;
		static_owner = owner;
		rva = c_runtime::static_class_rva( owner );
		instance = field->offset( );
		instance_resolved = true;
		return true;
	};
	if ( !instance_resolved )
	{
		if ( auto field = resolve_static_instance_value( klass, klass ) )
			accept_field( klass, field );
	}
	if ( !instance_resolved )
	{
		if ( auto field = resolve_static_instance_value( static_klass, klass ) )
			accept_field( static_klass, field );
	}
	if ( !instance_resolved )
	{
		if ( auto field = resolve_static_instance_list( klass, klass ) )
			accept_field( klass, field );
	}
	if ( !instance_resolved )
	{
		if ( auto field = resolve_static_instance_list( static_klass, klass ) )
			accept_field( static_klass, field );
	}
	if ( !instance_resolved )
	{
		if ( auto field = resolve_static_instance_field( klass, klass ) )
			accept_field( klass, field );
	}
	if ( !instance_resolved )
	{
		if ( auto field = resolve_static_instance_field( static_klass, klass ) )
			accept_field( static_klass, field );
	}
	if ( !instance_resolved && static_klass && klass )
	{
		if ( auto field = il2cpp::get_static_field_if_value_is<void*>(
			static_klass, klass, 0, 0, []( void* value ) { return value != nullptr; } ) )
			accept_field( static_klass, field );
	}
	if ( !instance_resolved )
		instance = static_owner ? field_offset_by_type_class( static_owner, klass ) : 0;
	if ( !instance_resolved && !instance )
		instance = field_offset_by_name( static_owner, "Instance" );
	if ( !instance_resolved && !instance )
		instance = field_offset_by_name( static_owner, "instance" );
	if ( !instance_resolved && !instance )
		instance = class_field_offset_by_type_contains( static_owner, "TOD_Sky" );
	if ( !instance_resolved && !instance )
		instance = class_field_offset_by_name_contains( static_owner, "sky" );
	if ( instance )
		instance_resolved = true;
	if ( !rva )
		rva = c_runtime::static_class_rva( static_owner );
	if ( tod_sky_rva )
		rva = tod_sky_rva;
	std::printf( "[morphine-dumper] tod_sky static: owner=%s rva=0x%llX instance=0x%llX%s\n",
		static_owner && static_owner->name( ) ? static_owner->name( ) : "?",
		( unsigned long long )rva,
		( unsigned long long )instance,
		instance_resolved ? "" : " unresolved" );
	auto cycle_off = field_offset_by_name( klass, "Cycle" );
	auto atmosphere_off = field_offset_by_name( klass, "Atmosphere" );
	auto day_off = field_offset_by_name( klass, "Day" );
	auto night_off = field_offset_by_name( klass, "Night" );
	auto stars_off = field_offset_by_name( klass, "Stars" );
	auto clouds_off = field_offset_by_name( klass, "Clouds" );
	auto ambient_off = field_offset_by_name( klass, "Ambient" );
	auto time_ambient = field_offset_by_name( klass, "timeSinceAmbientUpdate" );
	auto time_reflection = field_offset_by_name( klass, "timeSinceReflectionUpdate" );
	if ( !( time_ambient && time_reflection ) )
	{
		std::uint64_t first = 0;
		std::uint64_t second = 0;
		if ( last_consecutive_field_pair_by_type( klass, "Single", ambient_off, first, second ) )
		{
			if ( !time_ambient )
				time_ambient = first;
			if ( !time_reflection )
				time_reflection = second;
		}
	}

	w.struct_begin( "tod_sky_static" );
	w.struct_field( "tod_sky_c",        rva );
	w.struct_field( "static_fields",     0xB8 );
	if ( instance_resolved )
		w.struct_field_probed( "instance", instance );
	else
		w.struct_field( "instance", instance );
	w.struct_end( );

	w.struct_begin( "tod_sky" );
	w.struct_field( "Cycle",      cycle_off );
	w.struct_field( "Atmosphere", atmosphere_off );
	w.struct_field( "Day",        day_off );
	w.struct_field( "Night",      night_off );
	w.struct_field( "Stars",      stars_off );
	w.struct_field( "Clouds",     clouds_off );
	w.struct_field( "Ambient",    ambient_off );
	w.struct_field( "timeSinceAmbientUpdate", time_ambient );
	w.struct_field( "timeSinceReflectionUpdate", time_reflection );
	w.struct_end( );

	auto cycle = c_runtime::find_class( "TOD_CycleParameters" );
	w.struct_begin( "tod_cycle_parameters" );
	w.struct_field( "Hour",  field_offset_by_name( cycle, "Hour" ) );
	w.struct_field( "Day",   field_offset_by_name( cycle, "Day" ) );
	w.struct_field( "Month", field_offset_by_name( cycle, "Month" ) );
	w.struct_field( "Year",  field_offset_by_name( cycle, "Year" ) );
	w.struct_end( );

	auto ambient = find_named_class( "TOD_AmbientParameters" );
	w.struct_begin( "tod_ambient_parameters" );
	w.struct_field( "Saturation", field_offset_by_name( ambient, "Saturation" ) );
	w.struct_end( );

	auto night = find_named_class( "TOD_NightParameters" );
	auto ambient_color = field_offset_by_name( night, "AmbientColor" );
	auto ambient_multiplier = field_offset_by_name( night, "AmbientMultiplier" );
	if ( !ambient_multiplier )
		ambient_multiplier = field_offset_by_name_contains_type( night, "ambient", "multiplier", "Single" );
	if ( !ambient_multiplier )
		ambient_multiplier = highest_field_offset_by_type_contains( night, "Single" );
	if ( ambient_color && ambient_multiplier && ambient_multiplier <= ambient_color )
		ambient_multiplier = first_field_offset_after_type( night, ambient_color, "Single" );
	w.struct_begin( "tod_night_parameters" );
	w.struct_field( "AmbientMultiplier", ambient_multiplier );
	w.struct_end( );
}

static void write_unity_constants( c_writer& w )
{
	w.struct_begin( "unity_transform_native" );
	w.struct_field( "access_struct_off", 0x28 );
	w.struct_end( );

	w.struct_begin( "transform_data" );
	w.struct_field( "pos_base_off",       0x18 );
	w.struct_field( "indices_b_off",      0x20 );
	w.struct_field( "root_pos_off",       0x90 );
	w.struct_field( "root_quat_off",      0xA0 );
	w.struct_field( "matrix_stride",      0x30 );
	w.struct_field( "walker_max_depth",   0x10 );
	w.struct_field( "slot_pad_off",       0xC );
	w.struct_field( "slot_quat_off",      0x10 );
	w.struct_field( "slot_scale_off",     0x20 );
	w.struct_field( "slot_pos_quat_size", 0x20 );
	w.struct_end( );

	w.struct_begin( "unity_object" );
	w.struct_field( "cached_ptr", 0x10 );
	w.struct_field( "m_CachedPtr", 0x10 );
	w.struct_end( );

	w.struct_begin( "unity_component" );
	w.struct_field( "game_object", 0x20 );
	w.struct_field( "m_GameObject", 0x20 );
	w.struct_end( );

	w.struct_begin( "unity_game_object" );
	w.struct_field( "components",                 0x20 );
	w.struct_field( "component_count",            0x30 );
	w.struct_field( "component_stride",           0x10 );
	w.struct_field( "component_ptr_in_entry",     0x8 );
	w.struct_field( "component_handle_in_native", 0x18 );
	w.struct_end( );

	w.struct_begin( "unity_transform" );
	w.struct_field( "indirect_ptr_off", 0x28 );
	w.struct_field( "world_pos_off",    0x90 );
	w.struct_field( "encoding_hint",    0x0 );
	w.struct_end( );

	w.struct_begin( "unity_string" );
	w.struct_field( "m_stringLength", 0x10 );
	w.struct_field( "first_char",     0x14 );
	w.struct_end( );

	w.struct_begin( "system_list" );
	w.struct_field( "array",               0x10 );
	w.struct_field( "size",                0x18 );
	w.struct_field( "array_first_element", 0x20 );
	w.struct_end( );
}

void c_core::write_recoil_properties( c_writer& w )
{
	auto klass = c_runtime::find_class( "RecoilProperties" );
	const char* new_recoil_override_names[] = { "newRecoilOverride", "_newRecoilOverride" };
	const char* aimcone_curve_scale_names[] = { "aimconeCurveScale", "aimConeCurveScale", "_aimconeCurveScale", "aimconeProbabilityCurve" };

	w.struct_begin( "RecoilProperties" );
	w.struct_field( "recoilYawMin",     field_offset_by_name( klass, "recoilYawMin" ) );
	w.struct_field( "recoilYawMax",     field_offset_by_name( klass, "recoilYawMax" ) );
	w.struct_field( "recoilPitchMin",   field_offset_by_name( klass, "recoilPitchMin" ) );
	w.struct_field( "recoilPitchMax",   field_offset_by_name( klass, "recoilPitchMax" ) );
	w.struct_field( "timeToTakeMin",    field_offset_by_name( klass, "timeToTakeMin" ) );
	w.struct_field( "timeToTakeMax",    field_offset_by_name( klass, "timeToTakeMax" ) );
	w.struct_field( "ADSScale",         field_offset_by_name( klass, "ADSScale" ) );
	w.struct_field( "movementPenalty",  field_offset_by_name( klass, "movementPenalty" ) );
	w.struct_field( "clampPitch",       field_offset_by_name( klass, "clampPitch" ) );
	w.struct_field( "pitchCurve",       field_offset_by_name( klass, "pitchCurve" ) );
	w.struct_field( "yawCurve",         field_offset_by_name( klass, "yawCurve" ) );
	w.struct_field( "newRecoilOverride", field_offset_by_names( klass, new_recoil_override_names, 2 ) );
	w.struct_field( "overrideAimconeWithCurve", field_offset_by_name( klass, "overrideAimconeWithCurve" ) );
	w.struct_field( "aimconeProbabilityCurve", field_offset_by_name( klass, "aimconeProbabilityCurve" ) );
	w.struct_field( "aimconeCurveScale", field_offset_by_names( klass, aimcone_curve_scale_names, 4 ) );
	w.struct_end( );
}

void c_core::write_flint_strike_weapon( c_writer& w )
{
	auto klass = c_runtime::find_class( "FlintStrikeWeapon" );
	auto strike_recoil = field_offset_by_name( klass, "strikeRecoil" );
	auto did_spark = field_offset_by_name( klass, "_didSparkThisFrame" );
	if ( !did_spark && strike_recoil )
		did_spark = field_offset_after( klass, strike_recoil );

	w.struct_begin( "FlintStrikeWeapon" );
	w.struct_field( "successFraction", field_offset_by_name( klass, "successFraction" ) );
	w.struct_field( "strikeRecoil",    strike_recoil );
	w.struct_field( "_didSparkThisFrame", did_spark );
	w.struct_end( );
}

void c_core::write_world_item( c_writer& w )
{
	auto klass = c_runtime::find_class( "WorldItem" );
	if ( !klass )
		klass = class_name_contains( "WorldItem" );
	if ( !klass )
		klass = c_runtime::find_class( "DroppedItem" );

	const char* item_names[] = { "item", "_item" };
	const char* allow_pickup_names[] = { "allowPickup", "_allowPickup" };
	auto item_offset = field_offset_by_names( klass, item_names, 2 );
	if ( !item_offset )
		item_offset = field_offset_by_type_class( klass, m_item_class );

	w.struct_begin( "WorldItem" );
	w.struct_field( "item", item_offset );
	w.struct_field( "allowPickup", field_offset_by_names( klass, allow_pickup_names, 2 ) );
	w.struct_end( );
}

static void write_hackable_locked_crate( c_writer& w )
{
	auto klass = find_named_class( "HackableLockedCrate" );
	if ( !klass )
		klass = class_name_contains( "Hackable", "Crate" );
	if ( !klass )
		klass = class_name_contains_i( "Hackable", "Crate" );

	const char* timer_names[] = {
		"hackSeconds",
		"_hackSeconds",
		"hackTime",
		"hackDuration",
		"timerSeconds",
		"timer_seconds",
		"secondsToHack",
		"hackableSeconds"
	};
	const char* timer_text_names[] = { "timerText", "_timerText", "hackText", "timeText" };
	auto timer_text = field_offset_by_names( klass, timer_text_names, 4 );
	if ( !timer_text )
		timer_text = field_offset_by_name_contains_type( klass, "timer", "text", "Text" );
	if ( !timer_text )
		timer_text = field_offset_by_name_contains_type( klass, "time", "text", "Text" );
	if ( !timer_text )
		timer_text = field_offset_by_name_contains_type( klass, "hack", "text", "Text" );

	auto hack_seconds = field_offset_by_names( klass, timer_names, 8 );
	if ( timer_text )
	{
		auto anchored_seconds = first_field_offset_after_type( klass, timer_text, "Single" );
		if ( !anchored_seconds )
			anchored_seconds = first_field_offset_after_type( klass, timer_text, "Int32" );
		if ( anchored_seconds )
			hack_seconds = anchored_seconds;
	}
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "hack", "seconds", "Single" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "hack", "time", "Single" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "hack", nullptr, "Single" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "timer", "seconds", "Single" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "seconds", nullptr, "Single" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "time", nullptr, "Single" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "hack", "seconds", "Int32" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "hack", nullptr, "Int32" );
	if ( !hack_seconds )
		hack_seconds = field_offset_by_name_contains_type( klass, "timer", "seconds", "Int32" );
	if ( !timer_text )
		timer_text = field_offset_by_type_contains_after( klass, "Text", hack_seconds );

	std::printf( "[morphine-dumper] HackableLockedCrate: class=%p hackSeconds=0x%llX timerText=0x%llX\n",
		klass, hack_seconds, timer_text );

	w.struct_begin( "HackableLockedCrate" );
	w.struct_field_u32( "hackSeconds", ( std::uint32_t )hack_seconds );
	w.struct_field( "timerText", timer_text );
	w.struct_end( );
}

void c_core::write_item_icon( c_writer& w )
{
	auto klass = c_runtime::find_class( "ItemIcon" );
	if ( !klass )
		klass = class_name_contains( "ItemIcon" );
	auto static_klass = c_runtime::get_inner_static_class( klass );
	auto rva = static_klass ? c_runtime::static_class_rva( static_klass ) : c_runtime::static_class_rva( klass );
	std::uint64_t instance = 0;
	if ( static_klass )
	{
		const char* instance_names[] = { "instance", "Instance", "_instance", "singleton", "Singleton" };
		instance = field_offset_by_names( static_klass, instance_names, 5 );
		if ( !instance )
		{
			void* iter = nullptr;
			while ( auto f = static_klass->fields( &iter ) )
			{
				auto t = f ? f->type( ) : nullptr;
				auto tn = t ? t->name( ) : nullptr;
				if ( tn && std::strstr( tn, "ItemIcon" ) )
				{
					instance = f->offset( );
					break;
				}
			}
		}
	}
	if ( !instance && rva )
		instance = 0xB8;
	auto background_image = field_offset_by_name( klass, "backgroundImage" );
	if ( !background_image )
		background_image = field_offset_by_name_contains_type( klass, "background", nullptr, "Image" );
	if ( !background_image )
		background_image = field_offset_by_name_contains( klass, "background", "image" );

	w.struct_begin( "item_icon" );
	w.struct_field( "item_icon_c", rva );
	w.struct_field( "static_fields", instance );
	w.struct_field( "backgroundImage", background_image );
	w.struct_end( );
}

static void write_model( c_writer& w, const c_core::model_bone_indices_t& bones )
{
	auto base_entity = c_runtime::find_class( "BaseEntity" );
	auto klass = field_type_class_by_name( base_entity, "model" );
	if ( !klass )
		klass = c_runtime::find_class( "Model" );
	std::printf( "[morphine-dumper] model class: %p boneTransforms=0x%llX boneNames=0x%llX\n",
		klass,
		field_offset_by_name( klass, "boneTransforms" ),
		field_offset_by_name( klass, "boneNames" ) );
	auto write_bone = [&]( const char* name, std::int32_t value )
	{
		if ( value >= 0 )
			w.struct_field_probed( name, ( std::uint32_t )value );
		else
			w.struct_field_pending( name );
	};

	w.struct_begin( "model" );
	w.struct_field( "rootBone",           field_offset_by_name( klass, "rootBone" ) );
	w.struct_field( "headBone",           field_offset_by_name( klass, "headBone" ) );
	w.struct_field( "eyeBone",            field_offset_by_name( klass, "eyeBone" ) );
	w.struct_field( "boneTransforms",     field_offset_by_name( klass, "boneTransforms" ) );
	w.struct_field( "boneNames",          field_offset_by_name( klass, "boneNames" ) );
	w.struct_field( "bone_transform",     field_offset_by_name( klass, "boneTransforms" ) );
	w.struct_field( "bone_buffer",        field_offset_by_name( klass, "boneBuffer" ) );
	w.struct_field( "transform_data",     field_offset_by_name( klass, "transformData" ) );
	w.struct_field( "transform_position", field_offset_by_name( klass, "transformPosition" ) );
	write_bone( "pelvis_bone_idx", bones.pelvis );
	write_bone( "l_hip_bone_idx", bones.l_hip );
	write_bone( "l_knee_bone_idx", bones.l_knee );
	write_bone( "l_foot_bone_idx", bones.l_foot );
	write_bone( "r_hip_bone_idx", bones.r_hip );
	write_bone( "r_knee_bone_idx", bones.r_knee );
	write_bone( "r_foot_bone_idx", bones.r_foot );
	write_bone( "spine2_bone_idx", bones.spine2 );
	write_bone( "spine4_bone_idx", bones.spine4 );
	write_bone( "l_clavicle_bone_idx", bones.l_clavicle );
	write_bone( "l_upperarm_bone_idx", bones.l_upperarm );
	write_bone( "l_forearm_bone_idx", bones.l_forearm );
	write_bone( "l_hand_bone_idx", bones.l_hand );
	write_bone( "r_clavicle_bone_idx", bones.r_clavicle );
	write_bone( "r_upperarm_bone_idx", bones.r_upperarm );
	write_bone( "r_forearm_bone_idx", bones.r_forearm );
	write_bone( "r_hand_bone_idx", bones.r_hand );
	write_bone( "neck_bone_idx", bones.neck );
	write_bone( "head_bone_idx", bones.head );
	w.struct_field( "bone_array_count_off", 0x18 );
	w.struct_field( "bone_array_data_off",  0x20 );
	w.struct_field( "bone_array_elem_size", 0x8 );
	w.struct_end( );
}

static il2cpp::il2cpp_class_t* find_effect_network_class( )
{
	il2cpp::il2cpp_class_t* nested = nullptr;
	auto structural = find_class_with_nested_field( "effect", &nested );
	if ( structural && nested && static_class_rva_any( structural ) )
		return structural;

	auto klass = class_with_field_name_contains_type( "hitPosition", "Vector3", false );
	if ( field_offset_by_name_contains_type( klass, "hit", "position", "Vector3" ) )
		return klass;

	const char* vector_terms[] = { "Vector3" };
	klass = class_with_public_field_name_type_terms( "hitPosition", vector_terms, 1, false );
	if ( field_offset_by_name_contains_type( klass, "hit", "position", "Vector3" ) )
		return klass;

	klass = find_named_class( "EffectNetwork" );
	if ( !klass )
		klass = class_name_contains( "EffectNetwork" );
	if ( !klass )
		klass = class_name_contains_i( "EffectNetwork" );
	if ( !klass )
		klass = class_type_name_contains( "EffectNetwork" );
	if ( !klass )
		klass = class_name_contains( "Effect", "Network" );
	if ( !klass )
		klass = class_name_contains_i( "Effect", "Network" );
	if ( !klass )
		klass = class_type_name_contains( "Effect", "Network" );
	return klass;
}

static void write_effect_network( c_writer& w )
{
	auto klass = find_effect_network_class( );
	auto nested = c_runtime::get_inner_static_class( klass );
	std::uint64_t structural_instance = 0;
	if ( !klass || !nested )
	{
		il2cpp::il2cpp_class_t* structural_nested = nullptr;
		auto structural = find_class_with_nested_field( "effect", &structural_nested );
		if ( !klass )
			klass = structural;
		if ( !nested )
			nested = structural_nested;
	}
	if ( !klass || !nested )
	{
		il2cpp::il2cpp_class_t* structural_nested = nullptr;
		auto structural = find_class_with_nested_self_instance_field( &structural_nested, &structural_instance );
		if ( !klass )
			klass = structural;
		if ( !nested )
			nested = structural_nested;
	}
	auto type_info = c_runtime::static_class_rva( klass );
	if ( !type_info )
		type_info = static_class_rva_any( klass );
	auto static_type_info = nested ? c_runtime::static_class_rva( nested ) : 0;
	if ( !static_type_info )
		static_type_info = static_class_rva_any( nested );
	auto instance = field_offset_by_name( nested, "effect" );
	if ( !instance )
		instance = class_field_offset_by_name_contains( nested, "effect" );
	if ( !instance )
		instance = class_field_offset_by_type_contains( nested, "EffectNetwork" );
	if ( !instance )
		instance = structural_instance;
	auto hit_position = field_offset_by_name( klass, "hitPosition" );
	if ( !hit_position )
		hit_position = field_offset_by_name_contains_type( klass, "hit", "position", "Vector3" );

	auto class_name = klass ? klass->name( ) : nullptr;
	auto class_ns = klass ? klass->namespaze( ) : nullptr;
	auto class_type = klass ? klass->type( ) : nullptr;
	auto class_type_name = class_type ? class_type->name( ) : nullptr;
	auto nested_name = nested ? nested->name( ) : nullptr;
	auto nested_type = nested ? nested->type( ) : nullptr;
	auto nested_type_name = nested_type ? nested_type->name( ) : nullptr;

	std::printf(
		"[morphine-dumper] effect_network: class=%p static_class=%p type_info=0x%llX static_type_info=0x%llX instance=0x%llX hitPosition=0x%llX\n",
		klass,
		nested,
		type_info,
		static_type_info,
		instance,
		hit_position );
	std::printf(
		"[morphine-dumper] effect_network names: ns=%s class=%s type=%s static_class=%s static_type=%s\n",
		class_ns ? class_ns : "",
		class_name ? class_name : "",
		class_type_name ? class_type_name : "",
		nested_name ? nested_name : "",
		nested_type_name ? nested_type_name : "" );

	w.struct_begin( "effect_network" );
	w.struct_field( "type_info", type_info );
	w.struct_field( "static_type_info", static_type_info );
	w.struct_field( "static_fields", nested ? 0xB8 : 0 );
	w.struct_field( "instance", instance );
	w.struct_field( "hitPosition", hit_position );
	w.struct_end( );
}

static il2cpp::il2cpp_class_t* singleton_parent_class( const char* concrete_name )
{
	auto concrete = find_named_class( concrete_name );
	if ( !concrete )
		concrete = class_name_contains( concrete_name );
	if ( !concrete )
		return nullptr;

	auto parent = concrete->parent( );
	if ( !parent )
		return nullptr;

	auto parent_type = parent->type( );
	auto parent_type_name = parent_type ? parent_type->name( ) : nullptr;
	auto parent_name = parent->name( );
	if ( !ascii_contains_i( parent_type_name, "SingletonComponent" )
		&& !ascii_contains_i( parent_name, "SingletonComponent" ) )
		return nullptr;

	return parent;
}

static std::uint64_t singleton_parent_typeinfo_rva( const char* concrete_name )
{
	auto parent = singleton_parent_class( concrete_name );
	if ( !parent )
		return 0;
	return static_class_rva_any( parent );
}

static void write_singleton_typeinfos( c_writer& w )
{
	auto loading = class_type_name_contains( "SingletonComponent", "UI_LoadingScreen" );
	auto mixer = class_type_name_contains( "SingletonComponent", "MixerSnapshotManager" );
	auto loading_rva = static_class_rva_any( loading );
	auto mixer_rva = static_class_rva_any( mixer );
	if ( !loading_rva )
		loading_rva = singleton_parent_typeinfo_rva( "UI_LoadingScreen" );
	if ( !mixer_rva )
		mixer_rva = singleton_parent_typeinfo_rva( "MixerSnapshotManager" );
	if ( !loading_rva )
		loading_rva = static_class_rva_any( find_named_class( "UI_LoadingScreen" ) );
	if ( !mixer_rva )
		mixer_rva = static_class_rva_any( find_named_class( "MixerSnapshotManager" ) );
	if ( !loading_rva )
		loading_rva = static_class_rva_any( class_name_contains( "UI_LoadingScreen" ) );
	if ( !mixer_rva )
		mixer_rva = static_class_rva_any( class_name_contains( "MixerSnapshotManager" ) );
	if ( !mixer_rva )
		mixer_rva = static_class_rva_any( class_name_contains( "Mixer", "Snapshot" ) );
	if ( !mixer_rva )
		mixer_rva = static_class_rva_any( class_name_contains_i( "Mixer", "Snapshot" ) );

	w.struct_begin( "singleton_typeinfos" );
	w.struct_field( "SingletonComponent_UI_LoadingScreen", loading_rva );
	w.struct_field( "SingletonComponent_MixerSnapshotManager", mixer_rva );
	w.struct_end( );
}

static void write_fov( c_writer& w, std::uintptr_t fov_fn_addr )
{
	const char* resolution_list_terms[] = { "List", "Resolution" };
	auto graphics = class_with_public_static_field_type_terms( resolution_list_terms, 2 );
	if ( !graphics )
		graphics = c_runtime::find_class( "Graphics", "ConVar" );
	if ( !graphics )
		graphics = class_name_contains( "Graphics", "ConVar" );
	if ( !graphics )
		graphics = class_name_contains_i( "Graphics", "Convar" );
	if ( !graphics )
		graphics = class_with_field_name_contains_type( "fov", "Single", true );

	auto graphics_rva = static_class_rva_any( graphics );
	auto static_graphics = c_runtime::get_inner_static_class( graphics );
	auto graphics_instance = static_graphics ? field_offset_by_type_class( static_graphics, graphics ) : 0;
	if ( !graphics_instance )
	{
		const char* instance_names[] = { "instance", "Instance", "_instance" };
		graphics_instance = field_offset_by_names( static_graphics, instance_names, 3 );
	}
	if ( !graphics_instance )
		graphics_instance = class_field_offset_by_name_contains( static_graphics, "instance" );
	if ( !graphics_instance )
		graphics_instance = class_field_offset_by_type_contains( static_graphics, "Graphics" );
	if ( !graphics_instance && graphics_rva )
		graphics_instance = 0xB8;
	auto fov = field_offset_by_name( graphics, "fov" );
	if ( !fov )
		fov = field_offset_by_name( graphics, "_fov" );
	if ( !fov )
		fov = field_offset_by_name_contains_type( graphics, "fov", nullptr, "Single" );
	if ( !fov )
		if ( static_graphics )
			fov = field_offset_by_name( static_graphics, "fov" );
	if ( !fov )
		if ( static_graphics )
			fov = field_offset_by_name( static_graphics, "_fov" );
	if ( !fov )
		fov = class_field_offset_by_name_contains_type( static_graphics, "fov", nullptr, "Single" );
	if ( !fov )
		fov = fov_write_offset_from_console( );
	if ( !fov && fov_fn_addr )
		fov = fov_write_offset_from_fn( ( std::uint8_t* )fov_fn_addr, 0 );

	std::printf( "[morphine-dumper] convar graphics: class=%p rva=0x%llX instance=0x%llX fov=0x%llX\n",
		graphics, graphics_rva, graphics_instance, fov );

	// Resolve camspeed and camlerp for debug camera bypass
	auto camspeed = field_offset_by_name( graphics, "camspeed" );
	if ( !camspeed && static_graphics )
		camspeed = field_offset_by_name( static_graphics, "camspeed" );
	if ( !camspeed )
		camspeed = field_offset_by_name_contains( graphics, "camspeed" );
	if ( !camspeed && static_graphics )
		camspeed = field_offset_by_name_contains( static_graphics, "camspeed" );

	auto camlerp = field_offset_by_name( graphics, "camlerp" );
	if ( !camlerp && static_graphics )
		camlerp = field_offset_by_name( static_graphics, "camlerp" );
	if ( !camlerp )
		camlerp = field_offset_by_name_contains( graphics, "camlerp" );
	if ( !camlerp && static_graphics )
		camlerp = field_offset_by_name_contains( static_graphics, "camlerp" );

	std::printf( "[morphine-dumper] convar graphics: camspeed=0x%llX camlerp=0x%llX\n",
		camspeed, camlerp );

	w.struct_begin( "convar_graphics" );
	w.struct_field( "convar_graphics", graphics_rva );
	w.struct_field( "convar_graphics_instance", graphics_instance );
	w.struct_field( "fov",  fov );
	w.struct_field( "camspeed", camspeed );
	w.struct_field( "camlerp",  camlerp );
	w.struct_end( );
}

static void write_console_system( c_writer& w )
{
	auto find_rva = rust::console_system::console_system_index_client_find
		? ( std::uintptr_t )rust::console_system::console_system_index_client_find - c_runtime::game_base( ) : 0;

	w.struct_begin( "console_system" );
	w.struct_field( "find",        find_rva );
	w.struct_field( "get_override", rust::console_system::get_override_offset );
	w.struct_field( "set_override", rust::console_system::set_override_offset );
	w.struct_field( "call",         rust::console_system::call_offset );
	w.struct_end( );
}

static void write_convar_admin( c_writer& w )
{
	const char* player_ids_terms[] = { "UInt64" };
	il2cpp::il2cpp_class_t* server_ugc_info_class = nullptr;
	auto admin_owner = find_class_with_nested_field( "playerIds", &server_ugc_info_class );
	if ( !server_ugc_info_class )
		server_ugc_info_class = class_with_public_field_name_type_terms( "playerIds", player_ids_terms, 1, false );
	if ( !server_ugc_info_class )
		server_ugc_info_class = class_with_field_name_contains_type( "playerIds", "UInt64", false );
	if ( !server_ugc_info_class )
		server_ugc_info_class = class_name_contains( "ServerUGCInfo" );
	if ( !server_ugc_info_class )
		server_ugc_info_class = class_name_contains_i( "ServerUGC" );

	auto admin = nested_static_class_with_most_static_fields( admin_owner, server_ugc_info_class );
	if ( !admin )
		admin = class_with_public_field_name_type_terms( "playerIds", player_ids_terms, 1, true );
	if ( !admin )
		admin = class_with_field_name_contains_type( "serverUGC", nullptr, true );
	auto named_admin = c_runtime::find_class( "Admin", "ConVar" );
	if ( !named_admin )
		named_admin = class_name_contains( "Admin", "ConVar" );
	if ( !named_admin )
		named_admin = class_name_contains_i( "Admin", "Convar" );
	if ( ( !admin || !static_class_rva_any( admin ) ) && named_admin )
		admin = named_admin;

	auto admin_rva = static_class_rva_any( admin );
	auto player_ids = field_offset_by_name( server_ugc_info_class, "playerIds" );
	if ( !player_ids )
		player_ids = class_field_offset_by_name_contains( server_ugc_info_class, "player", "ids" );
	if ( !player_ids )
		player_ids = field_offset_by_name( admin, "playerIds" );
	if ( !player_ids )
		player_ids = class_field_offset_by_name_contains( admin, "player", "ids" );
	auto server_ugc_info = field_offset_by_name( admin, "serverUGCInfo" );
	if ( !server_ugc_info )
		server_ugc_info = field_offset_by_name( admin, "ServerUGCInfo" );
	if ( !server_ugc_info )
		server_ugc_info = class_field_offset_by_name_contains( admin, "server", "ugc" );
	if ( !server_ugc_info )
		server_ugc_info = class_field_offset_by_type_contains( admin, "ServerUGCInfo" );
	if ( !server_ugc_info )
		server_ugc_info = class_field_offset_by_type_contains( admin, "ServerUGC" );
	if ( !server_ugc_info && admin )
	{
		void* iter = nullptr;
		while ( auto field = admin->fields( &iter ) )
		{
			if ( field_type_matches( field, "ServerUGCInfo" ) )
			{
				server_ugc_info = field->offset( );
				break;
			}
		}
	}

	std::printf( "[morphine-dumper] convar admin: class=%p rva=0x%llX playerIds=0x%llX serverUGCInfo=0x%llX\n",
		admin, admin_rva, player_ids, server_ugc_info );

	w.struct_begin( "convar_admin" );
	w.struct_field( "convar_admin",  admin_rva );
	w.struct_field( "playerIds",     player_ids );
	w.struct_field( "serverUGCInfo", server_ugc_info );
	w.struct_end( );
}

static void write_admin_movement( c_writer& w )
{
	auto klass = c_runtime::find_class( "AdminMovement" );
	if ( !klass )
		klass = class_name_contains( "AdminMovement" );
	if ( !klass )
		klass = class_name_contains_i( "AdminMovement" );
	if ( !klass )
		klass = find_named_class( "PlayerWalkMovement" );
	auto high_friction = field_offset_by_name( klass, "highFrictionMaterial" );
	if ( !high_friction )
		high_friction = field_offset_by_type_contains_index( klass, "PhysicsMaterial", 1 );
	auto ground_angle = field_offset_by_name( klass, "groundAngle" );
	if ( !ground_angle )
		ground_angle = field_offset_by_name_contains( klass, "ground", "angle" );
	if ( !ground_angle )
		ground_angle = first_wrapper_field_after_type( klass, high_friction, "Single" );
	auto ground_angle_new = field_offset_by_name( klass, "groundAngleNew" );
	if ( !ground_angle_new )
		ground_angle_new = field_offset_by_name_contains( klass, "groundAngleNew" );
	if ( !ground_angle_new )
		ground_angle_new = field_offset_by_name_contains( klass, "angle", "new" );
	if ( !ground_angle_new )
		ground_angle_new = first_wrapper_field_after_type( klass, ground_angle, "Single" );
	auto ground_time = field_offset_by_name_contains( klass, "ground", "time" );
	if ( !ground_time )
		ground_time = first_wrapper_field_after_type( klass, ground_angle_new, "Single" );
	auto jump_time = field_offset_by_name_contains( klass, "jump", "time" );
	if ( !jump_time )
		jump_time = first_wrapper_field_after_type( klass, ground_time, "Single" );
	auto land_time = field_offset_by_name_contains( klass, "land", "time" );
	if ( !land_time )
		land_time = first_wrapper_field_after_type( klass, jump_time, "Single" );
	auto gravity_multiplier = field_offset_by_name_contains( klass, "gravity", "multiplier" );
	if ( !gravity_multiplier )
		gravity_multiplier = first_wrapper_field_after_type( klass, land_time, "Single" );
	auto target_movement = field_offset_by_name_contains( klass, "target", "movement" );
	if ( !target_movement )
		target_movement = first_wrapper_field_after_type( klass, gravity_multiplier, "Vector3" );

	w.struct_begin( "admin_movement" );
	w.struct_field( "ground_angle",     ground_angle );
	w.struct_field( "ground_angle_new", ground_angle_new );
	w.struct_field( "GroundTime",        ground_time );
	w.struct_field( "JumpTime",          jump_time );
	w.struct_field( "LandTime",          land_time );
	w.struct_field( "GravityMultiplier", gravity_multiplier );
	w.struct_field( "TargetMovement",    target_movement );
	w.struct_end( );
}

void c_core::write_decryptions( c_writer& w )
{
	if ( m_bn.decrypt_0.valid )
	{
		w.write_decryption_fn(
			"client_entities",
			m_bn.decrypt_0_addr - c_runtime::game_base( ),
			m_bn.decrypt_0,
			m_bn.hv_offset );
	}
	if ( m_bn.decrypt_1.valid )
	{
		w.write_decryption_fn(
			"entity_list",
			m_bn.decrypt_1_addr - c_runtime::game_base( ),
			m_bn.decrypt_1,
			m_bn.hv_offset );
	}
	if ( m_cl_active_item_constants.valid )
		w.write_direct_decryption_fn(
			"cl_active_item",
			m_cl_active_item_fn_addr - c_runtime::game_base( ),
			m_cl_active_item_constants );
	else
		w.write_decryption_unresolved( "cl_active_item" );
	if ( m_player_inventory_constants.valid )
		w.write_direct_decryption_fn(
			"player_inventory",
			m_player_inventory_fn_addr - c_runtime::game_base( ),
			m_player_inventory_constants );
	else
		w.write_decryption_unresolved( "player_inventory" );
	if ( m_player_eyes_constants.valid )
		w.write_direct_decryption_fn(
			"player_eyes",
			m_player_eyes_fn_addr - c_runtime::game_base( ),
			m_player_eyes_constants );
	else
		w.write_decryption_unresolved( "player_eyes" );

	if ( m_fov_constants.valid )
	{
		w.write_decrypt_fov(
			m_fov_fn_addr - c_runtime::game_base( ),
			m_fov_constants,
			m_fov_constants_are_encrypt );
		w.write_encrypt_fov(
			m_fov_fn_addr - c_runtime::game_base( ),
			m_fov_constants,
			m_fov_constants_are_encrypt );
	}
	else
	{
		w.write_fov_unresolved( );
	}
}

// ============================================================
// Extensions: anticheat, item_magazine, visibility, camspeed
// ============================================================

static void write_anticheat( c_writer& w )
{
	std::uint64_t typeinfo_rva = 0;
	std::uint64_t update_rva = 0;
	std::uint64_t sdk_loaded = 0;

	// Search for the anti-cheat class by common names
	il2cpp::il2cpp_class_t* ac_klass = nullptr;
	const char* ac_names[] = {
		"AntiCheat", "RustAntiCheat", "AnybrainSdk", "Anybrain",
		"AC", "CheatDetector", "HackDetector", "AntiHack"
	};
	for ( auto name : ac_names )
	{
		ac_klass = find_named_class( name );
		if ( ac_klass ) break;
	}
	if ( !ac_klass )
		ac_klass = class_name_contains( "AntiCheat" );
	if ( !ac_klass )
		ac_klass = class_name_contains( "Anybrain" );
	if ( !ac_klass )
		ac_klass = class_name_contains_i( "anticheat" );

	if ( ac_klass )
	{
		typeinfo_rva = metadata_typeinfo_rva( ac_klass );
		if ( !typeinfo_rva )
			typeinfo_rva = static_class_rva_any( ac_klass );

		// Find Update method
		auto update_method = il2cpp::get_method_by_name( ac_klass, "Update", 0 );
		if ( !update_method )
			update_method = il2cpp::get_method_by_name( ac_klass, "Update", 1 );
		if ( update_method )
		{
			using generic_fn_t = void(*)();
			auto fn = update_method->get_fn_ptr<generic_fn_t>();
			if ( fn )
				update_rva = ( std::uintptr_t )fn - c_runtime::game_base( );
		}

		// Find sdkLoaded field
		sdk_loaded = field_offset_by_name( ac_klass, "sdkLoaded" );
		if ( !sdk_loaded )
			sdk_loaded = field_offset_by_name_contains( ac_klass, "sdk" );
		if ( !sdk_loaded )
			sdk_loaded = field_offset_by_name_contains( ac_klass, "loaded" );
	}

	std::printf( "[morphine-dumper] anticheat: klass=%p typeinfo_rva=0x%llX update_rva=0x%llX sdkLoaded=0x%llX\n",
		ac_klass, typeinfo_rva, update_rva, sdk_loaded );

	w.struct_begin( "anticheat" );
	w.struct_field( "type_info", typeinfo_rva );
	w.struct_field( "static_fields", 0xB8 );
	w.struct_field( "sdkLoaded", sdk_loaded );
	w.struct_field( "update_method", update_rva );
	w.struct_end( );
}

static void write_item_magazine( c_writer& w )
{
	auto klass = find_named_class( "ItemMagazine" );
	if ( !klass )
		klass = class_name_contains( "Magazine" );
	if ( !klass )
		klass = find_named_class( "Magazine" );

	auto contents = field_offset_by_name( klass, "contents" );
	if ( !contents )
		contents = field_offset_by_name_contains( klass, "content" );
	auto ammo_type = field_offset_by_name( klass, "ammoType" );
	if ( !ammo_type )
		ammo_type = field_offset_by_name_contains( klass, "ammo" );
	auto amount = field_offset_by_name( klass, "amount" );
	if ( !amount )
		amount = field_offset_by_name_contains( klass, "amount" );

	w.struct_begin( "item_magazine" );
	w.struct_field( "contents", contents );
	w.struct_field( "ammoType", ammo_type );
	w.struct_field( "amount", amount );
	w.struct_end( );
}

static void write_base_entity_visibility( c_writer& w )
{
	auto klass = c_runtime::find_class( "BaseEntity" );
	if ( !klass )
		klass = find_named_class( "BaseEntity" );

	auto is_visible = field_offset_by_name( klass, "isVisible" );
	if ( !is_visible )
		is_visible = field_offset_by_name_contains( klass, "visible" );
	auto is_animator_visible = field_offset_by_name( klass, "isAnimatorVisible" );
	if ( !is_animator_visible )
		is_animator_visible = field_offset_by_name_contains( klass, "animator", "visible" );
	auto is_shadow_visible = field_offset_by_name( klass, "isShadowVisible" );
	if ( !is_shadow_visible )
		is_shadow_visible = field_offset_by_name_contains( klass, "shadow", "visible" );

	w.struct_begin( "base_entity_visibility" );
	w.struct_field( "isVisible", is_visible );
	w.struct_field( "isAnimatorVisible", is_animator_visible );
	w.struct_field( "isShadowVisible", is_shadow_visible );
	w.struct_end( );
}


bool c_core::run( const char* output_path, bool do_materials )
{
	if ( !c_runtime::init( ) )
	{
		std::printf( "[morphine-dumper] failed to init runtime\n" );
		return false;
	}
	std::printf( "[morphine-dumper] runtime: game_base=0x%llX size=0x%X\n",
		c_runtime::game_base( ), c_runtime::image_size( ) );
	load_local_il2cpp_files( );

	resolve_build_id( );

	m_gc_handles_rva = c_scanner::resolve_gc_handles( );
	std::printf( "[morphine-dumper] il2cpphandle rva = 0x%llX\n", m_gc_handles_rva );

	resolve_item_classes( );
	std::printf( "[morphine-dumper] item_class=%p item_id_class=%p\n",
		m_item_class, m_item_id_class );

	if ( resolve_console_system( ) )
		std::printf( "[morphine-dumper] console_system: find=%p get_override=0x%zX set_override=0x%zX\n",
			rust::console_system::console_system_index_client_find,
			rust::console_system::get_override_offset,
			rust::console_system::set_override_offset );
	else
		std::printf( "[morphine-dumper] console_system: failed to resolve\n" );

	if ( resolve_bn_chain( ) )
		std::printf( "[morphine-dumper] bn chain: base=0x%llX entities=0x%X parent_sf=0x%X wrapper=0x%X\n",
			m_bn.base_networkable_rva, m_bn.entities, m_bn.parent_static_fields, m_bn.wrapper_class_ptr );

	if ( resolve_camera_chain( ) )
		std::printf( "[morphine-dumper] camera: main=0x%llX object=0x%X\n",
			m_camera.main_camera_c, m_camera.camera_object );

	verify_bn_chain( );
	std::printf( "[morphine-dumper] bn chain verified: base=0x%llX entities=0x%X parent_sf=0x%X wrapper=0x%X\n",
		m_bn.base_networkable_rva,
		m_bn.entities,
		m_bn.parent_static_fields,
		m_bn.wrapper_class_ptr );

	resolve_player_inventory_decryption( );
	resolve_player_eyes_decryption( );
	resolve_player_inventory_offsets(
		m_player_inventory.main,
		m_player_inventory.belt,
		m_player_inventory.wear,
		m_item_class,
		m_item_container_class,
		m_bn.base_networkable_rva,
		m_bn.static_fields,
		m_bn.wrapper_class_ptr,
		m_bn.parent_static_fields,
		m_bn.entities,
		m_bn.hv_offset,
		m_bn.decrypt_0,
		m_bn.decrypt_1,
		m_player_inventory_constants );
	m_item_offsets = resolve_item_offsets( );
	resolve_cl_active_item_decryption( );
	resolve_model_bones( );
	resolve_base_player_live_offsets(
		m_bn.base_networkable_rva,
		m_bn.static_fields,
		m_bn.wrapper_class_ptr,
		m_bn.parent_static_fields,
		m_bn.entities,
		m_bn.hv_offset,
		m_bn.decrypt_0,
		m_bn.decrypt_1,
		m_item_class,
		m_item_container_class );

	resolve_fov_encryption( );
	if ( m_fov_constants.valid )
		std::printf( "[morphine-dumper] fov encryption: rva=0x%llX (%d ops)\n",
			m_fov_fn_addr - c_runtime::game_base( ), m_fov_constants.op_count );

	verify_all( );

	c_writer w;
	if ( !w.open( output_path ) )
	{
		std::printf( "[morphine-dumper] failed to open output file\n" );
		return false;
	}

	w.write_header( c_runtime::game_base( ), c_runtime::image_size( ) );
	w.write_build( m_build_id );
	write_il2cpp_api( w );
	write_il2cpphandle( w );
	write_klass_rvas( w, m_bn.base_networkable_rva, m_item_class, m_item_id_class, m_console_command_class );
	write_physx( w );
	write_game_manager( w );
	write_console_system( w );
	write_klass_layout( w );
	write_basenetworkable( w );
	write_camera( w );
	write_base_player( w );
	write_base_combat_entity( w );
	write_base_entity( w );
	write_base_entity_visibility( w );
	write_flag_constants( w );
	write_model_state( w );
	write_item_container( w );
	write_itemdefinition( w );
	write_item( w, m_item_class, m_item_id_class );
	write_base_projectile( w );
	write_item_mod_projectile( w );
	write_item_magazine( w );
	write_server_projectile( w );
	write_recoil_properties( w );
	write_flint_strike_weapon( w );
	write_player_eyes( w );
	write_player_input( w );
	write_player_model( w );
	write_skinned_multi_mesh( w );
	write_player_inventory( w, m_player_inventory.main, m_player_inventory.belt, m_player_inventory.wear );
	write_base_movement( w );
	write_player_walk_movement( w );
	write_world_item( w );
	write_auto_turret( w );
	write_player_corpse( w );
	write_hackable_locked_crate( w );
	write_held_entity( w, m_item_id_class );
	write_item_icon( w );
	write_view_model( w );
	write_base_view_model( w );
	write_model( w, m_model_bones );
	write_effect_network( w );
	write_singleton_typeinfos( w );
	write_tod_sky( w );
	write_unity_constants( w );
	write_fov( w, m_fov_fn_addr );
	write_convar_admin( w );
	write_admin_movement( w );
	write_anticheat( w );
	w.blank_line( );
	write_decryptions( w );

	w.close( );

	if ( do_materials )
	{
		char mat_path[ MAX_PATH ]{ };
		std::strncpy( mat_path, output_path, MAX_PATH - 1 );
		auto slash = std::strrchr( mat_path, '\\' );
		if ( slash )
			std::snprintf( slash + 1, MAX_PATH - ( slash - mat_path + 1 ), "materials.h" );
		else
			std::snprintf( mat_path, MAX_PATH, "materials.h" );

		std::printf( "[morphine-dumper] materials output: %s\n", mat_path );
		c_materials::dump( mat_path );
	}

	// Dump PhysX collision mesh for VisCheck
	{
		char tri_path[ MAX_PATH ]{ };
		std::strncpy( tri_path, output_path, MAX_PATH - 1 );
		auto slash = std::strrchr( tri_path, '\\' );
		if ( slash )
			std::snprintf( slash + 1, MAX_PATH - ( slash - tri_path + 1 ), "rust_mesh.tri" );
		else
			std::snprintf( tri_path, MAX_PATH, "rust_mesh.tri" );

		std::printf( "[morphine-dumper] physx mesh output: %s\n", tri_path );
		c_physx_dumper::dump_mesh( tri_path );
	}

	return true;
}
