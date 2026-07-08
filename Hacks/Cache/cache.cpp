#include "cache.hpp"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <thread>
#include <filesystem>
#include <string>
#include <vector>
#include <d3d11.h>
#include "../../Logger.hpp"
#include "../../Translation.hpp"
#include "../VisCheck/VisCheck.hpp"

#pragma comment(lib, "d3d11.lib")

std::unordered_map<uintptr_t, EspCacheData> g_EspCache;
std::shared_mutex g_EspCacheMutex;
std::unordered_map<uintptr_t, std::vector<Vector3>> g_Trails;
std::mutex g_TrailsMutex;
std::unordered_map<uintptr_t, GhostData> g_GhostCache;
std::mutex g_GhostCacheMutex;
Vector3 g_LocalPlayerPos;
Vector3 g_LocalNeckPos;
Vector3 g_LocalBodyAngles;
Vector3 g_CameraWorldPos;
uint64_t g_LocalPlayerTeam = 0;
std::mutex g_LocalPlayerDataMutex;
std::atomic<int> g_BnStableCycles{ 0 };
std::atomic<uint64_t> g_CacheHeartbeatMs{ 0 };
std::atomic<int> g_CacheLastEntity{ 0 };
std::atomic<int> g_CacheStep{ 0 };

std::atomic<uint32_t> g_CacheThreadEpoch{ 0 };
std::atomic<uintptr_t> g_LocalPlayerAddr{ 0 };
std::atomic<uint64_t> g_LocalPlayerGeneration{ 1 };
std::atomic<uintptr_t> g_LocalPlayerEyesPtr{ 0 };
std::atomic<uint64_t> g_FastRefreshHeartbeatMs{ 0 };
std::atomic<uint32_t> g_FastRefreshEpoch{ 0 };
std::atomic<uint64_t> g_SkeletonHeartbeatMs{ 0 };
std::atomic<uint32_t> g_SkeletonEpoch{ 0 };
uintptr_t g_HotbarTarget = 0;

namespace {
    constexpr bool kCacheVerboseLogs = false;
}

void cache::do_Cache() {
    static bool first_log = true;
    static int failCount = 0;
    const uint32_t threadEpoch = g_CacheThreadEpoch.load(std::memory_order_acquire);

    struct PrefabNameCacheEntry { uintptr_t key; char name[128]; uint64_t cycle; };
    static PrefabNameCacheEntry s_prefabNameCache[256];
    static int s_prefabNameCacheCount = 0;

    auto publishLocalPlayer = [&](Rust::BaseEntity* lp) {
        uintptr_t newAddr = (uintptr_t)lp;
        uintptr_t oldAddr = g_LocalPlayerAddr.exchange(newAddr, std::memory_order_acq_rel);
        if (oldAddr != newAddr) {
            g_LocalPlayerGeneration.fetch_add(1, std::memory_order_acq_rel);
        }
        if (src) src->LocalPlayer = lp;
    };

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (MISC::ShutdownRequested) {
            LOG("CACHE: ShutdownRequested — exiting do_Cache");
            return;
        }
        static uint64_t cacheCycleCount = 0;
        cacheCycleCount++;
        if (threadEpoch != g_CacheThreadEpoch.load(std::memory_order_acquire)) {
            LOG("CACHE: epoch change detected, exiting old worker (epoch=%u)", threadEpoch);
            return;
        }
        g_CacheHeartbeatMs.store(GetTickCount64(), std::memory_order_release);

        static bool vischeckInit = false;
        if (!vischeckInit) {
            vischeckInit = true;
            vischeck::g_UnityPlayerBase = UnityPlayer;
            if (ESP::VisMode > 0) {
                vischeck::InitVisCheck();
            } else {
                LOG("VisCheck: VisMode=0 (raw flag), skipping mesh data load");
            }
        }

        // === Process-death detection: if BN chain keeps failing, check if Rust is still alive ===
        if (failCount > 0 && (failCount % 20) == 0) {
            if (Drv && !Drv->IsProcessAlive()) {
                LOG("CACHE: RustClient.exe no longer running (failCount=%d) — shutting down cheat to prevent BSOD", failCount);
                g_process_dead.store(true, std::memory_order_relaxed);
                if (Drv) Drv->ioctl_blocked.store(true, std::memory_order_relaxed);
                MISC::ShutdownRequested = true;
                return;
            }
        }

        auto markUnavailable = [&]() {
            if (Drv && Drv->ioctl_blocked.load(std::memory_order_relaxed)) {
                return;
            }
            publishLocalPlayer(nullptr);
            g_BnStableCycles.store(0, std::memory_order_relaxed);

            {
                std::lock_guard<std::mutex> lk(cac->entity_mutex);
                entity_List.clear();
            }
            {
                std::lock_guard<std::mutex> lk(cac->npc_mutex);
                npc_List.clear();
            }
            {
                std::lock_guard<std::mutex> lk(cac->animal_mutex);
                animal_List.clear();
            }
            {
                std::lock_guard<std::mutex> lk(cac->prefab_mutex);
                prefab_List.clear();
            }
            {
                std::unique_lock<std::shared_mutex> lk(g_EspCacheMutex);
                g_EspCache.clear();
            }
            s_prefabNameCacheCount = 0;
        };

        uint64_t basenetworkable = read<uint64_t>(GameAssembly + offsets::basenetworkable_pointer);
        if (!basenetworkable || !is_valid(basenetworkable)) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: basenetworkable=0 or invalid (GA+0x%I64X)", offsets::basenetworkable_pointer);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t bn_static_fields = read<uint64_t>(basenetworkable + offsets::BaseNetworkable::static_fields);
        if (!bn_static_fields || !is_valid(bn_static_fields)) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: static_fields=0 or invalid (bn=0x%I64X +0x%I64X)", basenetworkable, offsets::BaseNetworkable::static_fields);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t wrapper_class_ptr = read<uint64_t>(bn_static_fields + offsets::BaseNetworkable::wrapper_class);
        if (!wrapper_class_ptr || !is_valid(wrapper_class_ptr)) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: wrapper_class_ptr=0 or invalid (sf=0x%I64X +0x%I64X)", bn_static_fields, offsets::BaseNetworkable::wrapper_class);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t wrapper_class = decrypt::networkable_key(wrapper_class_ptr);
        if (!wrapper_class) {
            markUnavailable();
            uint64_t raw_enc = 0;
            if (wrapper_class_ptr && wrapper_class_ptr > 0x10000)
                raw_enc = read<uint64_t>(wrapper_class_ptr + 0x18);
            if (++failCount == 1) LOG("BN chain FAIL: decrypt networkable_key returned 0 (input=0x%I64X raw_enc=0x%I64X)", wrapper_class_ptr, raw_enc);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t parent_sf = read<uint64_t>(wrapper_class + offsets::BaseNetworkable::parent_static_fields);
        if (!parent_sf || !is_valid(parent_sf)) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: parent_static_fields=0 or invalid (wc=0x%I64X +0x%I64X)", wrapper_class, offsets::BaseNetworkable::parent_static_fields);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t parent_class = decrypt::networkable_key2(parent_sf);
        if (!parent_class) {
            markUnavailable();
            uint64_t raw_enc2 = 0;
            if (parent_sf && parent_sf > 0x10000)
                raw_enc2 = read<uint64_t>(parent_sf + 0x18);
            if (++failCount == 1) LOG("BN chain FAIL: decrypt networkable_key2 returned 0 (input=0x%I64X raw_enc=0x%I64X)", parent_sf, raw_enc2);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t entity = read<uint64_t>(parent_class + offsets::BaseNetworkable::entity);
        if (!entity || !is_valid(entity)) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: entity(BufferList)=0 or invalid (pc=0x%I64X +0x%I64X)", parent_class, offsets::BaseNetworkable::entity);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        auto entity_size = read<uint32_t>(entity + 0x18);
        if (!entity_size) {
            markUnavailable();
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }
        if (entity_size > 50000) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: entity_size unreasonable (%u)", entity_size);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }
        // Cap entity iteration to prevent excessive IOCTL cycles.
        // 8000 covers all entities on any server (players + animals + NPCs + nearby items).
        if (entity_size > 8000) {
            static int capWarnTick = 0;
            if ((++capWarnTick % 30) == 0)
                LOG("CACHE: entity_size=%u, capping at 8000", entity_size);
            entity_size = 8000;
        }

        // Stability check: if entity_size dropped >40% from last successful cycle, skip walk
        // This prevents crashes when the game swaps/reorganizes the entity list
        static uint32_t lastStableSize = 0;
        static uint32_t stabilizationDelay = 0;
        if (lastStableSize > 0 && entity_size < (uint32_t)(lastStableSize * 0.6f)) {
            markUnavailable();
            if (stabilizationDelay < 3) {
                stabilizationDelay++;
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
                continue;
            }
        }
        if (stabilizationDelay > 0) stabilizationDelay--;
        lastStableSize = entity_size;

        auto entity_array = read<uint64_t>(entity + 0x10);
        if (!entity_array || !is_valid(entity_array)) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: entity_array=0 or invalid (entity=0x%I64X +0x10)", entity);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        Rust::BaseEntity* localPlayer = read<Rust::BaseEntity*>((uint64_t)entity_array + 0x20);

        if (!localPlayer) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: localPlayer=0 at entity_array+0x20");
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        if (!is_valid((uintptr_t)localPlayer)) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: localPlayer invalid (0x%I64X)", (uint64_t)localPlayer);
            continue;
        }

        publishLocalPlayer(localPlayer);

        // BN chain succeeded — reset fail counter
        static bool bnFirstSuccess = false;
        if (!bnFirstSuccess) {
            bnFirstSuccess = true;
            LOG("BN chain SUCCESS: entity_size=%u localPlayer=0x%I64X array=0x%I64X", entity_size, (uint64_t)localPlayer, entity_array);
        }
        if (failCount > 0) {
            LOG("BN chain RECOVERED after %d failures", failCount);
            failCount = 0;
        }

        // Periodic CR3 re-validation: every ~3 cycles, refresh CR3 and verify process is alive
        if ((cacheCycleCount % 3) == 0 && Drv) {
            if (!Drv->IsProcessAlive()) {
                LOG("CACHE: RustClient.exe died during normal operation — shutting down to prevent BSOD");
                g_process_dead.store(true, std::memory_order_relaxed);
                Drv->ioctl_blocked.store(true, std::memory_order_relaxed);
                MISC::ShutdownRequested = true;
                return;
            }
            uint64_t current_cr3 = 0;
            if (Drv->GetCR3_1(Drv->processid, false, current_cr3) && current_cr3 != Drv->process_cr3) {
                LOG("CACHE: CR3 changed (old=0x%I64X new=0x%I64X) — updating", Drv->process_cr3, current_cr3);
                Drv->process_cr3 = current_cr3;
            }
        }
        int stable = g_BnStableCycles.load(std::memory_order_relaxed);
        if (stable < 1000) g_BnStableCycles.store(stable + 1, std::memory_order_relaxed);

        if (first_log) {
            LOG("BN chain SUCCESS!");
            LOG_HEX("  basenetworkable", basenetworkable);
            LOG_HEX("  static_fields", bn_static_fields);
            LOG_HEX("  wrapper_class_ptr", wrapper_class_ptr);
            LOG_HEX("  wrapper_class(decrypted)", wrapper_class);
            LOG_HEX("  parent_sf", parent_sf);
            LOG_HEX("  parent_class(decrypted)", parent_class);
            LOG_HEX("  entity(BufferList)", entity);
            LOG("  entity_size: %u", entity_size);
            LOG_HEX("  entity_array", entity_array);
            LOG_HEX("  LocalPlayer", (uint64_t)localPlayer);

            // Track entity_size changes across cycles for BN chain diagnostics
            static uint32_t lastSz = 0;
            static int szCycleCount = 0;
            szCycleCount++;
            if (entity_size != lastSz) {
                LOG("CACHE entity_size changed: %u -> %u (cycle %d)", lastSz, entity_size, szCycleCount);
                lastSz = entity_size;
            }

            if (kCacheVerboseLogs) {
                LOG("=== CAMERA SCAN (game loaded, scanning all offsets) ===");

                uint64_t cam_typeinfos[] = { offsets::camera_pointer, offsets::cam_typeinfo_singleton, offsets::cam_typeinfo_camera_c };
                const char* cam_names[] = { "MainCamera", "SingletonComponent<MainCamera>", "Camera_c" };
                for (int t = 0; t < 3; t++) {
                    uint64_t ti = read<uint64_t>(GameAssembly + cam_typeinfos[t]);
                    LOG("--- %s (GA+0x%I64X) TypeInfo=0x%I64X ---", cam_names[t], cam_typeinfos[t], ti);
                    if (!ti || ti < 0x10000) { LOG("  INVALID TypeInfo, skipping"); continue; }
                    uint64_t sf = read<uint64_t>(ti + offsets::BaseCamera::static_fields);
                    LOG("  static_fields (+0x%B8) = 0x%I64X", (uint64_t)offsets::BaseCamera::static_fields, sf);
                    if (!sf || sf < 0x10000) { LOG("  INVALID static_fields, skipping"); continue; }
                    int limit = (t == 1) ? 0x40 : 0x200;
                    for (int off = 0; off <= limit; off += 8) {
                        uint64_t val = read<uint64_t>(sf + off);
                        if (val != 0 && val > 0x10000) {
                            uint64_t test_entity = read<uint64_t>(val + offsets::BaseCamera::entity);
                            uint64_t test_matrix_check = read<uint64_t>(val + offsets::BaseCamera::viewMatrix);
                            LOG("  +0x%02X = 0x%I64X (->+0x%X=0x%I64X, ->+0x%X=0x%I64X)", off, val, (uint64_t)offsets::BaseCamera::entity, test_entity, (uint64_t)offsets::BaseCamera::viewMatrix, test_matrix_check);
                        }
                    }
                }
                LOG("=== END CAMERA SCAN ===");
            }

            first_log = false;
        }

        std::vector<Rust::BaseEntity*> tempList;
        std::vector<Rust::BaseEntity*> tempNpcList;
        std::vector<Rust::BaseEntity*> tempAnimalList;
        std::vector<Rust::PrefabData> tempPrefabList;
        std::vector<OccluderAABB> tempOccluders;
        std::unordered_map<uintptr_t, EspCacheData> tempEspCache;
        int classifiedPlayers = 0;
        int classifiedNpcs = 0;
        int classifiedAnimals = 0;
        std::string samplePlayer;
        std::string sampleNpc;

        Vector3 localPos;
        if (localPlayer) {
            localPos = localPlayer->Get_ObjectPosition(); // verified working chain
            // One-time diagnostic: compare PlayerEyes vs transform-chain position
            static bool posDiag = false;
            if (kCacheVerboseLogs && !posDiag && !localPos.Empty()) {
                uint64_t eyes_raw = read<uint64_t>((uintptr_t)localPlayer + offsets::BasePlayer::PlayerEyes);
                LOG("POS_DIAG: Get_ObjectPosition=(%.1f,%.1f,%.1f) eyes_raw=0x%I64X",
                    localPos.x, localPos.y, localPos.z, eyes_raw);
                if (eyes_raw) {
                    uint64_t eyes_decoded = eyes_raw;
                    if (eyes_raw & 1) eyes_decoded = decrypt::decrypt_eyes(eyes_raw);
                    uint64_t eyes_ptr = decrypt::Il2cppGetHandle(eyes_decoded);
                    if (!eyes_ptr) eyes_ptr = eyes_decoded & ~0x7ULL;
                    Vector3 eyesPos = read<Vector3>(eyes_ptr + offsets::PlayerEyes::headPosition);
                    LOG("POS_DIAG: eyes_ptr=0x%I64X eyesPos=(%.1f,%.1f,%.1f)",
                        eyes_ptr, eyesPos.x, eyesPos.y, eyesPos.z);
                }
                posDiag = true;
            }
        }

        auto cycleStart = GetTickCount64();

        // Compute max draw distance for early entity culling
        // Entities beyond ALL draw distances are skipped before classification — saves 5+ IOCTLs each
        float maxDrawDist = 0;
        if (ESP::ESPEnabled) maxDrawDist = std::max(maxDrawDist, (float)ESP::draw_distance);
        if (NPC_ESP::Enabled) maxDrawDist = std::max(maxDrawDist, (float)NPC_ESP::draw_distance);
        if (ANIMAL_ESP::Enabled) maxDrawDist = std::max(maxDrawDist, (float)ANIMAL_ESP::draw_distance);
        if (WORLD::AnyEspEnabled()) maxDrawDist = std::max(maxDrawDist, WORLD::draw_distance);
        if (maxDrawDist < 1.f) maxDrawDist = 1.f; // safety — always allow at least 1m

        g_CacheStep.store(10, std::memory_order_relaxed);
        for (auto i = 0; i < entity_size; i++) {
            if (i > 0 && (i % 50) == 0) {
                g_CacheHeartbeatMs.store(GetTickCount64(), std::memory_order_release);
                g_CacheLastEntity.store(i, std::memory_order_relaxed);
                if (MISC::ShutdownRequested) { LOG("CACHE: aborting at entity %d/%d (shutdown)", i, (int)entity_size); break; }
                if ((i % 500) == 0 && Drv && !Drv->IsProcessAlive()) {
                    LOG("CACHE: RustClient.exe died mid-loop at entity %d — aborting to prevent BSOD", i);
                    g_process_dead.store(true, std::memory_order_relaxed);
                    Drv->ioctl_blocked.store(true, std::memory_order_relaxed);
                    MISC::ShutdownRequested = true;
                    break;
                }
            }

            g_CacheStep.store(20, std::memory_order_relaxed);
            uintptr_t entity_ptr_raw = read<uintptr_t>((uint64_t)entity_array + 0x20 + (i * 0x8));
            if (!entity_ptr_raw) continue;
            
            g_CacheStep.store(30, std::memory_order_relaxed);
            uintptr_t entity;
            if (entity_ptr_raw & 1) {
                __try {
                    entity = decrypt::Il2cppGetHandle(entity_ptr_raw);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    entity = 0;
                }
            } else {
                entity = entity_ptr_raw;
            }
            if (!is_valid(entity)) continue;

            g_CacheStep.store(40, std::memory_order_relaxed);
            {
                uintptr_t lerp = read<uint64_t>(entity + offsets::BaseEntity::positionLerp);
                if (lerp && is_valid(lerp)) {
                    uintptr_t interp = read<uint64_t>(lerp + offsets::BaseEntity::posChain0);
                    if (interp && is_valid(interp)) {
                        Vector3 earlyPos = read<Vector3>(interp + offsets::BaseEntity::posFinal);
                        if (!earlyPos.Zero() && !localPos.Empty()) {
                            float earlyDist = localPos.DistTo(earlyPos);
                            if (earlyDist > maxDrawDist) continue;
                        }
                    }
                }
            }

            g_CacheStep.store(50, std::memory_order_relaxed);
            uintptr_t object = read<uintptr_t>(entity + 0x10);
            if (!object || !is_valid(object)) continue;

            g_CacheStep.store(60, std::memory_order_relaxed);
            uintptr_t objectClass = read<uintptr_t>(object + 0x20);
            if (!objectClass || !is_valid(objectClass)) continue;

            g_CacheStep.store(70, std::memory_order_relaxed);

            std::string buff;
            {
                const char* cachedName = nullptr;
                for (int j = 0; j < s_prefabNameCacheCount; j++) {
                    if (s_prefabNameCache[j].key == objectClass && (cacheCycleCount - s_prefabNameCache[j].cycle) < 10) {
                        cachedName = s_prefabNameCache[j].name;
                        break;
                    }
                }
                if (cachedName) {
                    buff = cachedName;
                } else {
                    uintptr_t namePtr = read<uintptr_t>(objectClass + 0x50);
                    if (!namePtr || !is_valid(namePtr)) continue;
                    monostr ms = read<monostr>(namePtr);
                    ms.buffer[127] = '\0';
                    buff = ms.buffer;
                    buff = std::string(buff.c_str());
                    if (buff.empty()) continue;
                    if (s_prefabNameCacheCount < 256) {
                        s_prefabNameCache[s_prefabNameCacheCount].key = objectClass;
                        strncpy_s(s_prefabNameCache[s_prefabNameCacheCount].name, 128, buff.c_str(), 127);
                        s_prefabNameCache[s_prefabNameCacheCount].cycle = cacheCycleCount;
                        s_prefabNameCacheCount++;
                    }
                }
            }
            if (buff.empty()) continue;

            // Log first 10 prefab names for diagnostics
            {
                if (kCacheVerboseLogs) {
                    static int nameSampleCount = 0;
                    static std::string lastNameLogged;
                    if (nameSampleCount < 10 && buff != lastNameLogged) {
                        LOG("CACHE name[%d]: '%s'", nameSampleCount, buff.c_str());
                        lastNameLogged = buff;
                        nameSampleCount++;
                    }
                }
            }

            // Diagnostic: log first 50 prefab names to find new NPC/animal/trap patterns
            {
                static int nameSampleCount = 0;
                static std::unordered_set<std::string> namesLogged;
                if (kCacheVerboseLogs && nameSampleCount < 50 && namesLogged.find(buff) == namesLogged.end()) {
                    namesLogged.insert(buff);
                    LOG("CACHE prefab[%d]: '%s'", nameSampleCount, buff.c_str());
                    nameSampleCount++;
                }
            }
            // Lowercase for case-insensitive matching (Unity 6 may vary casing)
            std::string low = buff;
            for (char& c : low) c = (char)tolower((unsigned char)c);

                bool keywordNpc = low.find("scientistnpc") != std::string::npos
                    || low.find("scarecrow") != std::string::npos
                    || low.find("scientist") != std::string::npos
                    || low.find("tunneldweller") != std::string::npos
                    || low.find("underwaterdweller") != std::string::npos
                    || low.find("murderer") != std::string::npos
                    || low.find("bandit_guard") != std::string::npos
                    || low.find("peacekeeper") != std::string::npos
                    || low.find("heavyscientist") != std::string::npos
                    || low.find("npc/bandit") != std::string::npos
                    || low.find("npc/scientist") != std::string::npos
                    || low.find("npc/murderer") != std::string::npos
                    || low.find("npc/scarecrow") != std::string::npos
                    || low.find("bandit/missionprovider") != std::string::npos
                    || low.find("npc/peacekeeper") != std::string::npos
                    || low.find("npc/heavyscientist") != std::string::npos
                    || low.find("/tunneldweller") != std::string::npos
                    || low.find("/underwaterdweller") != std::string::npos;
            bool playerPrefabPath = low.find("assets/prefabs/player/player.prefab") != std::string::npos
                || low.find("player.prefab") != std::string::npos
                || low.find("localplayer") != std::string::npos
                || low.find("assets/prefabs/player/") != std::string::npos
                || low.find("/player/player") != std::string::npos;
            bool isCorpseOrProp = low.find("corpse") != std::string::npos
                || low.find("prop") != std::string::npos
                || low.find("barricade") != std::string::npos
                || low.find("napalm") != std::string::npos
                || low.find("deployable") != std::string::npos;
            bool keywordAnimal = low.find("boar") != std::string::npos
                || low.find("chicken") != std::string::npos
                || low.find("cow") != std::string::npos
                || low.find("deer") != std::string::npos
                || low.find("pig") != std::string::npos
                || low.find("rabbit") != std::string::npos
                || low.find("goat") != std::string::npos
                || low.find("sheep") != std::string::npos
                || low.find("horse") != std::string::npos
                || low.find("wolf") != std::string::npos
                || low.find("bear") != std::string::npos
                || low.find("moose") != std::string::npos
                || low.find("elk") != std::string::npos
                || low.find("buffalo") != std::string::npos
                || low.find("bison") != std::string::npos
                || low.find("goose") != std::string::npos
                || low.find("duck") != std::string::npos
                || low.find("turkey") != std::string::npos
                || low.find("raccoon") != std::string::npos
                || low.find("fox") != std::string::npos
                || low.find("coyote") != std::string::npos;

            bool isNPC = keywordNpc && !isCorpseOrProp && !playerPrefabPath;
            bool isPlayer = playerPrefabPath && !isCorpseOrProp && !keywordNpc;

            // Diagnostic: log first 200 unmatched prefab names (for P=0 debugging)
            {
                static int unmatchedCount = 0;
                static std::unordered_set<std::string> unmatchedLogged;
                if (kCacheVerboseLogs && unmatchedCount < 200 && unmatchedLogged.find(buff) == unmatchedLogged.end()) {
                    if (!isNPC && !isPlayer) {
                        unmatchedLogged.insert(buff);
                        LOG("CACHE UNMATCHED[%d]: '%s'", unmatchedCount, buff.c_str());
                        unmatchedCount++;
                    }
                }
            }

            {
                if (kCacheVerboseLogs) {
                    static bool npcDiag = false;
                    if (!npcDiag && keywordNpc) {
                        LOG("CACHE NPC DIAG: prefab='%s' isNPC=%d keywordMatch=%d playerPath=%d", buff.c_str(), isNPC, keywordNpc, playerPrefabPath);
                        npcDiag = true;
                    }
                }
            }
            bool isAnimal = false;
            std::string animalType;
            bool isBear = ANIMAL_ESP::Bear && low.find("bear") != std::string::npos && low.find("polar") == std::string::npos;
            bool isPolarBear = ANIMAL_ESP::PolarBear && low.find("polarbear") != std::string::npos;
            bool isWolf = ANIMAL_ESP::Wolf && low.find("wolf") != std::string::npos;
            bool isBoar = ANIMAL_ESP::Boar && low.find("boar") != std::string::npos && low.find("cupboard") == std::string::npos;
            bool isStag = ANIMAL_ESP::Stag && (low.find("/stag.") != std::string::npos || low.find("/stag_") != std::string::npos);
            bool isChicken = ANIMAL_ESP::Chicken && low.find("chicken") != std::string::npos;
            bool isHorse = ANIMAL_ESP::Horse && low.find("horse") != std::string::npos;
            bool isPanther = ANIMAL_ESP::Panther && low.find("panther") != std::string::npos;
            bool isTiger = ANIMAL_ESP::Tiger && low.find("tiger") != std::string::npos;
            bool isSnake = ANIMAL_ESP::Snake && low.find("snake") != std::string::npos;
            if (isBear) { isAnimal = true; animalType = "Bear"; }
            else if (isPolarBear) { isAnimal = true; animalType = "Polar Bear"; }
            else if (isWolf) { isAnimal = true; animalType = "Wolf"; }
            else if (isBoar) { isAnimal = true; animalType = "Boar"; }
            else if (isStag) { isAnimal = true; animalType = "Stag"; }
            else if (isChicken) { isAnimal = true; animalType = "Chicken"; }
            else if (isHorse) { isAnimal = true; animalType = "Horse"; }
            else if (isPanther) { isAnimal = true; animalType = "Panther"; }
            else if (isTiger) { isAnimal = true; animalType = "Tiger"; }
            else if (isSnake) { isAnimal = true; animalType = "Snake"; }
            if (isAnimal && (low.find("trap") != std::string::npos || low.find("hide") != std::string::npos || low.find("skull") != std::string::npos || low.find("rug") != std::string::npos || low.find("meat") != std::string::npos || low.find("pelt") != std::string::npos || low.find("cooked") != std::string::npos || low.find("stew") != std::string::npos || low.find("trophy") != std::string::npos)) isAnimal = false;

            // Fallback: if prefab name didn't match player patterns, check IL2CPP class name.
            // Reads the entity's actual TYPE from IL2CPP metadata — world items have class names
            // like "TreeEntity", only BasePlayer entities have "BasePlayer" as their class name.
            // Pre-filter with PlayerInput pointer to avoid 3 IOCTLs on every entity.
            // Layout: entity+0x0 = Il2CppClass* klass, klass+0x10 = const char* name (Morphine: klass_layout::name_ptr)
            if (!isPlayer && !isNPC && !isAnimal && !isCorpseOrProp) {
                uintptr_t testInput = read<uintptr_t>(entity + offsets::BasePlayer::PlayerInput);
                if (testInput && is_valid(testInput)) {
                    uintptr_t klass = read<uintptr_t>(entity);
                    if (klass && is_valid(klass)) {
                        uintptr_t classNamePtr = read<uintptr_t>(klass + 0x10);
                        if (classNamePtr && is_valid(classNamePtr)) {
                            monostr cms = read<monostr>(classNamePtr);
                            cms.buffer[127] = '\0';
                            std::string className = cms.buffer;
                            if (className.find("BasePlayer") != std::string::npos) {
                                isPlayer = true;
                                static int fallbackCount = 0;
                                if (fallbackCount < 20) {
                                    LOG("CACHE PLAYER FALLBACK[%d]: prefab='%s' class='%s' -> player",
                                        fallbackCount, buff.c_str(), className.c_str());
                                    fallbackCount++;
                                }
                            }
                        }
                    }
                }
            }

            if (isPlayer) {
                g_CacheStep.store(100, std::memory_order_relaxed);
                classifiedPlayers++;
                if (samplePlayer.empty()) samplePlayer = low;
                Rust::BaseEntity* Player = (Rust::BaseEntity*)entity;
                if (!Player || !is_valid((uintptr_t)Player)) continue;
                if (localPlayer && (uintptr_t)Player == (uintptr_t)localPlayer) continue;

                // Fast distance cull first (2 IOCTLs), then full chain if in range (6 IOCTLs)
                auto fastPos = Player->Get_ObjectPosition_Fast();
                auto pos = fastPos.Empty() ? Player->Get_ObjectPosition() : fastPos;
                if (pos.Zero()) continue;
                float Distance = localPos.DistTo(pos);
                if (Distance > ESP::draw_distance) continue;

                // UC block read: read 0x424 bytes from Lifestate(0x298) to PlayerFlags(0x6B8)+4 in ONE IOCTL
                // Replaces 4 individual reads: PlayerFlags, combat block, CurrentTeam, ModelState ptr
                uint8_t playerBuf[0x424] = {};
                uintptr_t readBase = (uintptr_t)Player + offsets::BaseCombatEntity::Lifestate;
                if (!(g_UseInternalReads && ReadMemory_Internal(reinterpret_cast<PVOID>(readBase), playerBuf, 0x424)))
                    if (Drv && !Drv->ioctl_blocked.load(std::memory_order_relaxed) && !g_process_dead.load(std::memory_order_relaxed) && !g_shutting_down.load(std::memory_order_relaxed))
                        Drv->ReadMemory_ACE(reinterpret_cast<PVOID>(readBase), playerBuf, 0x424);
                int lifestate = *(int*)(playerBuf + 0x0);           // 0x298 - 0x298
                float health = *(float*)(playerBuf + 0xC);          // 0x2A4 - 0x298
                float maxHealth = *(float*)(playerBuf + 0x10);      // 0x2A8 - 0x298
                uintptr_t msPtr = *(uintptr_t*)(playerBuf + 0x280); // 0x518 - 0x298 (ModelState)
                uint64_t teamId = *(uint64_t*)(playerBuf + 0x2A0);  // 0x538 - 0x298 (CurrentTeam)
                int pFlags = *(int*)(playerBuf + 0x420);             // 0x6B8 - 0x298 (PlayerFlags)
                bool isDead = (lifestate == 1);

                // Dead players — show as corpse ESP (the dead body on the ground before it becomes a body bag)
                if (isDead) {
                    if (WORLD::BodyBag && Distance <= WORLD::draw_corpse) {
                        Rust::PrefabData data;
                        data.Position = pos;
                        data.Color = WORLD::color::BodyBag_Color;
                        std::string pname = Player->GetName();
                        data.name = (pname.empty() || pname == "?") ? "Dead Player" : ("Dead [" + pname + "]");
                        tempPrefabList.push_back(data);
                    }
                    continue;
                }
                if ((pFlags & (int)offsets::base_player_flags::Sleeping) != 0 && ESP::RemoveSleepers) continue;
                if ((pFlags & (int)offsets::base_player_flags::Wounded) != 0 && ESP::RemoveWounded) continue;

                EspCacheData ec;

                ec.isDead = isDead;
                ec.isSleeping = (pFlags & (int)offsets::base_player_flags::Sleeping) != 0;
                ec.isWounded = (pFlags & (int)offsets::base_player_flags::Wounded) != 0;
                ec.health = health;
                ec.maxHealth = maxHealth;
                ec.teamId = teamId;

                if (!ESP::VisCheck) {
                    ec.isVisibleRaw = true;
                    ec.isVisible = true;
                } else if (Distance <= 400.f || (cacheCycleCount % 2) == 0) {
                    ec.isVisibleRaw = Player->IsVisibleRawFlag();
                    ec.isVisible = Player->IsVisibleFiltered(ec.isVisibleRaw);
                } else {
                    ec.isVisibleRaw = false;
                    ec.isVisible = false;
                }

                {
                    if (msPtr && is_valid(msPtr)) {
                        int msFlags = read<int>(msPtr + offsets::ModelState::flags);
                        ec.isCrouching = (msFlags & offsets::ModelState::Ducked) != 0;
                        ec.isGrounded = (msFlags & offsets::ModelState::OnGround) != 0;
                    }
                }

                // Position is set here; velocity is updated by the position-only thread
                ec.headPos = pos;

                // BVH raycast overrides isVisible (but NOT isVisibleRaw, which stays as the game's flag for VisibleStrict)
                if (ESP::VisCheck && ESP::VisMode > 0 && vischeck::g_VisCheckLoaded && Distance <= 400.f) {
                    Vector3 camPos;
                    { std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex); camPos = g_CameraWorldPos; }
                    if (!camPos.Empty()) {
                        bool rayVisible = vischeck::g_VisCheck.IsVisible(camPos, ec.headPos);
                        ec.isVisible = rayVisible;
                    }
                }

                // Name / held item: lower LOD for distant players (matches stable version)
                if (Distance <= 100.f || (Distance <= 300.f && (cacheCycleCount % 2) == 0)) {
                    { auto n = Player->GetName(); cacheSetStr(ec.name, n.c_str()); }
                    if (Distance <= 100.f || (Distance <= 200.f && (cacheCycleCount % 4) == 0)) {
                        auto* h = Player->Get_HeldItem();
                        if (h) { auto wn = h->get_item_name(); cacheSetStr(ec.weaponName, wn.c_str()); }
                    }
                }
                ec.skeletonValid = false;

                // Inventory scan: close players only, every 4th cycle, only if ESP toggles active
                if (Distance <= 100.f && (ESP::hotbar_text || ESP::Clothing || ESP::ItemList || ESP::AmmoBar || ESP::PlayerInventoryPanel) && (cacheCycleCount % 4) == 0) {
                    auto belt = Player->Get_Hotbar_list();
                    for (size_t i = 0; i < belt.size() && i < 6; i++) {
                        if (belt[i]) { auto bn = belt[i]->get_item_name(); cacheSetStr(ec.belt[i], bn.c_str()); }
                    }
                    uintptr_t inv = Player->ResolvePlayerInventory();
                    if (inv && is_valid(inv)) {
                        uintptr_t wear = read<uintptr_t>(inv + offsets::PlayerInventory::Wear);
                        if (wear && is_valid(wear)) {
                            for (int ilo : { (int)offsets::ItemContainer::ItemList, (int)offsets::ItemContainer::ItemListFallback }) {
                                uintptr_t wearList = read<uintptr_t>(wear + ilo);
                                if (!wearList || !is_valid(wearList)) continue;
                                int count = read<int>(wearList + 0x18);
                                uintptr_t wearArr = read<uintptr_t>(wearList + 0x10);
                                if (!wearArr || !is_valid(wearArr) || count <= 0) continue;
                                if (count > 5) count = 5;
                                for (int i = 0; i < count; i++) {
                                    uintptr_t item = read<uintptr_t>(wearArr + 0x20 + (i * 8));
                                    if (!item || !is_valid(item)) continue;
                                    auto* wi = (Rust::BaseEntity::HeldItem*)item;
                                    { auto wn = wi->get_item_name(); cacheSetStr(ec.wear[i], wn.c_str()); }
                                }
                                break;
                            }
                        }
                    }
                    uintptr_t weapon = Player->Get_HeldWeapon();
                    if (weapon) {
                        uintptr_t mag = read<uintptr_t>(weapon + offsets::BaseProjectile::primaryMagazine);
                        if (mag && is_valid(mag)) ec.ammo = read<int>(mag + offsets::ItemMagazine::contents);
                    }
                    ec.invTick = GetTickCount64();
                }

                ec.cacheTick = GetTickCount64();
                tempEspCache[(uintptr_t)entity] = ec;

                tempList.push_back(Player);

                // Append to movement trail
                {
                    std::lock_guard<std::mutex> lk(g_TrailsMutex);
                    auto& tr = g_Trails[(uintptr_t)entity];
                    tr.push_back(pos);
                    if (tr.size() > 60) tr.erase(tr.begin()); // keep ~3s at 50ms cycle
                }
            }
            else if (isNPC && NPC_ESP::Enabled) {
                classifiedNpcs++;
                if (sampleNpc.empty()) sampleNpc = low;
                if ((!NPC_ESP::Scientists && low.find("scientist") != std::string::npos)
                 || (!NPC_ESP::TunnelDweller && low.find("tunneldweller") != std::string::npos)
                 || (!NPC_ESP::UnderwaterDweller && low.find("underwaterdweller") != std::string::npos))
                    continue;
                Rust::BaseEntity* NPC = (Rust::BaseEntity*)entity;
                if (!NPC || !is_valid((uintptr_t)NPC) || NPC->IsDead()) continue;
                auto fastPos = NPC->Get_ObjectPosition_Fast();
                auto pos = fastPos.Empty() ? NPC->Get_ObjectPosition() : fastPos;
                if (pos.Zero()) continue;
                float Distance = localPos.DistTo(pos);
                if (Distance > NPC_ESP::draw_distance) continue;
                EspCacheData ec;
                ec.headPos = pos;
                ec.feetPos = pos;
                ec.cacheTick = GetTickCount64();
                // Read velocity for prediction
                if (Distance <= 300.f) {
                    uintptr_t pm = read<uintptr_t>((uintptr_t)NPC + offsets::BasePlayer::PlayerModel);
                    if (pm && is_valid(pm))
                        ec.velocity = read<Vector3>(pm + offsets::PlayerModel::velocity);
                }
                // Populate skeleton bones for NPC if feature enabled and within range
                if (NPC_ESP::Skeleton && Distance <= NPC_ESP::NPCSkeletonDist) {
                    uintptr_t btf = NPC->Get_BoneTransforms();
                    if (btf) {
                        static const BoneList skelBones[] = {
                            BoneList::head, BoneList::neck, BoneList::spine1,
                            BoneList::r_hip, BoneList::r_knee, BoneList::r_foot,
                            BoneList::l_hip, BoneList::l_knee, BoneList::l_foot,
                            BoneList::l_upperarm, BoneList::l_forearm, BoneList::l_hand,
                            BoneList::r_upperarm, BoneList::r_forearm, BoneList::r_hand
                        };
                        int validBones = 0;
                        for (int b = 0; b < 15; b++) {
                            auto t = NPC->Get_Transformation_Fast(skelBones[b], btf);
                            if (!t) continue;
                            Vector3 bp = t->Get_Position();
                            if (bp.Empty()) continue;
                            ec.bones[b] = bp;
                            validBones++;
                        }
                        ec.skeletonValid = (validBones >= 4);
                        if (ec.skeletonValid) ec.skeletonTick = GetTickCount64();
                    }
                }
                if (NPC_ESP::HeldItem) {
                    auto* held = NPC->Get_HeldItem();
                    if (held) { auto wn = held->get_item_name(); cacheSetStr(ec.weaponName, wn.c_str()); }
                }
                tempEspCache[(uintptr_t)entity] = ec;
                tempNpcList.push_back(NPC);
            }
            else if (isAnimal && ANIMAL_ESP::Enabled) {
                classifiedAnimals++;
                Rust::BaseEntity* Animal = (Rust::BaseEntity*)entity;
                if (!Animal || !is_valid((uintptr_t)Animal)) continue;
                int lifestate = read<int>((uintptr_t)Animal + offsets::BaseCombatEntity::Lifestate);
                bool animalDead = (lifestate == 1);
                if (animalDead) continue;
                auto fastPos = Animal->Get_ObjectPosition_Fast();
                auto pos = fastPos.Empty() ? Animal->Get_ObjectPosition() : fastPos;
                if (pos.Zero()) continue;
                float Distance = localPos.DistTo(pos);
                if (Distance > ANIMAL_ESP::draw_distance) continue;
                EspCacheData& aec = tempEspCache[(uintptr_t)entity];
                aec.headPos = pos;
                aec.feetPos = pos;
                aec.cacheTick = GetTickCount64();
                cacheSetStr32(aec.animalType, animalType.c_str());
                aec.isDead = animalDead;
                aec.health = read<float>((uintptr_t)Animal + offsets::BaseCombatEntity::Health);
                aec.maxHealth = read<float>((uintptr_t)Animal + offsets::BaseCombatEntity::MaxHealth);
                tempAnimalList.push_back(Animal);
            }
            else {
                // WORLD PREFAB: skip entirely if no world ESP toggle is active
                if (!WORLD::AnyEspEnabled()) continue;

                // Fast 3-IOCTL position via PositionLerp (falls back to 6-IOCTL full chain)
                Rust::BaseEntity* worldEnt = (Rust::BaseEntity*)entity;
                auto fastPos = worldEnt->Get_ObjectPosition_Fast();
                auto EntityPosition = fastPos.Empty() ? worldEnt->Get_ObjectPosition() : fastPos;
                if (EntityPosition.Zero()) continue;

                float distance = localPos.DistTo(EntityPosition);
                if (distance > WORLD::draw_distance) continue;

                // Per-entity distance limits (each entity has its own draw distance)
                bool trapOk = (distance <= WORLD::draw_turret);
                bool shotgunOk = (distance <= WORLD::draw_shotguntrap);
                bool stashOk = (distance <= WORLD::draw_stash);
                bool copterOk = (distance <= WORLD::draw_minicopter);
                bool hempOk = (distance <= WORLD::draw_hemp);
                bool stoneOk = (distance <= WORLD::draw_stone);
                bool metalOk = (distance <= WORLD::draw_metal);
                bool sulfurOk = (distance <= WORLD::draw_sulfur);
                bool corpseOk = (distance <= WORLD::draw_corpse);
                bool backpackOk = (distance <= WORLD::draw_backpack);
                bool bradleyOk = (distance <= WORLD::draw_bradley);
                bool droppedOk = (distance <= WORLD::draw_dropped);
                bool samSiteOk = (distance <= WORLD::draw_samsite);
                bool bearTrapOk = (distance <= WORLD::draw_beartrap);
                bool landmineOk = (distance <= WORLD::draw_landmine);
                bool flameTurretOk = (distance <= WORLD::draw_flameturret);
                bool rowboatOk = (distance <= WORLD::draw_rowboat);
                bool rhibOk = (distance <= WORLD::draw_rhib);
                bool kayakOk = (distance <= WORLD::draw_kayak);
                bool submarineOk = (distance <= WORLD::draw_submarine);
                bool tugboatOk = (distance <= WORLD::draw_tugboat);
                bool transportHeliOk = (distance <= WORLD::draw_transportheli);
                bool attackHeliOk = (distance <= WORLD::draw_attackheli);
                bool balloonOk = (distance <= WORLD::draw_balloon);
                bool motorbikeOk = (distance <= WORLD::draw_motorbike);
                bool snowmobileOk = (distance <= WORLD::draw_snowmobile);
                bool barrelBeigeOk = (distance <= WORLD::draw_barrelbeige);
                bool barrelBlueOk = (distance <= WORLD::draw_barrelblue);
                bool barrelFuelOk = (distance <= WORLD::draw_barrelfuel);
                bool crateNormalOk = (distance <= WORLD::draw_crate_normal);
                bool crateMilitaryOk = (distance <= WORLD::draw_crate_military);
                bool crateEliteOk = (distance <= WORLD::draw_crate_elite);
                bool crateLockedOk = (distance <= WORLD::draw_crate_locked);
                bool lockersOk = (distance <= WORLD::draw_lockers);
                bool bagsOk = (distance <= WORLD::draw_bags);
                bool bedsOk = (distance <= WORLD::draw_beds);
                bool tcOk = (distance <= WORLD::draw_tc);
                bool vendingOk = (distance <= WORLD::draw_vending);

                if (WORLD::Hemp && hempOk && low.find("hemp-collectable.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Hemp");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Hemp_Color;
                    tempPrefabList.push_back(data);
                }
                if ( WORLD::Stash && stashOk && low.find("small_stash_deployed.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Stash");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Stash_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Metal && metalOk && low.find("metal-ore.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Metal Ore");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Metal_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Stone && stoneOk && low.find("stone-ore.prefab") != std::string::npos)
                {
                    Rust::PrefabData data;
                    data.name = TR("Stone Ore");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Stone_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Sulfer && sulfurOk && low.find("sulfur-ore.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Sulfer Ore");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Sulfer_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::BodyBag && corpseOk && (low.find("player_corpse") != std::string::npos || low.find("lootable_corpse") != std::string::npos || low.find("npc_corpse") != std::string::npos))
                {
                    Rust::PrefabData data;
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::BodyBag_Color;
                    
                    // Try to read player name from LootableCorpse._playerName (0x310)
                    std::string corpseName = TR("Corpse");
                    if (entity && is_valid((uintptr_t)entity)) {
                        uintptr_t nameStrPtr = read<uintptr_t>((uintptr_t)entity + 0x310);
                        if (nameStrPtr && is_valid(nameStrPtr)) {
                            std::string pname = read_wstr(nameStrPtr + offsets::unity_string::first_char);
                            if (!pname.empty() && pname != "?") {
                                corpseName = std::string("Corpse [") + pname + "]";
                            }
                        }
                    }
                    data.name = corpseName;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Turret && trapOk && low.find("autoturret_deployed") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Auto Turret");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Turret_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::ShotGunTrap && shotgunOk && low.find("guntrap") != std::string::npos)
                {
                    Rust::PrefabData data;
                    data.name = TR("ShotGun Trap");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::ShotGunTrap_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::MiniCopter && copterOk && low.find("minicopter.entity.prefab") != std::string::npos)
                {
                    Rust::PrefabData data;
                    data.name = TR("MiniCopter");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::MiniCopter_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Backpack && backpackOk && low.find("item_drop_backpack.prefab") != std::string::npos)
                {
                    Rust::PrefabData data;
                    data.name = TR("Backpack");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Backpack_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::BradlyAPC && bradleyOk && low.find("bradleyapc.prefab") != std::string::npos)
                {
                    Rust::PrefabData data;
                    data.name = TR("Bradley APC");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::BradlyAPC;
                    tempPrefabList.push_back(data);
                }
                // --- New world entities ---
                if (WORLD::StonePickup && low.find("stone-collectable.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Stone Pkup"; d.Position=EntityPosition; d.Color=WORLD::color::StonePickup; tempPrefabList.push_back(d); }
                if (WORLD::MetalPickup && low.find("metal-collectable.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Metal Pkup"; d.Position=EntityPosition; d.Color=WORLD::color::MetalPickup; tempPrefabList.push_back(d); }
                if (WORLD::SulfurPickup && low.find("sulfur-collectable.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Sulfur Pkup"; d.Position=EntityPosition; d.Color=WORLD::color::SulfurPickup; tempPrefabList.push_back(d); }
                if (WORLD::WoodPickup && low.find("wood-collectable.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Wood Pkup"; d.Position=EntityPosition; d.Color=WORLD::color::WoodPickup; tempPrefabList.push_back(d); }
                if (WORLD::SamSite && samSiteOk && low.find("sam_site") != std::string::npos)
                    { Rust::PrefabData d; d.name="Sam Site"; d.Position=EntityPosition; d.Color=WORLD::color::SamSite; tempPrefabList.push_back(d); }
                if (WORLD::BearTrap && bearTrapOk && low.find("beartrap") != std::string::npos)
                    { Rust::PrefabData d; d.name="Bear Trap"; d.Position=EntityPosition; d.Color=WORLD::color::BearTrap; tempPrefabList.push_back(d); }
                if (WORLD::Landmine && landmineOk && low.find("landmine") != std::string::npos)
                    { Rust::PrefabData d; d.name="Landmine"; d.Position=EntityPosition; d.Color=WORLD::color::Landmine; tempPrefabList.push_back(d); }
                if (WORLD::FlameTurret && flameTurretOk && low.find("flameturret") != std::string::npos)
                    { Rust::PrefabData d; d.name="Flame Turret"; d.Position=EntityPosition; d.Color=WORLD::color::FlameTurret; tempPrefabList.push_back(d); }
                // Ballista / Cannon siege weapons
                if (WORLD::Turret && trapOk && low.find("ballista") != std::string::npos)
                    { Rust::PrefabData d; d.name="Ballista"; d.Position=EntityPosition; d.Color=WORLD::color::Turret_Color; tempPrefabList.push_back(d); }
                if (WORLD::Turret && trapOk && low.find("catapult") != std::string::npos)
                    { Rust::PrefabData d; d.name="Catapult"; d.Position=EntityPosition; d.Color=WORLD::color::Turret_Color; tempPrefabList.push_back(d); }
                if (WORLD::Lockers && lockersOk && low.find("locker.deployed.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Locker"; d.Position=EntityPosition; d.Color=WORLD::color::Lockers; tempPrefabList.push_back(d); }
                if (WORLD::Bags && bagsOk && low.find("sleepingbag") != std::string::npos)
                    { Rust::PrefabData d; d.name="Sleeping Bag"; d.Position=EntityPosition; d.Color=WORLD::color::Bags; tempPrefabList.push_back(d); }
                if (WORLD::Beds && bedsOk && low.find("bed_deployed") != std::string::npos)
                    { Rust::PrefabData d; d.name="Bed"; d.Position=EntityPosition; d.Color=WORLD::color::Beds; tempPrefabList.push_back(d); }
                if (WORLD::TC && tcOk && low.find("cupboard.tool") != std::string::npos)
                    { Rust::PrefabData d; d.name="Tool Cupboard"; d.Position=EntityPosition; d.Color=WORLD::color::TC; tempPrefabList.push_back(d); }
                if (WORLD::Vending && vendingOk && low.find("vending") != std::string::npos)
                    { Rust::PrefabData d; d.name="Vending"; d.Position=EntityPosition; d.Color=WORLD::color::Vending; tempPrefabList.push_back(d); }
                if (WORLD::Workbench && (low.find("workbench1.deployed.prefab") != std::string::npos || low.find("workbench2.deployed.prefab") != std::string::npos || low.find("workbench3.deployed.prefab") != std::string::npos))
                    { Rust::PrefabData d; d.name="Workbench"; d.Position=EntityPosition; d.Color=WORLD::color::Workbench; tempPrefabList.push_back(d); }
                if (WORLD::LargeStorage && (low.find("storage_barrel_") != std::string::npos || low.find("box.wooden.large.prefab") != std::string::npos || low.find("coffinstorage.prefab") != std::string::npos))
                    { Rust::PrefabData d; d.name="Large Storage"; d.Position=EntityPosition; d.Color=WORLD::color::LargeStorage; tempPrefabList.push_back(d); }
                if (WORLD::Ladder && low.find("ladder.wooden") != std::string::npos)
                    { Rust::PrefabData d; d.name="Ladder"; d.Position=EntityPosition; d.Color=WORLD::color::Ladder; tempPrefabList.push_back(d); }
                if (WORLD::Generator && low.find("generator.") != std::string::npos)
                    { Rust::PrefabData d; d.name="Generator"; d.Position=EntityPosition; d.Color=WORLD::color::Generator; tempPrefabList.push_back(d); }
                if (WORLD::Battery && (low.find("rechargable.battery") != std::string::npos || low.find("rechargablebattery") != std::string::npos))
                    { Rust::PrefabData d; d.name="Battery"; d.Position=EntityPosition; d.Color=WORLD::color::Battery; tempPrefabList.push_back(d); }
                if (WORLD::BarrelBeige && barrelBeigeOk && low.find("loot_barrel_2") != std::string::npos)
                    { Rust::PrefabData d; d.name="Barrel"; d.Position=EntityPosition; d.Color=WORLD::color::BarrelBeige; tempPrefabList.push_back(d); }
                if (WORLD::BarrelBlue && barrelBlueOk && low.find("loot_barrel_1") != std::string::npos)
                    { Rust::PrefabData d; d.name="Barrel"; d.Position=EntityPosition; d.Color=WORLD::color::BarrelBlue; tempPrefabList.push_back(d); }
                if (WORLD::BarrelFuel && barrelFuelOk && low.find("oil_barrel") != std::string::npos)
                    { Rust::PrefabData d; d.name="Fuel Barrel"; d.Position=EntityPosition; d.Color=WORLD::color::BarrelFuel; tempPrefabList.push_back(d); }
                if (WORLD::CrateNormal && crateNormalOk && (low.find("crate_normal_2.prefab") != std::string::npos || low.find("crate_tools") != std::string::npos))
                    { Rust::PrefabData d; d.name="Crate"; d.Position=EntityPosition; d.Color=WORLD::color::NormalCrate; tempPrefabList.push_back(d); }
                if (WORLD::CrateMilitary && crateMilitaryOk && low.find("crate_normal.prefab") != std::string::npos && low.find("crate_normal_2.prefab") == std::string::npos)
                    { Rust::PrefabData d; d.name="Mil Crate"; d.Position=EntityPosition; d.Color=WORLD::color::MilitaryCrate; tempPrefabList.push_back(d); }
                if (WORLD::CrateElite && crateEliteOk && low.find("crate_elite") != std::string::npos)
                    { Rust::PrefabData d; d.name="Elite Crate"; d.Position=EntityPosition; d.Color=WORLD::color::EliteCrate; tempPrefabList.push_back(d); }
                if (WORLD::CrateLocked && crateLockedOk && low.find("codelockedhackablecrate") != std::string::npos)
                    { Rust::PrefabData d; d.name="Locked Crate"; d.Position=EntityPosition; d.Color=WORLD::color::LockedCrate; tempPrefabList.push_back(d); }
                if (WORLD::CrateMedical && (low.find("crate_normal_2_medical.prefab") != std::string::npos || low.find("crate_medical") != std::string::npos))
                    { Rust::PrefabData d; d.name="Med Crate"; d.Position=EntityPosition; d.Color=WORLD::color::MedicalCrate; tempPrefabList.push_back(d); }
                if (WORLD::CrateFood && (low.find("crate_normal_2_food.prefab") != std::string::npos || low.find("crate_food") != std::string::npos))
                    { Rust::PrefabData d; d.name="Food Crate"; d.Position=EntityPosition; d.Color=WORLD::color::FoodCrate; tempPrefabList.push_back(d); }
                if (WORLD::Rowboat && rowboatOk && low.find("rowboat.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Rowboat"; d.Position=EntityPosition; d.Color=WORLD::color::Rowboat; tempPrefabList.push_back(d); }
                if (WORLD::RHIB && rhibOk && low.find("rhib.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="RHIB"; d.Position=EntityPosition; d.Color=WORLD::color::RHIB; tempPrefabList.push_back(d); }
                if (WORLD::Kayak && kayakOk && low.find("kayak.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Kayak"; d.Position=EntityPosition; d.Color=WORLD::color::Kayak; tempPrefabList.push_back(d); }
                if (WORLD::Tugboat && tugboatOk && low.find("tugboat.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Tugboat"; d.Position=EntityPosition; d.Color=WORLD::color::Tugboat; tempPrefabList.push_back(d); }
                if (WORLD::Submarine && submarineOk && (low.find("submarinesolo.entity.prefab") != std::string::npos || low.find("submarineduo.entity.prefab") != std::string::npos))
                    { Rust::PrefabData d; d.name="Submarine"; d.Position=EntityPosition; d.Color=WORLD::color::Submarine; tempPrefabList.push_back(d); }
                if (WORLD::TransportHeli && transportHeliOk && low.find("scraptransporthelicopter.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Transport Heli"; d.Position=EntityPosition; d.Color=WORLD::color::TransportHeli; tempPrefabList.push_back(d); }
                if (WORLD::AttackHeli && attackHeliOk && low.find("attackhelicopter.entity.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Attack Heli"; d.Position=EntityPosition; d.Color=WORLD::color::AttackHeli; tempPrefabList.push_back(d); }
                if (WORLD::Balloon && balloonOk && low.find("hotairballoon.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Balloon"; d.Position=EntityPosition; d.Color=WORLD::color::Balloon; tempPrefabList.push_back(d); }
                if (WORLD::Motorbike && motorbikeOk && low.find("motorbike.prefab") != std::string::npos && low.find("motorbike_sidecar") == std::string::npos)
                    { Rust::PrefabData d; d.name="Motorbike"; d.Position=EntityPosition; d.Color=WORLD::color::Motorbike; tempPrefabList.push_back(d); }
                if (WORLD::MotorbikeSidecar && low.find("motorbike_sidecar.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Sidecar"; d.Position=EntityPosition; d.Color=WORLD::color::Sidecar; tempPrefabList.push_back(d); }
                if (WORLD::Trike && low.find("trike.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Trike"; d.Position=EntityPosition; d.Color=WORLD::color::Trike; tempPrefabList.push_back(d); }
                if (WORLD::Bicycle && low.find("pedalbike.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Bicycle"; d.Position=EntityPosition; d.Color=WORLD::color::Bicycle; tempPrefabList.push_back(d); }
                if (WORLD::Snowmobile && snowmobileOk && low.find("snowmobile.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Snowmobile"; d.Position=EntityPosition; d.Color=WORLD::color::Snowmobile; tempPrefabList.push_back(d); }
                if (WORLD::Shark && low.find("shark.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Shark"; d.Position=EntityPosition; d.Color=WORLD::color::Shark; tempPrefabList.push_back(d); }
                if (WORLD::CargoShip && low.find("cargo.prefab") != std::string::npos)
                    { Rust::PrefabData d; d.name="Cargo Ship"; d.Position=EntityPosition; d.Color=WORLD::color::CargoShip; tempPrefabList.push_back(d); }
                if (WORLD::SupplyDrop && low.find("supply_drop") != std::string::npos)
                    { Rust::PrefabData d; d.name="Supply Drop"; d.Position=EntityPosition; d.Color=WORLD::color::SupplyDrop; tempPrefabList.push_back(d); }
                if (WORLD::DroppedItem && low.find("(world)") != std::string::npos )
                {
                    if (distance > WORLD::draw_dropped) continue;
                    Rust::PrefabData data;
                    std::string itemName = buff;
                    const size_t start = itemName.find(" (");
                    const size_t end = itemName.rfind(")");
                    if (start != std::string::npos && end != std::string::npos && end >= start)
                        itemName.erase(start, end - start + 1);
                    for (char& c : itemName) {
                        if (c == '_' || c == '-') c = ' ';
                    }
                    bool skip_item = false;
                    std::string skip_keywords[] = { "torch", "rock", "asset", "fire", "dung", "arrow", "nail", "movepoint" };
                    for (const std::string& keyword : skip_keywords) {
                        if (itemName.find(keyword) != std::string::npos) {
                            skip_item = true;
                            break;
                        }
                    }

                    if (skip_item)
                        continue;

                    data.name = itemName;
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::DroppedItem_Color;
                    tempPrefabList.push_back(data);
            }
        }

        // Diagnostic: was the loop skipped entirely or names all empty?
        {
            static bool loopDiagDone = false;
            if (kCacheVerboseLogs && !loopDiagDone) {
                LOG("CACHE loop DONE: entities=%d players=%d npcs=%d animals=%d prefabs=%d",
                    (int)entity_size, classifiedPlayers, classifiedNpcs, classifiedAnimals, (int)tempPrefabList.size());
                if (classifiedPlayers + classifiedNpcs + classifiedAnimals == 0) {
                    LOG("CACHE WARNING: ZERO entities classified! Check prefab name format / offset.");
                    if (entity_size > 0 && classifiedPlayers == 0) {
                        // Read the first entity's raw data for debugging
                        uintptr_t ent0 = read<uintptr_t>((uint64_t)entity_array + 0x20);
                        LOG_HEX("  entity[0]", ent0);
                        if (ent0) {
                            uintptr_t obj0 = read<uintptr_t>(ent0 + 0x10);
                            LOG_HEX("  object[0]", obj0);
                            if (obj0) {
                                uintptr_t cls0 = read<uintptr_t>(obj0 + 0x20);
                                LOG_HEX("  objectClass[0]", cls0);
                                if (cls0) {
                                    uintptr_t namePtr = read<uintptr_t>(cls0 + 0x50);
                                    LOG_HEX("  namePtr (+0x50)", namePtr);
                                    if (namePtr) {
                                        monostr dms = read<monostr>(namePtr);
                                        dms.buffer[127] = '\0';
                                        std::string rawName = dms.buffer;
                                        LOG("  rawName: '%s' (len=%d)", rawName.c_str(), (int)rawName.length());
                                    }
                                }
                            }
                        }
                    }
                }
                loopDiagDone = true;
            }
            } // end else (world prefab classification)
        }

        {
            static int classifyTick = 0;
            if (kCacheVerboseLogs && (++classifyTick % 120) == 0) {
                LOG("CACHE classify: players=%d npcs=%d animals=%d | lists P=%d N=%d A=%d | sampleP='%s' sampleN='%s'",
                    classifiedPlayers,
                    classifiedNpcs,
                    classifiedAnimals,
                    (int)tempList.size(),
                    (int)tempNpcList.size(),
                    (int)tempAnimalList.size(),
                    samplePlayer.empty() ? "-" : samplePlayer.c_str(),
                    sampleNpc.empty() ? "-" : sampleNpc.c_str());
            }
        }

        // Cycle timing log
        {
            static int cycleLogTick = 0;
            ULONGLONG elapsed = GetTickCount64() - cycleStart;
            if ((++cycleLogTick % 240) == 0 || elapsed > 500) {
                LOG("CACHE cycle: %llums entities=%d P=%d N=%d A=%d W=%d",
                    elapsed, (int)entity_size, classifiedPlayers, classifiedNpcs, classifiedAnimals, (int)tempPrefabList.size());
            }
        }

        // Periodic entity_size diagnostic (unconditional, every 60 cycles)
        {
            static int entSizeTick = 0;
            if ((++entSizeTick % 60) == 0) {
                LOG("CACHE entity_size=%d (list after swap: P=%d N=%d A=%d W=%d)",
                    (int)entity_size,
                    (int)tempList.size(), (int)tempNpcList.size(),
                    (int)tempAnimalList.size(), (int)tempPrefabList.size());
            }
        }

        entity_mutex.lock();
        entity_List.swap(tempList);
        entity_mutex.unlock();

        npc_mutex.lock();
        npc_List.swap(tempNpcList);
        npc_mutex.unlock();

        animal_mutex.lock();
        animal_List.swap(tempAnimalList);
        animal_mutex.unlock();

        prefab_mutex.lock();
        prefab_List.swap(tempPrefabList);
        prefab_mutex.unlock();

        // AABB occluder swap disabled — ESP uses game visibility flags now

        // Merge slow data into g_EspCache — preserve fast-thread fields (bones, headPos, etc.)
        {
            std::unique_lock<std::shared_mutex> lk(g_EspCacheMutex);
            // Build a set of entities in the new cache for stale-entry removal
            std::unordered_set<uintptr_t> newKeys;
            for (auto& [k, v] : tempEspCache) {
                newKeys.insert(k);
                auto it = g_EspCache.find(k);
                if (it != g_EspCache.end()) {
                    // Merge: update slow fields, keep fast fields
                    if (!cacheStrEmpty(v.name)) cacheSetStr(it->second.name, v.name);
                    if (!cacheStrEmpty(v.weaponName)) cacheSetStr(it->second.weaponName, v.weaponName);
                    cacheSetStr32(it->second.animalType, v.animalType);
                    it->second.isDead = v.isDead;
                    it->second.isSleeping = v.isSleeping;
                    it->second.isWounded = v.isWounded;
                    it->second.isVisible = v.isVisible;
                    it->second.isVisibleRaw = v.isVisibleRaw;
                    it->second.isCrouching = v.isCrouching;
                    it->second.isGrounded = v.isGrounded;
                    it->second.health = v.health;
                    it->second.maxHealth = v.maxHealth;
                    it->second.teamId = v.teamId;
                    for (int i = 0; i < 6; i++) if (!cacheStrEmpty(v.belt[i])) cacheSetStr(it->second.belt[i], v.belt[i]);
                    for (int i = 0; i < 5; i++) if (!cacheStrEmpty(v.wear[i])) cacheSetStr(it->second.wear[i], v.wear[i]);
                    it->second.ammo = v.ammo;
                    it->second.invTick = v.invTick;
                    // Update bones for NPCs/animals (slow cache reads fresh bones for them).
                    // Players set skeletonValid=false in slow cache, so this won't clobber player bones
                    // (which are owned by the fast skeleton thread).
                    if (v.skeletonValid) {
                        for (int b = 0; b < 15; b++) it->second.bones[b] = v.bones[b];
                        it->second.skeletonValid = true;
                        it->second.skeletonTick = v.skeletonTick ? v.skeletonTick : GetTickCount64();
                    }
                    // Update position from slow cache if entity is new, or if slow cache has a fresher position.
                    // For players, the fast position thread sets a more recent cacheTick, so this won't overwrite.
                    // For animals/NPCs, the fast thread never updates them, so their cacheTick stays old
                    // and the slow cache will update positions every cycle.
                    if (it->second.cacheTick == 0 || v.cacheTick > it->second.cacheTick) {
                        it->second.headPos = v.headPos;
                        it->second.feetPos = v.feetPos;
                        it->second.cacheTick = v.cacheTick;
                        if (!v.velocity.Empty()) it->second.velocity = v.velocity;
                    }
                } else {
                    // New entity — insert with slow data
                    g_EspCache[k] = v;
                }
            }
            // Remove stale entries (players who left)
            for (auto it = g_EspCache.begin(); it != g_EspCache.end(); ) {
                if (newKeys.find(it->first) == newKeys.end())
                    it = g_EspCache.erase(it);
                else
                    ++it;
            }
        }

        ULONGLONG elapsed = GetTickCount64() - cycleStart;
        constexpr ULONGLONG kCacheTargetMs = 100;
        if (elapsed < kCacheTargetMs)
            std::this_thread::sleep_for(std::chrono::milliseconds(kCacheTargetMs - (DWORD)elapsed));
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// Position-only refresh — fast (~20ms for 50 players), updates headPos + velocity + camera
void cache::do_PositionRefresh() {
    const uint32_t threadEpoch = g_FastRefreshEpoch.load(std::memory_order_acquire);
    static bool posLog = false;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (threadEpoch != g_FastRefreshEpoch.load(std::memory_order_acquire)) {
            LOG("POS: epoch change detected, exiting old worker (epoch=%u)", threadEpoch);
            return;
        }
        if (MISC::ShutdownRequested) return;
        g_FastRefreshHeartbeatMs.store(GetTickCount64(), std::memory_order_release);

        static uint64_t posCycle = 0;
        static int posProcMiss = 0;
        if ((++posCycle % 20) == 0 && Drv) {
            if (!Drv->find_process(L"RustClient.exe")) {
                if (++posProcMiss >= 3) {
                    LOG("POS: RustClient.exe no longer running — shutting down to prevent BSOD");
                    g_process_dead.store(true, std::memory_order_relaxed);
                    MISC::ShutdownRequested = true;
                    return;
                }
            } else {
                posProcMiss = 0;
            }
        }

        if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) {
            g_LocalPlayerEyesPtr.store(0, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if (g_BnStableCycles.load(std::memory_order_relaxed) < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        // Cache LocalPlayer's PlayerEyes pointer when generation changes
        // This eliminates 708-IOCTL GetComponentByName() call every frame in silent aim
        {
            static uint64_t lastEyesGen = 0;
            uint64_t curGen = g_LocalPlayerGeneration.load(std::memory_order_acquire);
            if (curGen != lastEyesGen) {
                lastEyesGen = curGen;
                uintptr_t eyes = src->LocalPlayer->GetPlayerEyes();
                g_LocalPlayerEyesPtr.store(eyes, std::memory_order_relaxed);
                if (eyes) LOG("POS: cached LocalPlayer PlayerEyes=0x%I64X (gen=%llu)", eyes, curGen);
            }
        }

        auto posCycleStart = GetTickCount64();

        Vector3 localPos = src->LocalPlayer->Get_ObjectPosition();
        if (localPos.Empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Camera matrix read moved to render thread (ReadFrameCameraMatrix in Cheat.hpp)

        // Local player data — only read neck transform if OFFArrows needs it
        Vector3 cachedLocalNeck = {};
        Vector3 cachedBodyAngles = {};
        {
            if (ESP::OFFArrows) {
                auto localNeck = src->LocalPlayer->Get_Transformation(BoneList::neck);
                if (localNeck) cachedLocalNeck = localNeck->Get_Position();
            }
            uintptr_t input = read<uintptr_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::PlayerInput);
            if (input && is_valid(input))
                cachedBodyAngles = read<Vector3>(input + offsets::PlayerInput::bodyAngles);
        }
        uint64_t cachedTeam = 0;
        if (src->LocalPlayer && is_valid((uintptr_t)src->LocalPlayer))
            cachedTeam = read<uint64_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::CurrentTeam);
        {
            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
            g_LocalPlayerPos = localPos;
            g_LocalNeckPos = cachedLocalNeck;
            g_LocalBodyAngles = cachedBodyAngles;
            g_LocalPlayerTeam = cachedTeam;
        }

        // Camera matrix is now read FRESH in the render thread (ReadFrameCameraMatrix in Cheat.hpp)
        // Position thread only reads player positions + velocities — saves 7 IOCTLs per cycle

        // Player position refresh (only when other players exist)
        std::vector<Rust::BaseEntity*> playerList;
        {
            std::lock_guard<std::mutex> lk(entity_mutex);
            playerList = entity_List;
        }
        if (playerList.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Per-player: position via modelPtr (fresh read — no cross-cycle cache to avoid static init issues)
        for (size_t pi = 0; pi < playerList.size(); pi++) {
            auto* Player = playerList[pi];
            if (!Player || !is_valid((uintptr_t)Player)) continue;

            Vector3 pos;
            uintptr_t modelPtr = read<uintptr_t>((uintptr_t)Player + offsets::BasePlayer::PlayerModel);

            if (modelPtr && is_valid(modelPtr)) {
                pos = read<Vector3>(modelPtr + offsets::PlayerModel::position);
                if (pos.Empty()) {
                    modelPtr = read<uintptr_t>((uintptr_t)Player + offsets::BasePlayer::PlayerModel);
                    if (modelPtr && is_valid(modelPtr)) {
                        pos = read<Vector3>(modelPtr + offsets::PlayerModel::position);
                    }
                    if (pos.Empty()) continue;
                }
            } else {
                pos = Player->Get_ObjectPosition();
                if (pos.Empty()) continue;
            }

            // Read velocity for prediction (within range)
            Vector3 vel;
            if (localPos.DistTo(pos) <= 300.f)
                vel = Player->GetVelocity();

            uint64_t now = GetTickCount64();
            Vector3 feet = pos;
            Vector3 head = pos;
            head.y += 1.8f;
            {
                std::unique_lock<std::shared_mutex> lk(g_EspCacheMutex);
                auto it = g_EspCache.find((uintptr_t)Player);
                if (it != g_EspCache.end()) {
                    it->second.headPos = head;
                    it->second.feetPos = feet;
                    it->second.velocity = vel;
                    it->second.cacheTick = now;
                } else {
                    // Insert a minimal entry so ESP starts rendering immediately;
                    // the slow cache thread will fill in name/health/flags shortly.
                    EspCacheData ec;
                    ec.headPos = head;
                    ec.feetPos = feet;
                    ec.velocity = vel;
                    ec.cacheTick = now;
                    g_EspCache.emplace((uintptr_t)Player, ec);
                }
            }
        }

        if (!posLog) {
            LOG("POS: refresh thread running, players=%d", (int)playerList.size());
            posLog = true;
        }

        // NPC position refresh
        {
            std::vector<Rust::BaseEntity*> npcList;
            { std::lock_guard<std::mutex> lk(npc_mutex); npcList = npc_List; }
            for (auto* NPC : npcList) {
                if (!NPC || !is_valid((uintptr_t)NPC)) continue;
                auto fastPos = NPC->Get_ObjectPosition_Fast();
                auto pos = fastPos.Empty() ? NPC->Get_ObjectPosition() : fastPos;
                if (pos.Empty()) continue;
                Vector3 head = pos; head.y += 1.8f;
                uint64_t now = GetTickCount64();
                std::unique_lock<std::shared_mutex> lk(g_EspCacheMutex);
                auto it = g_EspCache.find((uintptr_t)NPC);
                if (it != g_EspCache.end()) {
                    it->second.headPos = head;
                    it->second.feetPos = pos;
                    it->second.cacheTick = now;
                }
            }
        }

        // Animal position refresh
        {
            std::vector<Rust::BaseEntity*> animalList;
            { std::lock_guard<std::mutex> lk(animal_mutex); animalList = animal_List; }
            for (auto* Animal : animalList) {
                if (!Animal || !is_valid((uintptr_t)Animal)) continue;
                auto fastPos = Animal->Get_ObjectPosition_Fast();
                auto pos = fastPos.Empty() ? Animal->Get_ObjectPosition() : fastPos;
                if (pos.Empty()) continue;
                uint64_t now = GetTickCount64();
                std::unique_lock<std::shared_mutex> lk(g_EspCacheMutex);
                auto it = g_EspCache.find((uintptr_t)Animal);
                if (it != g_EspCache.end()) {
                    it->second.headPos = pos;
                    it->second.feetPos = pos;
                    it->second.cacheTick = now;
                }
            }
        }

        {
            static int posCycleLogTick = 0;
            ULONGLONG elapsed = GetTickCount64() - posCycleStart;
            if ((++posCycleLogTick % 240) == 0 || elapsed > 50) {
                int npcC = 0, animalC = 0;
                { std::lock_guard<std::mutex> lk(npc_mutex); npcC = (int)npc_List.size(); }
                { std::lock_guard<std::mutex> lk(animal_mutex); animalC = (int)animal_List.size(); }
                LOG("POS cycle: %llums players=%d npcs=%d animals=%d", elapsed, (int)playerList.size(), npcC, animalC);
            }
        }

        // 1ms sleep at start of each iteration is sufficient — no conditional sleep needed
    }
}

// Skeleton refresh — reads 15 bones per player with per-player TTL caching
// Close players (≤20m): refreshed every 50ms (aimbot precision)
// Medium players (20-100m): refreshed every 100ms
// Far players (100-300m): refreshed every 200ms
// Each iteration only reads players whose cache has expired — staggered load
void cache::do_SkeletonRefresh() {
    const uint32_t threadEpoch = g_SkeletonEpoch.load(std::memory_order_acquire);
    static bool skelLog = false;

    static const BoneList skelBones[] = {
        BoneList::head, BoneList::neck, BoneList::spine1,
        BoneList::r_hip, BoneList::r_knee, BoneList::r_foot,
        BoneList::l_hip, BoneList::l_knee, BoneList::l_foot,
        BoneList::l_upperarm, BoneList::l_forearm, BoneList::l_hand,
        BoneList::r_upperarm, BoneList::r_forearm, BoneList::r_hand
    };

    // Per-player bone cache — array-based (safe for manually mapped DLLs)
    struct BoneCacheEntry {
        uintptr_t key;
        Vector3 bones[15];
        uint64_t lastReadMs;
        bool valid;
    };
    struct BoneHandleCacheEntry {
        uintptr_t key;
        Rust::BaseEntity::CachedTransformData handles[15];
        uint64_t lastRefreshMs;
        bool valid;
    };
    static BoneCacheEntry s_boneCacheArr[128];
    static int s_boneCacheCount = 0;
    static BoneHandleCacheEntry s_boneHandleCacheArr[128];
    static int s_boneHandleCacheCount = 0;
    static int s_cleanupTick = 0;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (threadEpoch != g_SkeletonEpoch.load(std::memory_order_acquire)) {
            LOG("SKEL: epoch change detected, exiting old worker (epoch=%u)", threadEpoch);
            return;
        }
        if (MISC::ShutdownRequested) return;
        g_SkeletonHeartbeatMs.store(GetTickCount64(), std::memory_order_release);

        static uint64_t skelCycle = 0;
        static int skelProcMiss = 0;
        if ((++skelCycle % 20) == 0 && Drv) {
            if (!Drv->find_process(L"RustClient.exe")) {
                if (++skelProcMiss >= 3) {
                    LOG("SKEL: RustClient.exe no longer running — shutting down to prevent BSOD");
                    g_process_dead.store(true, std::memory_order_relaxed);
                    MISC::ShutdownRequested = true;
                    return;
                }
            } else {
                skelProcMiss = 0;
            }
        }

        if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) continue;
        if (g_BnStableCycles.load(std::memory_order_relaxed) < 3) continue;

        std::vector<Rust::BaseEntity*> playerList;
        {
            std::lock_guard<std::mutex> lk(entity_mutex);
            playerList = entity_List;
        }
        if (playerList.empty()) continue;

        Vector3 localPos;
        {
            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
            localPos = g_LocalPlayerPos;
        }
        if (localPos.Empty()) continue;

        uint64_t now = GetTickCount64();

        // Find players whose bone cache has expired — only read those
        struct PendingRead { Rust::BaseEntity* p; float dist; uint64_t cacheAge; };
        std::vector<PendingRead> pending;
        {
            std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
            for (auto* Player : playerList) {
                if (!Player || !is_valid((uintptr_t)Player)) continue;
                auto it = g_EspCache.find((uintptr_t)Player);
                if (it == g_EspCache.end() || it->second.headPos.Empty()) continue;
                float d = localPos.DistTo(it->second.headPos);
                if (d > 300.f) continue;

                uint64_t cacheAge = 999999;
                for (int j = 0; j < s_boneCacheCount; j++) {
                    if (s_boneCacheArr[j].key == (uintptr_t)Player) {
                        cacheAge = now - s_boneCacheArr[j].lastReadMs;
                        break;
                    }
                }

                // TTL based on distance — closer players get fresher bones
                uint64_t ttl;
                if (d <= 20.f) ttl = 50;        // close: 50ms (aimbot precision)
                else if (d <= 100.f) ttl = 100; // medium: 100ms
                else ttl = 200;                  // far: 200ms

                if (cacheAge >= ttl) {
                    pending.push_back({ Player, d, cacheAge });
                }
            }
        }

        if (pending.empty()) continue;

        // Sort by cache age (oldest first) then by distance (closest first)
        std::sort(pending.begin(), pending.end(), [](const PendingRead& a, const PendingRead& b) {
            if (a.cacheAge != b.cacheAge) return a.cacheAge > b.cacheAge;
            return a.dist < b.dist;
        });

        auto skelCycleStart = GetTickCount64();
        int skelCount = 0;
        constexpr int kMaxPerIteration = 5;  // prevents IOCTL burst when all caches expire

        for (const auto& pr : pending) {
            if (skelCount >= kMaxPerIteration) break;
            auto* Player = pr.p;

            uintptr_t btf = Player->Get_BoneTransforms();
            if (!btf) continue;

            // Refresh bone handles every 200ms (saves 2 IOCTLs per bone per read)
            BoneHandleCacheEntry* hc = nullptr;
            for (int j = 0; j < s_boneHandleCacheCount; j++) {
                if (s_boneHandleCacheArr[j].key == (uintptr_t)Player) { hc = &s_boneHandleCacheArr[j]; break; }
            }
            if (!hc && s_boneHandleCacheCount < 128) {
                hc = &s_boneHandleCacheArr[s_boneHandleCacheCount++];
                hc->key = (uintptr_t)Player;
                hc->valid = false;
            }
            bool needsHandleRefresh = !hc || !hc->valid || (now - hc->lastRefreshMs >= 200);
            if (hc && needsHandleRefresh) {
                hc->valid = false;
                for (int b = 0; b < 15; b++) {
                    auto t = Player->Get_Transformation_Fast(skelBones[b], btf);
                    if (!t) { hc->handles[b] = {}; continue; }
                    hc->handles[b] = t->Get_Cached_Data();
                }
                hc->lastRefreshMs = GetTickCount64();
                hc->valid = true;
            }

            BoneCacheEntry bc;
            bc.key = (uintptr_t)Player;
            int validBones = 0;
            for (int b = 0; b < 15; b++) {
                Vector3 bp;
                // Fast path: use cached transform handles (skips 2 IOCTLs)
                if (hc && hc->handles[b].valid()) {
                    bp = Rust::BaseEntity::Transformation::Get_Position_From_Cache(hc->handles[b]);
                }
                // Fallback: full transform chain if cache invalid or returned empty
                if (bp.Empty()) {
                    auto t = Player->Get_Transformation_Fast(skelBones[b], btf);
                    if (!t) continue;
                    bp = t->Get_Position();
                }
                if (bp.Empty()) continue;
                bc.bones[b] = bp;
                validBones++;
            }
            bc.lastReadMs = GetTickCount64();
            bc.valid = (validBones >= 4);
            // Store in array cache
            bool bcFound = false;
            for (int j = 0; j < s_boneCacheCount; j++) {
                if (s_boneCacheArr[j].key == (uintptr_t)Player) {
                    s_boneCacheArr[j] = bc;
                    bcFound = true;
                    break;
                }
            }
            if (!bcFound && s_boneCacheCount < 128) {
                s_boneCacheArr[s_boneCacheCount++] = bc;
            }
            skelCount++;

            if (bc.valid) {
                std::unique_lock<std::shared_mutex> lk(g_EspCacheMutex);
                auto it = g_EspCache.find((uintptr_t)Player);
                if (it != g_EspCache.end()) {
                    for (int b = 0; b < 15; b++)
                        it->second.bones[b] = bc.bones[b];
                    it->second.boneAnchorPos = it->second.headPos;
                    it->second.skeletonValid = true;
                    it->second.skeletonTick = bc.lastReadMs;
                    if (!bc.bones[2].Empty())
                        it->second.spine1Pos = bc.bones[2];
                }
            }
        }

        // Periodic cleanup of stale bone cache + handle cache entries
        if (++s_cleanupTick >= 200) {
            s_cleanupTick = 0;
            // Simple: just reset arrays if they're getting full
            if (s_boneCacheCount > 100) s_boneCacheCount = 0;
            if (s_boneHandleCacheCount > 100) s_boneHandleCacheCount = 0;
        }

        if (!skelLog) {
            LOG("SKEL: refresh thread running, read %d/%d players (cache TTL)", skelCount, (int)pending.size());
            skelLog = true;
        }

        {
            static int skelCycleLogTick = 0;
            ULONGLONG elapsed = GetTickCount64() - skelCycleStart;
            if ((++skelCycleLogTick % 240) == 0 || elapsed > 100) {
                LOG("SKEL cycle: %llums read=%d pending=%d", elapsed, skelCount, (int)pending.size());
            }
        }
    }
}


