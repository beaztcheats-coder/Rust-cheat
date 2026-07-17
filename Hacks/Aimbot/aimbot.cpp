#include "aimbot.hpp"
#include "../Cache/cache.hpp"
#include <vector>
#include "../../Logger.hpp"
#include <unordered_map>
#include <cmath>

// Lightweight cache copy for aimbot — avoids copying 13 strings per player
struct AimbotCacheData {
    Vector3 headPos;
    Vector3 velocity;
    Vector3 bones[15];
    std::string weaponName;
    std::string name;
    uint64_t teamId = 0;
    bool skeletonValid = false;
    bool isDead = false;
    bool isSleeping = false;
    bool isWounded = false;
    bool isVisible = false;
    bool isVisibleRaw = false;
    bool isCrouching = false;
    bool isGrounded = true;
    uint64_t cacheTick = 0;
};

static AimbotCacheData CopyAimbotCache(const EspCacheData& src) {
    AimbotCacheData d;
    d.headPos = src.headPos;
    d.velocity = src.velocity;
    for (int i = 0; i < 15; i++) d.bones[i] = src.bones[i];
    d.weaponName = std::string(src.weaponName);
    d.name = std::string(src.name);
    d.teamId = src.teamId;
    d.skeletonValid = src.skeletonValid;
    d.isDead = src.isDead;
    d.isSleeping = src.isSleeping;
    d.isWounded = src.isWounded;
    d.isVisible = src.isVisible;
    d.isVisibleRaw = src.isVisibleRaw;
    d.isCrouching = src.isCrouching;
    d.isGrounded = src.isGrounded;
    d.cacheTick = src.cacheTick;
    return d;
}

static float GetFrameDt()
{
    static ULONGLONG last = 0;
    ULONGLONG now = GetTickCount64();
    float dt = (last == 0) ? 0.016f : (float)(now - last) / 1000.f;
    last = now;
    if (dt <= 0.f || dt > 0.5f) dt = 0.016f;
    return dt;
}

static std::unordered_map<uintptr_t, Vector3> prevPositions;

static int GetScreenMidX() { return g_ScreenW > 0 ? g_ScreenW / 2 : GetSystemMetrics(SM_CXSCREEN) / 2; }
static int GetScreenMidY() { return g_ScreenH > 0 ? g_ScreenH / 2 : GetSystemMetrics(SM_CYSCREEN) / 2; }

static bool ScreenPointValid(const Vector2& p)
{
    int sw = g_ScreenW > 0 ? g_ScreenW : GetSystemMetrics(SM_CXSCREEN);
    int sh = g_ScreenH > 0 ? g_ScreenH : GetSystemMetrics(SM_CYSCREEN);
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) return false;
    if (p.x <= 1.0f || p.y <= 1.0f) return false;
    if (p.x >= (float)sw - 1.0f || p.y >= (float)sh - 1.0f) return false;
    return true;
}

static uint32_t s_hRngState = 0x1234ABCD;
static inline float h_rand() {
    s_hRngState = s_hRngState * 1664525u + 1013904223u;
    return (float)((s_hRngState >> 8) & 0xFFFFFF) / (float)0x1000000;
}
static inline float h_gauss() {
    float u1 = h_rand(); float u2 = h_rand();
    if (u1 < 0.0001f) u1 = 0.0001f;
    return sqrtf(-2.f * logf(u1)) * cosf(2.f * 3.14159265f * u2);
}

static int GetBestBone(Rust::BaseEntity* player)
{
    // 0=Head, 1=Neck, 2=Chest, 3=Pelvis, 4=ClosestToCrosshair, 5=Smart
    int selectedBone;
    if (AIMBOT::BonePriority <= 3) {
        static const int singleBones[] = { 53, 52, 20, 14 };
        selectedBone = singleBones[AIMBOT::BonePriority];
    } else {
    // Priority 4/5: use CACHED bone positions
    static const int bonePool[] = { 53, 52, 20, 14 };
    static const int cacheIdx[] = { 0, 1, 2, 3 };
    static const int boneCount = 4;

    int screenMidX = GetScreenMidX();
    int screenMidY = GetScreenMidY();

    if (AIMBOT::BonePriority == 4 || AIMBOT::BonePriority == 5) {
        int bestBone = 53;
        float bestDist = FLT_MAX;

        Vector3 cachedBones[15] = {};
        Vector3 cachedHead;
        bool hasCache = false;
        bool isCrouching = false;
        {
            std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
            auto cd = g_EspCache.find((uintptr_t)player);
            if (cd != g_EspCache.end()) {
                cachedHead = cd->second.headPos;
                if (cd->second.skeletonValid) {
                    hasCache = true;
                    for (int bi = 0; bi < 15; ++bi) cachedBones[bi] = cd->second.bones[bi];
                }
                isCrouching = cd->second.isCrouching;
            }
        }

        for (int i = 0; i < boneCount; i++) {
            Vector3 pos;
            if (hasCache && cacheIdx[i] >= 0) {
                pos = cachedBones[cacheIdx[i]];
            } else {
                // Use cached headPos — NO live IOCTL reads through entity pointers
                pos = cachedHead;
                if (bonePool[i] == 53) pos.y += isCrouching ? 1.05f : 1.65f;
                else if (bonePool[i] == 52) pos.y += isCrouching ? 0.90f : 1.50f;
                else if (bonePool[i] == 20) pos.y += isCrouching ? 0.65f : 1.20f;
                else if (bonePool[i] == 14) pos.y += isCrouching ? 0.25f : 0.50f;
            }
            if (pos.Empty()) continue;
            auto screen = WorldToScreen(pos, src->GetMatrixSnapshot());
            if (screen.Empty()) continue;
            float dx = (float)(screen.x - screenMidX);
            float dy = (float)(screen.y - screenMidY);
            float dist = dx * dx + dy * dy;
            if (dist < bestDist) {
                bestDist = dist;
                bestBone = bonePool[i];
            }
        }

        // Priority 5 (Smart): prefer head if within 1.5x FOV
        if (AIMBOT::BonePriority == 5 && bestBone != 53) {
            if (bestDist <= (float)(AIMBOT::FovSize * AIMBOT::FovSize) * 2.25f)
                bestBone = 53;
        }

        selectedBone = bestBone;
    } else {
        selectedBone = AIMBOT::BoneSelector;
    }
    } // close else (BonePriority > 3)

    return selectedBone;
}

static Vector3 GetPredictedTargetPos(Rust::BaseEntity* player, int bone)
{
    // Use ONLY cached data — NO live IOCTL reads through potentially stale entity pointers
    Vector3 rootPos;
    {
        std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
        auto cd = g_EspCache.find((uintptr_t)player);
        if (cd != g_EspCache.end()) {
            rootPos = cd->second.headPos;
        }
    }
    if (rootPos.Empty()) return Vector3();

    // Try cached skeleton bones first (no IOCTL calls)
    // Cache index: 0=head(53), 1=neck(52), 2=spine1(20)
    static const int boneToCache[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, 3, -1, -1, -1, -1, -1,  // 0-19: r_hip=14→cache[3]
        2, -1, -1, -1, -1, 9, 10, -1, -1, 11,   // 20: spine1→cache[2], 25=l_upperarm→9, 26=l_forearm→10, 29=l_hand→11
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, 1, 0, -1, -1, -1, -1, -1, -1,   // 52=neck→1, 53=head→0
        -1, -1, -1, -1, -1, -1, -1, 12, 13, 14  // 60→-1, 61=r_upperarm→12, 62=r_forearm→13, 65=r_hand→14
    };

    Vector3 pos;
    int cacheIdx = (bone >= 0 && bone < 66) ? boneToCache[bone] : -1;
    if (cacheIdx >= 0) {
        std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
        auto cd = g_EspCache.find((uintptr_t)player);
        if (cd != g_EspCache.end() && cd->second.skeletonValid) {
            pos = cd->second.bones[cacheIdx];
        }
    }

    // Fallback: root + crouch-aware height offset
    if (pos.Empty()) {
        pos = rootPos;
        // Use cached crouch state (zero IOCTLs)
        bool isCrouching = false;
        {
            std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
            auto cd = g_EspCache.find((uintptr_t)player);
            if (cd != g_EspCache.end()) isCrouching = cd->second.isCrouching;
        }
        if (bone == 53) pos.y += isCrouching ? 1.05f : 1.65f;
        else if (bone == 52) pos.y += isCrouching ? 0.90f : 1.50f;
        else if (bone == 20) pos.y += isCrouching ? 0.65f : 1.20f;
        else if (bone == 14) pos.y += isCrouching ? 0.25f : 0.50f;
        else pos.y += isCrouching ? 0.50f : 1.0f;
    }

    if (pos.Empty()) return pos;

    if (AIMBOT::PredictionScale <= 0.001f) return pos;

    // Cap prevPositions growth
    {
        static int cleanTick = 0;
        if (++cleanTick >= 600) { cleanTick = 0; if (prevPositions.size() > 512) prevPositions.clear(); }
    }

    uintptr_t key = (uintptr_t)player;
    auto it = prevPositions.find(key);
    float dt = GetFrameDt();

    if (it != prevPositions.end()) {
        Vector3 diff = pos - it->second;
        float idt = 1.f / dt;
        Vector3 vel(diff.x * idt, diff.y * idt, diff.z * idt);
        float speed = vel.Length();
        if (speed > 0.1f && speed < 50.f) {
            Vector3 predicted = pos + vel * (dt * AIMBOT::PredictionScale);
            // Gravity compensation for bullet drop
            predicted.y += 4.905f * (dt * AIMBOT::PredictionScale) * (dt * AIMBOT::PredictionScale);
            prevPositions[key] = pos;
            return predicted;
        }
    }
    prevPositions[key] = pos;
    return pos;
}

Rust::BaseEntity* aimbot::Get_BestEntity()
{
    if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) return nullptr;

    float Closest = FLT_MAX;
    Rust::BaseEntity* RPlayer = nullptr;

    cac->entity_mutex.lock();
    std::vector<Rust::BaseEntity*> list = entity_List;
    cac->entity_mutex.unlock();

    if (AIMBOT::KeepTarget && lastTarget) {
        bool still_valid = false;
        for (const auto& p : list) {
            if (p == lastTarget && p && !p->IsDead()) {
                // Use cached bones instead of live Get_Transformation
                EspCacheData tc;
                bool hasTc = false;
                {
                    std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
                    auto it = g_EspCache.find((uintptr_t)p);
                    if (it != g_EspCache.end()) { tc = it->second; hasTc = true; }
                }
                if (hasTc) {
                    int bone = GetBestBone(p);
                    static const int boneToCache[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, -1, -1, 3, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, 9, 10, -1, -1, 11,
                        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14
                    };
                    Vector3 pos;
                    int ci = (bone >= 0 && bone < 66) ? boneToCache[bone] : -1;
                    if (ci >= 0 && tc.skeletonValid) pos = tc.bones[ci];
                    if (pos.Empty()) pos = tc.headPos;
                    if (!pos.Empty()) {
                        auto screen = WorldToScreen(pos, src->GetMatrixSnapshot());
                        if (!screen.Empty()) {
                            float len = sqrt(sqr(GetScreenMidX() - screen.x) + sqr(GetScreenMidY() - screen.y));
                            if (len <= AIMBOT::FovSize) still_valid = true;
                        }
                    }
                }
                break;
            }
        }
        if (still_valid) return lastTarget;
        lastTarget = nullptr;
    }

    uint64_t myTeam = 0;
    if (AIMBOT::IgnoreTeam)
        myTeam = read<uint64_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::CurrentTeam);

    for (const auto& Player : list) {
        if (!Player) continue;

        AimbotCacheData pcache;
        bool hasCache = false;
        {
            std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
            auto it = g_EspCache.find((uintptr_t)Player);
            if (it != g_EspCache.end()) {
                pcache = CopyAimbotCache(it->second);
                hasCache = true;
            }
        }
        if (!hasCache) continue;
        if (AIMBOT::IgnoreWounded && pcache.isWounded) continue;
        if (AIMBOT::IgnoreSleepers && pcache.isSleeping) continue;
        // Skip friends
        if (!pcache.name.empty()) {
            std::lock_guard<std::mutex> lk(g_FriendMutex);
            if (g_Friends.count(pcache.name)) continue;
        }
        if (AIMBOT::VisibleOnly) {
            bool vis = pcache.isVisible;
            if (AIMBOT::VisibleStrict) vis = pcache.isVisibleRaw && vis;
            if (!vis) continue;
        }
        if (AIMBOT::WeaponCheck) {
            if (pcache.weaponName.empty()) continue;
        }
        if (AIMBOT::IgnoreTeam && myTeam != 0) {
            if (pcache.teamId == myTeam) continue;
        }

        // Use cached bone positions (zero IOCTLs) instead of Get_Transformation
        int bone = GetBestBone(Player);
        Vector3 pos;
        {
            static const int boneToCache[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, 3, -1, -1, -1, -1, -1,
                2, -1, -1, -1, -1, 9, 10, -1, -1, 11,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, 1, 0, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, -1, -1, -1, 12, 13, 14
            };
            int cacheIdx = (bone >= 0 && bone < 66) ? boneToCache[bone] : -1;
            if (cacheIdx >= 0 && pcache.skeletonValid)
                pos = pcache.bones[cacheIdx];
            if (pos.Empty())
                pos = pcache.headPos;
        }
        if (pos.Empty()) continue;
        auto screen = WorldToScreen(pos, src->GetMatrixSnapshot());
        if (screen.Empty()) continue;

        float length = sqrt(sqr(GetScreenMidX() - screen.x) + sqr(GetScreenMidY() - screen.y));

        // Distance cap — skip targets beyond max_distance_aim (use cached local pos, zero IOCTLs)
        if (AIMBOT::MaxDistanceAim > 0.f) {
            Vector3 lpPos;
            {
                std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
                lpPos = g_LocalPlayerPos;
            }
            if (!lpPos.Empty()) {
                float dist = lpPos.DistTo(pos);
                if (dist > AIMBOT::MaxDistanceAim) continue;
            }
        }

        if (length < Closest && length <= AIMBOT::FovSize) {
            Closest = length;
            RPlayer = Player;
        }
    }

    lastTarget = RPlayer;
    return RPlayer;
}

void aimbot::do_aimbot()
{
    if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) return;
    if (g_BnStableCycles.load(std::memory_order_relaxed) < 3) return;

    // Early-out: skip the entire aimbot loop when no feature is active
    if (!AIMBOT::Memory && !AIMBOT::TargetLine && !AIMBOT::TargetText
        && !AIMBOT::PredictionIndicator && !ESP::hotbar_text && !ESP::PlayerInventoryPanel) return;

    {
        static ULONGLONG quarantineStart = 0;
        static uintptr_t quarantineLp = 0;
        static bool qLogged = false;
        const ULONGLONG now = GetTickCount64();
        const uintptr_t lp = (uintptr_t)src->LocalPlayer;
        if (quarantineStart == 0 || lp != quarantineLp) {
            quarantineStart = now;
            quarantineLp = lp;
            qLogged = false;
        }
        if ((now - quarantineStart) < 10000ULL) {
            if (!qLogged) {
                LOG("AIMBOT: WRITE SKIP (startup quarantine active: 10s)");
                qLogged = true;
            }
            return;
        }
    }

    const uint64_t lpGeneration = g_LocalPlayerGeneration.load(std::memory_order_acquire);
    auto lpStillStable = [&]() -> bool {
        return src->LocalPlayer
            && is_valid((uintptr_t)src->LocalPlayer)
            && g_BnStableCycles.load(std::memory_order_relaxed) >= 3
            && g_LocalPlayerGeneration.load(std::memory_order_acquire) == lpGeneration;
    };

    auto BestEntity = Get_BestEntity();
    if (!BestEntity) {
        static int noTargetCount = 0;
        if (noTargetCount < 3) {
            LOG("AIMBOT: no target found (list=%d)", (int)entity_List.size());
            noTargetCount++;
        }
        return;
    }

    // Diagnostic: first successful target acquisition
    static bool aimTargetLogged = false;
    if (!aimTargetLogged) {
        aimTargetLogged = true;
        LOG("AIMBOT: target acquired (0x%I64X) Memory=%d", (uint64_t)BestEntity, (int)AIMBOT::Memory);
    }

    if (ESP::hotbar_text || ESP::PlayerInventoryPanel) {
        EspCacheData tc;
        bool hasTc = false;
        {
            std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
            auto it = g_EspCache.find((uintptr_t)BestEntity);
            if (it != g_EspCache.end()) { tc = it->second; hasTc = true; }
        }
        if (hasTc) {
            for (int i = 0; i < 6; i++) {
                if (!cacheStrEmpty(tc.belt[i])) {
                    hotbar_mutex.lock();
                    hotbar_list.push_back(std::string(tc.belt[i]));
                    hotbar_mutex.unlock();
                }
            }
        }
    }

    // Calculate angle for memory aimbot
    if (AIMBOT::Memory) {
        int bone = GetBestBone(BestEntity);

        // Get target position with prediction (handles velocity + gravity comp)
        auto targetPos = GetPredictedTargetPos(BestEntity, bone);
        if (!targetPos.Empty()) {

        // Get LOCAL player position — from globals (zero IOCTLs, written by fast thread)
        Vector3 localPos;
        bool lpCrouching = false;
        {
            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
            localPos = g_LocalNeckPos;
            lpCrouching = g_LocalIsCrouching;
        }
        if (localPos.Empty()) {
            Vector3 lpPos;
            { std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex); lpPos = g_LocalPlayerPos; }
            if (!lpPos.Empty()) localPos = lpPos;
            localPos.y += lpCrouching ? 0.9f : 1.5f;
        }
        if (!localPos.Empty()) {
        auto angle = CalcAngle(localPos, targetPos);
        if (!angle.Empty()) {
            Normalize(angle.y, angle.x);

            if (AIMBOT::HumanizeEnabled) {
                if (AIMBOT::MissProbability > 0.f && h_rand() < AIMBOT::MissProbability)
                    { /* skip */ }
                else {
                    angle.x += h_gauss() * AIMBOT::JitterAmount;
                    angle.y += h_gauss() * AIMBOT::JitterAmount;
                }
            }

            static bool aim_logged = false;
            static int aim_count = 0;
            if (!aim_logged || aim_count < 10) {
                uintptr_t inp = read<uintptr_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::PlayerInput);
                Vector3 cur = read<Vector3>(inp + offsets::PlayerInput::bodyAngles);
                LOG("AIMBOT_WRITE[%d]: bone=%d angle=(%.1f,%.1f) cur=(%.1f,%.1f) localY=%.1f targetY=%.1f crouch=%d Mem=%d",
                    aim_count, bone, angle.x, angle.y, cur.x, cur.y,
                    localPos.y, targetPos.y, (int)lpCrouching, (int)AIMBOT::Memory);
                aim_logged = true;
                aim_count++;
            }

            // Memory aimbot — write bodyAngles (visible aim change)
            {
                uintptr_t input = read<uintptr_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::PlayerInput);
                if (is_valid(input) && is_valid(input + offsets::PlayerInput::bodyAngles)) {
                    int passes = AIMBOT::SpinWrites ? 3 : 1;
                    for (int p = 0; p < passes; p++) {
                        if (p > 0) {
                            if (!lpStillStable()) break;
                            Sleep(0);
                            input = read<uintptr_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::PlayerInput);
                            if (!is_valid(input) || !is_valid(input + offsets::PlayerInput::bodyAngles)) break;
                            Vector3 cur = read<Vector3>(input + offsets::PlayerInput::bodyAngles);
                            float dx = fabsf(angle.x - cur.x);
                            float dy = fabsf(angle.y - cur.y);
                            if (dy > 180.f) dy = 360.f - dy;
                            if (dx < 0.5f && dy < 0.5f) break;
                        }

                        if (AIMBOT::SMOOTHING > 1.f) {
                            // Phase 3: Ease-out aim path — decelerate near target
                            if (!lpStillStable()) break;
                            Vector3 currentAngle = read<Vector3>(input + offsets::PlayerInput::bodyAngles);
                            float smooth = AIMBOT::SMOOTHING;
                            if (AIMBOT::HumanizeEnabled) {
                                float variance = (h_rand() - 0.5f) * 2.f * AIMBOT::SmoothingVariance;
                                smooth *= (1.f + variance);
                                if (smooth < 1.f) smooth = 1.f;
                            }
                            float deltaX = angle.x - currentAngle.x;
                            float deltaY = angle.y - currentAngle.y;
                            if (deltaY > 180.f) deltaY -= 360.f;
                            if (deltaY < -180.f) deltaY += 360.f;

                            float remainingMag = sqrtf(deltaX * deltaX + deltaY * deltaY);
                            float effectiveFraction = 1.0f / smooth;

                            if (AIMBOT::HumanizeEnabled) {
                                if (remainingMag < 3.f && remainingMag > 0.5f) {
                                    float over = 1.f + (h_rand() * AIMBOT::OvershootAmount / 10.f);
                                    deltaX *= over;
                                    deltaY *= over;
                                }
                            }

                            float moveX = deltaX * effectiveFraction;
                            float moveY = deltaY * effectiveFraction;
                            if (fabsf(moveX) > 15.0f) moveX = (moveX > 0 ? 15.0f : -15.0f);
                            if (fabsf(moveY) > 15.0f) moveY = (moveY > 0 ? 15.0f : -15.0f);

                            Vector3 smoothed(currentAngle.x + moveX, currentAngle.y + moveY, currentAngle.z);
                            write<Vector3>(input + offsets::PlayerInput::bodyAngles, smoothed);
                        }
                        else {
                            if (!lpStillStable()) break;
                            Vector3 currentAngle = read<Vector3>(input + offsets::PlayerInput::bodyAngles);
                            float deltaX = angle.x - currentAngle.x;
                            float deltaY = angle.y - currentAngle.y;
                            if (deltaY > 180.f) deltaY -= 360.f;
                            if (deltaY < -180.f) deltaY += 360.f;

                            if (fabsf(deltaX) > 15.0f) deltaX = (deltaX > 0 ? 15.0f : -15.0f);
                            if (fabsf(deltaY) > 15.0f) deltaY = (deltaY > 0 ? 15.0f : -15.0f);
                            if (AIMBOT::HumanizeEnabled) {
                                deltaX += h_gauss() * AIMBOT::JitterAmount * 0.5f;
                                deltaY += h_gauss() * AIMBOT::JitterAmount * 0.5f;
                            }
                            Vector3 clamped(currentAngle.x + deltaX, currentAngle.y + deltaY, currentAngle.z);
                            write<Vector3>(input + offsets::PlayerInput::bodyAngles, clamped);
                        }
                    }
                }
            }
        } // !angle.Empty()
        } // !localPos.Empty()
        } // !targetPos.Empty()
    }

    // Target visuals (work independently of Memory Aimbot) — only if a visual feature is on
    if (AIMBOT::TargetLine || AIMBOT::TargetText || (AIMBOT::PredictionIndicator && AIMBOT::PredictionScale > 0.001f))
    {
        int drawBone = GetBestBone(BestEntity);
        // Use cached bone position instead of live Get_Transformation (zero IOCTLs)
        Vector3 drawPos;
        EspCacheData tc;
        bool hasTc = false;
        {
            std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex);
            auto it = g_EspCache.find((uintptr_t)BestEntity);
            if (it != g_EspCache.end()) { tc = it->second; hasTc = true; }
        }
        if (hasTc && tc.skeletonValid) {
            static const int boneToCache[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, -1, -1, 3, -1, -1, -1, -1, -1, 2, -1, -1, -1, -1, 9, 10, -1, -1, 11,
                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                -1, -1, 1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 13, 14
            };
            int ci = (drawBone >= 0 && drawBone < 66) ? boneToCache[drawBone] : -1;
            if (ci >= 0) drawPos = tc.bones[ci];
            if (drawPos.Empty()) drawPos = tc.headPos;
        }
        if (!drawPos.Empty()) {
            auto drawScreen = WorldToScreen(drawPos, src->GetMatrixSnapshot());
            if (ScreenPointValid(drawScreen)) {
                if (AIMBOT::TargetLine) {
                    Vector2 StartPos((float)GetScreenMidX(), (float)GetScreenMidY());
                    if (AIMBOT::TargetLineFromMuzzle && src->LocalPlayer) {
                        // Use cached local neck position (zero IOCTLs)
                        Vector3 muzzlePos;
                        {
                            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
                            muzzlePos = g_LocalNeckPos;
                        }
                        if (!muzzlePos.Empty()) {
                            Vector2 muzzleScreen = WorldToScreen(muzzlePos, src->GetMatrixSnapshot());
                            if (ScreenPointValid(muzzleScreen)) StartPos = muzzleScreen;
                        }
                    }
                    ImGui::GetBackgroundDrawList()->AddLine(
                        ImVec2(StartPos.x, StartPos.y),
                        ImVec2(drawScreen.x, drawScreen.y),
                        AIMBOT::color::TargetLine,
                        AIMBOT::TargetLineThickness);
                }
                if (AIMBOT::TargetText) {
                    auto* dl = ImGui::GetBackgroundDrawList();
                    dl->AddCircleFilled(ImVec2(drawScreen.x, drawScreen.y), 3.5f, AIMBOT::color::TargetText, 16);
                    dl->AddCircle(ImVec2(drawScreen.x, drawScreen.y), 5.0f, IM_COL32(0, 0, 0, 180), 16, 1.3f);
                }
                if (AIMBOT::PredictionIndicator && AIMBOT::PredictionScale > 0.001f) {
                    auto predPos = GetPredictedTargetPos(BestEntity, drawBone);
                    if (!predPos.Empty()) {
                        auto predScreen = WorldToScreen(predPos, src->GetMatrixSnapshot());
                        if (ScreenPointValid(predScreen)) {
                            auto* dl = ImGui::GetBackgroundDrawList();
                            dl->AddLine(ImVec2(drawScreen.x, drawScreen.y), ImVec2(predScreen.x, predScreen.y), IM_COL32(255, 230, 80, 200), 1.5f);
                            dl->AddCircle(ImVec2(predScreen.x, predScreen.y), 4.0f, IM_COL32(255, 230, 80, 230), 18, 1.6f);
                            dl->AddCircleFilled(ImVec2(predScreen.x, predScreen.y), 1.5f, IM_COL32(255, 255, 255, 220));
                        }
                    }
                }
            }
        }
    }
}
