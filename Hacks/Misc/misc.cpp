#include "misc.hpp"
#include "../../Logger.hpp"
#include "../../Hotkeys.hpp"
#include "../../OffsetManager.hpp"
#include <windows.h>
#include <chrono>

std::mutex misc::s_miscMutex;

static uint32_t s_miscRng = 0x789ABCDE;
static inline float misc_rand() {
    s_miscRng = s_miscRng * 1664525u + 1013904223u;
    return (float)((s_miscRng >> 8) & 0xFFFFFF) / (float)0x1000000;
}

// Free Camera state
static bool dcCamActive = false;
static Vector3 dcCamBasePos = {};
static Vector3 dcCamDelta = {};
static ULONGLONG dcCamLastWriteTick = 0;
static uintptr_t dcLoadingSnapshotBackup = 0;
static bool debugTimerStarted = false;
static std::chrono::steady_clock::time_point lastUpdateTime;
static uintptr_t gTodSkyPtr = 0;
static ULONGLONG gTodSkySetMs = 0;  // timestamp when gTodSkyPtr was set — for 30s staleness check

static uintptr_t ResolveCameraComponent()
{
    uint64_t cam_handle = read_chain<uint64_t>(GameAssembly, { offsets::camera_pointer, offsets::BaseCamera::static_fields, offsets::BaseCamera::wrapper_class });
    if (!cam_handle) return 0;
    uintptr_t camInstance = (cam_handle & 1) ? decrypt::Il2cppGetHandle(cam_handle) : cam_handle;
    if (!camInstance || !is_valid(camInstance)) return 0;
    // Deref +0x10 to get native camera struct (culling_mask, fov, world_position are here)
    uintptr_t nativeCam = read<uintptr_t>(camInstance + offsets::BaseCamera::entity);
    return (nativeCam && is_valid(nativeCam)) ? nativeCam : camInstance;
}

static void restoreCameraPosition() {
    if (dcCamActive && src && src->LocalPlayer) {
        // Write base position back via camera chain
        uintptr_t camObj = ResolveCameraComponent();
        if (camObj && is_valid(camObj) && is_valid(camObj + offsets::BaseCamera::world_position))
            write<Vector3>(camObj + offsets::BaseCamera::world_position, dcCamBasePos);
        // Clear Flying flag
        uintptr_t ms = read<uintptr_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::ModelState);
        if (ms && is_valid(ms) && is_valid(ms + offsets::ModelState::flags)) {
            int mf = read<int>(ms + offsets::ModelState::flags);
            write<int>(ms + offsets::ModelState::flags, mf & ~offsets::ModelState::Flying);
        }
    }
    dcCamActive = false;
    dcCamDelta = {};
    dcCamLastWriteTick = 0;
}

static void BeginDebugTimer()
{
    if (!debugTimerStarted)
    {
        lastUpdateTime = std::chrono::steady_clock::now();
        MISC::DebugCameraTimer = 0.0f;
        debugTimerStarted = true;
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdateTime);
    if (elapsed.count() >= 1)
    {
        MISC::DebugCameraTimer += (float)elapsed.count();
        lastUpdateTime = now;
    }
}

static void AssignLoadingScreenData(bool enable)
{
    // SingletonComponent<MixerSnapshotManager>
    uint64_t mixerTypeInfo = read<uint64_t>(GameAssembly + offsets::Class_SingletonComponent_MixerSnapshotManager__c);
    if (mixerTypeInfo && is_valid(mixerTypeInfo))
    {
        uint64_t staticFields = read<uint64_t>(mixerTypeInfo + offsets::BaseNetworkable::static_fields);
        if (staticFields && is_valid(staticFields))
        {
            uint64_t instance = read<uint64_t>(staticFields + offsets::SingletonComponent::Instance);
            if (instance && is_valid(instance))
            {
                if (!dcLoadingSnapshotBackup)
                    dcLoadingSnapshotBackup = read<uint64_t>(instance + offsets::MixerSnapshotManager::loadingSnapshot);

                uint64_t snapshotToSet = read<uint64_t>(instance + offsets::MixerSnapshotManager::defaultSnapshot);
                if (!enable)
                    snapshotToSet = dcLoadingSnapshotBackup;

                if (snapshotToSet && is_valid(snapshotToSet))
                    write<uint64_t>(instance + offsets::MixerSnapshotManager::loadingSnapshot, snapshotToSet);
            }
        }
    }

    // SingletonComponent<UI_LoadingScreen>
    uint64_t loadingTypeInfo = read<uint64_t>(GameAssembly + offsets::Class_SingletonComponent_UI_LoadingScreen);
    if (loadingTypeInfo && is_valid(loadingTypeInfo))
    {
        uint64_t staticFields = read<uint64_t>(loadingTypeInfo + offsets::BaseNetworkable::static_fields);
        if (staticFields && is_valid(staticFields))
        {
            uint64_t instance = read<uint64_t>(staticFields + offsets::SingletonComponent::Instance);
            if (instance && is_valid(instance))
                write<bool>(instance + offsets::UI_LoadingScreen::enabledFlag, enable);
        }
    }
}

static uintptr_t ResolveTodSkyPtr()
{
    if (gTodSkyPtr && is_valid(gTodSkyPtr))
        return gTodSkyPtr;

    uint64_t todTypeInfo = read<uint64_t>(GameAssembly + offsets::Class_TOD_Sky_Static);
    if (!todTypeInfo || !is_valid(todTypeInfo)) {
        static bool tod_diag1 = false;
        if (!tod_diag1) { LOG("TOD_DIAG: typeinfo=0 (RVA=0x%I64X)", offsets::Class_TOD_Sky_Static); tod_diag1 = true; }
        gTodSkyPtr = 0; return 0;
    }

    uint64_t staticFields = read<uint64_t>(todTypeInfo + offsets::BaseNetworkable::static_fields);
    if (!staticFields || !is_valid(staticFields)) {
        static bool tod_diag2 = false;
        if (!tod_diag2) { LOG("TOD_DIAG: staticFields=0 (typeinfo=0x%I64X)", todTypeInfo); tod_diag2 = true; }
        gTodSkyPtr = 0; return 0;
    }

    static int tod_diag_count = 0;

    // PRIMARY: Direct pointer — staticFields + instance may be a direct TOD_Sky pointer
    {
        uint64_t directSky = read<uint64_t>(staticFields + offsets::TOD_Sky_Static::instances);
        if (directSky && is_valid(directSky)) {
            uint64_t nightTest = read<uint64_t>(directSky + offsets::TOD_Sky::NightParameters);
            if (nightTest && is_valid(nightTest)) {
                float am = read<float>(nightTest + offsets::TOD_NightParameters::AmbientMultiplier);
                if (am >= 0.0f && am <= 10.0f && std::isfinite(am)) {
                    gTodSkyPtr = (uintptr_t)directSky;
                    gTodSkySetMs = GetTickCount64();
                    if (tod_diag_count < 3) {
                        LOG("TOD_DIAG: found sky via direct pointer at sf+0x%I64X -> sky=0x%I64X", offsets::TOD_Sky_Static::instances, directSky);
                        tod_diag_count++;
                    }
                    return gTodSkyPtr;
                }
            }
        }
    }

    // FALLBACK 1: List traversal — staticFields + instances → List → _items → [0]
    {
        uint64_t instances = read<uint64_t>(staticFields + offsets::TOD_Sky_Static::instances);
        if (instances && is_valid(instances)) {
            uint64_t items = read<uint64_t>(instances + 0x10);
            if (items && is_valid(items)) {
                uint64_t sky = read<uint64_t>(items + 0x20);
                if (sky && is_valid(sky)) {
                    gTodSkyPtr = (uintptr_t)sky;
                    if (tod_diag_count < 3) {
                        LOG("TOD_DIAG: found sky via list at sf+0x%I64X -> sky=0x%I64X", offsets::TOD_Sky_Static::instances, sky);
                        tod_diag_count++;
                    }
                    return gTodSkyPtr;
                }
            }
        }
        // Periodic retry log every ~5s
        static uint64_t lastRetryLog = 0;
        if (GetTickCount64() - lastRetryLog > 5000) {
            lastRetryLog = GetTickCount64();
            LOG("TOD_RETRY: typeinfo=0x%I64X sf=0x%I64X instances=0x%I64X items=0x%I64X — sky not found yet",
                todTypeInfo, staticFields, instances, (uint64_t)(instances ? read<uint64_t>(instances + 0x10) : 0));
        }
    }

    // FALLBACK: Scan staticFields 0x00 to 0x200 — try list traversal (+0x10 → +0x20)
    for (uint64_t off = 0x00; off <= 0x200; off += 8) {
        if (off == offsets::TOD_Sky_Static::instances) continue;
        uint64_t candidate = read<uint64_t>(staticFields + off);
        if (!candidate || !is_valid(candidate)) continue;

        uint64_t items = read<uint64_t>(candidate + 0x10);
        if (items && is_valid(items)) {
            uint64_t sky = read<uint64_t>(items + 0x20);
            if (sky && is_valid(sky)) {
                uint64_t nightTest = read<uint64_t>(sky + offsets::TOD_Sky::NightParameters);
                if (nightTest && is_valid(nightTest)) {
                    float am = read<float>(nightTest + offsets::TOD_NightParameters::AmbientMultiplier);
                    if (am >= 0.0f && am <= 10.0f && std::isfinite(am)) {
                    gTodSkyPtr = (uintptr_t)sky;
                    gTodSkySetMs = GetTickCount64();
                        if (tod_diag_count < 3) {
                            LOG("TOD_DIAG: found sky via list at sf+0x%I64X -> 0x%I64X", off, sky);
                            tod_diag_count++;
                        }
                        return gTodSkyPtr;
                    }
                }
            }
        }
    }

    if (tod_diag_count < 3) {
        LOG("TOD_DIAG: FAILED to find TOD_Sky (typeinfo=0x%I64X sf=0x%I64X scanned 0x00-0x200)",
            todTypeInfo, staticFields);
        tod_diag_count++;
    }
    gTodSkyPtr = 0;
    return 0;
}

auto FovChanger_Bypass(float fov) -> void
{
    uintptr_t camPtr = ResolveCameraComponent();
    if (camPtr && is_valid(camPtr)) {
        write<float>(camPtr + offsets::BaseCamera::field_of_view, fov);
        static int bpLog = 0; if (bpLog < 3) { LOG("FOV_BYPASS: wrote %f to cam=0x%I64X+0x%I64X", fov, (uint64_t)camPtr, offsets::BaseCamera::field_of_view); bpLog++; }
    } else {
        static int bpFail = 0; if (bpFail < 3) { LOG("FOV_BYPASS: camPtr invalid"); bpFail++; }
    }
}

auto FovChanger(float fov) -> void
{
    uintptr_t kl = read<uintptr_t>(GameAssembly + offsets::FOV::ConVar_Graphics);
    if (!is_valid(kl)) { static int f1=0; if(f1<3){LOG("FOV: ConVar_Graphics invalid kl=0x%I64X RVA=0x%I64X — using bypass", (uint64_t)kl, offsets::FOV::ConVar_Graphics); f1++;} FovChanger_Bypass(fov); return; }
    uintptr_t field = read<uintptr_t>(kl + offsets::BaseCamera::static_fields);
    if (!is_valid(field)) { static int f2=0; if(f2<3){LOG("FOV: static_fields invalid kl=0x%I64X — using bypass", (uint64_t)kl); f2++;} FovChanger_Bypass(fov); return; }
    uint32_t raw = *(uint32_t*)&fov;
    uint32_t encrypted = decrypt::encrypt_fov(raw);
    write<uint32_t>(field + offsets::FOV::fovWrite, encrypted);
    static int f3 = 0; if (f3 < 3) { LOG("FOV: wrote enc=%u to field=0x%I64X+0x%I64X (kl=0x%I64X fov=%.0f)", encrypted, (uint64_t)field, offsets::FOV::fovWrite, (uint64_t)kl, fov); f3++; }
};

void misc::do_misc() {

    std::lock_guard<std::mutex> miscLock(s_miscMutex);

    // Generation stability check + 10s quarantine — defined early so all phases can use it
    const uint64_t miscLpGeneration = g_LocalPlayerGeneration.load(std::memory_order_acquire);
    auto miscLpStable = [&]() -> bool {
        return src->LocalPlayer
            && is_valid((uintptr_t)src->LocalPlayer)
            && g_LocalPlayerGeneration.load(std::memory_order_acquire) == miscLpGeneration;
    };

    // 10-second startup quarantine after LocalPlayer change (prevents writes to uninitialized player)
    {
        static ULONGLONG miscQuarantineStart = 0;
        static uintptr_t miscQuarantineLp = 0;
        const ULONGLONG now = GetTickCount64();
        const uintptr_t lp = (uintptr_t)src->LocalPlayer;
        if (miscQuarantineStart == 0 || lp != miscQuarantineLp) {
            miscQuarantineStart = now;
            miscQuarantineLp = lp;
        }
        if ((now - miscQuarantineStart) < 10000ULL) return;
    }

    // One-time toggle status dump
    {
        static bool dumpedToggles = false;
        if (!dumpedToggles) {
            dumpedToggles = true;
            LOG("TOGGLES: Fov=%d Zoom=%d FovAmt=%.0f ZoomAmt=%.0f Bright=%d Time=%d timeval=%d Recoil=%d AdminFlags=%d",
                (int)MISC::FovChanger, (int)MISC::Zoom, MISC::FovAmount, MISC::ZoomAmount,
                (int)MISC::BrightNight, (int)MISC::Timechanger, MISC::timevalue,
                (int)MISC::RecoilEnabled, (int)MISC::AdminFlags);
        }
    }

    {
        static uint64_t lastLpGeneration = 0;
        const uint64_t lpGeneration = g_LocalPlayerGeneration.load(std::memory_order_acquire);
        if (lpGeneration != lastLpGeneration) {
            gTodSkyPtr = 0;
            lastLpGeneration = lpGeneration;
        }
        // Invalidate cached TOD_Sky after 30 seconds — prevents writes to freed memory
        // if the game unloads TOD_Sky during server transition without LocalPlayer gen change
        if (gTodSkyPtr && (GetTickCount64() - gTodSkySetMs) > 30000) {
            gTodSkyPtr = 0;
        }
    }

    // === PHASE 1: READ-SAFE / IMMEDIATE FEATURES (no LP needed) ===
    // FOV/Zoom
    {
        if (!SETTINGS::MenuOpen && HotkeyPressed("VISUAL.FovChanger"))
            MISC::FovChanger = !MISC::FovChanger;
        if (!SETTINGS::MenuOpen && HotkeyPressed("VISUAL.Zoom"))
            MISC::Zoom = !MISC::Zoom;

        static bool wasFovActive = false;
        bool fovActive = MISC::FovChanger || MISC::Zoom;
        float fovVal = 90.0f;
        if (MISC::Zoom) fovVal = MISC::ZoomAmount;
        else if (MISC::FovChanger) fovVal = MISC::FovAmount;
        if (fovVal < 1.0f) fovVal = 1.0f;
        if (fovVal > 180.0f) fovVal = 180.0f;
        if (fovActive) {
            FovChanger(fovVal);
        } else if (wasFovActive) {
            FovChanger(90.0f);
        }
        wasFovActive = fovActive;
    }

    // Remove Layers — disabled (culling_mask offset unverified for this build)

    // TOD Bright Night / Ambient path (static TOD_Sky chain)
    {
        static int brightNightFailCount = 0;

        if (MISC::BrightNight) {
            uintptr_t todSky = ResolveTodSkyPtr();
            if (todSky && is_valid(todSky)) {
                brightNightFailCount = 0;
                float ambientMult = MISC::ambientMultiplier;
                if (ambientMult < 0.0f) ambientMult = 0.0f;
                if (ambientMult > 20.0f) ambientMult = 20.0f;

                float saturation = MISC::AmbientSaturation;
                if (saturation < 0.0f) saturation = 0.0f;
                if (saturation > 5.0f) saturation = 5.0f;

                uintptr_t nightParams = read<uintptr_t>(todSky + offsets::TOD_Sky::NightParameters);
                if (nightParams && is_valid(nightParams) && is_valid(nightParams + offsets::TOD_NightParameters::AmbientMultiplier)) {
                    write<float>(nightParams + offsets::TOD_NightParameters::AmbientMultiplier, ambientMult);
                    static int bnLog = 0; if (bnLog < 5) { LOG("BRIGHTNIGHT: wrote ambient=%.1f to nightParams=0x%I64X", ambientMult, (uint64_t)nightParams); bnLog++; }
                }

                uintptr_t ambientParams = read<uintptr_t>(todSky + offsets::TOD_Sky::AmbientParameters);
                if (ambientParams && is_valid(ambientParams)) {
                    if (is_valid(ambientParams + offsets::TOD_AmbientParameters::Saturation))
                        write<float>(ambientParams + offsets::TOD_AmbientParameters::Saturation, saturation);
                    if (is_valid(ambientParams + offsets::TOD_AmbientParameters::UpdateInterval))
                        write<float>(ambientParams + offsets::TOD_AmbientParameters::UpdateInterval, 0.6f);
                }
                // Force game to recalculate ambient (reset timer)
                if (is_valid(todSky + offsets::TOD_Sky::timeSinceAmbientUpdate))
                    write<float>(todSky + offsets::TOD_Sky::timeSinceAmbientUpdate, 999.0f);
            } else {
                gTodSkyPtr = 0;
                if (++brightNightFailCount >= 500) {
                    MISC::BrightNight = false;
                    brightNightFailCount = 0;
                }
            }
        } else {
            brightNightFailCount = 0;
        }
    }

    // TOD TimeChanger (extends BrightNight)
    {
        if (MISC::Timechanger) {
            uintptr_t todSky = ResolveTodSkyPtr();
            if (todSky && is_valid(todSky)) {
                float t = (float)MISC::timevalue;
                if (t < 0.f) t = 0.f;
                if (t > 24.f) t = 24.f;

                // Try pointer path first (Cycle is a reference type)
                uintptr_t cycleParams = read<uintptr_t>(todSky + offsets::TOD_Sky::CycleParameters);
                if (cycleParams && is_valid(cycleParams)) {
                    write<float>(cycleParams + 0x10, t);
                    static int tcLog = 0; if (tcLog < 5) { LOG("TIMECHANGER: wrote time=%.0f via pointer cycleParams=0x%I64X", t, (uint64_t)cycleParams); tcLog++; }
                } else {
                    // Inline struct fallback — Cycle is a value type at todSky + 0x40
                    // Hour is at cycleStruct + 0x10 = todSky + 0x40 + 0x10 = todSky + 0x50
                    if (is_valid(todSky + 0x50)) {
                        write<float>(todSky + 0x50, t);
                        static int tcLog2 = 0; if (tcLog2 < 5) { LOG("TIMECHANGER: wrote time=%.0f via inline struct todSky+0x50", t); tcLog2++; }
                    }
                }
            }
        }
    }

    // === Admin Flags — sets PlayerFlags.IsAdmin every frame ===
    {
        if (HotkeyPressed("UTIL.AdminFlags")) {
            MISC::AdminFlags = !MISC::AdminFlags;
            LOG("AdminFlags: %s", MISC::AdminFlags ? "ON" : "OFF");
        }
        if (MISC::AdminFlags && src->LocalPlayer && miscLpStable()) {
            uintptr_t lp = (uintptr_t)src->LocalPlayer;
            int pf = read<int>(lp + offsets::BasePlayer::PlayerFlags);
            if (miscLpStable())
                write<int>(lp + offsets::BasePlayer::PlayerFlags, pf | (int)offsets::base_player_flags::IsAdmin);
        }
    }

    // === Debug Camera — camera position manipulation + Flying + body freeze ===
    {
        if (HotkeyPressed("UTIL.DebugCam") && MISC::AdminFlags) {
            MISC::DebugCamera = !MISC::DebugCamera;
            LOG("DebugCam: %s", MISC::DebugCamera ? "ON" : "OFF");
            if (!MISC::DebugCamera) restoreCameraPosition();
        }

        if (!MISC::AdminFlags)
            MISC::DebugCamera = false;

        if (MISC::DebugCameraTimer > 50.0f) {
            MISC::DebugCameraTimer = 0.0f;
            MISC::DebugCamera = false;
        }

        static bool wasDebugBypass = false;
        if (MISC::DebugCamera && MISC::DebugCameraTimer <= 50.0f && src->LocalPlayer && miscLpStable()) {
            uintptr_t lp = (uintptr_t)src->LocalPlayer;
            int pf = read<int>(lp + offsets::BasePlayer::PlayerFlags);
            write<int>(lp + offsets::BasePlayer::PlayerFlags, pf | (int)offsets::base_player_flags::IsAdmin);
            BeginDebugTimer();
            AssignLoadingScreenData(true);
            wasDebugBypass = true;
        } else if (wasDebugBypass && src->LocalPlayer) {
            uintptr_t lp = (uintptr_t)src->LocalPlayer;
            MISC::DebugCamera = false;
            if (!MISC::AdminFlags) {
                int pf = read<int>(lp + offsets::BasePlayer::PlayerFlags);
                write<int>(lp + offsets::BasePlayer::PlayerFlags, pf & ~(int)offsets::base_player_flags::IsAdmin);
            }
            debugTimerStarted = false;
            MISC::DebugCameraTimer = 0.0f;
            AssignLoadingScreenData(false);
            wasDebugBypass = false;
        }

        { static bool dcCamPrev = false; if (dcCamPrev && !MISC::DebugCamera && dcCamActive) restoreCameraPosition(); dcCamPrev = MISC::DebugCamera; }

        if (MISC::DebugCamera && src->LocalPlayer) {
            uintptr_t lp = (uintptr_t)src->LocalPlayer;

            do {
                // Camera position via chain
                uintptr_t camObj = ResolveCameraComponent();
                if (!camObj || !is_valid(camObj)) break;
                uintptr_t camPosAddr = camObj + offsets::BaseCamera::world_position;
                if (!is_valid(camPosAddr)) break;

                if (!dcCamActive) {
                    dcCamBasePos = read<Vector3>(camPosAddr);
                    if (!(dcCamBasePos.x == dcCamBasePos.x)) { LOG("DebugCam: activation FAILED"); break; }
                    dcCamDelta = {}; dcCamActive = true;
                    LOG("DebugCam: activated — camera pos saved at (%.0f,%.0f,%.0f)",
                        dcCamBasePos.x, dcCamBasePos.y, dcCamBasePos.z);
                }

                // Movement input
                float speed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 0.40f : 0.15f;
                float upDown = 0.f; if (GetAsyncKeyState(VK_SPACE) & 0x8000) upDown += speed; if (GetAsyncKeyState(VK_CONTROL) & 0x8000) upDown -= speed;
                float forward = 0.f, right = 0.f;
                if (GetAsyncKeyState('W') & 0x8000) forward += speed; if (GetAsyncKeyState('S') & 0x8000) forward -= speed;
                if (GetAsyncKeyState('D') & 0x8000) right += speed; if (GetAsyncKeyState('A') & 0x8000) right -= speed;
                float yawDeg = 0.f;
                uintptr_t input = read<uintptr_t>(lp + offsets::BasePlayer::PlayerInput);
                yawDeg = input ? read<float>(input + offsets::PlayerInput::yaw) : 0.f;
                float yaw = yawDeg * 3.1415926f / 180.0f; float sy = sinf(yaw); float cy = cosf(yaw);
                dcCamDelta.x += (forward * sy) + (right * cy); dcCamDelta.z += (forward * cy) - (right * sy); dcCamDelta.y += upDown;
                if (dcCamDelta.x > 200.f) dcCamDelta.x = 200.f; if (dcCamDelta.x < -200.f) dcCamDelta.x = -200.f;
                if (dcCamDelta.y > 200.f) dcCamDelta.y = 200.f; if (dcCamDelta.y < -200.f) dcCamDelta.y = -200.f;
                if (dcCamDelta.z > 200.f) dcCamDelta.z = 200.f; if (dcCamDelta.z < -200.f) dcCamDelta.z = -200.f;
                Vector3 newPos = dcCamBasePos + dcCamDelta;
                ULONGLONG nowTick = GetTickCount64();
                if (dcCamLastWriteTick == 0 || (nowTick - dcCamLastWriteTick >= 8ULL)) {
                    write<Vector3>(camPosAddr, newPos); dcCamLastWriteTick = nowTick;
                }
            } while (false);

            // ModelState.Flying + body freeze — throttled to 100ms (was every frame)
            // to prevent writes to freed memory if player object is stale
            if (miscLpStable()) {
                static ULONGLONG dcMsWriteTick = 0;
                ULONGLONG msNow = GetTickCount64();
                if (dcMsWriteTick == 0 || (msNow - dcMsWriteTick >= 100ULL)) {
                    dcMsWriteTick = msNow;
                    uintptr_t ms = read<uintptr_t>(lp + offsets::BasePlayer::ModelState);
                    if (ms && is_valid(ms) && is_valid(ms + offsets::ModelState::flags)) {
                        int mf = read<int>(ms + offsets::ModelState::flags);
                        write<int>(ms + offsets::ModelState::flags, mf | offsets::ModelState::Flying);
                    }
                    uintptr_t mov = read<uintptr_t>(lp + offsets::BasePlayer::BaseMovement);
                    if (mov && is_valid(mov) && is_valid(mov + offsets::BaseMovement2::target_movement)) {
                        write<Vector3>(mov + offsets::BaseMovement2::target_movement, Vector3{});
                    }
                }
            }
        }
    }

    // === PHASE 2: LP-DEPENDENT FEATURES ===

    if (!src->LocalPlayer || !is_valid((uintptr_t)src->LocalPlayer)) return;

    // === RECOIL === — modify ALL weapon children (brute force, can't identify held weapon)
    // Enter block when recoil/burst is active OR when we need to restore originals after toggling off
    static bool wasRecoilEnabled = false;
    bool needRecoilRestore = wasRecoilEnabled && !MISC::RecoilEnabled;
    wasRecoilEnabled = MISC::RecoilEnabled;    if ((MISC::RecoilEnabled || MISC::ChangeBurst || needRecoilRestore) && miscLpStable())
    {
        // Get all children
        uintptr_t childrenList = read<uintptr_t>((uintptr_t)src->LocalPlayer + offsets::BaseNetworkable::children);
        if (!childrenList || !is_valid(childrenList)) return;
        uintptr_t items = read<uintptr_t>(childrenList + 0x10);
        int count = read<int>(childrenList + 0x18);
        if (!items || !is_valid(items) || count <= 0 || count >= 32) return;

        // Persistent cache: stores original recoil values per weapon pointer
        struct WC {
            uintptr_t weapon;
            uintptr_t rp;
            float oRP[4];
            float oNRO[4];
            uintptr_t nro;
            bool active;
        };
        static WC wc[16] = {};

        // Build current children list
        uintptr_t kids[32] = {};
        int nkids = 0;
        for (int i = 0; i < count && nkids < 32; i++) {
            uintptr_t cr = read<uintptr_t>(items + 0x20 + i * 8);
            if (!cr) continue;
            uintptr_t ch = (cr & 1) ? decrypt::Il2cppGetHandle(cr) : cr;
            if (ch && is_valid(ch)) kids[nkids++] = ch;
        }

        // Add new weapons to cache (only if non-zero recoil = real weapon)
        for (int ci = 0; ci < nkids; ci++) {
            uintptr_t rp = read<uintptr_t>(kids[ci] + offsets::BaseProjectile::recoil);
            if (!rp || !is_valid(rp)) continue;
            bool inCache = false;
            for (int c = 0; c < 16; c++) if (wc[c].active && wc[c].weapon == kids[ci]) { inCache = true; wc[c].rp = rp; break; }
            if (inCache) continue;
            float yMin = read<float>(rp + offsets::RecoilProperties::RecoilYawMin);
            float yMax = read<float>(rp + offsets::RecoilProperties::RecoilYawMax);
            float pMin = read<float>(rp + offsets::RecoilProperties::RecoilPitchMin);
            float pMax = read<float>(rp + offsets::RecoilProperties::RecoilPitchMax);
            if (!std::isfinite(yMin) || fabsf(yMin) >= 50.f) continue;
            if (yMin == 0.f && yMax == 0.f && pMin == 0.f && pMax == 0.f) continue;
            for (int c = 0; c < 16; c++) {
                if (!wc[c].active) {
                    wc[c].active = true;
                    wc[c].weapon = kids[ci];
                    wc[c].rp = rp;
                    wc[c].nro = read<uintptr_t>(rp + offsets::RecoilProperties::NewRecoilOverride);
                    wc[c].oRP[0] = yMin; wc[c].oRP[1] = yMax;
                    wc[c].oRP[2] = pMin; wc[c].oRP[3] = pMax;
                    if (wc[c].nro && is_valid(wc[c].nro)) {
                        wc[c].oNRO[0] = read<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMin);
                        wc[c].oNRO[1] = read<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMax);
                        wc[c].oNRO[2] = read<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMin);
                        wc[c].oNRO[3] = read<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMax);
                    }
                    LOG("RECOIL_CACHE: weapon=0x%I64X rp=0x%I64X yawMin=%.2f", kids[ci], rp, yMin);
                    break;
                }
            }
        }

        // Remove stale entries (weapon left children list) - restore recoil
        // Re-read rp from the weapon to verify it's still alive before writing
        for (int c = 0; c < 16; c++) {
            if (!wc[c].active) continue;
            bool found = false;
            for (int ci = 0; ci < nkids; ci++) if (kids[ci] == wc[c].weapon) { found = true; break; }
            if (!found) {
                // Weapon left children list — verify it's still readable before restoring
                uintptr_t rp_check = read<uintptr_t>(wc[c].weapon + offsets::BaseProjectile::recoil);
                if (rp_check && is_valid(rp_check) && rp_check == wc[c].rp) {
                    // Weapon still alive and recoil pointer matches — safe to restore
                    write<float>(wc[c].rp + offsets::RecoilProperties::RecoilYawMin, wc[c].oRP[0]);
                    write<float>(wc[c].rp + offsets::RecoilProperties::RecoilYawMax, wc[c].oRP[1]);
                    write<float>(wc[c].rp + offsets::RecoilProperties::RecoilPitchMin, wc[c].oRP[2]);
                    write<float>(wc[c].rp + offsets::RecoilProperties::RecoilPitchMax, wc[c].oRP[3]);
                    if (wc[c].nro && is_valid(wc[c].nro)) {
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMin, wc[c].oNRO[0]);
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMax, wc[c].oNRO[1]);
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMin, wc[c].oNRO[2]);
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMax, wc[c].oNRO[3]);
                    }
                    LOG("RECOIL_RESTORE: weapon=0x%I64X restored (left children)", wc[c].weapon);
                }
                // Clear slot regardless — weapon is no longer in children list
                wc[c].active = false;
                wc[c].weapon = 0;
                wc[c].rp = 0;
                wc[c].nro = 0;
            }
        }

        // Write to cached weapons — THROTTLED to 100ms + re-validate before EVERY write
        static ULONGLONG lastRecoilWrite = 0;
        ULONGLONG nowTick = GetTickCount64();
        bool doWrite = (nowTick - lastRecoilWrite) >= 100; // 10 writes/sec max

        float modVal = MISC::RecoilModifier;
        if (modVal < 0.f) modVal = 0.f;
        if (modVal > 100.f) modVal = 100.f;
        float scale = MISC::RecoilEnabled ? (100.f - modVal) / 100.f : 1.0f;
        if (MISC::RecoilEnabled && scale < MISC::RecoilFloor) scale = MISC::RecoilFloor;

        // If recoil was just toggled off, restore ALL original values and clear cache
        if (needRecoilRestore) {
            for (int c = 0; c < 16; c++) {
                if (!wc[c].active) continue;
                // Re-read rp from weapon to verify it's still alive
                uintptr_t rp = read<uintptr_t>(wc[c].weapon + offsets::BaseProjectile::recoil);
                if (rp && is_valid(rp)) {
                    write<float>(rp + offsets::RecoilProperties::RecoilYawMin, wc[c].oRP[0]);
                    write<float>(rp + offsets::RecoilProperties::RecoilYawMax, wc[c].oRP[1]);
                    write<float>(rp + offsets::RecoilProperties::RecoilPitchMin, wc[c].oRP[2]);
                    write<float>(rp + offsets::RecoilProperties::RecoilPitchMax, wc[c].oRP[3]);
                    if (wc[c].nro && is_valid(wc[c].nro)) {
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMin, wc[c].oNRO[0]);
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMax, wc[c].oNRO[1]);
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMin, wc[c].oNRO[2]);
                        write<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMax, wc[c].oNRO[3]);
                    }
                    LOG("RECOIL_RESTORE: weapon=0x%I64X restored (toggle off)", wc[c].weapon);
                }
                wc[c].active = false;
                wc[c].weapon = 0;
                wc[c].rp = 0;
                wc[c].nro = 0;
            }
        }
        else if (MISC::RecoilEnabled && doWrite) {
            lastRecoilWrite = nowTick;
            // Write to ALL cached weapons — not just the first one (fixes slot 2-6 not working)
            for (int c = 0; c < 16; c++) {
                if (!wc[c].active) continue;
                uintptr_t rp = read<uintptr_t>(wc[c].weapon + offsets::BaseProjectile::recoil);
                if (!rp || !is_valid(rp) || !is_valid(rp + 0x30)) {
                    wc[c].active = false;
                    continue;
                }
                wc[c].rp = rp;
                float ws = scale;
                if (MISC::RecoilVariance) {
                    float v = (misc_rand() - 0.5f) * 0.2f;
                    ws = scale * (1.f + v);
                    if (ws < MISC::RecoilFloor) ws = MISC::RecoilFloor;
                }
                write<float>(rp + offsets::RecoilProperties::RecoilYawMin, wc[c].oRP[0] * ws);
                write<float>(rp + offsets::RecoilProperties::RecoilYawMax, wc[c].oRP[1] * ws);
                write<float>(rp + offsets::RecoilProperties::RecoilPitchMin, wc[c].oRP[2] * ws);
                write<float>(rp + offsets::RecoilProperties::RecoilPitchMax, wc[c].oRP[3] * ws);
                if (wc[c].nro && is_valid(wc[c].nro) && is_valid(wc[c].nro + 0x30)) {
                    float wsNro = ws;
                    if (MISC::RecoilVariance) {
                        float v = (misc_rand() - 0.5f) * 0.2f;
                        wsNro = scale * (1.f + v);
                        if (wsNro < MISC::RecoilFloor) wsNro = MISC::RecoilFloor;
                    }
                    write<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMin, wc[c].oNRO[0] * wsNro);
                    write<float>(wc[c].nro + offsets::RecoilProperties::RecoilYawMax, wc[c].oNRO[1] * wsNro);
                    write<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMin, wc[c].oNRO[2] * wsNro);
                    write<float>(wc[c].nro + offsets::RecoilProperties::RecoilPitchMax, wc[c].oNRO[3] * wsNro);
                }
            }
        }

        {
            static int diag = 0;
            if (diag < 3) {
                int na = 0; for (int c = 0; c < 16; c++) if (wc[c].active) na++;
                LOG("RECOIL: %d cached enabled=%d scale=%.2f", na, (int)MISC::RecoilEnabled, scale);
                diag++;
            }
        }

        // Burst mode — only first active weapon
        static bool wasBurst = false;
        static uintptr_t lastBurst = 0;
        if (MISC::ChangeBurst && doWrite) {
            for (int c = 0; c < 16; c++) {
                if (wc[c].active && is_valid(wc[c].weapon) && is_valid(wc[c].weapon + offsets::BaseProjectile::internalBurstFireRateScale)) {
                    write<bool>(wc[c].weapon + offsets::BaseProjectile::isBurstWeapon, true);
                    write<bool>(wc[c].weapon + offsets::BaseProjectile::canChangeFireModes, true);
                    lastBurst = wc[c].weapon;
                    break;
                }
            }
        } else if (wasBurst && !MISC::ChangeBurst && lastBurst && is_valid(lastBurst)) {
            // Verify lastBurst is still in current children list before restoring
            bool burstStillAlive = false;
            for (int ci = 0; ci < nkids; ci++) if (kids[ci] == lastBurst) { burstStillAlive = true; break; }
            if (burstStillAlive && is_valid(lastBurst + offsets::BaseProjectile::internalBurstFireRateScale)) {
                write<bool>(lastBurst + offsets::BaseProjectile::isBurstWeapon, false);
            }
            lastBurst = 0;
        }
        wasBurst = MISC::ChangeBurst;
    }

}
