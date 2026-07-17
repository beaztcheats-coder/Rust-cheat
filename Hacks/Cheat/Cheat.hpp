#include "Hacks/Aimbot/aimbot.hpp"
#include "../../Logger.hpp"
#include "../../Hotkeys.hpp"
#include <cmath>

void SafeDoCheat();

namespace Xheat {

    bool ReadFrameCameraMatrix() {
        static int camDiag = 0;
        static uint64_t cached_native_cam = 0;
        static int cache_refresh_counter = 0;
        bool rechain = false;
        if (!cached_native_cam || !is_valid(cached_native_cam)) rechain = true;
        if (++cache_refresh_counter >= 200) { cache_refresh_counter = 0; rechain = true; }

        uint64_t native_cam;
        if (rechain) {
            uint64_t cam_typeinfo = read<uint64_t>(GameAssembly + offsets::camera_pointer);
            if (!cam_typeinfo || !is_valid(cam_typeinfo)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_typeinfo invalid (0x%I64X)", cam_typeinfo); camDiag++; } return false; }
            uint64_t cam_sf = read<uint64_t>(cam_typeinfo + offsets::BaseCamera::static_fields);
            if (!cam_sf || !is_valid(cam_sf)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_sf invalid (0x%I64X)", cam_sf); camDiag++; } return false; }
            uint64_t cam_instance = read<uint64_t>(cam_sf + offsets::BaseCamera::wrapper_class);
            if (!cam_instance || !is_valid(cam_instance)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_instance invalid (0x%I64X)", cam_instance); camDiag++; } return false; }
            native_cam = read<uint64_t>(cam_instance + offsets::BaseCamera::entity);
            if (!native_cam || !is_valid(native_cam)) native_cam = cam_instance;
            if (!native_cam || !is_valid(native_cam)) { if (camDiag < 5) { LOG("CAM_CHAIN: native_cam invalid"); camDiag++; } return false; }
            cached_native_cam = native_cam;
        } else {
            native_cam = cached_native_cam;
        }

        float fov_test = read<float>(native_cam + offsets::BaseCamera::field_of_view);
        if (!(fov_test > 1.0f && fov_test < 180.0f && std::isfinite(fov_test))) {
            if (camDiag < 5) { LOG("CAM_CHAIN: fov=%.2f FAILED ncam=0x%I64X", fov_test, native_cam); camDiag++; }
            cached_native_cam = 0;
            return false;
        }

        Vector3 camWorldPos = read<Vector3>(native_cam + offsets::BaseCamera::world_position);
        {
            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
            g_CameraWorldPos = camWorldPos;
        }

        Matrix4x4 camMatrix = read<Matrix4x4>(native_cam + offsets::BaseCamera::viewMatrix);
        if (!std::isfinite(camMatrix._11) || camMatrix._11 == 0) {
            if (camDiag < 5) { LOG("CAM_CHAIN: matrix zero/NaN -- _11=%.4f ncam=0x%I64X", camMatrix._11, native_cam); camDiag++; }
            return false;
        }

        bool isViewOnly = (std::fabs(camMatrix._14) < 0.001f &&
                           std::fabs(camMatrix._24) < 0.001f &&
                           std::fabs(camMatrix._34) < 0.001f);

        if (isViewOnly) {
            Matrix4x4 projCM = read<Matrix4x4>(native_cam + offsets::BaseCamera::projectionMatrix);
            if (!std::isfinite(projCM._11) || projCM._11 == 0) {
                if (camDiag < 5) { LOG("CAM_CHAIN: proj zero/NaN -- p._11=%.4f ncam=0x%I64X", projCM._11, native_cam); camDiag++; }
                return false;
            }
            if (camDiag < 3) { LOG("CAM_CHAIN: view+proj path ncam=0x%I64X fov=%.1f v._11=%.4f p._11=%.4f", native_cam, fov_test, camMatrix._11, projCM._11); camDiag++; }
            Matrix4x4 viewRM, projRM;
            viewRM._11=camMatrix._11; viewRM._12=camMatrix._21; viewRM._13=camMatrix._31; viewRM._14=camMatrix._41;
            viewRM._21=camMatrix._12; viewRM._22=camMatrix._22; viewRM._23=camMatrix._32; viewRM._24=camMatrix._42;
            viewRM._31=camMatrix._13; viewRM._32=camMatrix._23; viewRM._33=camMatrix._33; viewRM._34=camMatrix._43;
            viewRM._41=camMatrix._14; viewRM._42=camMatrix._24; viewRM._43=camMatrix._34; viewRM._44=camMatrix._44;
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
            Matrix4x4 vpT;
            vpT._11=vp._11; vpT._12=vp._21; vpT._13=vp._31; vpT._14=vp._41;
            vpT._21=vp._12; vpT._22=vp._22; vpT._23=vp._32; vpT._24=vp._42;
            vpT._31=vp._13; vpT._32=vp._23; vpT._33=vp._33; vpT._34=vp._43;
            vpT._41=vp._14; vpT._42=vp._24; vpT._43=vp._34; vpT._44=vp._44;
            src->PublishMatrix(vpT);
        } else {
            if (camDiag < 3) { LOG("CAM_CHAIN: VP direct ncam=0x%I64X fov=%.1f _11=%.4f _44=%.4f", native_cam, fov_test, camMatrix._11, camMatrix._44); camDiag++; }
            src->PublishMatrix(camMatrix);
        }
        return true;
    }

    void __fastcall Do_Cheat()
    {
        if (MISC::ShutdownRequested) return;

        if (!src->LocalPlayer) { static int c1=0; if(c1<3){LOG("Do_Cheat SKIP: no LocalPlayer");c1++;} return; }
        if (!is_valid((uintptr_t)src->LocalPlayer)) { static int c2=0; if(c2<3){LOG("Do_Cheat SKIP: LocalPlayer invalid");c2++;} return; }
        if (g_BnStableCycles.load(std::memory_order_relaxed) < 3) {
            static int c3=0; static int lastCycle=-1;
            int cyc = g_BnStableCycles.load(std::memory_order_relaxed);
            if (cyc != lastCycle && c3 < 20) { LOG("Do_Cheat SKIP: BnStableCycles=%d (need 3)", cyc); lastCycle=cyc; c3++; }
            return;
        }

        const uint64_t lpGeneration = g_LocalPlayerGeneration.load(std::memory_order_acquire);
        auto lpStillStable = [&]() -> bool {
            return src->LocalPlayer
                && is_valid((uintptr_t)src->LocalPlayer)
                && g_BnStableCycles.load(std::memory_order_relaxed) >= 3
                && g_LocalPlayerGeneration.load(std::memory_order_acquire) == lpGeneration;
        };

        // Read camera matrix FRESH in the render thread — eliminates swimming/lag
        if (!ReadFrameCameraMatrix()) {
            static int c4=0; if(c4<3){LOG("Do_Cheat SKIP: ReadFrameCameraMatrix failed");c4++;}
            return;
        }
        Matrix4x4 frameMatrix = src->GetMatrixSnapshot();
        if (!lpStillStable()) { static int c5=0; if(c5<3){LOG("Do_Cheat SKIP: lpStillStable failed after matrix check");c5++;} return; }
        static bool firstRun = true;
        if (firstRun) { LOG("Do_Cheat RUNNING — first frame (all gates passed)"); firstRun = false; }

        // g_ScreenW/g_ScreenH are global (vectorMath.hpp), updated every frame in render.hpp

        // Cache LocalPlayer position from globals (zero IOCTLs — written by fast thread)
        Vector3 lpPos;
        {
            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
            lpPos = g_LocalPlayerPos;
        }
        // Fallback: one live read if fast thread hasn't populated yet (startup)
        if (lpPos.Empty() && src->LocalPlayer) {
            lpPos = src->LocalPlayer->Get_ObjectPosition();
        }

        {
            if (!SETTINGS::MenuOpen && HotkeyPressed("UTIL.CombatMode"))
                MISC::CombatMode = !MISC::CombatMode;
            if (!SETTINGS::MenuOpen && HotkeyPressed("UTIL.BattleMode"))
                SETTINGS::BattleMode = !SETTINGS::BattleMode;
        }

        // In-game time display
        if (SETTINGS::ShowTime && g_GameTimeHour >= 0.f) {
            float h = g_GameTimeHour;
            int hh = (int)h;
            int mm = (int)((h - (float)hh) * 60.f);
            if (hh >= 24) hh = 23;
            if (mm >= 60) mm = 59;
            bool isDay = h >= 6.f && h < 19.f;
            char timeBuf[32];
            wsprintfA(timeBuf, "%02d:%02d %s", hh, mm, isDay ? "Day" : "Night");
            float tw = ImGui::CalcTextSize(timeBuf).x;
            draw_text_outline_font((float)g_ScreenW - tw - 10.f, 10.f, isDay ? ImColor(255, 220, 100, 255) : ImColor(100, 150, 255, 255), timeBuf);
        }

        // Feature indicators — top-left corner
        if (SETTINGS::FeatureIndicators) {
            float fy = 10.f;
            auto drawInd = [&](const char* label, bool on, ImColor onColor) {
                if (!on) return;
                draw_text_outline_font(10.f, fy, onColor, label);
                fy += 16.f;
            };
            drawInd("Combat Mode", MISC::CombatMode, ImColor(255, 100, 100, 255));
            drawInd("Battle Mode", SETTINGS::BattleMode, ImColor(100, 200, 255, 255));
            drawInd("Recoil Mod", MISC::RecoilEnabled, ImColor(100, 255, 100, 255));
            drawInd("No Sway", MISC::NoSway, ImColor(100, 255, 100, 255));
            drawInd("Force Auto", MISC::ForceAutomatic, ImColor(100, 255, 100, 255));
            drawInd("Bright Night", MISC::BrightNight, ImColor(200, 200, 100, 255));
            drawInd("Admin Flags", MISC::AdminFlags, ImColor(255, 200, 50, 255));
            drawInd("Debug Cam", MISC::DebugCamera, ImColor(200, 100, 255, 255));
            drawInd("Time Changer", MISC::Timechanger, ImColor(200, 200, 100, 255));
        }

        // Debug Camera on-screen instructions
        if (MISC::DebugCamera && !SETTINGS::MenuOpen) {
            float cx = (float)g_ScreenW * 0.5f;
            float cy = (float)g_ScreenH * 0.5f;
            const char* hint = "Admin ON — Press F1, type: debugcamera";
            ImVec2 sz = ImGui::CalcTextSize(hint);
            draw_text_outline_font(cx - sz.x * 0.5f, cy - 40.f, ImColor(200, 100, 255, 255), hint);
        }

        const bool battleMode = SETTINGS::BattleMode;
        const bool allowPlayers = !battleMode || BATTLE::Players;
        const bool allowWorld = !battleMode || BATTLE::World;
        const bool allowNPC = !battleMode || BATTLE::NPC;
        const bool allowAnimals = !battleMode || BATTLE::Animals;
        const bool allowAimbot = !battleMode || BATTLE::Aimbot;

        if (allowWorld) {
            static std::vector<Rust::PrefabData> pList;
            { std::unique_lock<std::mutex> lk(cac->prefab_mutex, std::try_to_lock); if (lk.owns_lock()) pList = prefab_List; }

            for (const auto& prefab : pList)
        {
            Vector2 Screen = WorldToScreen(prefab.Position, frameMatrix);
            if (!std::isfinite(Screen.x) || !std::isfinite(Screen.y)
                || Screen.x <= 1 || Screen.x >= (float)g_ScreenW - 1
                || Screen.y <= 1 || Screen.y >= (float)g_ScreenH - 1) {
                continue;
            }

            ImVec2 Size = ImGui::CalcTextSize(prefab.name.c_str());
            auto position = Vector2(Screen.x - (Size.x / 2), Screen.y - (Size.y / 2));
            draw_text_outline_font(position.x, position.y, prefab.Color, prefab.name.c_str());

            if (WORLD::Distance)
            {
                float Distance = lpPos.DistTo(prefab.Position);
                auto txt = std::string("[" + std::to_string(int(Distance)) + "M]");
                auto sz = ImGui::CalcTextSize(txt.c_str());
                draw_text_outline_font(position.x, position.y - 13, WORLD::color::Distance_Color, txt.c_str());
            }

            // Vehicle health bar
            if (prefab.health >= 0.f && prefab.maxHealth > 0.f) {
                float hpPct = prefab.health / prefab.maxHealth;
                if (hpPct < 0.f) hpPct = 0.f;
                if (hpPct > 1.f) hpPct = 1.f;
                float barW = 60.f;
                float barH = 4.f;
                float barX = Screen.x - barW / 2.f;
                float barY = position.y + Size.y + 2;
                ImColor barBg = ImColor(0, 0, 0, 180);
                ImColor barFg = hpPct > 0.5f ? ImColor(0, 220, 0, 255) : (hpPct > 0.25f ? ImColor(220, 180, 0, 255) : ImColor(220, 40, 40, 255));
                ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(barX - 1, barY - 1), ImVec2(barX + barW + 1, barY + barH + 1), barBg);
                ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barW * hpPct, barY + barH), barFg);
            }

            // Hackable crate timer
            if (prefab.hackSeconds > 0.f) {
                int totalSec = (int)prefab.hackSeconds;
                int mins = totalSec / 60;
                int secs = totalSec % 60;
                char timerTxt[32];
                wsprintfA(timerTxt, "Hack: %dm %ds", mins, secs);
                auto tsz = ImGui::CalcTextSize(timerTxt);
                draw_text_outline_font(Screen.x - tsz.x / 2, position.y + Size.y + 8, ImColor(255, 200, 50, 255), timerTxt);
            }
        }

        // Saved stashes (persisted across sessions)
        if (WORLD::StashPersist) {
            std::vector<SavedStash> saved;
            { std::lock_guard<std::mutex> lk(g_StashMutex); saved = g_SavedStashes; }
            for (const auto& ss : saved) {
                float dist = lpPos.DistTo(ss.position);
                if (dist > WORLD::draw_stash) continue;
                Vector2 Screen = WorldToScreen(ss.position, frameMatrix);
                if (!std::isfinite(Screen.x) || !std::isfinite(Screen.y)
                    || Screen.x <= 1 || Screen.x >= (float)g_ScreenW - 1
                    || Screen.y <= 1 || Screen.y >= (float)g_ScreenH - 1) continue;
                const char* txt = "Stash (saved)";
                ImVec2 sz = ImGui::CalcTextSize(txt);
                draw_text_outline_font(Screen.x - sz.x / 2, Screen.y - sz.y / 2, ImColor(180, 180, 80, 200), txt);
                if (WORLD::Distance) {
                    char dtxt[32]; wsprintfA(dtxt, "[%dm]", (int)dist);
                    auto dsz = ImGui::CalcTextSize(dtxt);
                    draw_text_outline_font(Screen.x - dsz.x / 2, Screen.y + sz.y / 2, WORLD::color::Distance_Color, dtxt);
                }
            }
        }

        // Raid ESP
        if (WORLD::RaidESP) {
            std::vector<RaidEvent> raids;
            { std::lock_guard<std::mutex> lk(g_RaidMutex); raids = g_RaidEvents; }
            uint64_t nowMs = GetTickCount64();
            for (const auto& raid : raids) {
                float dist = lpPos.DistTo(raid.position);
                if (dist > WORLD::draw_raid) continue;
                Vector2 Screen = WorldToScreen(raid.position, frameMatrix);
                if (!std::isfinite(Screen.x) || !std::isfinite(Screen.y)
                    || Screen.x <= 1 || Screen.x >= (float)g_ScreenW - 1
                    || Screen.y <= 1 || Screen.y >= (float)g_ScreenH - 1) {
                    continue;
                }
                int elapsed = (int)((nowMs - raid.timestamp) / 1000);
                int remaining = 120 - elapsed;
                if (remaining <= 0) continue;
                char raidTxt[64];
                wsprintfA(raidTxt, "RAID [%dx] %ds", raid.count, remaining);
                ImVec2 sz = ImGui::CalcTextSize(raidTxt);
                float rx = Screen.x - sz.x / 2;
                float ry = Screen.y - sz.y / 2;
                ImColor raidColor = elapsed < 30 ? ImColor(255, 50, 50, 255) : ImColor(255, 150, 50, 255);
                draw_text_outline_font(rx, ry, raidColor, raidTxt);
                if (WORLD::Distance) {
                    char distTxt[32];
                    wsprintfA(distTxt, "[%dm]", (int)dist);
                    auto dsz = ImGui::CalcTextSize(distTxt);
                    draw_text_outline_font(Screen.x - dsz.x / 2, ry - 13, WORLD::color::Distance_Color, distTxt);
                }
            }
        }
        }

        static std::vector<Rust::BaseEntity*> entlist;
        { std::unique_lock<std::mutex> lk(cac->entity_mutex, std::try_to_lock); if (lk.owns_lock()) entlist = entity_List; }

        {
            static bool entlist_empty_logged = false;
            static ULONGLONG emptySince = 0;
            if (entlist.empty()) {
                if (emptySince == 0) emptySince = GetTickCount64();
                if (!entlist_empty_logged && (GetTickCount64() - emptySince) > 3000) {
                    LOG("Do_Cheat: entity_List STILL EMPTY after 3s — check BN chain / classification");
                    entlist_empty_logged = true;
                }
            } else {
                emptySince = 0;
                entlist_empty_logged = false;
            }
        }

        {
            static int logTick = 0;
            if ((++logTick % 240) == 0) {
                int pCount = (int)entlist.size();
                int nCount = 0;
                int aCount = 0;
                { std::lock_guard<std::mutex> lk(cac->npc_mutex); nCount = (int)npc_List.size(); }
                { std::lock_guard<std::mutex> lk(cac->animal_mutex); aCount = (int)animal_List.size(); }
                LOG("ESP Lists: players=%d npcs=%d animals=%d | toggles P=%d N=%d A=%d",
                    pCount, nCount, aCount,
                    (int)(ESP::Box || ESP::Name || ESP::Distance || ESP::Skeleton || ESP::HealthBar || ESP::Weapon),
                    (int)NPC_ESP::Enabled,
                    (int)ANIMAL_ESP::Enabled);
            }
        }

        static bool dist_diag = false;

        // Copy entire cache ONCE per frame — try_to_lock (never blocks render thread)
        // If cache worker is merging, use previous frame's copy (at most 100ms stale)
        static std::unordered_map<uintptr_t, EspCacheData>* pCacheCopy = nullptr;
        if (!pCacheCopy) pCacheCopy = new std::unordered_map<uintptr_t, EspCacheData>();
        auto& cacheCopy = *pCacheCopy;
        {
            std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex, std::try_to_lock);
            if (lk.owns_lock()) {
                cacheCopy = g_EspCache;
            }
        }

        // Compute target player closest to crosshair (for hotbar + inventory panel)
        // Only considers players (entlist), not animals/NPCs
        if (ESP::hotbar_text || ESP::PlayerInventoryPanel) {
            uintptr_t bestTgt = 0;
            float best = FLT_MAX;
            float cx = (float)g_ScreenW * 0.5f;
            float cy = (float)g_ScreenH * 0.5f;
            float fovLimit = ESP::PlayerInventoryPanel ? 150.0f * 150.0f : FLT_MAX;
            for (auto* Player : entlist) {
                if (!Player || !is_valid((uintptr_t)Player)) continue;
                auto it = cacheCopy.find((uintptr_t)Player);
                if (it == cacheCopy.end()) continue;
                const auto& v = it->second;
                if (v.isDead) continue;
                Vector3 sp = v.spine1Pos;
                if (sp.Empty()) sp = v.headPos;
                if (sp.Empty()) continue;
                Vector2 ps = WorldToScreen(sp, frameMatrix);
                if (!std::isfinite(ps.x) || !std::isfinite(ps.y)
                    || ps.x <= 1 || ps.x >= (float)g_ScreenW - 1
                    || ps.y <= 1 || ps.y >= (float)g_ScreenH - 1) continue;
                float dx = ps.x - cx;
                float dy = ps.y - cy;
                float d2 = dx * dx + dy * dy;
                if (d2 < best && d2 <= fovLimit) { best = d2; bestTgt = (uintptr_t)Player; }
            }
            g_HotbarTarget = bestTgt;
        } else {
            g_HotbarTarget = 0;
        }

        // Toggle friend on player closest to crosshair
        if (!SETTINGS::MenuOpen && HotkeyPressed("UTIL.MarkFriend") && g_HotbarTarget) {
            auto fit = cacheCopy.find(g_HotbarTarget);
            if (fit != cacheCopy.end()) {
                std::string name = fit->second.name;
                if (!name.empty()) {
                    std::lock_guard<std::mutex> lk(g_FriendMutex);
                    if (g_Friends.count(name)) { g_Friends.erase(name); LOG("FRIENDS: removed %s", name.c_str()); }
                    else { g_Friends.insert(name); LOG("FRIENDS: added %s", name.c_str()); }
                    SaveFriends();
                }
            }
        }

        if (allowPlayers) {
        if (!ESP::ESPEnabled) goto skipPlayers;

        for (const auto& Player : entlist)
        {
            if (!Player || !is_valid((uintptr_t)Player)) continue;

            auto cit = cacheCopy.find((uintptr_t)Player);
            if (cit == cacheCopy.end()) continue;
            const EspCacheData& pcache = cit->second;

            // Check if this player is a teammate — teammates are exempt from
            // dead/sleeper/wounded filters so you always see them.
            bool isTeammate = false;
            if (pcache.teamId != 0) {
                uint64_t myTeam = 0;
                { std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex); myTeam = g_LocalPlayerTeam; }
                isTeammate = (myTeam != 0 && pcache.teamId == myTeam);
            }

            if (pcache.isDead && !isTeammate) continue;
            if (ESP::RemoveWounded && pcache.isWounded && !isTeammate) continue;
            if (ESP::RemoveSleepers && pcache.isSleeping && !isTeammate) continue;
            if (ESP::RemoveTeam && isTeammate) continue;

            Vector3 tPos = pcache.headPos;
            {
                uint64_t age = GetTickCount64() - pcache.cacheTick;
                float predDt = (float)age * 0.001f;
                if (predDt > 0.0f && predDt < 1.0f) {
                    tPos.x += pcache.velocity.x * predDt;
                    tPos.y += pcache.velocity.y * predDt;
                    tPos.z += pcache.velocity.z * predDt;
                    // Gravity correction for airborne players so jumping/falling
                    // tracking follows the parabola instead of floating in the air.
                    if (!pcache.isGrounded) {
                        tPos.y -= 0.5f * 9.81f * predDt * predDt;
                    }
                }
            }
            if (tPos.Empty()) continue;

            float dist = lpPos.DistTo(tPos);
            if (!dist_diag && !lpPos.Empty() && !tPos.Empty()) {
                LOG("Do_Cheat: LocalPlayer spine1=(%d,%d,%d) target=(%d,%d,%d) dist=%d",
                    (int)lpPos.x, (int)lpPos.y, (int)lpPos.z, (int)tPos.x, (int)tPos.y, (int)tPos.z, (int)dist);
                dist_diag = true;
            }
            if (dist > ESP::draw_distance) continue;

            Vector2 Screen = WorldToScreen(tPos, frameMatrix);     
            if (!std::isfinite(Screen.x) || !std::isfinite(Screen.y)
                || Screen.x <= 1 || Screen.x >= (float)g_ScreenW - 1
                || Screen.y <= 1 || Screen.y >= (float)g_ScreenH - 1) {
                continue;
            }


            esp->do_Visuals(Player, lpPos, pcache, frameMatrix);
            
        }
        }
        skipPlayers:;

        // Player Inventory Panel — shows inventory of player you're looking at
        if (ESP::PlayerInventoryPanel && g_HotbarTarget && !SETTINGS::MenuOpen) {
            auto pit = cacheCopy.find(g_HotbarTarget);
            if (pit != cacheCopy.end()) {
                const EspCacheData& tc = pit->second;
                float panelDist = lpPos.DistTo(tc.headPos);
                if (panelDist <= ESP::draw_distance && !tc.isDead) {
                    float panelW = 230.0f;
                    float panelX = (float)g_ScreenW - panelW - 10.0f;
                    ImGui::SetNextWindowPos(ImVec2(panelX, (float)g_ScreenH * 0.5f), ImGuiCond_Always, ImVec2(0.0f, 0.5f));
                    ImGui::SetNextWindowSize(ImVec2(panelW, 0), ImGuiCond_Always);
                    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.08f, 0.88f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.35f, 0.6f));
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
                    ImGui::Begin("##InvPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);

                    ImGui::TextColored(ImColor(200, 200, 210, 255), "%s", cacheStrEmpty(tc.name) ? "Unknown" : tc.name);
                    ImGui::TextColored(ImColor(150, 170, 200, 255), "Distance: %dm", (int)panelDist);
                    ImGui::Separator();

                    ImGui::TextColored(ImColor(180, 190, 200, 255), "Held: %s", cacheStrEmpty(tc.weaponName) ? "Empty" : tc.weaponName);
                    if (tc.ammo > 0)
                        ImGui::TextColored(ImColor(150, 170, 200, 255), "Ammo: %d", tc.ammo);
                    ImGui::Separator();

                    ImGui::TextColored(ImColor(150, 170, 200, 255), "Belt:");
                    for (int i = 0; i < 6; i++) {
                        if (!cacheStrEmpty(tc.belt[i])) {
                            bool isActive = !cacheStrEmpty(tc.weaponName) && strcmp(tc.belt[i], tc.weaponName) == 0;
                            if (isActive)
                                ImGui::TextColored(ImColor(100, 255, 100, 255), "  %d. %s  <<<", i + 1, tc.belt[i]);
                            else
                                ImGui::TextColored(ImColor(200, 200, 210, 255), "  %d. %s", i + 1, tc.belt[i]);
                        } else {
                            ImGui::TextColored(ImColor(90, 90, 90, 255), "  %d. ---", i + 1);
                        }
                    }
                    ImGui::Separator();

                    bool hasWear = false;
                    for (int i = 0; i < 5; i++) if (!cacheStrEmpty(tc.wear[i])) { hasWear = true; break; }
                    if (hasWear) {
                        ImGui::TextColored(ImColor(150, 170, 200, 255), "Wear:");
                        for (int i = 0; i < 5; i++) {
                            if (!cacheStrEmpty(tc.wear[i]))
                                ImGui::TextColored(ImColor(200, 200, 210, 255), "  %s", tc.wear[i]);
                        }
                    }

                    ImGui::End();
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(2);
                }
            }
        }

        // Player List Window
        if (SETTINGS::PlayerList && !SETTINGS::MenuOpen) {
            ImGui::SetNextWindowPos(ImVec2(10, (float)g_ScreenH * 0.15f), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(340, (float)g_ScreenH * 0.5f), ImGuiCond_FirstUseEver);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.08f, 0.88f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.35f, 0.6f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
            if (ImGui::Begin("Player List##PL", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                ImGui::TextColored(ImColor(200, 200, 210, 255), "Players (%d)", (int)entlist.size());
                ImGui::Separator();
                for (const auto& Player : entlist) {
                    if (!Player || !is_valid((uintptr_t)Player)) continue;
                    auto it = cacheCopy.find((uintptr_t)Player);
                    if (it == cacheCopy.end()) continue;
                    const auto& ec = it->second;
                    if (ec.isDead) continue;
                    float dist = lpPos.DistTo(ec.headPos);
                    if (dist > ESP::draw_distance) continue;
                    int hp = (int)ec.health;
                    int mhp = (int)ec.maxHealth;
                    if (mhp <= 0) mhp = 100;
                    ImColor nameColor = ec.isSleeping ? ImColor(150, 150, 150, 255) :
                                        ec.isWounded ? ImColor(200, 100, 100, 255) :
                                        ec.teamId == g_LocalPlayerTeam ? ImColor(100, 200, 100, 255) : ImColor(220, 220, 220, 255);
                    ImGui::TextColored(nameColor, "%-16s", cacheStrEmpty(ec.name) ? "Unknown" : ec.name);
                    ImGui::SameLine(170);
                    ImGui::TextColored(ImColor(150, 170, 200, 255), "%dm", (int)dist);
                    ImGui::SameLine(220);
                    float hpPct = (float)hp / (float)mhp;
                    ImColor hpColor = hpPct > 0.5f ? ImColor(100, 255, 100, 255) : (hpPct > 0.25f ? ImColor(255, 200, 50, 255) : ImColor(255, 80, 80, 255));
                    ImGui::TextColored(hpColor, "%dhp", hp);
                    ImGui::SameLine(260);
                    if (!cacheStrEmpty(ec.weaponName))
                        ImGui::TextColored(ImColor(180, 180, 180, 220), "%.12s", ec.weaponName);
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
        }

        if (allowNPC && NPC_ESP::Enabled && !MISC::CombatMode) {
            static std::vector<Rust::BaseEntity*> npclist;
            { std::unique_lock<std::mutex> lk(cac->npc_mutex, std::try_to_lock); if (lk.owns_lock()) npclist = npc_List; }
            int npcCount = 0;
            static int npcDiagTick = 0;
            static int npcTotal = 0, npcValid = 0, npcDead = 0, npcCacheMiss = 0, npcPosEmpty = 0, npcDistExceeded = 0, npcRendered = 0;
            npcTotal += (int)npclist.size();
            for (const auto& NPC : npclist) {
                if (!NPC || !is_valid((uintptr_t)NPC)) { npcValid++; continue; }
                auto cit = cacheCopy.find((uintptr_t)NPC);
                if (cit == cacheCopy.end()) { npcCacheMiss++; continue; }
                const EspCacheData& ncache = cit->second;
                if (ncache.isDead) { npcDead++; continue; }
                Vector3 npcPos = ncache.headPos;
                if (npcPos.Empty()) { npcPosEmpty++; continue; }
                float nd = lpPos.DistTo(npcPos);
                if (nd > NPC_ESP::draw_distance) { npcDistExceeded++; continue; }
                esp->do_NPC_Visuals(NPC, lpPos, ncache, frameMatrix);
                npcRendered++;
                npcCount++;
            }
            if ((++npcDiagTick % 240) == 0) {
                LOG("NPC FILTER: total=%d validOK=%d dead=%d cacheMiss=%d posEmpty=%d distExceeded=%d rendered=%d",
                    npcTotal, npcValid, npcDead, npcCacheMiss, npcPosEmpty, npcDistExceeded, npcRendered);
                npcTotal = npcValid = npcDead = npcCacheMiss = npcPosEmpty = npcDistExceeded = npcRendered = 0;
            }
        }

        if (allowAnimals && ANIMAL_ESP::Enabled && !MISC::CombatMode) {
            static std::vector<Rust::BaseEntity*> animallist;
            { std::unique_lock<std::mutex> lk(cac->animal_mutex, std::try_to_lock); if (lk.owns_lock()) animallist = animal_List; }
            for (const auto& Animal : animallist) {
                if (!Animal || !is_valid((uintptr_t)Animal)) continue;
                auto cit = cacheCopy.find((uintptr_t)Animal);
                if (cit == cacheCopy.end()) continue;
                const EspCacheData& acache = cit->second;
                if (acache.isDead) continue;
                Vector3 apos = acache.headPos;
                if (apos.Empty()) continue;
                float ad = lpPos.DistTo(apos);
                if (ad > ANIMAL_ESP::draw_distance) continue;
                esp->do_Animal_Visuals(Animal, lpPos, acache, frameMatrix);
            }
        }

        // do_misc and do_aimbot now run on background threads (see Rust Prv Ext.cpp)

        if (SETTINGS::BattleMode) {
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)g_ScreenH - 25), ImColor(255, 50, 50, 255), TR("BATTLE MODE"));
        } else if (MISC::CombatMode) {
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)g_ScreenH - 25), ImColor(255, 50, 50, 255), TR("COMBAT MODE"));
        }

        // ESP status: show entity counts on screen
        {
            static int espTick = 0;
            if ((++espTick % 60) == 0) {
                int pCount = (int)entity_List.size();
                int nCount = 0, aCount = 0;
                { std::lock_guard<std::mutex> lk(cac->npc_mutex); nCount = (int)npc_List.size(); }
                { std::lock_guard<std::mutex> lk(cac->animal_mutex); aCount = (int)animal_List.size(); }
                char buf[64];
                sprintf_s(buf, "ESP: %dP %dN %dA", pCount, nCount, aCount);
                ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)g_ScreenH - 45), ImColor(100, 220, 100, 200), buf);
            }
        }

        if (MISC::DebugCamera) {
            float remaining = 50.0f - MISC::DebugCameraTimer;
            if (remaining < 0.0f) remaining = 0.0f;
            ImColor timerColor = (remaining <= 10.0f) ? ImColor(255, 50, 50, 255) : ImColor(255, 220, 80, 255);
            char debugCamText[64] = {};
            sprintf_s(debugCamText, "DEBUG CAM EXIT IN: %.0fs", remaining);
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)g_ScreenH - 45), timerColor, debugCamText);
        }

        // Aimbot runs in render thread (like SHA source) — uses validated LocalPlayer, no race conditions
        if (!SETTINGS::BattleMode || BATTLE::Aimbot) {
            aim->do_aimbot();
        }

    }

}

static int s_doCheatCrashCount = 0;

void SafeDoCheat() {
    __try {
        Xheat::Do_Cheat();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        s_doCheatCrashCount++;
        LOG("CRASH in Do_Cheat! code=0x%X crash#%d", GetExceptionCode(), s_doCheatCrashCount);
    }
}

// ===================== BIG MAP OVERLAY =====================

static uint64_t s_worldSizeLastRead = 0;

static void ReadWorldSize() {
    if (GetTickCount64() - s_worldSizeLastRead < 30000) return;
    s_worldSizeLastRead = GetTickCount64();
    uintptr_t rustWorldTI = read<uint64_t>(GameAssembly + offsets::mapview::RVA_RustWorld);
    if (!rustWorldTI || !is_valid(rustWorldTI)) return;
    uintptr_t sf = read<uint64_t>(rustWorldTI + offsets::mapview::static_fields_off);
    if (!sf || !is_valid(sf)) return;
    float ws = read<float>(sf + offsets::mapview::World_Size_off);
    if (ws > 100.f && ws < 100000.f && std::isfinite(ws))
        g_WorldSize = ws;
}

// Map overlay toggle — hotkey based, no auto-detection false positives
static bool g_MapToggleState = false;
static bool g_MapPrevG = false;

static bool IsMapOpen() {
    // Toggle on G key rising edge (syncs with in-game map open/close)
    bool gDown = (GetAsyncKeyState('G') & 0x8000) != 0;
    if (gDown && !g_MapPrevG) {
        g_MapToggleState = !g_MapToggleState;
        LOG("MAP_TOGGLE: %s", g_MapToggleState ? "ON" : "OFF");
    }
    g_MapPrevG = gDown;
    return g_MapToggleState;
}

static void DrawMapOverlay() {
    if (!src->LocalPlayer) return;

    float ws = g_WorldSize;
    float halfWs = ws * 0.5f;

    // Player-centered map: full world spread across 80% of screen
    float cx = g_ScreenW * 0.5f;
    float cy = g_ScreenH * 0.5f;
    float halfW = g_ScreenW * 0.4f;
    float halfH = g_ScreenH * 0.4f;

    auto* dl = ImGui::GetBackgroundDrawList();
    float dotSz = BIGMAP::DotSize;
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    uintptr_t hoverPlayer = 0;
    float hoverBest = 400.f;

    static int mapDrawDiag = 0;
    bool doDiag = ((++mapDrawDiag % 300) == 0);

    // World to screen: full world map, north-up
    auto worldToScreen = [&](Vector3 wp) -> ImVec2 {
        float normX = (wp.x + halfWs) / ws;   // 0=west, 1=east
        float normY = (wp.z + halfWs) / ws;   // 0=south, 1=north
        float sx = (cx - halfW) + normX * halfW * 2.f;
        float sy = (cy + halfH) - normY * halfH * 2.f;  // north=top
        return ImVec2(sx, sy);
    };

    auto inView = [&](ImVec2 p) -> bool {
        return p.x >= cx - halfW && p.x <= cx + halfW && p.y >= cy - halfH && p.y <= cy + halfH;
    };

    Vector3 lpPos;
    { std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex); lpPos = g_LocalPlayerPos; }

    if (doDiag) {
        ImVec2 lpScr = worldToScreen(lpPos);
        LOG("MAP_DRAW: ws=%.0f lp=(%.0f,%.0f,%.0f) screen=(%.0f,%.0f) sw=%d sh=%d",
            ws, lpPos.x, lpPos.y, lpPos.z, lpScr.x, lpScr.y, g_ScreenW, g_ScreenH);
    }

    // Draw players
    if (BIGMAP::ShowPlayers) {
        static std::vector<Rust::BaseEntity*> entlist;
        { std::unique_lock<std::mutex> lk(cac->entity_mutex, std::try_to_lock); if (lk.owns_lock()) entlist = entity_List; }

        static std::unordered_map<uintptr_t, EspCacheData>* pCC = nullptr;
        if (!pCC) pCC = new std::unordered_map<uintptr_t, EspCacheData>();
        auto& cacheCopy = *pCC;
        { std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex, std::try_to_lock); if (lk.owns_lock()) cacheCopy = g_EspCache; }

        uint64_t myTeam = 0;
        { std::lock_guard<std::mutex> tlk(g_LocalPlayerDataMutex); myTeam = g_LocalPlayerTeam; }

        for (auto* Player : entlist) {
            if (!Player || !is_valid((uintptr_t)Player)) continue;
            auto it = cacheCopy.find((uintptr_t)Player);
            if (it == cacheCopy.end()) continue;
            const auto& ec = it->second;
            if (ec.isDead) continue;

            ImVec2 pos = worldToScreen(ec.headPos);
            if (!inView(pos)) continue;

            if (doDiag) {
                LOG("MAP_DRAW: player '%s' world=(%.0f,%.0f,%.0f) screen=(%.0f,%.0f) dist=%.0fm",
                    cacheStrEmpty(ec.name) ? "?" : ec.name,
                    ec.headPos.x, ec.headPos.y, ec.headPos.z,
                    pos.x, pos.y, lpPos.DistTo(ec.headPos));
            }

            bool isTeammate = (ec.teamId != 0 && myTeam != 0 && ec.teamId == myTeam);
            ImColor col = ec.isSleeping ? BIGMAP::SleeperColor :
                          isTeammate ? BIGMAP::TeammateColor : BIGMAP::PlayerColor;

            dl->AddCircleFilled(pos, dotSz, col);
            dl->AddCircle(pos, dotSz, ImColor(0, 0, 0, 200), 0, 1.f);

            if (BIGMAP::ShowNames && !cacheStrEmpty(ec.name)) {
                float dist = lpPos.DistTo(ec.headPos);
                char label[80];
                if (BIGMAP::ShowDistance)
                    snprintf(label, sizeof(label), "%s [%dm]", ec.name, (int)dist);
                else
                    snprintf(label, sizeof(label), "%s", ec.name);
                dl->AddText(ImVec2(pos.x + dotSz + 2, pos.y - 6), col, label);
            }

            float dx = mousePos.x - pos.x, dy = mousePos.y - pos.y;
            float d2 = dx * dx + dy * dy;
            if (d2 < hoverBest) { hoverBest = d2; hoverPlayer = (uintptr_t)Player; }
        }

        if (hoverPlayer) {
            auto it = cacheCopy.find(hoverPlayer);
            if (it != cacheCopy.end()) {
                const auto& ec = it->second;
                ImGui::BeginTooltip();
                ImGui::TextColored(ImColor(220, 220, 220, 255), "%s", cacheStrEmpty(ec.name) ? "Player" : ec.name);
                int hp = (int)ec.health, mhp = (int)ec.maxHealth;
                if (mhp <= 0) mhp = 100;
                ImGui::TextColored(ImColor(100, 255, 100, 255), "HP: %d/%d", hp, mhp);
                float dist = lpPos.DistTo(ec.headPos);
                ImGui::TextColored(ImColor(150, 170, 200, 255), "Distance: %dm", (int)dist);
                if (!cacheStrEmpty(ec.weaponName))
                    ImGui::TextColored(ImColor(200, 200, 100, 255), "Weapon: %s", ec.weaponName);
                bool hasBelt = false;
                for (int i = 0; i < 6; i++) if (!cacheStrEmpty(ec.belt[i])) { hasBelt = true; break; }
                if (hasBelt) {
                    ImGui::Separator();
                    ImGui::TextColored(ImColor(150, 170, 200, 255), "Belt:");
                    for (int i = 0; i < 6; i++) {
                        if (!cacheStrEmpty(ec.belt[i])) {
                            bool active = !cacheStrEmpty(ec.weaponName) && strcmp(ec.belt[i], ec.weaponName) == 0;
                            ImGui::TextColored(active ? ImColor(100, 255, 100, 255) : ImColor(200, 200, 210, 255),
                                "  %d. %s%s", i + 1, ec.belt[i], active ? " <<" : "");
                        }
                    }
                }
                ImGui::EndTooltip();
            }
        }
    }

    // Draw NPCs
    if (BIGMAP::ShowNPCs) {
        static std::vector<Rust::BaseEntity*> npclist;
        { std::unique_lock<std::mutex> lk(cac->npc_mutex, std::try_to_lock); if (lk.owns_lock()) npclist = npc_List; }

        static std::unordered_map<uintptr_t, EspCacheData>* pNPC = nullptr;
        if (!pNPC) pNPC = new std::unordered_map<uintptr_t, EspCacheData>();
        auto& npcCache = *pNPC;
        { std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex, std::try_to_lock); if (lk.owns_lock()) npcCache = g_EspCache; }

        for (auto* NPC : npclist) {
            if (!NPC || !is_valid((uintptr_t)NPC)) continue;
            auto it = npcCache.find((uintptr_t)NPC);
            if (it == npcCache.end()) continue;
            if (it->second.isDead) continue;
            ImVec2 pos = worldToScreen(it->second.headPos);
            if (!inView(pos)) continue;
            dl->AddCircleFilled(pos, dotSz * 0.7f, BIGMAP::NpcColor);
        }
    }

    // Draw animals
    if (BIGMAP::ShowAnimals) {
        static std::vector<Rust::BaseEntity*> animallist;
        { std::unique_lock<std::mutex> lk(cac->animal_mutex, std::try_to_lock); if (lk.owns_lock()) animallist = animal_List; }

        static std::unordered_map<uintptr_t, EspCacheData>* pAn = nullptr;
        if (!pAn) pAn = new std::unordered_map<uintptr_t, EspCacheData>();
        auto& anCache = *pAn;
        { std::shared_lock<std::shared_mutex> lk(g_EspCacheMutex, std::try_to_lock); if (lk.owns_lock()) anCache = g_EspCache; }

        for (auto* Animal : animallist) {
            if (!Animal || !is_valid((uintptr_t)Animal)) continue;
            auto it = anCache.find((uintptr_t)Animal);
            if (it == anCache.end()) continue;
            if (it->second.isDead) continue;
            ImVec2 pos = worldToScreen(it->second.headPos);
            if (!inView(pos)) continue;
            dl->AddCircleFilled(pos, dotSz * 0.7f, BIGMAP::AnimalColor);
        }
    }

    // Draw world items from prefab_List
    {
        static std::vector<Rust::PrefabData> pList;
        { std::unique_lock<std::mutex> lk(cac->prefab_mutex, std::try_to_lock); if (lk.owns_lock()) pList = prefab_List; }

        for (const auto& prefab : pList) {
            const std::string& name = prefab.name;
            std::string low = name;
            for (char& c : low) c = (char)tolower((unsigned char)c);

            bool draw = false;
            ImColor col = prefab.Color;
            float sz = dotSz * 0.55f;

            if (BIGMAP::ShowOres && (low.find("stone") != std::string::npos || low.find("metal") != std::string::npos || low.find("sulfur") != std::string::npos || low.find("ore") != std::string::npos)) {
                draw = true;
                if (low.find("sulfur") != std::string::npos) col = ImColor(255, 255, 100, 255);
                else if (low.find("metal") != std::string::npos) col = ImColor(180, 180, 180, 255);
                else col = ImColor(100, 150, 255, 255);
            } else if (BIGMAP::ShowHemp && low.find("hemp") != std::string::npos) {
                draw = true; col = ImColor(100, 255, 100, 255);
            } else if (BIGMAP::ShowStashes && low.find("stash") != std::string::npos) {
                draw = true; sz = dotSz * 0.7f; col = ImColor(255, 255, 100, 255);
            } else if (BIGMAP::ShowTCs && low.find("cupboard") != std::string::npos) {
                draw = true; sz = dotSz * 0.7f; col = ImColor(160, 120, 80, 255);
            } else if (BIGMAP::ShowCrates && (low.find("crate") != std::string::npos || low.find("hackable") != std::string::npos)) {
                draw = true; sz = dotSz * 0.7f; col = ImColor(100, 150, 255, 255);
            } else if (BIGMAP::ShowVehicles && (low.find("boat") != std::string::npos || low.find("heli") != std::string::npos || low.find("balloon") != std::string::npos || low.find("car") != std::string::npos || low.find("bike") != std::string::npos)) {
                draw = true; col = ImColor(0, 200, 200, 255);
            } else if (BIGMAP::ShowTurrets && (low.find("turret") != std::string::npos || low.find("trap") != std::string::npos || low.find("sam") != std::string::npos)) {
                draw = true; col = ImColor(255, 80, 80, 255);
            }

            if (!draw) continue;
            ImVec2 pos = worldToScreen(prefab.Position);
            if (!inView(pos)) continue;

            // Squares for TC/turrets, circles for everything else — dots only, no text
            if (low.find("cupboard") != std::string::npos || low.find("turret") != std::string::npos || low.find("trap") != std::string::npos) {
                dl->AddRectFilled(ImVec2(pos.x - sz, pos.y - sz), ImVec2(pos.x + sz, pos.y + sz), col);
            } else {
                dl->AddCircleFilled(pos, sz, col);
            }
        }
    }

    // Draw local player marker (white diamond)
    if (!lpPos.Empty()) {
        ImVec2 pos = worldToScreen(lpPos);
        if (inView(pos)) {
            float s = dotSz + 2;
            dl->AddQuadFilled(
                ImVec2(pos.x, pos.y - s), ImVec2(pos.x + s, pos.y),
                ImVec2(pos.x, pos.y + s), ImVec2(pos.x - s, pos.y),
                ImColor(255, 255, 255, 255));
        }
    }
}

void SafeDrawMap() {
    __try {
        ReadWorldSize(); // throttled to every 30s — 1 IOCTL
        bool mapOpen = IsMapOpen(); // cursor check + throttled instance check — ~0 IOCTLs

        g_MapOpen.store(mapOpen, std::memory_order_relaxed);

        if (mapOpen) {
            __try {
                DrawMapOverlay();
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                static int mapCrashCount = 0;
                mapCrashCount++;
                if (mapCrashCount <= 3)
                    LOG("CRASH in DrawMapOverlay! code=0x%X crash#%d", GetExceptionCode(), mapCrashCount);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}
