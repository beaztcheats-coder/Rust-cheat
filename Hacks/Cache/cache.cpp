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
std::mutex g_EspCacheMutex;
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
std::atomic<uint32_t> g_CacheThreadEpoch{ 0 };
std::atomic<uintptr_t> g_LocalPlayerAddr{ 0 };
std::atomic<uint64_t> g_LocalPlayerGeneration{ 1 };
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

    auto publishLocalPlayer = [&](Rust::BaseEntity* lp) {
        uintptr_t newAddr = (uintptr_t)lp;
        uintptr_t oldAddr = g_LocalPlayerAddr.exchange(newAddr, std::memory_order_acq_rel);
        if (oldAddr != newAddr) {
            g_LocalPlayerGeneration.fetch_add(1, std::memory_order_acq_rel);
        }
        if (src) src->LocalPlayer = lp;
    };

    while (true) {
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
            vischeck::InitVisCheck();
        }

        // === Process-death detection: if BN chain keeps failing, check if Rust is still alive ===
        if (failCount > 0 && (failCount % 80) == 0) {
            static int bnProcMiss = 0;
            if (Drv && !Drv->find_process(L"RustClient.exe")) {
                if (++bnProcMiss >= 3) {
                    LOG("CACHE: RustClient.exe no longer running (failCount=%d) — shutting down cheat to prevent BSOD", failCount);
                    MISC::ShutdownRequested = true;
                    return;
                }
            } else {
                bnProcMiss = 0;
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
                std::lock_guard<std::mutex> lk(g_EspCacheMutex);
                g_EspCache.clear();
            }
        };

        uint64_t basenetworkable = read<uint64_t>(GameAssembly + offsets::basenetworkable_pointer);
        if (!basenetworkable) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: basenetworkable=0 (GA+0x%I64X)", offsets::basenetworkable_pointer);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t bn_static_fields = read<uint64_t>(basenetworkable + offsets::BaseNetworkable::static_fields);
        if (!bn_static_fields) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: static_fields=0 (bn=0x%I64X +0x%I64X)", basenetworkable, offsets::BaseNetworkable::static_fields);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        uint64_t wrapper_class_ptr = read<uint64_t>(bn_static_fields + offsets::BaseNetworkable::wrapper_class);
        if (!wrapper_class_ptr) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: wrapper_class_ptr=0 (sf=0x%I64X +0x%I64X)", bn_static_fields, offsets::BaseNetworkable::wrapper_class);
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
        if (!parent_sf) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: parent_static_fields=0 (wc=0x%I64X +0x%I64X)", wrapper_class, offsets::BaseNetworkable::parent_static_fields);
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
        if (!entity) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: entity(BufferList)=0 (pc=0x%I64X +0x%I64X)", parent_class, offsets::BaseNetworkable::entity);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        auto entity_size = read<uint32_t>(entity + 0x18);
        if (!entity_size) {
            markUnavailable();
            continue;
        }
        if (entity_size > 50000) {
            markUnavailable();
            if (++failCount == 1) LOG("BN chain FAIL: entity_size unreasonable (%u)", entity_size);
            continue;
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
        if (!entity_array) {
            // Transient — no sleep needed
            markUnavailable();
            continue;
        }

        Rust::BaseEntity* localPlayer = read<Rust::BaseEntity*>((uint64_t)entity_array + 0x20);

        if (!localPlayer) {
            // Transient — no sleep needed
            markUnavailable();
            continue;
        }

        if (!is_valid((uintptr_t)localPlayer)) {
            markUnavailable();
            continue;
        }

        publishLocalPlayer(localPlayer);

        // BN chain succeeded — reset fail counter
        if (failCount > 0) {
            LOG("BN chain RECOVERED after %d failures", failCount);
            failCount = 0;
        }

        // Periodic CR3 re-validation: every ~3s, re-check the process is still alive
        if ((cacheCycleCount % 3) == 0 && Drv) {
            uint64_t current_cr3 = 0;
            if (!Drv->GetCR3_1(Drv->processid, false, current_cr3) || current_cr3 != Drv->process_cr3) {
                static int cr3ProcMiss = 0;
                if (!Drv->find_process(L"RustClient.exe")) {
                    if (++cr3ProcMiss >= 3) {
                        LOG("CACHE: RustClient.exe died during normal operation — shutting down to prevent BSOD");
                        MISC::ShutdownRequested = true;
                        return;
                    }
                } else {
                    cr3ProcMiss = 0;
                }
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
            if (!posDiag && !localPos.Empty()) {
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

        for (auto i = 0; i < entity_size; i++) {
            // Shutdown check + heartbeat every 50 entities (was 500 — too slow, caused BSOD)
            if (i > 0 && (i % 50) == 0) {
                g_CacheHeartbeatMs.store(GetTickCount64(), std::memory_order_release);
                if (MISC::ShutdownRequested) { LOG("CACHE: aborting at entity %d/%d (shutdown)", i, (int)entity_size); break; }
            }

            uintptr_t entity_ptr_raw = read<uintptr_t>((uint64_t)entity_array + 0x20 + (i * 0x8));
            if (!entity_ptr_raw) continue;
            
            // Entity pointers from buffer list are already valid GCHandles or pointers
            // Do NOT apply networkable_key - that reads from entity_ptr+0x18 (wrong memory)
            uintptr_t entity = (entity_ptr_raw & 1) ? decrypt::Il2cppGetHandle(entity_ptr_raw) : entity_ptr_raw;
            if (!is_valid(entity)) continue;

            uintptr_t object = read<uintptr_t>(entity + 0x10);
            if (!object) continue;

            uintptr_t objectClass = read<uintptr_t>(object + 0x20);
            if (!objectClass) continue;

            // Cache prefab name by objectClass pointer — avoids re-reading 128 bytes
            // for thousands of identical world objects every cycle.
            // TTL: re-read after 10 cycles to handle Il2Cpp address reuse
            struct PrefabCacheEntry { std::string name; uint64_t cycle; };
            static std::unordered_map<uintptr_t, PrefabCacheEntry> s_prefabNameCache;
            std::string buff;
            {
                auto pn = s_prefabNameCache.find(objectClass);
                if (pn != s_prefabNameCache.end() && (cacheCycleCount - pn->second.cycle) < 10) {
                    buff = pn->second.name;
                } else {
                    uintptr_t namePtr = read<uintptr_t>(objectClass + 0x50);
                    if (!namePtr || !is_valid(namePtr)) continue;
                    buff = read<monostr>(namePtr).buffer;
                    buff = std::string(buff.c_str());
                    if (buff.empty()) continue;
                    if (s_prefabNameCache.size() > 2000) s_prefabNameCache.clear();
                    s_prefabNameCache[objectClass] = { buff, cacheCycleCount };
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
                if (unmatchedCount < 200 && unmatchedLogged.find(buff) == unmatchedLogged.end()) {
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
            if (isAnimal && (low.find("trap") != std::string::npos || low.find("hide") != std::string::npos || low.find("skull") != std::string::npos)) isAnimal = false;

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
                            std::string className = read<monostr>(classNamePtr).buffer;
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

                // Single PlayerFlags read replaces IsSleeping / Is_Wounded calls
                int pFlags = read<int>((uintptr_t)Player + offsets::BasePlayer::PlayerFlags);
                bool isDead = Player->IsDead();
                if (isDead) continue;
                if ((pFlags & (int)offsets::base_player_flags::Sleeping) != 0 && ESP::RemoveSleepers) continue;
                if ((pFlags & (int)offsets::base_player_flags::Wounded) != 0 && ESP::RemoveWounded) continue;

                EspCacheData ec;

                ec.isDead = isDead;
                ec.isSleeping = (pFlags & (int)offsets::base_player_flags::Sleeping) != 0;
                ec.isWounded = (pFlags & (int)offsets::base_player_flags::Wounded) != 0;
                ec.health = read<float>((uintptr_t)Player + offsets::BaseCombatEntity::Health);
                ec.maxHealth = read<float>((uintptr_t)Player + offsets::BaseCombatEntity::MaxHealth);
                ec.teamId = read<uint64_t>((uintptr_t)Player + offsets::BasePlayer::CurrentTeam);

                if (Distance <= 400.f || (cacheCycleCount % 2) == 0) {
                    ec.isVisibleRaw = Player->IsVisibleRawFlag();
                    ec.isVisible = Player->IsVisibleFiltered(ec.isVisibleRaw);
                } else {
                    ec.isVisibleRaw = false;
                    ec.isVisible = false;
                }

                {
                    uintptr_t ms = read<uintptr_t>((uintptr_t)Player + offsets::BasePlayer::ModelState);
                    if (ms && is_valid(ms)) {
                        int msFlags = read<int>(ms + offsets::ModelState::flags);
                        ec.isCrouching = (msFlags & offsets::ModelState::Ducked) != 0;
                        ec.isGrounded = (msFlags & offsets::ModelState::OnGround) != 0;
                    }
                }

                // Position is set here; velocity is updated by the position-only thread
                ec.headPos = pos;

                // BVH raycast is PRIMARY visibility check when VisCheck enabled
                if (ESP::VisCheck && vischeck::g_VisCheckLoaded && Distance <= 400.f) {
                    Vector3 camPos;
                    { std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex); camPos = g_CameraWorldPos; }
                    if (!camPos.Empty()) {
                        bool rayVisible = vischeck::g_VisCheck.IsVisible(camPos, ec.headPos);
                        ec.isVisible = rayVisible;
                        ec.isVisibleRaw = rayVisible;
                    }
                }

                // Name / held item: lower LOD for distant players
                if (Distance <= 100.f || (Distance <= 300.f && (cacheCycleCount % 2) == 0)) {
                    ec.name = Player->GetName();
                    if (Distance <= 100.f || (Distance <= 200.f && (cacheCycleCount % 4) == 0)) {
                        auto* h = Player->Get_HeldItem();
                        ec.weaponName = h ? h->get_item_name() : "";
                    }
                }
                ec.skeletonValid = false;

                // Inventory is the most expensive block after skeletons; only read it for close players
                if (Distance <= 100.f && (ESP::hotbar_text || ESP::Clothing || ESP::ItemList || ESP::AmmoBar) && (cacheCycleCount % 4) == 0) {
                    auto belt = Player->Get_Hotbar_list();
                    for (size_t i = 0; i < belt.size() && i < 6; i++) {
                        if (belt[i]) ec.belt[i] = belt[i]->get_item_name();
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
                                    ec.wear[i] = wi->get_item_name();
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
                    }
                }
                if (NPC_ESP::HeldItem) {
                    auto* held = NPC->Get_HeldItem();
                    if (held) ec.weaponName = held->get_item_name();
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
                aec.animalType = animalType;
                aec.isDead = animalDead;
                aec.health = read<float>((uintptr_t)Animal + offsets::BaseCombatEntity::Health);
                aec.maxHealth = read<float>((uintptr_t)Animal + offsets::BaseCombatEntity::MaxHealth);
                tempAnimalList.push_back(Animal);
            }
            else {
                // AABB occluder collection disabled — ESP uses game visibility flags now

                // WORLD PREFAB: skip entirely if no world ESP toggle is active
                if (!WORLD::AnyEspEnabled()) continue;

                Vector3 EntityPosition = ((Rust::BaseEntity*)entity)->Get_ObjectPosition();
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
                if ( WORLD::Stash && low.find("small_stash_deployed.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Stash");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Stash_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Metal && low.find("metal-ore.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Metal Ore");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Metal_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Stone && low.find("stone-ore.prefab") != std::string::npos)
                {
                    Rust::PrefabData data;
                    data.name = TR("Stone Ore");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Stone_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::Sulfer && low.find("sulfur-ore.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Sulfer Ore");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Sulfer_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::BodyBag && low.find("player_corpse.prefab") != std::string::npos )
                {
                    Rust::PrefabData data;
                    data.name = TR("Corpse");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::BodyBag_Color;
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
                if (WORLD::Backpack && low.find("item_drop_backpack.prefab") != std::string::npos)
                {
                    Rust::PrefabData data;
                    data.name = TR("Backpack");
                    data.Position = EntityPosition;
                    data.Color = WORLD::color::Backpack_Color;
                    tempPrefabList.push_back(data);
                }
                if (WORLD::BradlyAPC && low.find("bradleyapc.prefab") != std::string::npos)
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
                                        std::string rawName = read<monostr>(namePtr).buffer;
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
            std::lock_guard<std::mutex> lk(g_EspCacheMutex);
            // Build a set of entities in the new cache for stale-entry removal
            std::unordered_set<uintptr_t> newKeys;
            for (auto& [k, v] : tempEspCache) {
                newKeys.insert(k);
                auto it = g_EspCache.find(k);
                if (it != g_EspCache.end()) {
                    // Merge: update slow fields, keep fast fields
                    if (!v.name.empty()) it->second.name = v.name;
                    if (!v.weaponName.empty()) it->second.weaponName = v.weaponName;
                    it->second.animalType = v.animalType;
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
                    for (int i = 0; i < 6; i++) if (!v.belt[i].empty()) it->second.belt[i] = v.belt[i];
                    for (int i = 0; i < 5; i++) if (!v.wear[i].empty()) it->second.wear[i] = v.wear[i];
                    it->second.ammo = v.ammo;
                    it->second.invTick = v.invTick;
                    it->second.headPos = v.headPos;
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
        constexpr ULONGLONG kCacheTargetMs = 250;
        if (elapsed < kCacheTargetMs)
            std::this_thread::sleep_for(std::chrono::milliseconds(kCacheTargetMs - (DWORD)elapsed));
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

// Position-only refresh — fast (~20ms for 50 players), updates headPos + velocity + camera
void cache::do_PositionRefresh() {
    const uint32_t threadEpoch = g_FastRefreshEpoch.load(std::memory_order_acquire);
    static bool posLog = false;

    while (true) {
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
                    MISC::ShutdownRequested = true;
                    return;
                }
            } else {
                posProcMiss = 0;
            }
        }

        if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if (g_BnStableCycles.load(std::memory_order_relaxed) < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        auto posCycleStart = GetTickCount64();

        Vector3 localPos = src->LocalPlayer->Get_ObjectPosition();
        if (localPos.Empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Camera matrix (7 IOCTLs) — must run even when playerList is empty (needed for ALL ESP)
        {
            static int camDiag = 0;
            do {
                uint64_t cam_typeinfo = read<uint64_t>(GameAssembly + offsets::camera_pointer);
                if (!cam_typeinfo || !is_valid(cam_typeinfo)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_typeinfo invalid (0x%I64X)", cam_typeinfo); camDiag++; } break; }
                uint64_t cam_sf = read<uint64_t>(cam_typeinfo + offsets::BaseCamera::static_fields);
                if (!cam_sf || !is_valid(cam_sf)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_sf invalid (0x%I64X)", cam_sf); camDiag++; } break; }
                uint64_t cam_instance = read<uint64_t>(cam_sf + offsets::BaseCamera::wrapper_class);
                if (!cam_instance || !is_valid(cam_instance)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_instance invalid (0x%I64X)", cam_instance); camDiag++; } break; }
                uint64_t native_cam = read<uint64_t>(cam_instance + offsets::BaseCamera::entity);
                if (!native_cam || !is_valid(native_cam)) native_cam = cam_instance;
                if (!native_cam || !is_valid(native_cam)) { if (camDiag < 5) { LOG("CAM_CHAIN: native_cam invalid"); camDiag++; } break; }
                float fov_test = read<float>(native_cam + offsets::BaseCamera::field_of_view);
                if (!(fov_test > 1.0f && fov_test < 180.0f && std::isfinite(fov_test))) {
                    if (camDiag < 5) { float f170 = read<float>(native_cam + 0x170); float f4c = read<float>(native_cam + 0x4C); LOG("CAM_CHAIN: fov=%.2f(0x170) FAILED -- 0x170=%.2f 0x4C=%.2f ncam=0x%I64X", fov_test, f170, f4c, native_cam); camDiag++; }
                    break;
                }

                Vector3 camWorldPos = read<Vector3>(native_cam + offsets::BaseCamera::world_position);
                {
                    std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
                    g_CameraWorldPos = camWorldPos;
                }

                Matrix4x4 viewCM = read<Matrix4x4>(native_cam + offsets::BaseCamera::viewMatrix);
                Matrix4x4 projCM = read<Matrix4x4>(native_cam + offsets::BaseCamera::projectionMatrix);
                if (!std::isfinite(viewCM._11) || !std::isfinite(projCM._11) || viewCM._11 == 0 || projCM._11 == 0) {
                    if (camDiag < 5) {
                        float v_2fc = read<float>(native_cam + 0x2FC);
                        float v_24c = read<float>(native_cam + 0x24C);
                        float p_18c = read<float>(native_cam + 0x18C);
                        float p_b0 = read<float>(native_cam + 0xB0);
                        float p_144 = read<float>(native_cam + 0x144);
                        LOG("CAM_CHAIN: matrix zero -- v._11=%.4f(0x2FC) p._11=%.4f(0x18C) | scan: 0x2FC=%.4f 0x24C=%.4f 0x18C=%.4f 0xB0=%.4f 0x144=%.4f",
                            viewCM._11, projCM._11, v_2fc, v_24c, p_18c, p_b0, p_144);
                        camDiag++;
                    }
                    break;
                }
                if (camDiag < 3) { LOG("CAM_CHAIN: OK ncam=0x%I64X fov=%.1f v._11=%.4f p._11=%.4f", native_cam, fov_test, viewCM._11, projCM._11); camDiag++; }
                Matrix4x4 viewRM, projRM;
                viewRM._11=viewCM._11; viewRM._12=viewCM._21; viewRM._13=viewCM._31; viewRM._14=viewCM._41;
                viewRM._21=viewCM._12; viewRM._22=viewCM._22; viewRM._23=viewCM._32; viewRM._24=viewCM._42;
                viewRM._31=viewCM._13; viewRM._32=viewCM._23; viewRM._33=viewCM._33; viewRM._34=viewCM._43;
                viewRM._41=viewCM._14; viewRM._42=viewCM._24; viewRM._43=viewCM._34; viewRM._44=viewCM._44;
                projRM._11=projCM._11; projRM._12=projCM._21; projRM._13=projCM._31; projRM._14=projCM._41;
                projRM._21=projCM._12; projRM._22=projCM._22; projRM._23=projCM._32; projRM._24=projCM._42;
                projRM._31=projCM._13; projRM._32=projCM._23; projRM._33=projCM._33; projRM._34=projCM._43;
                projRM._41=projCM._14; projRM._42=projCM._24; projRM._43=projCM._34; projRM._44=projCM._44;
                Matrix4x4 vp{};
                vp._11=projRM._11*viewRM._11+projRM._12*viewRM._21+projRM._13*viewRM._31+projRM._14*viewRM._41;
                vp._12=projRM._11*viewRM._12+projRM._12*viewRM._22+projRM._13*viewRM._32+projRM._14*viewRM._42;
                vp._13=projRM._11*viewRM._13+projRM._12*viewRM._23+projRM._13*viewRM._33+projRM._14*viewRM._43;
                vp._14=projRM._11*viewRM._14+projRM._12*viewRM._24+projRM._13*viewRM._34+projRM._14*viewRM._44;
                vp._21=projRM._21*viewRM._11+projRM._22*viewRM._21+projRM._23*viewRM._31+projRM._24*viewRM._41;
                vp._22=projRM._21*viewRM._12+projRM._22*viewRM._22+projRM._23*viewRM._32+projRM._24*viewRM._42;
                vp._23=projRM._21*viewRM._13+projRM._22*viewRM._23+projRM._23*viewRM._33+projRM._24*viewRM._43;
                vp._24=projRM._21*viewRM._14+projRM._22*viewRM._24+projRM._23*viewRM._34+projRM._24*viewRM._44;
                vp._31=projRM._31*viewRM._11+projRM._32*viewRM._21+projRM._33*viewRM._31+projRM._34*viewRM._41;
                vp._32=projRM._31*viewRM._12+projRM._32*viewRM._22+projRM._33*viewRM._32+projRM._34*viewRM._42;
                vp._33=projRM._31*viewRM._13+projRM._32*viewRM._23+projRM._33*viewRM._33+projRM._34*viewRM._43;
                vp._34=projRM._31*viewRM._14+projRM._32*viewRM._24+projRM._33*viewRM._34+projRM._34*viewRM._44;
                vp._41=projRM._41*viewRM._11+projRM._42*viewRM._21+projRM._43*viewRM._31+projRM._44*viewRM._41;
                vp._42=projRM._41*viewRM._12+projRM._42*viewRM._22+projRM._43*viewRM._32+projRM._44*viewRM._42;
                vp._43=projRM._41*viewRM._13+projRM._42*viewRM._23+projRM._43*viewRM._33+projRM._44*viewRM._43;
                vp._44=projRM._41*viewRM._14+projRM._42*viewRM._24+projRM._43*viewRM._34+projRM._44*viewRM._44;
                // Write to back buffer, then atomically publish via sequence increment
                src->matrixBackBuffer._11=vp._11; src->matrixBackBuffer._12=vp._21; src->matrixBackBuffer._13=vp._31; src->matrixBackBuffer._14=vp._41;
                src->matrixBackBuffer._21=vp._12; src->matrixBackBuffer._22=vp._22; src->matrixBackBuffer._23=vp._32; src->matrixBackBuffer._24=vp._42;
                src->matrixBackBuffer._31=vp._13; src->matrixBackBuffer._32=vp._23; src->matrixBackBuffer._33=vp._33; src->matrixBackBuffer._34=vp._43;
                src->matrixBackBuffer._41=vp._14; src->matrixBackBuffer._42=vp._24; src->matrixBackBuffer._43=vp._34; src->matrixBackBuffer._44=vp._44;
                // Also update LocalPlayer_Matrix for backwards compat (render.hpp FOV)
                src->LocalPlayer_Matrix = src->matrixBackBuffer;
                // Publish — InterlockedIncrement acts as a full memory barrier
                InterlockedIncrement(&src->matrixSequence);
            } while(false);
        }

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

        // Per-player: ONLY root position + velocity (cheap: 8 IOCTLs each)
        for (auto* Player : playerList) {
            if (!Player || !is_valid((uintptr_t)Player)) continue;

            auto pos = Player->Get_ObjectPosition();
            if (pos.Empty()) continue;

            Vector3 vel;
            if (localPos.DistTo(pos) <= 300.f)
                vel = Player->GetVelocity();

            uint64_t now = GetTickCount64();
            Vector3 feet = pos;
            Vector3 head = pos;
            head.y += 1.8f;
            {
                std::lock_guard<std::mutex> lk(g_EspCacheMutex);
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

        {
            static int posCycleLogTick = 0;
            ULONGLONG elapsed = GetTickCount64() - posCycleStart;
            if ((++posCycleLogTick % 240) == 0 || elapsed > 50) {
                LOG("POS cycle: %llums players=%d", elapsed, (int)playerList.size());
            }
        }

        ULONGLONG elapsed = GetTickCount64() - posCycleStart;
        constexpr ULONGLONG kPosTargetMs = 8;
        if (elapsed < kPosTargetMs)
            std::this_thread::sleep_for(std::chrono::milliseconds(kPosTargetMs - (DWORD)elapsed));
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

// Skeleton + feet refresh — slower (~68ms for 10 close players), updates bones + head/feet
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

    while (true) {
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
                    MISC::ShutdownRequested = true;
                    return;
                }
            } else {
                skelProcMiss = 0;
            }
        }

        if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        if (g_BnStableCycles.load(std::memory_order_relaxed) < 3) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            continue;
        }

        std::vector<Rust::BaseEntity*> playerList;
        {
            std::lock_guard<std::mutex> lk(entity_mutex);
            playerList = entity_List;
        }
        if (playerList.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        Vector3 localPos;
        {
            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
            localPos = g_LocalPlayerPos;
        }
        if (localPos.Empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        constexpr float kFullSkeletonDist = 20.f;
        constexpr float kPartialSkeletonDist = 300.f;

        struct DistPlayer { Rust::BaseEntity* p; float d; };
        std::vector<DistPlayer> closePlayers;
        {
            std::lock_guard<std::mutex> lk(g_EspCacheMutex);
            for (auto* Player : playerList) {
                if (!Player || !is_valid((uintptr_t)Player)) continue;
                auto it = g_EspCache.find((uintptr_t)Player);
                if (it == g_EspCache.end() || it->second.headPos.Empty()) continue;
                float d = localPos.DistTo(it->second.headPos);
                if (d <= kPartialSkeletonDist) closePlayers.push_back({ Player, d });
            }
        }
        if (closePlayers.size() > 1) {
            std::sort(closePlayers.begin(), closePlayers.end(), [](const DistPlayer& a, const DistPlayer& b) {
                return a.d < b.d;
            });
        }

        auto skelCycleStart = GetTickCount64();
        int skelCount = 0;
        for (const auto& dp : closePlayers) {
            auto* Player = dp.p;
            float dist = dp.d;

            uintptr_t btf = Player->Get_BoneTransforms();
            if (!btf) continue;

            Vector3 fastBones[15] = {};
            bool fastSkelValid = false;

            int validBones = 0;
            if (dist <= kFullSkeletonDist) {
                for (int b = 0; b < 15; b++) {
                    auto t = Player->Get_Transformation_Fast(skelBones[b], btf);
                    if (!t) continue;
                    Vector3 bp = t->Get_Position();
                    if (bp.Empty()) continue;
                    fastBones[b] = bp;
                    validBones++;
                }
            } else {
                // 9 key bones: head, neck, spine1, r_hip, r_knee, r_foot, l_hip, l_knee, l_foot
                // Forms a connected upper-body + legs skeleton (7 drawable lines)
                static const int keyIdx[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
                for (int k = 0; k < 9; k++) {
                    int b = keyIdx[k];
                    auto t = Player->Get_Transformation_Fast(skelBones[b], btf);
                    if (!t) continue;
                    Vector3 bp = t->Get_Position();
                    if (bp.Empty()) continue;
                    fastBones[b] = bp;
                    validBones++;
                }
            }
            fastSkelValid = (validBones >= 4);
            skelCount++;

            // Per-bone interpolation on the skeleton thread — smooths between reads
            // so the render thread gets pre-stabilized bones (no 15 lerps/player on render thread)
            {
                struct SkelInterpState { Vector3 bones[15]; uint64_t tick; };
                static std::unordered_map<uintptr_t, SkelInterpState> prevBones;
                static int cleanupTick = 0;

                if (fastSkelValid) {
                    // Read player velocity from cache for adaptive alpha
                    float speedSq = 0.0f;
                    {
                        std::lock_guard<std::mutex> lk(g_EspCacheMutex);
                        auto cit = g_EspCache.find((uintptr_t)Player);
                        if (cit != g_EspCache.end()) {
                            auto& v = cit->second.velocity;
                            speedSq = v.x*v.x + v.y*v.y + v.z*v.z;
                        }
                    }
                    // Adaptive alpha: fast-moving players get near-zero smoothing (prevents lag)
                    // stationary players get moderate smoothing (prevents jitter)
                    float alpha = (speedSq > 9.0f) ? 0.92f : 0.80f;  // 3 m/s threshold

                    auto pit = prevBones.find((uintptr_t)Player);
                    if (pit != prevBones.end()) {
                        for (int b = 0; b < 15; b++) {
                            if (fastBones[b].Empty()) continue;
                            if (pit->second.bones[b].Empty()) continue;
                            // Skip absurd teleports
                            float d = fastBones[b].DistTo(pit->second.bones[b]);
                            if (d > 5.0f) continue;
                            fastBones[b] = pit->second.bones[b] * (1.0f - alpha) + fastBones[b] * alpha;
                        }
                    }
                    SkelInterpState st{};
                    for (int b = 0; b < 15; b++) st.bones[b] = fastBones[b];
                    st.tick = GetTickCount64();
                    prevBones[(uintptr_t)Player] = st;
                }

                // Periodic cleanup of stale entries
                if (++cleanupTick >= 200) {
                    cleanupTick = 0;
                    uint64_t now = GetTickCount64();
                    for (auto it2 = prevBones.begin(); it2 != prevBones.end(); ) {
                        if (now - it2->second.tick > 5000) it2 = prevBones.erase(it2);
                        else ++it2;
                    }
                }
            }

            {
                std::lock_guard<std::mutex> lk(g_EspCacheMutex);
                auto it = g_EspCache.find((uintptr_t)Player);
                if (it != g_EspCache.end()) {
                    // Only write skeleton data — never overwrite headPos/feetPos,
                    // those stay locked to the real-time position thread.
                    if (fastSkelValid) {
                        for (int b = 0; b < 15; b++)
                            it->second.bones[b] = fastBones[b];
                        it->second.skeletonValid = true;
                        it->second.skeletonTick = GetTickCount64();
                        if (!fastBones[2].Empty())
                            it->second.spine1Pos = fastBones[2];
                    }
                }
            }
        }

        if (!skelLog) {
            LOG("SKEL: refresh thread running, close players=%d", skelCount);
            skelLog = true;
        }

        {
            static int skelCycleLogTick = 0;
            ULONGLONG elapsed = GetTickCount64() - skelCycleStart;
            if ((++skelCycleLogTick % 240) == 0 || elapsed > 100) {
                LOG("SKEL cycle: %llums players=%d", elapsed, skelCount);
            }
        }

        ULONGLONG elapsed = GetTickCount64() - skelCycleStart;
        constexpr ULONGLONG kSkeletonTargetMs = 12;
        if (elapsed < kSkeletonTargetMs)
            std::this_thread::sleep_for(std::chrono::milliseconds(kSkeletonTargetMs - (DWORD)elapsed));
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}


