#include "Hacks/Aimbot/aimbot.hpp"
#include "../AntiCheat/anybrain.hpp"
#include "../../Logger.hpp"
#include "../../Hotkeys.hpp"
#include "../../Occlusion.hpp"
#include <cmath>

namespace Xheat {

    bool ReadFrameCameraMatrix() {
        static int camDiag = 0;
        uint64_t cam_typeinfo = read<uint64_t>(GameAssembly + offsets::camera_pointer);
        if (!cam_typeinfo || !is_valid(cam_typeinfo)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_typeinfo invalid (0x%I64X)", cam_typeinfo); camDiag++; } return false; }
        uint64_t cam_sf = read<uint64_t>(cam_typeinfo + offsets::BaseCamera::static_fields);
        if (!cam_sf || !is_valid(cam_sf)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_sf invalid (0x%I64X)", cam_sf); camDiag++; } return false; }
        uint64_t cam_instance = read<uint64_t>(cam_sf + offsets::BaseCamera::wrapper_class);
        if (!cam_instance || !is_valid(cam_instance)) { if (camDiag < 5) { LOG("CAM_CHAIN: cam_instance invalid (0x%I64X)", cam_instance); camDiag++; } return false; }
        uint64_t native_cam = read<uint64_t>(cam_instance + offsets::BaseCamera::entity);
        if (!native_cam || !is_valid(native_cam)) native_cam = cam_instance;
        if (!native_cam || !is_valid(native_cam)) { if (camDiag < 5) { LOG("CAM_CHAIN: native_cam invalid"); camDiag++; } return false; }
        float fov_test = read<float>(native_cam + offsets::BaseCamera::field_of_view);
        if (!(fov_test > 1.0f && fov_test < 180.0f && std::isfinite(fov_test))) {
            if (camDiag < 5) { LOG("CAM_CHAIN: fov=%.2f FAILED ncam=0x%I64X", fov_test, native_cam); camDiag++; }
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

        ReadFrameCameraMatrix();

        if ((src->LocalPlayer_Matrix._11 == 0 && src->LocalPlayer_Matrix._22 == 0 && src->LocalPlayer_Matrix._33 == 0 && src->LocalPlayer_Matrix._44 == 0)
            || !std::isfinite(src->LocalPlayer_Matrix._11) || !std::isfinite(src->LocalPlayer_Matrix._22)
            || !std::isfinite(src->LocalPlayer_Matrix._33) || !std::isfinite(src->LocalPlayer_Matrix._44)) {
            static int c4=0; if(c4<3){LOG("Do_Cheat SKIP: LocalPlayer_Matrix is zero/NaN (_11=%.2f _22=%.2f _33=%.2f _44=%.2f)", src->LocalPlayer_Matrix._11, src->LocalPlayer_Matrix._22, src->LocalPlayer_Matrix._33, src->LocalPlayer_Matrix._44);c4++;}
            return;
        }
        Matrix4x4 frameMatrix = src->GetMatrixSnapshot();
        // Validate the snapshot too
        if ((frameMatrix._11 == 0 && frameMatrix._22 == 0 && frameMatrix._33 == 0 && frameMatrix._44 == 0)
            || !std::isfinite(frameMatrix._11) || !std::isfinite(frameMatrix._22)
            || !std::isfinite(frameMatrix._33) || !std::isfinite(frameMatrix._44)) {
            frameMatrix = src->LocalPlayer_Matrix; // fallback to last-known-good
        }
        if (!lpStillStable()) { static int c5=0; if(c5<3){LOG("Do_Cheat SKIP: lpStillStable failed after matrix check");c5++;} return; }
        static bool firstRun = true;
        if (firstRun) { LOG("Do_Cheat RUNNING — first frame (all gates passed)"); firstRun = false; }

        Anybrain::Neutralize();
        static int screenWidth = 0;
        static int screenHeight = 0;
        if (!screenWidth) {
            screenWidth = GetSystemMetrics(SM_CXSCREEN);
            screenHeight = GetSystemMetrics(SM_CYSCREEN);
        }

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

        const bool battleMode = SETTINGS::BattleMode;
        const bool allowPlayers = !battleMode || BATTLE::Players;
        const bool allowWorld = !battleMode || BATTLE::World;
        const bool allowNPC = !battleMode || BATTLE::NPC;
        const bool allowAnimals = !battleMode || BATTLE::Animals;
        const bool allowAimbot = !battleMode || BATTLE::Aimbot;

        if (allowWorld) {
            std::vector<Rust::PrefabData> pList;
            { std::lock_guard<std::mutex> lk(cac->prefab_mutex); pList = prefab_List; }

            for (const auto& prefab : pList)
        {
            Vector2 Screen = WorldToScreen(prefab.Position, frameMatrix);
            if (!std::isfinite(Screen.x) || !std::isfinite(Screen.y)
                || Screen.x <= 1 || Screen.x >= (float)screenWidth - 1
                || Screen.y <= 1 || Screen.y >= (float)screenHeight - 1) {
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
        }
        }

        std::vector<Rust::BaseEntity*> entlist;
        { std::lock_guard<std::mutex> lk(cac->entity_mutex); entlist = entity_List; }

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

        // Copy entire cache ONCE per frame (1 lock instead of 50 per player)
        std::unordered_map<uintptr_t, EspCacheData> cacheCopy;
        {
            std::lock_guard<std::mutex> lk(g_EspCacheMutex);
            cacheCopy.reserve(g_EspCache.size());
            cacheCopy = g_EspCache;
        }

        // AABB occlusion system disabled — ESP uses game visibility flags now
        // (occluder list + camera position copy removed to save CPU)

        // Compute hotbar target once per frame (was O(N) per player inside do_Visuals)
        if (ESP::hotbar_text) {
            uintptr_t bestTgt = 0;
            float best = FLT_MAX;
            float cx = (float)screenWidth * 0.5f;
            float cy = (float)screenHeight * 0.5f;
            for (const auto& [k, v] : cacheCopy) {
                if (v.isDead) continue;
                Vector3 sp = v.spine1Pos;
                if (sp.Empty()) sp = v.headPos;
                if (sp.Empty()) continue;
                Vector2 ps = WorldToScreen(sp, frameMatrix);
                if (!std::isfinite(ps.x) || !std::isfinite(ps.y)
                    || ps.x <= 1 || ps.x >= (float)screenWidth - 1
                    || ps.y <= 1 || ps.y >= (float)screenHeight - 1) continue;
                float dx = ps.x - cx;
                float dy = ps.y - cy;
                float d2 = dx * dx + dy * dy;
                if (d2 < best) { best = d2; bestTgt = k; }
            }
            g_HotbarTarget = bestTgt;
        } else {
            g_HotbarTarget = 0;
        }

        if (allowPlayers) {
        if (!ESP::ESPEnabled) goto skipPlayers;

        for (const auto& Player : entlist)
        {
            if (!Player || !is_valid((uintptr_t)Player)) continue;

            auto cit = cacheCopy.find((uintptr_t)Player);
            if (cit == cacheCopy.end()) continue;
            const EspCacheData& pcache = cit->second;
            if (pcache.isDead) continue;
            if (ESP::RemoveWounded && pcache.isWounded) continue;
            if (ESP::RemoveSleepers && pcache.isSleeping) continue;
            if (ESP::RemoveTeam && pcache.teamId != 0) {
                uint64_t myTeam = 0;
                { std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex); myTeam = g_LocalPlayerTeam; }
                if (myTeam != 0 && pcache.teamId == myTeam) continue;
            }

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
                || Screen.x <= 1 || Screen.x >= (float)screenWidth - 1
                || Screen.y <= 1 || Screen.y >= (float)screenHeight - 1) {
                continue;
            }

            cit->second.isAabbOccluded = false;

            esp->do_Visuals(Player, lpPos, pcache, frameMatrix);
            
        }
        }
        skipPlayers:;

        if (allowNPC && NPC_ESP::Enabled && !MISC::CombatMode) {
            std::vector<Rust::BaseEntity*> npclist;
            { std::lock_guard<std::mutex> lk(cac->npc_mutex); npclist = npc_List; }
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
            std::vector<Rust::BaseEntity*> animallist;
            { std::lock_guard<std::mutex> lk(cac->animal_mutex); animallist = animal_List; }
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
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)screenHeight - 25), ImColor(255, 50, 50, 255), TR("BATTLE MODE"));
        } else if (MISC::CombatMode) {
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)screenHeight - 25), ImColor(255, 50, 50, 255), TR("COMBAT MODE"));
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
                ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)screenHeight - 45), ImColor(100, 220, 100, 200), buf);
            }
        }

        if (MISC::DebugCamera) {
            float remaining = 50.0f - MISC::DebugCameraTimer;
            if (remaining < 0.0f) remaining = 0.0f;
            ImColor timerColor = (remaining <= 10.0f) ? ImColor(255, 50, 50, 255) : ImColor(255, 220, 80, 255);
            char debugCamText[64] = {};
            sprintf_s(debugCamText, "DEBUG CAM EXIT IN: %.0fs", remaining);
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, (float)screenHeight - 45), timerColor, debugCamText);
        }

    }

}
