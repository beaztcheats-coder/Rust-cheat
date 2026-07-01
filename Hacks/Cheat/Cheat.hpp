#include "Hacks/Aimbot/aimbot.hpp"
#include "../../Logger.hpp"
#include "../../Hotkeys.hpp"
#include "../../Occlusion.hpp"
#include <cmath>

namespace Xheat {

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

        // Camera matrix — take one atomic snapshot per frame (eliminates torn reads)
        if ((src->LocalPlayer_Matrix._11 == 0 && src->LocalPlayer_Matrix._22 == 0 && src->LocalPlayer_Matrix._33 == 0 && src->LocalPlayer_Matrix._44 == 0)
            || !std::isfinite(src->LocalPlayer_Matrix._11) || !std::isfinite(src->LocalPlayer_Matrix._22)
            || !std::isfinite(src->LocalPlayer_Matrix._33) || !std::isfinite(src->LocalPlayer_Matrix._44)) {
            static int c4=0; if(c4<3){LOG("Do_Cheat SKIP: LocalPlayer_Matrix is zero/NaN (_11=%.2f _22=%.2f _33=%.2f _44=%.2f)", src->LocalPlayer_Matrix._11, src->LocalPlayer_Matrix._22, src->LocalPlayer_Matrix._33, src->LocalPlayer_Matrix._44);c4++;}
            return;
        }
        // Per-frame snapshot via seqlock — all WorldToScreen calls in this frame use the same matrix
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
