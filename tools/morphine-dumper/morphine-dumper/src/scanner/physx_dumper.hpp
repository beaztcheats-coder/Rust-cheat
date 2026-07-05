#pragma once

#include <cstdint>

class c_physx_dumper
{
public:
	static bool dump_mesh( const char* output_path );
	static std::uint64_t find_physics_sdk( );
};
