#pragma once
#include "vectorMath.hpp"
#include <mutex>
#include <vector>

// Unity Bounds struct: { Vector3 center; Vector3 extents; } — 24 bytes at BaseEntity+0x17c
struct Bounds {
	Vector3 center;
	Vector3 extents;
};

// Precomputed AABB for fast ray intersection
struct OccluderAABB {
	Vector3 min;
	Vector3 max;
};

// Ray-AABB intersection using the slab method.
// Returns true if the ray hits the box at a distance < maxDist.
// hitDist is set to the distance from origin to the box surface.
inline bool RayAABBIntersect(Vector3 origin, Vector3 dir, float maxDist,
                              Vector3 bmin, Vector3 bmax, float* hitDist) {
	float tmin = 0.0f;
	float tmax = maxDist;

	for (int i = 0; i < 3; i++) {
		float o = (&origin.x)[i];
		float d = (&dir.x)[i];
		float lo = (&bmin.x)[i];
		float hi = (&bmax.x)[i];

		if (fabsf(d) < 1e-8f) {
			if (o < lo || o > hi) return false;
		} else {
			float invD = 1.0f / d;
			float t1 = (lo - o) * invD;
			float t2 = (hi - o) * invD;
			if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			if (tmin > tmax) return false;
		}
	}
	if (hitDist) *hitDist = tmin;
	return tmin >= 0.0f;
}

// Test if a single point is occluded by any AABB in the list.
// rayOrigin = camera position, target = point to test (e.g. player head)
// Returns true if occluded (wall between camera and target).
inline bool IsPointOccluded(Vector3 rayOrigin, Vector3 target,
                            const std::vector<OccluderAABB>& occluders) {
	Vector3 diff = target - rayOrigin;
	float dist = (float)diff.Length();
	if (dist < 0.1f) return false;
	Vector3 dir = diff * (1.0f / dist);

	for (const auto& aabb : occluders) {
		float hitDist;
		if (RayAABBIntersect(rayOrigin, dir, dist, aabb.min, aabb.max, &hitDist)) {
			if (hitDist < dist - 0.5f) return true;
		}
	}
	return false;
}

static inline bool Vec3Empty(Vector3 v) {
	return (v.x > -0.1f && v.x < 0.1f && v.y > -0.1f && v.y < 0.1f && v.z > -0.1f && v.z < 0.1f);
}

// Multi-point visibility test.
// If ANY of head/chest/pelvis is unoccluded, the player is considered visible.
// This handles partial cover (e.g. head visible over a low wall).
inline bool IsPlayerVisible(Vector3 camPos,
                            Vector3 head, Vector3 chest, Vector3 pelvis,
                            const std::vector<OccluderAABB>& occluders) {
	if (occluders.empty()) return true;
	if (!IsPointOccluded(camPos, head, occluders)) return true;
	if (!Vec3Empty(chest) && !IsPointOccluded(camPos, chest, occluders)) return true;
	if (!Vec3Empty(pelvis) && !IsPointOccluded(camPos, pelvis, occluders)) return true;
	return false;
}

// Overload for head-only (when skeleton not valid)
inline bool IsPlayerVisible(Vector3 camPos, Vector3 head,
                            const std::vector<OccluderAABB>& occluders) {
	if (occluders.empty()) return true;
	return !IsPointOccluded(camPos, head, occluders);
}

// Globals — written by cache thread, read by render thread
inline std::vector<OccluderAABB> g_OccluderList;
inline std::mutex g_OccluderMutex;
inline Vector3 g_CameraPos;
inline std::mutex g_CameraPosMutex;
