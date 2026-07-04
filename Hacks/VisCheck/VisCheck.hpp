#pragma once
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <string>
#include <cstring>
#include <windows.h>
#include <shlobj.h>
#include <mutex>
#include <atomic>
#include <numeric>
#include "../../vectorMath.hpp"
#include "../../Logger.hpp"
#include "../../Driver.hpp"

#pragma comment(lib, "shell32.lib")

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace vischeck {

// ===== Math helpers =====
inline float vdot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 vcross(const Vector3& a, const Vector3& b) {
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline Vector3 vsub(const Vector3& a, const Vector3& b) {
    return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
}

inline Vector3 vadd(const Vector3& a, const Vector3& b) {
    return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
}

inline Vector3 vmul(const Vector3& a, float s) {
    return Vector3(a.x * s, a.y * s, a.z * s);
}

inline Vector3 vnormalize(const Vector3& a) {
    float len = (float)sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    if (len < 1e-8f) return Vector3(0, 0, 0);
    return Vector3(a.x / len, a.y / len, a.z / len);
}

// ===== Quaternion rotation =====
// q = (qx, qy, qz, qw) — unit quaternion
// rotated = v + 2*cross(q.xyz, cross(q.xyz, v) + qw*v)
inline Vector3 quat_rotate(float qx, float qy, float qz, float qw, const Vector3& v) {
    Vector3 qv(qx, qy, qz);
    Vector3 t = vadd(vcross(qv, v), vmul(v, qw));
    t = vmul(t, 2.0f);
    return vadd(v, vadd(vmul(t, qw), vcross(qv, t)));
}

// ===== Bulk read helper =====
template <typename T>
std::vector<T> read_vec(uint64_t address, uint32_t count) {
    std::vector<T> result;
    if (count == 0 || !is_valid(address)) return result;
    result.resize(count);
    uint32_t bytes = count * (uint32_t)sizeof(T);
    if (g_UseInternalReads && ReadMemory_Internal((PVOID)address, result.data(), bytes))
        return result;
    if (Drv && !Drv->ioctl_blocked && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed))
        Drv->ReadMemory_ACE((PVOID)address, result.data(), bytes);
    return result;
}

// ===== PhysX POD struct for transform (28 bytes) =====
struct PxTransform {
    float qx, qy, qz, qw;
    float px, py, pz;

    Vector3 transformPoint(const Vector3& p) const {
        return vadd(quat_rotate(qx, qy, qz, qw, p), Vector3(px, py, pz));
    }
};
static_assert(sizeof(PxTransform) == 28, "PxTransform must be 28 bytes");

// ===== BVH types =====
struct Tri {
    Vector3 v0, v1, v2;
};

struct AABB {
    Vector3 mn, mx;
    AABB() : mn(1e30f, 1e30f, 1e30f), mx(-1e30f, -1e30f, -1e30f) {}
    void expand(const Vector3& p) {
        if (p.x < mn.x) mn.x = p.x;
        if (p.y < mn.y) mn.y = p.y;
        if (p.z < mn.z) mn.z = p.z;
        if (p.x > mx.x) mx.x = p.x;
        if (p.y > mx.y) mx.y = p.y;
        if (p.z > mx.z) mx.z = p.z;
    }
    void expand(const AABB& b) { expand(b.mn); expand(b.mx); }
};

struct BVHNode {
    AABB box;
    int left = -1, right = -1;
    int start = 0, count = 0;
    bool isLeaf() const { return left < 0; }
};

// ===== Intersection tests =====
static bool segmentAABB(const Vector3& o, const Vector3& d, const AABB& box) {
    float tmin = 0.0f, tmax = 1.0f;
    for (int axis = 0; axis < 3; ++axis) {
        float oa = (axis == 0) ? o.x : (axis == 1) ? o.y : o.z;
        float da = (axis == 0) ? d.x : (axis == 1) ? d.y : d.z;
        float bmin = (axis == 0) ? box.mn.x : (axis == 1) ? box.mn.y : box.mn.z;
        float bmax = (axis == 0) ? box.mx.x : (axis == 1) ? box.mx.y : box.mx.z;
        if (fabsf(da) > 1e-8f) {
            float inv = 1.0f / da;
            float t0 = (bmin - oa) * inv;
            float t1 = (bmax - oa) * inv;
            if (t0 > t1) { float tmp = t0; t0 = t1; t1 = tmp; }
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmax < tmin) return false;
        } else if (oa < bmin || oa > bmax) {
            return false;
        }
    }
    return tmax >= tmin && tmax >= 0.0f && tmin <= 1.0f;
}

static bool segmentTri(const Vector3& o, const Vector3& d, const Tri& t) {
    const float EPS = 1e-6f;
    Vector3 e1 = vsub(t.v1, t.v0);
    Vector3 e2 = vsub(t.v2, t.v0);
    Vector3 p = vcross(d, e2);
    float det = vdot(e1, p);
    if (fabsf(det) < EPS) return false;
    float invDet = 1.0f / det;
    Vector3 s = vsub(o, t.v0);
    float u = invDet * vdot(s, p);
    if (u < -EPS || u > 1.0f + EPS) return false;
    Vector3 q = vcross(s, e1);
    float v = invDet * vdot(d, q);
    if (v < -EPS || (u + v) > 1.0f + EPS) return false;
    float tt = invDet * vdot(e2, q);
    return tt > EPS && tt < 1.0f - EPS;
}

// ===== BVH engine =====
class MeshBVH {
public:
    mutable std::mutex m_mutex;

    void SetTriangles(const std::vector<Tri>& tris) {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_tris = tris;
        m_indices.clear();
        m_centroids.clear();
        m_nodes.clear();
        m_bvhBuilt = false;
        if (m_tris.empty()) return;
        BuildBVH();
    }

    bool LoadTriFile(const char* path) {
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) {
            LOG("VisCheck: failed to open %s", path);
            return false;
        }
        in.seekg(0, std::ios::end);
        size_t bytes = (size_t)in.tellg();
        in.seekg(0, std::ios::beg);
        if (bytes == 0 || bytes % sizeof(Tri) != 0) {
            LOG("VisCheck: invalid file size %zu bytes (Tri=%zu)", bytes, sizeof(Tri));
            return false;
        }
        size_t triCount = bytes / sizeof(Tri);
        std::vector<Tri> tris(triCount);
        in.read(reinterpret_cast<char*>(tris.data()), (std::streamsize)bytes);
        in.close();
        LOG("VisCheck: loaded %zu triangles from %s (%.1f MB)", triCount, path, bytes / 1048576.0);
        SetTriangles(tris);
        return m_bvhBuilt;
    }

    bool IsVisible(const Vector3& from, const Vector3& to) const {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (!m_bvhBuilt || m_nodes.empty()) return true;
        Vector3 d = vsub(to, from);
        if (fabsf(d.x) < 1e-8f && fabsf(d.y) < 1e-8f && fabsf(d.z) < 1e-8f)
            return true;
        int stack[64];
        int sp = 0;
        stack[sp++] = (int)m_nodes.size() - 1;
        while (sp > 0) {
            int ni = stack[--sp];
            const BVHNode& n = m_nodes[ni];
            if (!segmentAABB(from, d, n.box)) continue;
            if (n.isLeaf()) {
                for (int i = 0; i < n.count; ++i) {
                    if (segmentTri(from, d, m_tris[m_indices[n.start + i]]))
                        return false;
                }
            } else {
                if (sp < 63) stack[sp++] = n.left;
                if (sp < 63) stack[sp++] = n.right;
            }
        }
        return true;
    }

    bool IsReady() const { return m_bvhBuilt && !m_nodes.empty(); }
    int TriangleCount() const { return (int)m_tris.size(); }
    int NodeCount() const { return (int)m_nodes.size(); }

private:
    std::vector<Tri> m_tris;
    std::vector<int> m_indices;
    std::vector<Vector3> m_centroids;
    std::vector<BVHNode> m_nodes;
    bool m_bvhBuilt = false;

    void BuildBVH() {
        if (m_tris.empty()) return;
        m_indices.resize(m_tris.size());
        m_centroids.resize(m_tris.size());
        for (size_t i = 0; i < m_tris.size(); ++i) {
            m_indices[i] = (int)i;
            m_centroids[i] = Vector3(
                (m_tris[i].v0.x + m_tris[i].v1.x + m_tris[i].v2.x) / 3.0f,
                (m_tris[i].v0.y + m_tris[i].v1.y + m_tris[i].v2.y) / 3.0f,
                (m_tris[i].v0.z + m_tris[i].v1.z + m_tris[i].v2.z) / 3.0f
            );
        }
        m_nodes.clear();
        m_nodes.reserve(m_tris.size() * 2);
        buildRecursive(0, (int)m_tris.size());
        m_bvhBuilt = true;
        LOG("VisCheck: BVH built — %d nodes, %d tris", (int)m_nodes.size(), (int)m_tris.size());
    }

    AABB triBounds(int idx) const {
        const Tri& t = m_tris[idx];
        AABB b;
        b.expand(t.v0); b.expand(t.v1); b.expand(t.v2);
        return b;
    }

    int buildRecursive(int begin, int end) {
        BVHNode node;
        AABB bounds;
        for (int i = begin; i < end; ++i)
            bounds.expand(triBounds(m_indices[i]));
        int count = end - begin;
        if (count <= 8) {
            node.box = bounds;
            node.start = begin;
            node.count = count;
            int idx = (int)m_nodes.size();
            m_nodes.push_back(node);
            return idx;
        }
        AABB cbox;
        for (int i = begin; i < end; ++i)
            cbox.expand(m_centroids[m_indices[i]]);
        Vector3 ext = vsub(cbox.mx, cbox.mn);
        int axis = (ext.x > ext.y && ext.x > ext.z) ? 0 : (ext.y > ext.z ? 1 : 2);
        int mid = (begin + end) / 2;
        std::nth_element(
            m_indices.begin() + begin,
            m_indices.begin() + mid,
            m_indices.begin() + end,
            [&](int a, int b) -> bool {
                float va = (axis == 0) ? m_centroids[a].x : (axis == 1) ? m_centroids[a].y : m_centroids[a].z;
                float vb = (axis == 0) ? m_centroids[b].x : (axis == 1) ? m_centroids[b].y : m_centroids[b].z;
                return va < vb;
            }
        );
        int leftIdx = buildRecursive(begin, mid);
        int rightIdx = buildRecursive(mid, end);
        node.left = leftIdx;
        node.right = rightIdx;
        node.box = m_nodes[leftIdx].box;
        node.box.expand(m_nodes[rightIdx].box);
        int idx = (int)m_nodes.size();
        m_nodes.push_back(node);
        return idx;
    }
};

// ===== PhysX constants =====
// UnityPlayer.dll + PX_SDK_OFFSET → PxPhysics*
constexpr uint64_t PX_SDK_OFFSET = 0x1C3B3D0;

// PxConcreteType enum values
constexpr uint16_t PX_TYPE_RIGID_STATIC  = 7;
constexpr uint16_t PX_TYPE_RIGID_DYNAMIC = 6;

// PxGeometryType enum values
constexpr int32_t PX_GEOM_SPHERE       = 0;
constexpr int32_t PX_GEOM_PLANE        = 1;
constexpr int32_t PX_GEOM_CAPSULE      = 2;
constexpr int32_t PX_GEOM_BOX          = 3;
constexpr int32_t PX_GEOM_CONVEXMESH   = 4;
constexpr int32_t PX_GEOM_TRIANGLEMESH = 5;
constexpr int32_t PX_GEOM_HEIGHTFIELD  = 6;

// ===== PhysX memory offsets =====
// These are computed from the PhysX struct layouts (MSVC x64).
// See UC thread: https://www.unknowncheats.me/forum/rust/709796-physx-rust.html

// PxPhysics (np_physics_t)
constexpr uint64_t PX_SCENE_DATA = 0x08;  // array_t.data
constexpr uint64_t PX_SCENE_SIZE = 0x10;  // array_t.size

// Scene (np_scene_t)
constexpr uint64_t SCENE_ACTOR_DATA = 0x23C8;  // array_t.data
constexpr uint64_t SCENE_ACTOR_SIZE = 0x23D0;  // array_t.size

// Actor (px_rigid_actor_t)
constexpr uint64_t ACTOR_TYPE         = 0x08;  // uint16
constexpr uint64_t ACTOR_SHAPES_PTR   = 0x28;  // ptr_table.single (union with .list)
constexpr uint64_t ACTOR_SHAPES_COUNT = 0x30;  // ptr_table.count (uint16)

// Body (body_t starts at actor + ACTOR_BODY_OFFSET)
// Computed: sizeof(px_rigid_actor_t) = 0x58 (with alignment padding)
constexpr uint64_t ACTOR_BODY_OFFSET    = 0x58;
constexpr uint64_t BODY_CONTROL_STATE   = 0x08;  // uint64 (relative to body)
constexpr uint64_t BODY_STREAM_PTR      = 0x10;  // uint64 (relative to body)
// Transform: body + 0x20 (body_core_t, 16-aligned) + 0x10 (rigid_core_t pad) = body + 0x30
constexpr uint64_t BODY_TRANSFORM       = 0x30;  // PxTransform (28 bytes, relative to body)

// Shape (np_shape_t)
// Geometry union is at shape + SHAPE_GEOMETRY
// Computed: shape_t(0x30) + shape_core_t(0x20) + px_shape_core_t(0x20) + geometry(0x28)
//         = 0x30 + 0x20 + 0x20 + 0x28 = 0x98
constexpr uint64_t SHAPE_GEOMETRY          = 0x98;
constexpr uint64_t GEOM_TYPE_OFFSET        = 0x00;  // int32
constexpr uint64_t GEOM_BOX_HALF_EXTENTS   = 0x04;  // Vector3 (12 bytes)
constexpr uint64_t GEOM_MESH_SCALE         = 0x04;  // 28 bytes (vec3 + vec4)
constexpr uint64_t GEOM_MESH_FLAGS         = 0x20;  // uint8
constexpr uint64_t GEOM_MESH_PTR           = 0x28;  // ptr (8-byte aligned after scale+flags+pad)

// Shape local transform (px_shape_core_t.m_transform)
// At shape + 0x30(shape_t) + 0x20(shape_core_t) + 0x00(px_shape_core_t) = shape + 0x50
// But shape_core_t has alignas(16) on m_core, so m_core is at shape_core_t + 0x20
// px_shape_core_t starts at shape_core_t + 0x20 = shape + 0x30 + 0x20 = shape + 0x50
// m_transform at px_shape_core_t + 0x00 = shape + 0x50
// Wait — shape_core_t is at shape_t + 0x20 (due to 16-byte alignment of body_core_t)
// Actually: shape_t has m_scene(8) + m_control_state(4) + pad(4) + m_stream_ptr(8) = 0x18
// Then m_shape_core (shape_core_t, align 16) at shape_t + 0x20 (padded from 0x18)
// shape_core_t: query_filter(16) + sim_filter(16) + m_core(alignas 16) at +0x20
// px_shape_core_t: m_transform(alignas 16) at +0x00
// So m_transform = shape + 0x30 + 0x20 + 0x20 + 0x00 = shape + 0x70
constexpr uint64_t SHAPE_LOCAL_TRANSFORM = 0x70;  // PxTransform (28 bytes)

// TriangleMesh (triangle_mesh_t)
constexpr uint64_t MESH_NB_VERTICES  = 0x20;  // uint32
constexpr uint64_t MESH_NB_TRIANGLES = 0x24;  // uint32
constexpr uint64_t MESH_VERTICES     = 0x28;  // ptr (Vector3*)
constexpr uint64_t MESH_TRIANGLES    = 0x30;  // ptr (void*)
constexpr uint64_t MESH_FLAGS        = 0x5C;  // uint8 (bit 1 = 16-bit indices)

// Mesh scale (px_mesh_scale_t): vec3 scale(12) + vec4 rotation(16) = 28 bytes
struct MeshScale {
    float sx, sy, sz;
    float rx, ry, rz, rw;

    Vector3 transform(const Vector3& v) const {
        return vadd(quat_rotate(rx, ry, rz, rw, v), Vector3(0, 0, 0));
    }

    Vector3 scaleOnly(const Vector3& v) const {
        return Vector3(v.x * sx, v.y * sy, v.z * sz);
    }
};
static_assert(sizeof(MeshScale) == 28, "MeshScale must be 28 bytes");

// ===== Triangle generators =====

std::vector<Tri> generate_box_triangles(const PxTransform& actorTransform,
                                         const PxTransform& shapeTransform,
                                         const Vector3& halfExtents) {
    std::vector<Tri> tris;
    // 8 corners in local space
    Vector3 corners[8];
    int idx = 0;
    for (int x : {-1, 1}) {
        for (int y : {-1, 1}) {
            for (int z : {-1, 1}) {
                Vector3 local(x * halfExtents.x, y * halfExtents.y, z * halfExtents.z);
                // Apply shape local transform, then actor global transform
                Vector3 shapeSpace = shapeTransform.transformPoint(local);
                Vector3 world = actorTransform.transformPoint(shapeSpace);
                corners[idx++] = world;
            }
        }
    }

    // 12 triangles (6 faces × 2)
    static const int faces[12][3] = {
        {0,1,2}, {1,3,2},  // -Z
        {4,6,5}, {5,6,7},  // +Z
        {0,2,4}, {2,6,4},  // -X
        {1,5,3}, {3,5,7},  // +X
        {0,4,1}, {1,4,5},  // -Y
        {2,3,6}, {3,7,6}   // +Y
    };

    tris.reserve(12);
    for (const auto& f : faces) {
        Vector3 v0 = corners[f[0]];
        Vector3 v1 = corners[f[1]];
        Vector3 v2 = corners[f[2]];
        // Skip degenerate triangles
        Vector3 normal = vcross(vsub(v1, v0), vsub(v2, v0));
        float lenSq = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
        if (lenSq < 1e-6f) continue;
        tris.push_back({v0, v1, v2});
    }
    return tris;
}

std::vector<Tri> generate_triangle_mesh_triangles(const PxTransform& actorTransform,
                                                   const PxTransform& shapeTransform,
                                                   uint64_t meshPtr,
                                                   const MeshScale& scale) {
    std::vector<Tri> triangles;
    if (!meshPtr || !is_valid(meshPtr)) return triangles;

    uint32_t nbVerts = read<uint32_t>(meshPtr + MESH_NB_VERTICES);
    uint32_t nbTris  = read<uint32_t>(meshPtr + MESH_NB_TRIANGLES);
    uint64_t vertPtr = read<uint64_t>(meshPtr + MESH_VERTICES);
    uint64_t idxPtr  = read<uint64_t>(meshPtr + MESH_TRIANGLES);
    uint8_t  meshFlags = read<uint8_t>(meshPtr + MESH_FLAGS);

    if (nbVerts == 0 || nbTris == 0 || !vertPtr || !idxPtr) return triangles;
    if (nbVerts > 5000000 || nbTris > 10000000) return triangles; // sanity limit

    // Read vertices (bulk)
    auto vertices = read_vec<Vector3>(vertPtr, nbVerts);
    if (vertices.size() != nbVerts) return triangles;

    // Read indices
    std::vector<uint32_t> indices;
    bool has16Bit = (meshFlags & 2u) != 0;

    if (has16Bit) {
        auto smallIdx = read_vec<uint16_t>(idxPtr, nbTris * 3);
        if (smallIdx.size() != nbTris * 3) return triangles;
        indices.reserve(smallIdx.size());
        for (auto i : smallIdx) indices.push_back((uint32_t)i);
    } else {
        indices = read_vec<uint32_t>(idxPtr, nbTris * 3);
        if (indices.size() != nbTris * 3) return triangles;
    }

    triangles.reserve(nbTris);
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        if (i0 >= nbVerts || i1 >= nbVerts || i2 >= nbVerts) continue;

        const Vector3& v0 = vertices[i0];
        const Vector3& v1 = vertices[i1];
        const Vector3& v2 = vertices[i2];

        // Apply mesh scale, then shape transform, then actor transform
        Vector3 s0 = scale.scaleOnly(v0);
        Vector3 s1 = scale.scaleOnly(v1);
        Vector3 s2 = scale.scaleOnly(v2);

        Vector3 w0 = actorTransform.transformPoint(shapeTransform.transformPoint(s0));
        Vector3 w1 = actorTransform.transformPoint(shapeTransform.transformPoint(s1));
        Vector3 w2 = actorTransform.transformPoint(shapeTransform.transformPoint(s2));

        // Skip degenerate
        Vector3 normal = vcross(vsub(w1, w0), vsub(w2, w0));
        float lenSq = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
        if (lenSq < 1e-6f) continue;

        triangles.push_back({w0, w1, w2});
    }
    return triangles;
}

// ===== BVH globals =====
inline MeshBVH g_VisCheck;
inline bool g_VisCheckLoaded = false;
inline std::string g_VisCheckStatus = "Not loaded";

// ===== PhysX scene reader =====
inline uint64_t g_UnityPlayerBase = 0;
inline uint64_t g_LastPhysXCacheMs = 0;
inline int g_PhysXActorCount = 0;
inline int g_PhysXTriCount = 0;
inline bool g_PhysXAttempted = false;

PxTransform readActorTransform(uint64_t actorPtr) {
    uint64_t bodyAddr = actorPtr + ACTOR_BODY_OFFSET;
    uint64_t controlState = read<uint64_t>(bodyAddr + BODY_CONTROL_STATE);

    if (controlState & 40) {
        // Buffered — read from stream
        uint64_t streamPtr = read<uint64_t>(bodyAddr + BODY_STREAM_PTR);
        if (streamPtr && is_valid(streamPtr)) {
            uint64_t corePtr = read<uint64_t>(streamPtr + 0xB0);
            if (corePtr && is_valid(corePtr)) {
                return read<PxTransform>(corePtr);
            }
        }
    }
    // Read directly from body
    return read<PxTransform>(bodyAddr + BODY_TRANSFORM);
}

void CachePhysXActors() {
    if (!g_UnityPlayerBase) return;
    if (g_PhysXAttempted) return; // Only try once — no retry to prevent driver instability

    uint64_t sdk = read<uint64_t>(g_UnityPlayerBase + PX_SDK_OFFSET);
    if (!sdk || !is_valid(sdk)) {
        LOG("VisCheck PhysX: SDK ptr null at UnityPlayer+0x%llX", (unsigned long long)PX_SDK_OFFSET);
        g_PhysXAttempted = true;
        return;
    }

    uint64_t sceneData = read<uint64_t>(sdk + PX_SCENE_DATA);
    uint32_t sceneSize = read<uint32_t>(sdk + PX_SCENE_SIZE);
    if (!sceneData || sceneSize == 0 || sceneSize > 64) {
        LOG("VisCheck PhysX: no scenes (data=0x%llX size=%u)", (unsigned long long)sceneData, sceneSize);
        g_PhysXAttempted = true;
        return;
    }

    auto scenePtrs = read_vec<uint64_t>(sceneData, sceneSize);
    if (scenePtrs.empty()) {
        g_PhysXAttempted = true;
        return;
    }

    std::vector<Tri> allTriangles;
    int actorCount = 0;

    for (uint64_t scenePtr : scenePtrs) {
        if (!scenePtr || !is_valid(scenePtr)) continue;
        if (MISC::ShutdownRequested) return;

        uint64_t actorData = read<uint64_t>(scenePtr + SCENE_ACTOR_DATA);
        uint32_t actorSize = read<uint32_t>(scenePtr + SCENE_ACTOR_SIZE);
        if (!actorData || actorSize == 0 || actorSize > 100000) continue;

        auto actorPtrs = read_vec<uint64_t>(actorData, actorSize);
        if (actorPtrs.empty()) continue;

        for (uint64_t actorPtr : actorPtrs) {
            if (!actorPtr || !is_valid(actorPtr)) continue;
            if (MISC::ShutdownRequested) return;

            uint16_t actorType = read<uint16_t>(actorPtr + ACTOR_TYPE);
            // Process BOTH rigid_static AND rigid_dynamic
            // (UC code only processed dynamic — that's a bug for Rust building blocks)
            if (actorType != PX_TYPE_RIGID_STATIC && actorType != PX_TYPE_RIGID_DYNAMIC)
                continue;

            // Read actor global transform
            PxTransform actorTransform = readActorTransform(actorPtr);

            // Read shapes
            uint64_t shapesSingle = read<uint64_t>(actorPtr + ACTOR_SHAPES_PTR);
            uint16_t shapeCount = read<uint16_t>(actorPtr + ACTOR_SHAPES_COUNT);

            if (!shapesSingle || shapeCount == 0) continue;
            if (shapeCount > 256) continue; // sanity limit

            // Get shape pointers
            std::vector<uint64_t> shapePtrs;
            if (shapeCount == 1) {
                shapePtrs.push_back(shapesSingle);
            } else {
                shapePtrs = read_vec<uint64_t>(shapesSingle, shapeCount);
            }

            for (uint64_t shapePtr : shapePtrs) {
                if (!shapePtr || !is_valid(shapePtr)) continue;

                // Read shape local transform
                PxTransform shapeTransform = read<PxTransform>(shapePtr + SHAPE_LOCAL_TRANSFORM);

                // Read geometry type
                uint64_t geomAddr = shapePtr + SHAPE_GEOMETRY;
                int32_t geomType = read<int32_t>(geomAddr + GEOM_TYPE_OFFSET);

                switch (geomType) {
                    case PX_GEOM_BOX: {
                        Vector3 halfExtents = read<Vector3>(geomAddr + GEOM_BOX_HALF_EXTENTS);
                        if (halfExtents.x > 0.01f && halfExtents.y > 0.01f && halfExtents.z > 0.01f) {
                            auto boxTris = generate_box_triangles(actorTransform, shapeTransform, halfExtents);
                            allTriangles.insert(allTriangles.end(), boxTris.begin(), boxTris.end());
                        }
                        break;
                    }
                    case PX_GEOM_TRIANGLEMESH: {
                        MeshScale scale = read<MeshScale>(geomAddr + GEOM_MESH_SCALE);
                        uint64_t meshPtr = read<uint64_t>(geomAddr + GEOM_MESH_PTR);
                        if (meshPtr && is_valid(meshPtr)) {
                            auto meshTris = generate_triangle_mesh_triangles(
                                actorTransform, shapeTransform, meshPtr, scale);
                            allTriangles.insert(allTriangles.end(), meshTris.begin(), meshTris.end());
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            actorCount++;
        }
    }

    g_PhysXAttempted = true;

    if (allTriangles.empty()) {
        LOG("VisCheck PhysX: no triangles collected (actors=%d scenes=%u)", actorCount, (unsigned)sceneSize);
        return;
    }

    // Filter out triangles with invalid coordinates
    allTriangles.erase(
        std::remove_if(allTriangles.begin(), allTriangles.end(),
            [](const Tri& t) {
                auto bad = [](const Vector3& v) {
                    return v.x != v.x || v.y != v.y || v.z != v.z; // NaN check
                };
                return bad(t.v0) || bad(t.v1) || bad(t.v2);
            }),
        allTriangles.end());

    if (allTriangles.empty()) {
        LOG("VisCheck PhysX: all triangles were invalid after filtering");
        return;
    }

    g_PhysXActorCount = actorCount;
    g_PhysXTriCount = (int)allTriangles.size();

    g_VisCheck.SetTriangles(allTriangles);

    if (!g_VisCheckLoaded) {
        g_VisCheckLoaded = true;
        LOG("VisCheck PhysX: loaded %d triangles from %d actors", g_PhysXTriCount, actorCount);
    }
    g_VisCheckStatus = "PhysX: " + std::to_string(g_PhysXTriCount) + " tris (" +
                        std::to_string(actorCount) + " actors)";
    g_LastPhysXCacheMs = GetTickCount64();
}

// ===== .tri file fallback =====
static void GetDllDir(char* buf, size_t bufSize) {
    HMODULE self = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&GetDllDir, &self)) {
        buf[0] = 0;
        return;
    }
    char path[MAX_PATH] = {};
    if (!GetModuleFileNameA(self, path, MAX_PATH)) {
        buf[0] = 0;
        return;
    }
    size_t len = strlen(path);
    while (len > 0 && path[len-1] != '\\' && path[len-1] != '/') --len;
    if (len >= bufSize) len = bufSize - 1;
    memcpy(buf, path, len);
    buf[len] = 0;
}

static void GetLocalAppDataDir(char* buf, size_t bufSize) {
    if (FAILED(SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, buf))) {
        buf[0] = 0;
    }
}

static bool TryLoadTriFile() {
    char dllDir[MAX_PATH] = {};
    GetDllDir(dllDir, sizeof(dllDir));

    char localAppData[MAX_PATH] = {};
    GetLocalAppDataDir(localAppData, sizeof(localAppData));

    char dllTriPath[MAX_PATH] = {};
    if (dllDir[0]) {
        _snprintf_s(dllTriPath, MAX_PATH, _TRUNCATE, "%srust_mesh.tri", dllDir);
    }

    char localAppDataTriPath[MAX_PATH] = {};
    if (localAppData[0]) {
        _snprintf_s(localAppDataTriPath, MAX_PATH, _TRUNCATE, "%s\\rust_mesh.tri", localAppData);
    }

    const char* paths[] = {
        dllTriPath[0] ? dllTriPath : nullptr,
        localAppDataTriPath[0] ? localAppDataTriPath : nullptr,
        "C:\\rust_mesh.tri",
        "C:\\rust_meshes.tri",
        nullptr
    };

    for (int i = 0; paths[i]; ++i) {
        LOG("VisCheck: trying .tri file %s", paths[i]);
        if (g_VisCheck.LoadTriFile(paths[i])) {
            g_VisCheckLoaded = true;
            g_VisCheckStatus = "File: " + std::to_string(g_VisCheck.TriangleCount()) + " tris";
            return true;
        }
    }
    return false;
}

// ===== Init / cache cycle =====

// Called once at startup — tries PhysX (known offset only), falls back to .tri file
inline void InitVisCheck() {
    if (g_VisCheckLoaded) return;

    // Try PhysX (single attempt, known offset only — no scanning)
    CachePhysXActors();

    if (!g_VisCheckLoaded) {
        LOG("VisCheck: PhysX not available, trying .tri file fallback");
        TryLoadTriFile();
    }

    if (!g_VisCheckLoaded) {
        g_VisCheckStatus = "No mesh data (PhysX or .tri)";
        LOG("VisCheck: no mesh data found — VisCheck disabled");
    }
}

}
