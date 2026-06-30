#pragma once

#include "../Visuals/visuals.hpp"
#include <mutex>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <atomic>

struct EspCacheData {
    Vector3 headPos;
    Vector3 feetPos;
    Vector3 spine1Pos;
    Vector3 velocity;
    Vector3 bones[15];
    Vector3 localNeckPos;
    Vector3 localEyePos;
    Vector3 bodyAngles;
    std::string name;
    std::string weaponName;
    std::string animalType;
    std::string belt[6];
    std::string wear[5];
    int ammo = -1;
    bool skeletonValid = false;
    bool isDead = false;
    bool isSleeping = false;
    bool isWounded = false;
    bool isVisible = false;
    bool isVisibleRaw = false;
    bool isAabbOccluded = false; // AABB-based occlusion result (wall check)
    bool isCrouching = false;
    bool isGrounded = true;
    float health = 0.f;
    float maxHealth = 100.f;
    uint64_t teamId = 0;
    uint64_t cacheTick = 0;
    uint64_t skeletonTick = 0;
    uint64_t invTick = 0;
};
extern std::unordered_map<uintptr_t, EspCacheData> g_EspCache;
extern std::mutex g_EspCacheMutex;
extern std::unordered_map<uintptr_t, std::vector<Vector3>> g_Trails;
extern std::mutex g_TrailsMutex;

// LocalPlayer fast data — written by fast refresh thread, read by render loop (zero IOCTLs)
extern Vector3 g_LocalPlayerPos;
extern Vector3 g_LocalNeckPos;
extern Vector3 g_LocalBodyAngles;
extern Vector3 g_CameraWorldPos;
extern uint64_t g_LocalPlayerTeam;
extern std::mutex g_LocalPlayerDataMutex;

struct GhostData {
    Vector3 lastPos;
    uint64_t vanishTime;
    std::string name;
    int entityType; // 0=player, 1=npc, 2=animal
};
extern std::unordered_map<uintptr_t, GhostData> g_GhostCache;
extern std::mutex g_GhostCacheMutex;

extern std::atomic<int> g_BnStableCycles;
extern std::atomic<uint64_t> g_CacheHeartbeatMs;
extern std::atomic<uint32_t> g_CacheThreadEpoch;
extern std::atomic<uintptr_t> g_LocalPlayerAddr;
extern std::atomic<uint64_t> g_LocalPlayerGeneration;
extern std::atomic<uint64_t> g_FastRefreshHeartbeatMs;
extern std::atomic<uint32_t> g_FastRefreshEpoch;
extern std::atomic<uint64_t> g_SkeletonHeartbeatMs;
extern std::atomic<uint32_t> g_SkeletonEpoch;

// Hotbar target — computed once per frame by Do_Cheat, read by do_Visuals
extern uintptr_t g_HotbarTarget;

class cache {
public:
    std::mutex entity_mutex;
    std::mutex npc_mutex;
    std::mutex animal_mutex;
    std::mutex prefab_mutex;
    std::vector<Rust::BaseEntity*> cached_entities;
    std::vector<Rust::PrefabData> cached_prefabs;
    uint64_t last_cache_time = 0;

    void do_Cache();
    void do_FastRefresh();
    void do_PositionRefresh();
    void do_SkeletonRefresh();
    void update_cache();
};
inline cache* cac = nullptr;
