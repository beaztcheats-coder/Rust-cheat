#include "visuals.hpp"
#include "../../Translation.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <cmath>
#include <cfloat>
#include <array>

#include "../Cache/cache.hpp"

extern std::unordered_map<uintptr_t, EspCacheData> g_EspCache;
extern std::shared_mutex g_EspCacheMutex;
extern std::unordered_map<uintptr_t, std::vector<Vector3>> g_Trails;
extern std::mutex g_TrailsMutex;

ImFont* VisualFont = nullptr;
ImFont* MenuFont = nullptr;
ImFont* MenuFont2 = nullptr;
ImFont* MenuFont3 = nullptr;
ImFont* BeaztFont = nullptr;
ImFont* BeaztFontTitle = nullptr;
ImFont* BeaztFontDesc = nullptr;

ImVec2 ToImVec2(const Vector2& vec) {
    return ImVec2(vec.x, vec.y);
}
// g_ScreenW/g_ScreenH removed — use g_ScreenW/g_ScreenH (updated every frame from game window)

static bool ScreenPointValid(const Vector2& p, int w, int h)
{
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) return false;
    if (p.x <= 1.0f || p.y <= 1.0f) return false;
    if (p.x >= (float)w - 1.0f || p.y >= (float)h - 1.0f) return false;
    return true;
}

static float GetEspFontSize()
{
    float scale = ESP::EspFontSize;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;
    float base = VisualFont ? VisualFont->FontSize : ImGui::GetFontSize();
    return base * scale;
}

static ImVec2 CalcEspTextSize(const char* text)
{
    if (!text) return ImVec2(0, 0);
    if (VisualFont) {
        return VisualFont->CalcTextSizeA(GetEspFontSize(), FLT_MAX, 0.0f, text);
    }
    return ImGui::CalcTextSize(text);
}

static void DrawEspText(float x, float y, ImColor color, const char* text)
{
    if (!text || !text[0]) return;
    auto* dl = ImGui::GetBackgroundDrawList();
    float fs = GetEspFontSize();
    if (VisualFont) {
        dl->AddText(VisualFont, fs, ImVec2(x + 1, y + 1), ImColor(0, 0, 0, 200), text);
        dl->AddText(VisualFont, fs, ImVec2(x, y), color, text);
    } else {
        draw_text_outline_font(x, y, color, text);
    }
}

struct SkeletonInterpFrame {
    std::array<Vector3, 15> bones;
    uint64_t tick = 0;
};

static std::unordered_map<uintptr_t, SkeletonInterpFrame> g_SkeletonInterp;
static int g_SkeletonInterpCleanupTick = 0;

void Visuals::do_Visuals(Rust::BaseEntity* Player, Vector3 lpPos, const EspCacheData& cached, const Matrix4x4& frameMatrix) {
    if (!Player || cached.isDead || !src->LocalPlayer) return;

    // g_ScreenW/g_ScreenH are updated every frame in render.hpp from GetClientRect(game_window)

    Vector3 headPos = cached.headPos;
    Vector3 feetPos = cached.feetPos;
    Vector3 velocity = cached.velocity;
    std::string pname = cached.name;
    std::string weaponName = cached.weaponName;
    bool skelValid = cached.skeletonValid;
    Vector3 bones[15] = {};
    uint64_t cacheTick = cached.cacheTick;
    uint64_t skelTick = cached.skeletonTick ? cached.skeletonTick : cacheTick;
    if (skelValid) {
        for (int bi = 0; bi < 15; ++bi) bones[bi] = cached.bones[bi];
    }
    if (headPos.Empty()) return;

    // If head and feet are too close (bone reads failed — both at root level),
    // treat position as feet and compute head with height offset
    float headFeetDiff = fabsf(headPos.y - feetPos.y);
    if (feetPos.Empty() || headFeetDiff < 0.5f) {
        feetPos = headPos;      // root position = feet
        headPos.y += 1.8f;      // approximate head height
    }

    // Velocity-based position prediction (vs old fixed 0.3/0.7 lerp)
    {
        static std::unordered_map<uintptr_t, Vector3>* pPrevHead = nullptr;
        static std::unordered_map<uintptr_t, Vector3>* pPrevFeet = nullptr;
        if (!pPrevHead) pPrevHead = new std::unordered_map<uintptr_t, Vector3>();
        if (!pPrevFeet) pPrevFeet = new std::unordered_map<uintptr_t, Vector3>();
        auto& prevHead = *pPrevHead;
        auto& prevFeet = *pPrevFeet;
        static int cleanupTick = 0;

        uint64_t now = GetTickCount64();
        float predDt = (float)(now - cacheTick) / 1000.0f;
        if (predDt > 0.0f && predDt < 0.5f && !velocity.Empty() && velocity.x == velocity.x) {
            // Predict forward using cached velocity
            headPos.x += velocity.x * predDt;
            headPos.y += velocity.y * predDt;
            headPos.z += velocity.z * predDt;
            feetPos.x += velocity.x * predDt;
            feetPos.y += velocity.y * predDt;
            feetPos.z += velocity.z * predDt;
            // Gravity correction for airborne players
            if (!cached.isGrounded) {
                float g = 0.5f * 9.81f * predDt * predDt;
                headPos.y -= g;
                feetPos.y -= g;
            }
        } else {
            // Fallback: blend with previous frame
            auto ph = prevHead.find((uintptr_t)Player);
            if (ph != prevHead.end() && !ph->second.Empty()) {
                headPos = ph->second * 0.3f + headPos * 0.7f;
            }
            auto pf = prevFeet.find((uintptr_t)Player);
            if (pf != prevFeet.end() && !pf->second.Empty()) {
                feetPos = pf->second * 0.3f + feetPos * 0.7f;
            }
        }

        prevHead[(uintptr_t)Player] = headPos;
        prevFeet[(uintptr_t)Player] = feetPos;
        if (++cleanupTick >= 300) {
            cleanupTick = 0;
            if (prevHead.size() > 512) prevHead.clear();
            if (prevFeet.size() > 512) prevFeet.clear();
        }
    }

    // Bone position correction — shift skeleton to match current predicted position
    // Bones are cached (50-200ms old), but headPos is fresh. Shift bones by the offset.
    if (skelValid && !cached.boneAnchorPos.Empty()) {
        Vector3 boneOffset = headPos - cached.boneAnchorPos;
        for (int bi = 0; bi < 15; ++bi) {
            if (!bones[bi].Empty())
                bones[bi] = bones[bi] + boneOffset;
        }
    }

    ImGui::PushFont(VisualFont);

    Vector2 headScreen = WorldToScreen(headPos, frameMatrix);
    Vector2 feetScreen = WorldToScreen(feetPos, frameMatrix);
    if (!ScreenPointValid(headScreen, g_ScreenW, g_ScreenH) || !ScreenPointValid(feetScreen, g_ScreenW, g_ScreenH)) {
        ImGui::PopFont();
        return;
    }
    Vector2 Feet = feetScreen;

    float h = fabsf(feetScreen.y - headScreen.y);
    float w = h * 0.5f;
    float boxX = headScreen.x - w * 0.5f;
    float boxY = headScreen.y;
    float width = w;
    float height = h;

    float     padding = 5.0f;
    boxX -= padding; width += padding * 2.f;
    boxY -= padding; height += padding * 2.f;

    float Distance = lpPos.DistTo(headPos);

    ImColor boxCol = ESP::color::Box;
    ImColor fillCol = ESP::color::FilledBox;
    ImColor headCol = ESP::color::HeadCircle;
    ImColor skelCol = ESP::color::Skeleton;
    ImColor nameCol = ESP::color::Name;

    // Teammate color override
    bool isTeammate = false;
    if (cached.teamId != 0) {
        uint64_t myTeam = 0;
        { std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex); myTeam = g_LocalPlayerTeam; }
        if (myTeam != 0 && cached.teamId == myTeam) {
            isTeammate = true;
            boxCol = ESP::color::Teammate;
            fillCol = ESP::color::Teammate;
            headCol = ESP::color::Teammate;
            skelCol = ESP::color::Teammate;
            nameCol = ESP::color::Teammate;
        }
    }

    if (cached.isWounded && ESP::HighlightWounded) {
        boxCol = ESP::color::Wounded;
        fillCol = ESP::color::Wounded;
        headCol = ESP::color::Wounded;
        skelCol = ESP::color::Wounded;
    }
    if (cached.isSleeping && ESP::HighlightSleepers) {
        boxCol = ESP::color::Sleepers;
        fillCol = ESP::color::Sleepers;
        headCol = ESP::color::Sleepers;
        skelCol = ESP::color::Sleepers;
    }
    if (ESP::VisCheck) {
        bool vis = cached.isVisible;
        ImColor visCol = vis ? ESP::color::Visible : ESP::color::Invisible;
        boxCol = visCol;
        fillCol = visCol;
        headCol = visCol;
        skelCol = vis ? ESP::color::SkeletonVisible : ESP::color::SkeletonInvisible;
    }

    bool adv = ESP::ESPAdvanced;
    bool drawBoxDist = !adv || Distance <= ESP::BoxDist;
    bool drawSkelDist = !adv || Distance <= ESP::SkeletonDist;
    bool drawNameDist = !adv || Distance <= ESP::NameDist;
    bool drawWeaponDist = !adv || Distance <= ESP::WeaponDist;
    bool drawHealthDist = false; // health is server-side only in Rust
    bool drawSnapDist = !adv || Distance <= ESP::SnaplineDist;
    bool drawHeadDist = !adv || Distance <= ESP::HeadCircleDist;
    bool drawOffArrowDist = !adv || Distance <= ESP::OFFArrowDist;

    if (ESP::FilledBox && drawBoxDist)
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(boxX, boxY),
            ImVec2(boxX + width, boxY + height),
            IM_COL32((int)(fillCol.Value.x * 255), (int)(fillCol.Value.y * 255), (int)(fillCol.Value.z * 255), (int)(fillCol.Value.w * 255)));
    if (ESP::CornerBox && drawBoxDist) {
        float cl = 2.0f;
        auto* dl = ImGui::GetBackgroundDrawList();
        ImU32 c = IM_COL32((int)(boxCol.Value.x * 255), (int)(boxCol.Value.y * 255), (int)(boxCol.Value.z * 255), 255);
        dl->AddLine(ImVec2(boxX, boxY), ImVec2(boxX + width * 0.25f, boxY), c);
        dl->AddLine(ImVec2(boxX, boxY), ImVec2(boxX, boxY + height * 0.25f), c);
        dl->AddLine(ImVec2(boxX + width, boxY), ImVec2(boxX + width * 0.75f, boxY), c);
        dl->AddLine(ImVec2(boxX + width, boxY), ImVec2(boxX + width, boxY + height * 0.25f), c);
        dl->AddLine(ImVec2(boxX, boxY + height), ImVec2(boxX + width * 0.25f, boxY + height), c);
        dl->AddLine(ImVec2(boxX, boxY + height), ImVec2(boxX, boxY + height * 0.75f), c);
        dl->AddLine(ImVec2(boxX + width, boxY + height), ImVec2(boxX + width * 0.75f, boxY + height), c);
        dl->AddLine(ImVec2(boxX + width, boxY + height), ImVec2(boxX + width, boxY + height * 0.75f), c);
    }
    if (ESP::Box && drawBoxDist) {
        float bt = (float)ESP::BoxThickness;
        if (ESP::BoxOutline) {
            ImGui::GetBackgroundDrawList()->AddRect(
                ImVec2(boxX - 1.f, boxY - 1.f),
                ImVec2(boxX + width + 1.f, boxY + height + 1.f),
                IM_COL32(0, 0, 0, 180), 0.f, 0, bt + 2.f);
        }
        ImGui::GetBackgroundDrawList()->AddRect(
            ImVec2(boxX, boxY),
            ImVec2(boxX + width, boxY + height),
            IM_COL32((int)(boxCol.Value.x * 255), (int)(boxCol.Value.y * 255), (int)(boxCol.Value.z * 255), 255), 0.f, 0, bt);
    }

    if (ESP::HealthBar && drawHealthDist) {
        // DEPRECATED: health is server-side only in Rust, not readable from client memory
    }
    if (ESP::HeadCircle && drawHeadDist) {
        if (ScreenPointValid(headScreen, g_ScreenW, g_ScreenH))
            ImGui::GetBackgroundDrawList()->AddCircle(
                ImVec2(headScreen.x, headScreen.y),
                5.f + (200.f / (Distance + 1.f)),
                IM_COL32((int)(headCol.Value.x * 255), (int)(headCol.Value.y * 255), (int)(headCol.Value.z * 255), 255),
                24,
                1.5f);
    }
    float topY = boxY - 4.0f;
    float bottomY = boxY + height + 4.0f;
    auto DrawTopLine = [&](const std::string& txt, ImColor col) {
        if (txt.empty()) return;
        ImVec2 sz = CalcEspTextSize(txt.c_str());
        topY -= (sz.y + 2.0f);
        DrawEspText(headScreen.x - sz.x * 0.5f, topY, col, txt.c_str());
    };
    auto DrawBottomLine = [&](const std::string& txt, ImColor col) {
        if (txt.empty()) return;
        ImVec2 sz = CalcEspTextSize(txt.c_str());
        DrawEspText(headScreen.x - sz.x * 0.5f, bottomY, col, txt.c_str());
        bottomY += (sz.y + 2.0f);
    };

    if (ESP::Name && drawNameDist) {
        std::string displayname = pname.empty() ? TR("Player") : pname;
        DrawTopLine(displayname, nameCol);
    }
    if (ESP::TeamID) {
        uint64_t tid = cached.teamId;
        if (tid != 0) DrawTopLine("[T:" + std::to_string(tid) + "]", ESP::color::Team);
    }
    if (ESP::Distance && drawNameDist) {
        const char* brackets[] = {"(","[","{",""};
        const char* bClose[] = {")","]","}",""};
        int s = ESP::DistanceBracketStyle;
        DrawBottomLine(std::string(brackets[s]) + std::to_string((int)Distance) + "M" + bClose[s], ESP::color::Distance);
    }
    if (ESP::ShowVisibility && drawNameDist) {
        if (cached.isVisible)
            DrawBottomLine("VISIBLE", IM_COL32(80, 255, 80, 255));
        else
            DrawBottomLine("HIDDEN", IM_COL32(255, 80, 80, 255));
    }
    if (ESP::Weapon && drawWeaponDist && !weaponName.empty()) {
        DrawBottomLine(weaponName, ESP::color::Weapon);
    }
    if (ESP::PlayerFlags && cached.playerFlags[0] != 0) {
        DrawBottomLine(cached.playerFlags, ImColor(255, 100, 100, 255));
    }

    // Use cached inventory from slow thread (zero IOCTLs in render loop)
    struct CachedInv { char belt[6][64] = {}; char wear[5][64] = {}; int ammo = -1; };
    CachedInv ex;
    for (int i = 0; i < 6; i++) cacheSetStr(ex.belt[i], cached.belt[i]);
    for (int i = 0; i < 5; i++) cacheSetStr(ex.wear[i], cached.wear[i]);
    ex.ammo = cached.ammo;
    bool hasBelt = false;
    for (int i = 0; i < 6; i++) if (!cacheStrEmpty(ex.belt[i])) { hasBelt = true; break; }

    if (ESP::hotbar_text && hasBelt) {
        if (ESP::HotbarIcons) {
            // Draw item icons horizontally
            float iconSize = 24.0f;
            float iconSpacing = 2.0f;
            float totalW = 0;
            int iconCount = 0;
            for (int i = 0; i < 6; ++i) if (!cacheStrEmpty(ex.belt[i])) iconCount++;
            if (iconCount > 0) {
                totalW = iconCount * (iconSize + iconSpacing) - iconSpacing;
                float startX = headScreen.x - totalW * 0.5f;
                auto* dl = ImGui::GetBackgroundDrawList();
                for (int i = 0; i < 6; ++i) {
                    if (cacheStrEmpty(ex.belt[i])) continue;
                    ID3D11ShaderResourceView* icon = g_ItemIcons.Get(std::string(ex.belt[i]));
                    if (icon) {
                        ImVec2 pos1(startX, bottomY);
                        ImVec2 pos2(startX + iconSize, bottomY + iconSize);
                        dl->AddImage((ImTextureID)icon, pos1, pos2);
                    } else {
                        // Fallback to text
                        ImVec2 sz = CalcEspTextSize(ex.belt[i]);
                        DrawEspText(startX, bottomY, ESP::color::Weapon, ex.belt[i]);
                    }
                    startX += iconSize + iconSpacing;
                }
                bottomY += iconSize + 2.0f;
            }
        } else {
            for (int i = 0; i < 6; ++i) {
                if (!cacheStrEmpty(ex.belt[i])) DrawBottomLine(std::string(ex.belt[i]), ESP::color::Weapon);
            }
        }
    }
    bool hasWear = false;
    for (int i = 0; i < 5; i++) if (!cacheStrEmpty(ex.wear[i])) { hasWear = true; break; }
    if (ESP::Clothing && hasWear) {
        DrawBottomLine(TR("-- Wear --"), ESP::color::Distance);
        for (int i = 0; i < 5; ++i) if (!cacheStrEmpty(ex.wear[i])) DrawBottomLine(std::string(ex.wear[i]), ESP::color::Name);
    }
    if (ESP::ItemList && hasBelt) {
        DrawBottomLine(TR("-- Belt --"), ESP::color::Distance);
        for (int i = 0; i < 6; ++i) if (!cacheStrEmpty(ex.belt[i])) DrawBottomLine(std::string(ex.belt[i]), ESP::color::Weapon);
    }
    if (ESP::AmmoBar && ex.ammo > 0) {
        DrawBottomLine(TR("Ammo: ") + std::to_string(ex.ammo), ESP::color::Distance);
    }
    if (ESP::OFFArrows && drawOffArrowDist) {
        // Use global local player data (zero IOCTLs in render loop)
        Vector3 va, localNeck;
        {
            std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
            va = g_LocalBodyAngles;
            localNeck = g_LocalNeckPos;
        }
        if (!va.Empty() && !localNeck.Empty()) {
            float ofw = 5.f;
            float angle = CalcAngle(localNeck, headPos).y - va.y - 90;
            arc(g_ScreenW / 2, g_ScreenH / 2, 300.f, angle - ofw, angle + ofw, ESP::color::OFFArrowColor, 4.f);
        }
    }
    if (ESP::SnapLines && drawSnapDist) {
        float ox = (float)(g_ScreenW/2), oy = (float)g_ScreenH;
        if (ESP::SnaplineMode == 3) oy = (float)(g_ScreenH * 0.1f);
        else if (ESP::SnaplineMode == 1) oy = (float)(g_ScreenH * 0.9f);
        float st = (float)ESP::SnaplineThickness;
        if (ESP::SnaplineOutline)
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(ox, oy), ImVec2(Feet.x, Feet.y), IM_COL32(0,0,0,180), st + 2.f);
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(ox, oy), ImVec2(Feet.x, Feet.y), ESP::color::Snaplines, st);
    }
    if (ESP::Skeleton && drawSkelDist && skelValid) {
        // Velocity prediction is already applied via boneOffset (headPos_predicted - boneAnchorPos)
        // — no additional per-bone prediction needed here

        // Bone interpolation now happens on the skeleton thread (12ms cycle)
        // — no per-bone lerp needed on the render thread anymore

        Vector2 scr[15]; bool ok[15] = {}; int cnt = 0;
        for (int i = 0; i < 15; i++) {
            if (bones[i].Empty()) continue;
            Vector2 sp = WorldToScreen(bones[i], frameMatrix);
            if (!ScreenPointValid(sp, g_ScreenW, g_ScreenH)) continue;
            scr[i] = sp; ok[i] = true; cnt++;
        }
            static bool skelRenderDiag = false;
            if (!skelRenderDiag && cnt >= 4) {
                LOG("VISUALS: skeleton rendering active (cnt=%d bones)", cnt);
                skelRenderDiag = true;
            }
            if (cnt >= 4) {
                auto* dl = ImGui::GetBackgroundDrawList();
                float skThick = (float)ESP::SkeletonThickness;
                ImU32 sc = IM_COL32((int)(skelCol.Value.x * 255), (int)(skelCol.Value.y * 255), (int)(skelCol.Value.z * 255), 180);

                // Batched skeleton rendering via AddPolyline chains (4 chains vs 13 AddLine calls per pass)
                // Bone indices: 0=head,1=neck,2=spine1,3=hips,4=r_knee,5=r_foot,6=l_hip,7=l_knee,8=l_foot,
                //               9=r_upperarm,10=r_forearm,11=r_hand,12=l_upperarm,13=l_forearm,14=l_hand
                static const int chain1[] = {0,1,2,3,4,5};  // head→neck→spine→hip→knee→foot (right leg)
                static const int chain2[] = {2,6,7,8};      // spine→l_hip→l_knee→l_foot (left leg)
                static const int chain3[] = {1,9,10,11};     // neck→r_upperarm→r_forearm→r_hand (right arm)
                static const int chain4[] = {1,12,13,14};    // neck→l_upperarm→l_forearm→l_hand (left arm)

                auto drawChains = [&](ImU32 color, float thickness) {
                    auto drawChain = [&](const int* indices, int count) {
                        ImVec2 pts[6];
                        int n = 0;
                        for (int i = 0; i < count; i++) {
                            if (ok[indices[i]]) {
                                pts[n++] = ImVec2(scr[indices[i]].x, scr[indices[i]].y);
                            } else {
                                if (n >= 2) dl->AddPolyline(pts, n, color, false, thickness);
                                n = 0;
                            }
                        }
                        if (n >= 2) dl->AddPolyline(pts, n, color, false, thickness);
                    };
                    drawChain(chain1, 6);
                    drawChain(chain2, 4);
                    drawChain(chain3, 4);
                    drawChain(chain4, 4);
                };

                // Skeleton outline pass
                if (ESP::SkeletonOutline) {
                    drawChains(IM_COL32(0, 0, 0, 160), skThick + 2.0f);
                }
                // Color pass
                drawChains(sc, skThick);
        }
    }
    // Movement trails
    if (ESP::MovementTrails) {
        decltype(g_Trails)::mapped_type trails;
        {
            std::lock_guard<std::mutex> lk(g_TrailsMutex);
            auto it = g_Trails.find((uintptr_t)Player);
            if (it != g_Trails.end()) trails = it->second;
        }
        if (trails.size() >= 2) {
            auto* dl = ImGui::GetBackgroundDrawList();
            for (size_t i = 1; i < trails.size(); i++) {
                Vector2 a = WorldToScreen(trails[i-1], frameMatrix);
                Vector2 b = WorldToScreen(trails[i], frameMatrix);
                if (ScreenPointValid(a, g_ScreenW, g_ScreenH) && ScreenPointValid(b, g_ScreenW, g_ScreenH)) {
                    float alpha = (float)i / (float)trails.size() * 0.5f;
                    ImU32 c = IM_COL32((int)(skelCol.Value.x*255),(int)(skelCol.Value.y*255),(int)(skelCol.Value.z*255),(int)(alpha*255));
                    dl->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), c, 1.5f);
                }
            }
        }
    }

    ImGui::PopFont();
}

void Visuals::do_NPC_Visuals(Rust::BaseEntity* Entity, Vector3 lpPos, const EspCacheData& cached, const Matrix4x4& frameMatrix) {
    if (!Entity || cached.isDead || !src->LocalPlayer) return;
    // Use feetPos (root) as base — headPos is root+1.8 from position thread
    Vector3 feetPos3D = cached.feetPos;
    if (feetPos3D.Empty()) {
        // Fallback: if feetPos invalid, use headPos and subtract offset
        feetPos3D = cached.headPos;
        if (!feetPos3D.Empty()) feetPos3D.y -= 1.8f;
    }
    if (feetPos3D.Empty()) {
        auto headTf = Entity->Get_Transformation(BoneList::head);
        if (headTf) feetPos3D = headTf->Get_Position();
    }
    if (feetPos3D.Empty()) return;

    // Apply velocity prediction to NPC position (same as Do_Cheat does for players)
    if (!cached.velocity.Empty() && cached.velocity.x == cached.velocity.x && cached.cacheTick > 0) {
        float predDt = (float)(GetTickCount64() - cached.cacheTick) * 0.001f;
        if (predDt > 0.0f && predDt < 0.5f) {
            feetPos3D.x += cached.velocity.x * predDt;
            feetPos3D.y += cached.velocity.y * predDt;
            feetPos3D.z += cached.velocity.z * predDt;
        }
    }

    Vector3 headPos3D = feetPos3D;
    headPos3D.y += 1.8f; // approximate head height above root

    Vector2 headScreen = WorldToScreen(headPos3D, frameMatrix);
    if (!ScreenPointValid(headScreen, g_ScreenW, g_ScreenH)) return;
    Vector2 feetScreen = WorldToScreen(feetPos3D, frameMatrix);
    if (!ScreenPointValid(feetScreen, g_ScreenW, g_ScreenH)) {
        feetScreen = headScreen;
        feetScreen.y += 95.f;
    }

    float h = feetScreen.y - headScreen.y;
    float w = h * 0.5f;
    float x = headScreen.x - w * 0.5f;
    float y = headScreen.y;
    float Distance = lpPos.DistTo(feetPos3D);
    ImColor col = NPC_ESP::color::Box;

    bool adv = NPC_ESP::ESPAdvanced;
    bool drawBoxDist = !adv || Distance <= NPC_ESP::NPCBoxDist;
    bool drawNameDist = !adv || Distance <= NPC_ESP::NPCNameDist;
    bool drawDistDist = !adv || Distance <= NPC_ESP::NPCDistTextDist;
    bool drawHealthDist = false; // health is server-side only in Rust
    bool drawSkeletonDist = !adv || Distance <= NPC_ESP::NPCSkeletonDist;
    bool drawHeldDist = !adv || Distance <= NPC_ESP::NPCHeldItemDist;
    bool drawSnapDist = !adv || Distance <= NPC_ESP::NPCSnaplineDist;

    if (NPC_ESP::Box && drawBoxDist)
        ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), col);
    if (NPC_ESP::Name && drawNameDist) {
        std::string name = cached.name;
        if (name.empty()) name = TR("NPC");
        ImVec2 sz = CalcEspTextSize(name.c_str());
        DrawEspText(headScreen.x - sz.x * 0.5f, y - sz.y - 3, NPC_ESP::color::Name, name.c_str());
    }
    if (NPC_ESP::Distance && drawDistDist) {
        const char* bk[] = {"(","[","{",""};
        const char* bc[] = {")","]","}",""};
        std::string dt = std::string(bk[ESP::DistanceBracketStyle]) + std::to_string((int)Distance) + "M" + bc[ESP::DistanceBracketStyle];
        ImVec2 sz = CalcEspTextSize(dt.c_str());
        DrawEspText(headScreen.x - sz.x * 0.5f, y + h + 3, NPC_ESP::color::Distance, dt.c_str());
    }
    if (NPC_ESP::HealthBar && drawHealthDist) {
        // DEPRECATED: health is server-side only in Rust, not readable from client memory
    }
    if (NPC_ESP::Skeleton && drawSkeletonDist) {
        bool skelValid = cached.skeletonValid;
        Vector3 bones[15] = {};
        if (skelValid) {
            for (int bi = 0; bi < 15; ++bi) bones[bi] = cached.bones[bi];
        }
        // Apply velocity prediction to NPC bones (same as player path)
        if (skelValid) {
            const Vector3& velocity = cached.velocity;
            uint64_t skelTick = cached.skeletonTick;
            if (!velocity.Empty() && velocity.x == velocity.x && skelTick > 0) {
                float dt = (float)(GetTickCount64() - skelTick) / 1000.0f;
                if (dt > 0.0f && dt < 0.5f) {
                    float g = 0.5f * 9.81f * dt * dt;
                    for (int i = 0; i < 15; i++) {
                        if (bones[i].Empty()) continue;
                        bones[i].x += velocity.x * dt;
                        bones[i].y += velocity.y * dt;
                        bones[i].z += velocity.z * dt;
                        bones[i].y -= g;
                    }
                }
            }
        }
        if (skelValid) {
            Vector2 scr[15]; bool ok[15] = {}; int cnt = 0;
            for (int i = 0; i < 15; i++) {
                if (bones[i].Empty()) continue;
                Vector2 sp = WorldToScreen(bones[i], frameMatrix);
                if (!ScreenPointValid(sp, g_ScreenW, g_ScreenH)) continue;
                scr[i] = sp; ok[i] = true; cnt++;
            }
            if (cnt >= 4) {
                auto* dl = ImGui::GetBackgroundDrawList();
                float skThick = (float)ESP::SkeletonThickness;
                ImU32 sc = IM_COL32((int)(NPC_ESP::color::Skeleton.Value.x * 255), (int)(NPC_ESP::color::Skeleton.Value.y * 255), (int)(NPC_ESP::color::Skeleton.Value.z * 255), 180);
                // Batched skeleton via polyline chains
                static const int c1[] = {0,1,2,3,4,5};
                static const int c2[] = {2,6,7,8};
                static const int c3[] = {1,9,10,11};
                static const int c4[] = {1,12,13,14};
                auto drawChain = [&](const int* idx, int n) {
                    ImVec2 pts[6]; int m = 0;
                    for (int i = 0; i < n; i++) {
                        if (ok[idx[i]]) pts[m++] = ImVec2(scr[idx[i]].x, scr[idx[i]].y);
                        else { if (m >= 2) dl->AddPolyline(pts, m, sc, false, skThick); m = 0; }
                    }
                    if (m >= 2) dl->AddPolyline(pts, m, sc, false, skThick);
                };
                drawChain(c1, 6); drawChain(c2, 4); drawChain(c3, 4); drawChain(c4, 4);
            }
        } else {
            // Fallback: simple head-to-neck line
            auto neckTf = Entity->Get_Transformation(BoneList::neck);
            Vector3 neck = neckTf ? neckTf->Get_Position() : Vector3();
            if (!neck.Empty() && !headPos3D.Empty()) {
                Vector2 n = WorldToScreen(neck, frameMatrix);
                Vector2 hn = WorldToScreen(headPos3D, frameMatrix);
                if (ScreenPointValid(n, g_ScreenW, g_ScreenH) && ScreenPointValid(hn, g_ScreenW, g_ScreenH)) {
                    auto* d = ImGui::GetBackgroundDrawList();
                    d->AddLine(ImVec2(hn.x, hn.y), ImVec2(n.x, n.y), NPC_ESP::color::Skeleton);
                }
            }
        }
    }
    if (NPC_ESP::SnapLines && drawSnapDist) {
        float ox = (float)g_ScreenW * 0.5f, oy = (float)g_ScreenH * 0.5f;
        if (NPC_ESP::SnaplineMode == 3) oy = (float)g_ScreenH * 0.1f;
        else if (NPC_ESP::SnaplineMode == 1) oy = (float)g_ScreenH * 0.9f;
        ImGui::GetBackgroundDrawList()->AddLine(ImVec2(ox, oy), ImVec2(feetScreen.x, feetScreen.y), NPC_ESP::color::Snaplines);
    }
    if (NPC_ESP::HeldItem && drawHeldDist) {
        std::string itemName = cached.weaponName;
        if (!itemName.empty()) {
            ImVec2 sz = CalcEspTextSize(itemName.c_str());
            DrawEspText(feetScreen.x - sz.x * 0.5f, y + h + 18, NPC_ESP::color::Name, itemName.c_str());
        }
    }
}

void Visuals::do_Animal_Visuals(Rust::BaseEntity* Entity, Vector3 lpPos, const EspCacheData& cached, const Matrix4x4& frameMatrix) {
    if (!Entity || cached.isDead || !src->LocalPlayer) return;
    // g_ScreenW/g_ScreenH updated every frame in render.hpp
    Vector3 pos = cached.headPos;
    if (pos.Empty()) return;

    // Apply velocity prediction to animal position
    if (!cached.velocity.Empty() && cached.velocity.x == cached.velocity.x && cached.cacheTick > 0) {
        float predDt = (float)(GetTickCount64() - cached.cacheTick) * 0.001f;
        if (predDt > 0.0f && predDt < 0.5f) {
            pos.x += cached.velocity.x * predDt;
            pos.y += cached.velocity.y * predDt;
            pos.z += cached.velocity.z * predDt;
        }
    }

    float Distance = lpPos.DistTo(pos);

    // Project head (top) and feet (ground) to get a properly-sized box over the animal
    // Animal root pos from Get_ObjectPosition is at ground/feet level
    Vector3 headPos3D = pos;
    headPos3D.y += 1.5f;  // approximate animal height
    Vector3 feetPos3D = pos;
    Vector2 headScreen = WorldToScreen(headPos3D, frameMatrix);
    Vector2 feetScreen = WorldToScreen(feetPos3D, frameMatrix);
    if (!ScreenPointValid(headScreen, g_ScreenW, g_ScreenH)) return;
    if (!ScreenPointValid(feetScreen, g_ScreenW, g_ScreenH)) {
        feetScreen = headScreen;
        feetScreen.y += 60.f;
    }

    float h = fabsf(feetScreen.y - headScreen.y);
    if (h < 8.f) h = 8.f;
    float w = h * 0.6f;
    float boxX = headScreen.x - w * 0.5f;
    float boxY = headScreen.y;
    Vector2 screen = headScreen;
    ImColor col = ANIMAL_ESP::color::Box;

    if (ANIMAL_ESP::Box) {
        if (!ANIMAL_ESP::ESPAdvanced || Distance <= ANIMAL_ESP::AnimalBoxDist) {
            ImGui::GetBackgroundDrawList()->AddRect(
                ImVec2(boxX, boxY), ImVec2(boxX + w, boxY + h), col);
        }
    }
    if (ANIMAL_ESP::Name) {
        if (!ANIMAL_ESP::ESPAdvanced || Distance <= ANIMAL_ESP::AnimalNameDist) {
            std::string name = cached.animalType;
            if (name.empty()) name = TR("Animal");
            ImVec2 ts = CalcEspTextSize(name.c_str());
            DrawEspText(screen.x - ts.x * 0.5f, boxY - ts.y - 3, ANIMAL_ESP::color::Name, name.c_str());
        }
    }
    if (ANIMAL_ESP::Distance) {
        if (!ANIMAL_ESP::ESPAdvanced || Distance <= ANIMAL_ESP::AnimalDistTextDist) {
            const char* bk[] = {"(","[","{",""};
            const char* bc[] = {")","]","}",""};
            std::string dt = std::string(bk[ESP::DistanceBracketStyle]) + std::to_string((int)Distance) + "M" + bc[ESP::DistanceBracketStyle];
            ImVec2 ts = CalcEspTextSize(dt.c_str());
            DrawEspText(screen.x - ts.x * 0.5f, boxY + h + 3, ANIMAL_ESP::color::Distance, dt.c_str());
        }
    }
    if (ANIMAL_ESP::HealthBar) {
        if (!ANIMAL_ESP::ESPAdvanced || Distance <= ANIMAL_ESP::AnimalHealthBarDist) {
            float hp = cached.health;
            float maxHp = cached.maxHealth;
            if (maxHp > 0.f) {
                float ratio = hp / maxHp;
                if (ratio > 1.f) ratio = 1.f; if (ratio < 0.f) ratio = 0.f;
                float barW = w;
                ImColor hpCol = ImColor((int)(255 * (1.f - ratio)), (int)(255 * ratio), 0, 255);
                ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(boxX, boxY + h + 16), ImVec2(boxX + barW * ratio, boxY + h + 19), hpCol);
                ImGui::GetBackgroundDrawList()->AddRect(ImVec2(boxX, boxY + h + 16), ImVec2(boxX + barW, boxY + h + 19), ImColor(0, 0, 0, 200));
            }
        }
    }
    if (ANIMAL_ESP::SnapLines) {
        if (!ANIMAL_ESP::ESPAdvanced || Distance <= ANIMAL_ESP::AnimalSnaplineDist) {
            float ox = (float)g_ScreenW * 0.5f, oy = (float)g_ScreenH * 0.5f;
            if (ANIMAL_ESP::SnaplineMode == 3) oy = (float)g_ScreenH * 0.1f;
            else if (ANIMAL_ESP::SnaplineMode == 1) oy = (float)g_ScreenH * 0.9f;
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(ox, oy),
                ImVec2(screen.x, screen.y), ANIMAL_ESP::color::Snaplines);
        }
    }
}

void Visuals::do_Ghost_Visuals() {
    Matrix4x4 frameMatrix = src->GetMatrixSnapshot();
    if (!ESP::VisGhost) return;
    if (!src || !src->LocalPlayer) return;

    // g_ScreenW/g_ScreenH are global, updated every frame in render.hpp
    Vector3 lpPos;
    {
        std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
        lpPos = g_LocalPlayerPos;
    }
    if (lpPos.Empty()) return;
    ImColor ghostColor = ESP::color::Ghost;

    std::lock_guard<std::mutex> lk(g_GhostCacheMutex);
    uint64_t now = GetTickCount64();
    int timeoutMs = ESP::VisGhostMs;
    if (timeoutMs < 1000) timeoutMs = 1000;

    for (const auto& [key, gd] : g_GhostCache) {
        if (now - gd.vanishTime > (uint64_t)timeoutMs) continue;
        Vector3 pos = gd.lastPos;
        if (pos.Empty()) continue;

        float elapsed = (float)(now - gd.vanishTime) / (float)timeoutMs;
        int alpha = (int)(180.0f * (1.0f - elapsed));
        if (alpha < 20) alpha = 20;

        auto screen = WorldToScreen(gd.lastPos, frameMatrix);
        if (screen.Empty()) continue;

        float dist = lpPos.DistTo(gd.lastPos);
        if (dist > ESP::draw_distance) continue;

        ImColor col = ImColor((int)ghostColor.Value.x, (int)ghostColor.Value.y, (int)ghostColor.Value.z, alpha * 255 / 180);

        // Ghost box
        Vector3 headPos = gd.lastPos; headPos.y += 1.8f;
        auto headScreen = WorldToScreen(headPos, frameMatrix);
        auto feetScreen = WorldToScreen(gd.lastPos, frameMatrix);
        if (!ScreenPointValid(feetScreen, g_ScreenW, g_ScreenH) || !ScreenPointValid(headScreen, g_ScreenW, g_ScreenH)) continue;

        float h = fabsf(feetScreen.y - headScreen.y);
        float w = h * 0.45f;
        float bx = headScreen.x - w * 0.5f;
        float by = headScreen.y;

        ImGui::GetBackgroundDrawList()->AddRect(ImVec2(bx, by), ImVec2(bx + w, by + h), col, 0.f, 0, 1.0f);
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(bx, by), ImVec2(bx + w, by + h), ImColor((int)col.Value.x, (int)col.Value.y, (int)col.Value.z, 8));
        if (ESP::Name && !gd.name.empty()) {
            ImVec2 sz = CalcEspTextSize((gd.name + TR(" (ghost)")).c_str());
            DrawEspText(headScreen.x - sz.x * 0.5f, by + h + 2.0f, col, (gd.name + TR(" (ghost)")).c_str());
        }
    }
}
