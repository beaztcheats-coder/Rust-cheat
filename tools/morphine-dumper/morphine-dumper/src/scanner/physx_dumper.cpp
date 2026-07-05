#include "physx_dumper.hpp"
#include "../runtime/runtime.hpp"

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>
#include <filesystem>

// PhysX struct offsets (from VisCheck.hpp / UC PhysX thread)
// These may need updating for Unity 6 — verified at runtime
namespace px {
	constexpr uint64_t PX_SCENE_DATA = 0x08;
	constexpr uint64_t PX_SCENE_SIZE = 0x10;

	constexpr uint16_t TYPE_RIGID_STATIC = 7;
	constexpr uint16_t TYPE_RIGID_DYNAMIC = 6;

	constexpr int32_t GEOM_BOX = 3;
	constexpr int32_t GEOM_TRIANGLEMESH = 5;

	constexpr uint64_t SCENE_ACTOR_DATA = 0x23C8;
	constexpr uint64_t SCENE_ACTOR_SIZE = 0x23D0;

	constexpr uint64_t ACTOR_TYPE = 0x08;
	constexpr uint64_t ACTOR_SHAPES_PTR = 0x28;
	constexpr uint64_t ACTOR_SHAPES_COUNT = 0x30;
	constexpr uint64_t ACTOR_BODY_OFFSET = 0x58;

	constexpr uint64_t BODY_CONTROL_STATE = 0x00;
	constexpr uint64_t BODY_TRANSFORM = 0x20;
	constexpr uint64_t BODY_STREAM_PTR = 0x08;
	constexpr uint64_t SHAPE_LOCAL_TRANSFORM = 0x30;
	constexpr uint64_t SHAPE_GEOMETRY = 0x108;
	constexpr uint64_t GEOM_TYPE_OFFSET = 0x00;
	constexpr uint64_t GEOM_BOX_HALF_EXTENTS = 0x08;
	constexpr uint64_t GEOM_MESH_PTR = 0x08;
	constexpr uint64_t GEOM_MESH_SCALE = 0x10;

	constexpr uint64_t MESH_NB_VERTICES = 0x28;
	constexpr uint64_t MESH_NB_TRIANGLES = 0x2C;
	constexpr uint64_t MESH_VERTICES = 0x38;
	constexpr uint64_t MESH_TRIANGLES = 0x40;
	constexpr uint64_t MESH_FLAGS = 0x50;
}

struct Vec3 { float x, y, z; };
struct Quat { float x, y, z, w; };
struct PxTransform { Quat q; Vec3 p; };
struct MeshScale { Vec3 scale; Quat rotation; };

static bool valid_float( float v ) { return std::isfinite( v ) && std::fabs( v ) < 1000000.f; }
static bool valid_vec3( const Vec3& v ) { return valid_float( v.x ) && valid_float( v.y ) && valid_float( v.z ); }

static bool valid_userland( std::uint64_t p ) {
	return p && p > 0x10000 && p < 0x7FFFFFFFFFFF;
}

// Transform a point by a PxTransform (quaternion rotation + position)
static Vec3 transform_point( const PxTransform& t, const Vec3& p ) {
	// Rotate by quaternion
	float qx = t.q.x, qy = t.q.y, qz = t.q.z, qw = t.q.w;
	float sx = 2 * ( qw * p.x + qy * p.z - qz * p.y );
	float sy = 2 * ( qw * p.y + qz * p.x - qx * p.z );
	float sz = 2 * ( qw * p.z + qx * p.y - qy * p.x );
	float rx = ( 1 - 2 * ( qy * qy + qz * qz ) ) * p.x + 2 * ( qx * qy - qw * qz ) * p.y + 2 * ( qx * qz + qw * qy ) * p.z;
	float ry = 2 * ( qx * qy + qw * qz ) * p.x + ( 1 - 2 * ( qx * qx + qz * qz ) ) * p.y + 2 * ( qy * qz - qw * qx ) * p.z;
	float rz = 2 * ( qx * qz - qw * qy ) * p.x + 2 * ( qy * qz + qw * qx ) * p.y + ( 1 - 2 * ( qx * qx + qy * qy ) ) * p.z;
	return { rx + t.p.x, ry + t.p.y, rz + t.p.z };
}

// Apply mesh scale to a vertex
static Vec3 scale_point( const MeshScale& ms, const Vec3& p ) {
	// Scale first, then rotate by scale quaternion
	Vec3 scaled = { p.x * ms.scale.x, p.y * ms.scale.y, p.z * ms.scale.z };
	PxTransform id; id.q = ms.rotation; id.p = { 0, 0, 0 };
	return transform_point( id, scaled );
}

struct TriFile { Vec3 v0, v1, v2; };

std::uint64_t c_physx_dumper::find_physics_sdk( )
{
	auto unity = ( std::uint64_t )GetModuleHandleA( "UnityPlayer.dll" );
	if ( !unity )
	{
		std::printf( "[physx] UnityPlayer.dll not found\n" );
		return 0;
	}

	auto dos = ( PIMAGE_DOS_HEADER )unity;
	auto nt = ( PIMAGE_NT_HEADERS )( unity + dos->e_lfanew );
	auto sections = IMAGE_FIRST_SECTION( nt );

	std::printf( "[physx] UnityPlayer.dll base=0x%llX\n", ( unsigned long long )unity );

	// Scan .data and .rdata sections for PxPhysics pointer
	// PxPhysics has: +0x08 = scene array ptr, +0x10 = scene count (1-8)
	int candidates_checked = 0;
	for ( int si = 0; si < nt->FileHeader.NumberOfSections; si++ )
	{
		auto& sec = sections[ si ];
		if ( !( sec.Characteristics & IMAGE_SCN_MEM_READ ) ) continue;
		if ( sec.Characteristics & IMAGE_SCN_MEM_EXECUTE ) continue; // skip .text
		if ( sec.Misc.VirtualSize < 0x1000 ) continue;

		auto secStart = unity + sec.VirtualAddress;
		auto secEnd = secStart + sec.Misc.VirtualSize;

		// Scan in 8-byte aligned steps
		for ( auto addr = secStart; addr + 0x18 <= secEnd; addr += 8 )
		{
			auto ptr = *( std::uint64_t* )addr;
			if ( !valid_userland( ptr ) ) continue;

			// Check if ptr looks like PxPhysics
			// +0x08 = scene data ptr, +0x10 = scene count
			auto sceneData = *( std::uint64_t* )( ptr + px::PX_SCENE_DATA );
			auto sceneCount = *( std::uint32_t* )( ptr + px::PX_SCENE_SIZE );

			if ( !valid_userland( sceneData ) ) continue;
			if ( sceneCount == 0 || sceneCount > 16 ) continue;

			// Verify first scene looks valid
			auto firstScene = *( std::uint64_t* )sceneData;
			if ( !valid_userland( firstScene ) ) continue;

			auto actorData = *( std::uint64_t* )( firstScene + px::SCENE_ACTOR_DATA );
			auto actorCount = *( std::uint32_t* )( firstScene + px::SCENE_ACTOR_SIZE );

			if ( !valid_userland( actorData ) ) continue;
			if ( actorCount == 0 || actorCount > 500000 ) continue;

			candidates_checked++;
			std::printf( "[physx] candidate PxPhysics at UnityPlayer+0x%llX (scenes=%u actors=%u)\n",
				( unsigned long long )( addr - unity ), sceneCount, actorCount );
			return ptr;
		}
	}

	std::printf( "[physx] PxPhysics not found (checked %d candidates)\n", candidates_checked );
	return 0;
}

bool c_physx_dumper::dump_mesh( const char* output_path )
{
	std::printf( "[physx] Starting collision mesh dump...\n" );

	auto sdk = find_physics_sdk( );
	if ( !sdk )
	{
		std::printf( "[physx] Failed to find PxPhysics SDK — cannot dump mesh\n" );
		return false;
	}

	auto sceneData = *( std::uint64_t* )( sdk + px::PX_SCENE_DATA );
	auto sceneCount = *( std::uint32_t* )( sdk + px::PX_SCENE_SIZE );

	std::printf( "[physx] SDK=0x%llX scenes=%u\n", ( unsigned long long )sdk, sceneCount );

	std::vector<TriFile> allTriangles;
	int totalActors = 0;
	int totalShapes = 0;
	int totalMeshes = 0;

	for ( std::uint32_t si = 0; si < sceneCount && si < 16; si++ )
	{
		auto scenePtr = *( std::uint64_t* )( sceneData + si * 8 );
		if ( !valid_userland( scenePtr ) ) continue;

		auto actorData = *( std::uint64_t* )( scenePtr + px::SCENE_ACTOR_DATA );
		auto actorCount = *( std::uint32_t* )( scenePtr + px::SCENE_ACTOR_SIZE );
		if ( !valid_userland( actorData ) || actorCount == 0 || actorCount > 500000 ) continue;

		std::printf( "[physx] Scene %u: %u actors\n", si, actorCount );

		for ( std::uint32_t ai = 0; ai < actorCount; ai++ )
		{
			auto actorPtr = *( std::uint64_t* )( actorData + ai * 8 );
			if ( !valid_userland( actorPtr ) ) continue;

			auto actorType = *( std::uint16_t* )( actorPtr + px::ACTOR_TYPE );
			if ( actorType != px::TYPE_RIGID_STATIC && actorType != px::TYPE_RIGID_DYNAMIC )
				continue;

			// Read actor transform
			auto bodyAddr = actorPtr + px::ACTOR_BODY_OFFSET;
			auto controlState = *( std::uint64_t* )bodyAddr;
			PxTransform actorTf;

			if ( controlState & 40 )
			{
				auto streamPtr = *( std::uint64_t* )( bodyAddr + px::BODY_STREAM_PTR );
				if ( valid_userland( streamPtr ) )
				{
					auto corePtr = *( std::uint64_t* )( streamPtr + 0xB0 );
					if ( valid_userland( corePtr ) )
					{
						actorTf = *( PxTransform* )corePtr;
					}
					else continue;
				}
				else continue;
			}
			else
			{
				actorTf = *( PxTransform* )( bodyAddr + px::BODY_TRANSFORM );
			}

			// Read shapes
			auto shapesSingle = *( std::uint64_t* )( actorPtr + px::ACTOR_SHAPES_PTR );
			auto shapeCount = *( std::uint16_t* )( actorPtr + px::ACTOR_SHAPES_COUNT );
			if ( !valid_userland( shapesSingle ) || shapeCount == 0 || shapeCount > 256 ) continue;

			std::vector<std::uint64_t> shapePtrs;
			if ( shapeCount == 1 )
			{
				shapePtrs.push_back( shapesSingle );
			}
			else
			{
				for ( int sh = 0; sh < shapeCount; sh++ )
				{
					auto sp = *( std::uint64_t* )( shapesSingle + sh * 8 );
					if ( valid_userland( sp ) ) shapePtrs.push_back( sp );
				}
			}

			for ( auto shapePtr : shapePtrs )
			{
				if ( !valid_userland( shapePtr ) ) continue;
				totalShapes++;

				auto shapeTf = *( PxTransform* )( shapePtr + px::SHAPE_LOCAL_TRANSFORM );
				auto geomAddr = shapePtr + px::SHAPE_GEOMETRY;
				auto geomType = *( std::int32_t* )( geomAddr + px::GEOM_TYPE_OFFSET );

				if ( geomType == px::GEOM_TRIANGLEMESH )
				{
					auto meshPtr = *( std::uint64_t* )( geomAddr + px::GEOM_MESH_PTR );
					if ( !valid_userland( meshPtr ) ) continue;

					MeshScale ms = *( MeshScale* )( geomAddr + px::GEOM_MESH_SCALE );
					auto nbVerts = *( std::uint32_t* )( meshPtr + px::MESH_NB_VERTICES );
					auto nbTris = *( std::uint32_t* )( meshPtr + px::MESH_NB_TRIANGLES );
					auto vertPtr = *( std::uint64_t* )( meshPtr + px::MESH_VERTICES );
					auto idxPtr = *( std::uint64_t* )( meshPtr + px::MESH_TRIANGLES );
					auto meshFlags = *( std::uint8_t* )( meshPtr + px::MESH_FLAGS );

					if ( !valid_userland( vertPtr ) || !valid_userland( idxPtr ) ) continue;
					if ( nbTris == 0 || nbTris > 1000000 || nbVerts == 0 || nbVerts > 1000000 ) continue;

					bool has16Bit = ( meshFlags & 2u ) != 0;
					totalMeshes++;

					// Read vertices
					std::vector<Vec3> verts( nbVerts );
					if ( has16Bit )
					{
						std::memcpy( verts.data( ), ( void* )vertPtr, nbVerts * sizeof( Vec3 ) );
					}
					else
					{
						std::memcpy( verts.data( ), ( void* )vertPtr, nbVerts * sizeof( Vec3 ) );
					}

					// Read indices and generate triangles
					for ( std::uint32_t ti = 0; ti < nbTris; ti++ )
					{
						std::uint32_t i0, i1, i2;
						if ( has16Bit )
						{
							auto idx16 = ( std::uint16_t* )idxPtr;
							i0 = idx16[ ti * 3 ];
							i1 = idx16[ ti * 3 + 1 ];
							i2 = idx16[ ti * 3 + 2 ];
						}
						else
						{
							auto idx32 = ( std::uint32_t* )idxPtr;
							i0 = idx32[ ti * 3 ];
							i1 = idx32[ ti * 3 + 1 ];
							i2 = idx32[ ti * 3 + 2 ];
						}

						if ( i0 >= nbVerts || i1 >= nbVerts || i2 >= nbVerts ) continue;

						Vec3 v0 = verts[ i0 ], v1 = verts[ i1 ], v2 = verts[ i2 ];

						// Apply mesh scale
						v0 = scale_point( ms, v0 );
						v1 = scale_point( ms, v1 );
						v2 = scale_point( ms, v2 );

						// Apply shape transform
						v0 = transform_point( shapeTf, v0 );
						v1 = transform_point( shapeTf, v1 );
						v2 = transform_point( shapeTf, v2 );

						// Apply actor transform
						v0 = transform_point( actorTf, v0 );
						v1 = transform_point( actorTf, v1 );
						v2 = transform_point( actorTf, v2 );

						if ( !valid_vec3( v0 ) || !valid_vec3( v1 ) || !valid_vec3( v2 ) ) continue;

						allTriangles.push_back( { v0, v1, v2 } );
					}
				}
				else if ( geomType == px::GEOM_BOX )
				{
					Vec3 halfExtents = *( Vec3* )( geomAddr + px::GEOM_BOX_HALF_EXTENTS );
					if ( halfExtents.x > 0.01f && halfExtents.y > 0.01f && halfExtents.z > 0.01f &&
						valid_vec3( halfExtents ) )
					{
						// Generate 12 triangles for a box
						Vec3 corners[ 8 ] = {
							{ -halfExtents.x, -halfExtents.y, -halfExtents.z },
							{ halfExtents.x, -halfExtents.y, -halfExtents.z },
							{ halfExtents.x, halfExtents.y, -halfExtents.z },
							{ -halfExtents.x, halfExtents.y, -halfExtents.z },
							{ -halfExtents.x, -halfExtents.y, halfExtents.z },
							{ halfExtents.x, -halfExtents.y, halfExtents.z },
							{ halfExtents.x, halfExtents.y, halfExtents.z },
							{ -halfExtents.x, halfExtents.y, halfExtents.z }
						};
						int boxTris[ 12 ][ 3 ] = {
							{ 0,1,2 },{ 0,2,3 },{ 4,6,5 },{ 4,7,6 },
							{ 0,3,7 },{ 0,7,4 },{ 1,5,6 },{ 1,6,2 },
							{ 3,2,6 },{ 3,6,7 },{ 0,4,5 },{ 0,5,1 }
						};
						for ( int bi = 0; bi < 12; bi++ )
						{
							Vec3 v0 = transform_point( actorTf, transform_point( shapeTf, corners[ boxTris[ bi ][ 0 ] ] ) );
							Vec3 v1 = transform_point( actorTf, transform_point( shapeTf, corners[ boxTris[ bi ][ 1 ] ] ) );
							Vec3 v2 = transform_point( actorTf, transform_point( shapeTf, corners[ boxTris[ bi ][ 2 ] ] ) );
							if ( valid_vec3( v0 ) && valid_vec3( v1 ) && valid_vec3( v2 ) )
								allTriangles.push_back( { v0, v1, v2 } );
						}
					}
				}
			}
			totalActors++;
		}
	}

	std::printf( "[physx] Processed %d actors, %d shapes, %d meshes\n", totalActors, totalShapes, totalMeshes );

	if ( allTriangles.empty( ) )
	{
		std::printf( "[physx] No triangles extracted — check struct offsets\n" );
		return false;
	}

	// Write .tri file
	FILE* f = std::fopen( output_path, "wb" );
	if ( !f )
	{
		std::printf( "[physx] Failed to open output: %s\n", output_path );
		return false;
	}

	auto written = std::fwrite( allTriangles.data( ), sizeof( TriFile ), allTriangles.size( ), f );
	std::fclose( f );

	float sizeMB = ( float )( allTriangles.size( ) * sizeof( TriFile ) ) / ( 1024.f * 1024.f );
	std::printf( "[physx] Wrote %zu triangles (%.1f MB) to %s\n", allTriangles.size( ), sizeMB, output_path );

	return true;
}
