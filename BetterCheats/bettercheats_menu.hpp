#pragma once
#include "../imgui/imgui.h"
#include "../offsets.hpp"
#include "../Config.hpp"
#include "../tracers.hpp"
#include "../Render/icons.cpp"
#include "../Hotkeys.hpp"
#include "../Translation.hpp"
#include "../RuntimePaths.hpp"
#include <string>

#ifdef BETTERCHEATS

namespace BetterCheatsMenu {

static ID3D11ShaderResourceView* g_BetterLogo = nullptr;
static ImVec2 g_BetterLogoSize = ImVec2(245.f, 72.f);

inline void LoadBetterLogo(ID3D11Device* dev) {
    if (g_BetterLogo) return;
    #include "bettercheats_logo.h"
    if (bettercheats_logo_png_len > 0 && dev) {
        D3DX11_IMAGE_LOAD_INFO info;
        ZeroMemory(&info, sizeof(info));
        info.Width = D3DX11_DEFAULT;
        info.Height = D3DX11_DEFAULT;
        info.MipLevels = D3DX11_DEFAULT;
        info.Format = DXGI_FORMAT_FROM_FILE;
        info.Usage = D3D11_USAGE_DEFAULT;
        info.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        info.CpuAccessFlags = 0;
        info.MiscFlags = 0;
        ID3DX11ThreadPump* pump = nullptr;
        HRESULT hr = D3DX11CreateShaderResourceViewFromMemory(
            dev, bettercheats_logo_png, bettercheats_logo_png_len,
            &info, pump, &g_BetterLogo, nullptr);
        if (FAILED(hr)) g_BetterLogo = nullptr;
    }
}

static int main_tab = 0;

namespace BetterUI
{
    static const ImVec4 Yellow  = ImVec4(0.95f, 0.78f, 0.05f, 1.00f);
    static const ImVec4 Yellow2 = ImVec4(0.55f, 0.45f, 0.03f, 1.00f);
    static const ImVec4 Blue    = ImVec4(0.15f, 0.35f, 0.85f, 1.00f);
    static const ImVec4 Blue2   = ImVec4(0.08f, 0.15f, 0.35f, 1.00f);
    static const ImVec4 DkBlue  = ImVec4(0.02f, 0.03f, 0.08f, 1.00f);
    static const ImVec4 White   = ImVec4(0.92f, 0.94f, 0.98f, 1.00f);
    static const ImVec4 Muted   = ImVec4(0.45f, 0.50f, 0.60f, 1.00f);

    static ImU32 U32(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }

    static void ApplyStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(12.f, 12.f);
        style.FramePadding = ImVec2(10.f, 6.f);
        style.ItemSpacing = ImVec2(10.f, 8.f);
        style.ItemInnerSpacing = ImVec2(8.f, 5.f);
        style.ScrollbarSize = 12.f;
        style.WindowRounding = 0.f;
        style.ChildRounding = 0.f;
        style.FrameRounding = 2.f;
        style.PopupRounding = 2.f;
        style.ScrollbarRounding = 2.f;
        style.GrabRounding = 2.f;
        style.TabRounding = 2.f;
        style.WindowBorderSize = 1.f;
        style.ChildBorderSize = 1.f;
        style.FrameBorderSize = 1.f;

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text] = White;
        colors[ImGuiCol_TextDisabled] = Muted;
        colors[ImGuiCol_WindowBg] = ImVec4(0.015f, 0.020f, 0.050f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.025f, 0.030f, 0.065f, 0.98f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.030f, 0.035f, 0.075f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.10f, 0.15f, 0.30f, 0.88f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.035f, 0.045f, 0.090f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.06f, 0.10f, 0.25f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.10f, 0.15f, 0.40f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.040f, 0.050f, 0.100f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.08f, 0.12f, 0.30f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.55f, 0.45f, 0.03f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.06f, 0.10f, 0.25f, 0.90f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.10f, 0.15f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.45f, 0.03f, 1.00f);
        colors[ImGuiCol_CheckMark] = Yellow;
        colors[ImGuiCol_SliderGrab] = Yellow;
        colors[ImGuiCol_SliderGrabActive] = Blue;
        colors[ImGuiCol_Separator] = ImVec4(0.10f, 0.15f, 0.30f, 0.65f);
        colors[ImGuiCol_SeparatorHovered] = Yellow;
        colors[ImGuiCol_SeparatorActive] = Blue;
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.020f, 0.025f, 0.060f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.05f, 0.08f, 0.20f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = Blue2;
        colors[ImGuiCol_ScrollbarGrabActive] = Blue;
        colors[ImGuiCol_Tab] = ImVec4(0.030f, 0.040f, 0.080f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.08f, 0.12f, 0.30f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.55f, 0.45f, 0.03f, 1.00f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    }

    static void DrawAngledRect(ImDrawList* draw, ImVec2 p, ImVec2 s, ImU32 fill, ImU32 border, float cut = 12.f)
    {
        ImVec2 pts[4] = {
            ImVec2(p.x + cut, p.y),
            ImVec2(p.x + s.x, p.y),
            ImVec2(p.x + s.x - cut, p.y + s.y),
            ImVec2(p.x, p.y + s.y)
        };
        draw->AddConvexPolyFilled(pts, 4, fill);
        draw->AddPolyline(pts, 4, border, true, 1.4f);
    }

    static void SectionTitle(const char* text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Yellow);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    static void TextMuted(const char* text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Muted);
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }

    static void DrawHeaderLogo()
    {
        if (g_BetterLogo)
        {
            ImGui::Image((ImTextureID)g_BetterLogo, g_BetterLogoSize);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, White);
            ImGui::TextUnformatted("BETTER ");
            ImGui::SameLine(0.f, 0.f);
            ImGui::PushStyleColor(ImGuiCol_Text, Yellow);
            ImGui::TextUnformatted("CHEATS");
            ImGui::PopStyleColor(2);
        }
    }

    static void DrawTopHeader()
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        const float h = 92.f;

        draw->AddRectFilled(p, ImVec2(p.x + avail.x, p.y + h), IM_COL32(4, 6, 18, 255));
        draw->AddRectFilledMultiColor(ImVec2(p.x, p.y + h - 4), ImVec2(p.x + avail.x, p.y + h), U32(Blue), U32(Yellow), U32(Blue2), U32(Yellow2));

        ImGui::BeginChild("##BetterTopHeader", ImVec2(0, h), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetCursorPos(ImVec2(18.f, 10.f));
        DrawHeaderLogo();
        ImGui::EndChild();
    }

    static bool TopTab(const char* label, bool active, ImVec2 size)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImU32 fill = active ? U32(Yellow2) : IM_COL32(8, 12, 30, 255);
        ImU32 line = active ? U32(Yellow) : U32(Blue2);
        DrawAngledRect(draw, p, size, fill, line, 10.f);
        ImGui::InvisibleButton(label, size);
        bool clicked = ImGui::IsItemClicked();
        ImVec2 ts = ImGui::CalcTextSize(label);
        draw->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (size.y - ts.y) * 0.5f), active ? U32(White) : U32(Muted), label);
        return clicked;
    }

    static bool SideButton(const char* id, const char* icon, const char* label, bool active, ImVec2 size)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImU32 fill = active ? IM_COL32(8, 25, 60, 225) : IM_COL32(8, 12, 28, 180);
        ImU32 line = active ? U32(Yellow) : IM_COL32(30, 40, 70, 170);
        draw->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), fill, 0.f);
        draw->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), line, 0.f, 0, 1.f);
        if (active)
            draw->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + 4.f, p.y + size.y), U32(Yellow));
        ImGui::InvisibleButton(id, size);
        bool clicked = ImGui::IsItemClicked();
        draw->AddText(ImVec2(p.x + 14.f, p.y + 11.f), active ? U32(White) : U32(Muted), icon);
        draw->AddText(ImVec2(p.x + 42.f, p.y + 11.f), active ? U32(White) : U32(Muted), label);
        return clicked;
    }

    static void BeginCard(const char* id, const char* title, ImVec2 size)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.020f, 0.025f, 0.055f, 0.98f));
        ImGui::BeginChild(id, size, true);
        SectionTitle(title);
    }

    static void EndCard()
    {
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    static bool SubTab(const char* label, bool active, ImVec2 size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, active ? Yellow2 : ImVec4(0.03f, 0.04f, 0.08f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, active ? Yellow : Blue2);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Yellow);
        bool clicked = ImGui::Button(label, size);
        ImGui::PopStyleColor(3);
        return clicked;
    }

    static void ColorEditSmall(const char* label, float* color)
    {
        ImGui::TextUnformatted(label);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 52.f);
        std::string id = std::string("##") + label;
        ImGui::ColorEdit4(id.c_str(), color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar);
    }

    static void SameLineNext() { ImGui::SameLine(0.f, 10.f); }

    static void Hotkey(const char* label, const char* featureId)
    {
        auto& bind = g_Hotkeys[featureId];
        bool hasKey = (bind.key != 0 && bind.mode != HK_DISABLED);
        bool isConflict = hasKey && HasHotkeyConflict(featureId, bind.key);

        ImGui::TextUnformatted(label);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80.f);

        ImGui::PushID(featureId);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.03f, 0.045f, 0.10f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.06f, 0.10f, 0.25f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Blue2);
        ImGui::PushStyleColor(ImGuiCol_Border, isConflict ? ImVec4(1.f, 0.2f, 0.2f, 0.8f) : ImVec4(0.10f, 0.15f, 0.30f, 0.65f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

        char chip[32] = {};
        wsprintfA(chip, "[%s]", hasKey ? GetKeyName(bind.key) : "None");
        if (ImGui::Button(chip, ImVec2(74, 22)))
            ImGui::OpenPopup("hk_bind");

        if (ImGui::BeginPopup("hk_bind"))
        {
            ImGui::Text("Bind: %s", hasKey ? GetKeyName(bind.key) : "None");
            ImGui::Text("Mode: %s", GetModeName(bind.mode));
            ImGui::Separator();
            ImGui::TextColored(Yellow, "Press any key or mouse button...");
            ImGuiIO& io = ImGui::GetIO();
            bool captured = false;
            auto tryAssignKey = [&](int key) { std::string b; if (!CanAssignHotkey(featureId, key, &b)) return false; bind.key = key; return true; };
            static const int mouseVKs[5] = { 0, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
            for (int m = 1; m < 5; m++) { if (io.MouseDown[m] && io.MouseDownDuration[m] == 0.0f) { captured = tryAssignKey(mouseVKs[m]); break; } }
            if (!captured) {
                for (int k = 8; k < 256; k++) {
                    if (GetAsyncKeyState(k) & 0x8000) {
                        if (k == VK_ESCAPE || k == VK_BACK || k == VK_INSERT || k == VK_END) { bind.key = 0; }
                        else { captured = tryAssignKey(k); }
                        if (k == VK_ESCAPE || k == VK_BACK || k == VK_INSERT || k == VK_END) captured = true;
                        break;
                    }
                }
            }
            if (captured) ImGui::CloseCurrentPopup();
            if (ImGui::Button("Clear")) { bind.key = 0; bind.mode = HK_DISABLED; }
            ImGui::SameLine();
            if (ImGui::BeginCombo("##mode", GetModeName(bind.mode))) {
                for (int m = HK_TOGGLE; m <= HK_ALWAYS; m++) { if (ImGui::Selectable(GetModeName(m), bind.mode == m)) bind.mode = m; }
                ImGui::EndCombo();
            }
            if (isConflict)
                ImGui::TextColored(ImVec4(1.f, 0.3f, 0.43f, 1.f), "Conflict: %s", GetConflictFeature(featureId, bind.key).c_str());
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::PopID();
    }
}

using namespace BetterUI;

inline void Aim()
{
    ImGui::BeginChild("AimbotPage", ImVec2(0, 0), false);
    ImGui::PushStyleColor(ImGuiCol_Text, White);
    ImGui::TextUnformatted("AIMBOT");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    TextMuted("// precision controls");
    ImGui::Separator();

    const float avail = ImGui::GetContentRegionAvail().x;
    const float col = (avail - 24.f) / 3.f;

    ImGui::BeginGroup();
    BeginCard("aim_basic", "BASIC", ImVec2(col, 130.f));
    ImGui::Checkbox("Aimbot Enable", &AIMBOT::Memory);
    ImGui::Combo("Bone Priority", &AIMBOT::BonePriority, "Head\0Neck\0Chest\0Pelvis\0Closest\0Smart\0");
    EndCard();

    BeginCard("aim_hotkeys", "HOTKEY", ImVec2(col, 80.f));
    Hotkey("Aim Hotkey", "COMBAT.Aimbot");
    EndCard();

    BeginCard("aim_targets", "TARGET FILTER", ImVec2(col, 150.f));
    ImGui::Checkbox("Ignore Team", &AIMBOT::IgnoreTeam);
    ImGui::Checkbox("Ignore Wounded", &AIMBOT::IgnoreWounded);
    ImGui::Checkbox("Ignore Sleepers", &AIMBOT::IgnoreSleepers);
    ImGui::Checkbox("Weapon Check", &AIMBOT::WeaponCheck);
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("aim_position", "TARGETING", ImVec2(col, 100.f));
    ImGui::Checkbox("Keep Target", &AIMBOT::KeepTarget);
    ImGui::Checkbox("Spin Writes", &AIMBOT::SpinWrites);
    EndCard();

    BeginCard("aim_range", "AIM RANGE", ImVec2(col, 124.f));
    ImGui::SliderFloat("Max Distance", &AIMBOT::MaxDistanceAim, 1.f, 500.f, "%.0f m");
    ImGui::SliderInt("FOV Size", &AIMBOT::FovSize, 0, 200);
    EndCard();

    BeginCard("aim_speed", "SPEED & PREDICTION", ImVec2(col, 124.f));
    ImGui::SliderFloat("Smoothing", &AIMBOT::SMOOTHING, 1.f, 20.f, "%.2f");
    ImGui::SliderFloat("Prediction", &AIMBOT::PredictionScale, 0.0f, 2.0f, "%.2fx");
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("aim_display", "DISPLAY & FILTER", ImVec2(col, 182.f));
    ImGui::Checkbox("Show Aim FOV", &AIMBOT::FovCircle);
    if (AIMBOT::FovCircle)
        ColorEditSmall("FOV Color", (float*)&AIMBOT::FovColor);
    ImGui::Checkbox("FOV Outline", &AIMBOT::FovOutline);
    ImGui::Checkbox("Show Target Line", &AIMBOT::TargetLine);
    ImGui::Checkbox("Show Target Text", &AIMBOT::TargetText);
    ImGui::Checkbox("Prediction Indicator", &AIMBOT::PredictionIndicator);
    EndCard();

    BeginCard("aim_recoil", "RECOIL", ImVec2(col, 160.f));
    ImGui::Checkbox("Recoil Mod", &MISC::RecoilEnabled);
    if (MISC::RecoilEnabled) {
        ImGui::SliderFloat("Recoil %", &MISC::RecoilModifier, 0.f, 100.f, "%.0f%%");
        ImGui::Checkbox("Recoil Variance", &MISC::RecoilVariance);
        ImGui::SliderFloat("Floor", &MISC::RecoilFloor, 0.10f, 0.50f, "%.2f");
    }
    EndCard();

    BeginCard("aim_human", "HUMANIZATION", ImVec2(col, 160.f));
    ImGui::Checkbox("Humanize Aim", &AIMBOT::HumanizeEnabled);
    if (AIMBOT::HumanizeEnabled) {
        ImGui::SliderFloat("Jitter", &AIMBOT::JitterAmount, 0.f, 5.f, "%.1f");
        ImGui::SliderFloat("Overshoot", &AIMBOT::OvershootAmount, 0.f, 10.f, "%.1f");
        ImGui::SliderFloat("Smooth Var", &AIMBOT::SmoothingVariance, 0.f, 0.5f, "%.2f");
        ImGui::SliderFloat("Miss %", &AIMBOT::MissProbability, 0.f, 0.10f, "%.2f");
    }
    EndCard();

    BeginCard("aim_anticheat", "ANTI-CHEAT", ImVec2(col, 60.f));
    ImGui::Checkbox("Anti Anybrain", &MISC::AntiAnybrain);
    EndCard();
    ImGui::EndGroup();

    ImGui::EndChild();
}

inline void Pawn()
{
    static int pawnIndex = 0;
    ImGui::BeginChild("VisualsPage", ImVec2(0, 0), false);
    ImGui::PushStyleColor(ImGuiCol_Text, White);
    ImGui::TextUnformatted("VISUALS");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    TextMuted("// awareness layout");
    ImGui::Separator();

    const float avail = ImGui::GetContentRegionAvail().x;
    const float left = 220.f;
    const float right = avail - left - 12.f;

    ImGui::BeginGroup();
    BeginCard("visual_type", "TYPE", ImVec2(left, 80.f));
    ImGui::Combo("Type", &pawnIndex, "Player\0NPC\0Animal\0\0");
    EndCard();

    BeginCard("visual_filters", "FILTERS", ImVec2(left, 180.f));
    if (pawnIndex == 0) {
        ImGui::Checkbox("Remove Team", &ESP::RemoveTeam);
        ImGui::SameLine(0, 4);
        ColorEditSmall("Teammate Color", (float*)&ESP::color::Teammate);
        ImGui::Checkbox("Remove Wounded", &ESP::RemoveWounded);
        ImGui::Checkbox("Remove Sleepers", &ESP::RemoveSleepers);
        ImGui::Checkbox("Highlight Wounded", &ESP::HighlightWounded);
        ImGui::Checkbox("Highlight Sleepers", &ESP::HighlightSleepers);
    } else if (pawnIndex == 1) {
        ImGui::Checkbox("Scientists", &NPC_ESP::Scientists);
        ImGui::Checkbox("Tunnel Dweller", &NPC_ESP::TunnelDweller);
        ImGui::Checkbox("Underwater Dweller", &NPC_ESP::UnderwaterDweller);
    } else {
        ImGui::Checkbox("Bear", &ANIMAL_ESP::Bear);
        ImGui::Checkbox("Wolf", &ANIMAL_ESP::Wolf);
        ImGui::Checkbox("Boar", &ANIMAL_ESP::Boar);
        ImGui::Checkbox("Stag", &ANIMAL_ESP::Stag);
        ImGui::Checkbox("Chicken", &ANIMAL_ESP::Chicken);
    }
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    if (pawnIndex == 0)
    {
        BeginCard("visual_display", "DISPLAY SETTINGS", ImVec2(right, 280.f));
        ImGui::Checkbox("Master Switch", &ESP::ESPEnabled);
        ImGui::Separator();
        ImGui::Columns(2, "visual_display_cols", false);
        ImGui::Checkbox("Skeleton", &ESP::Skeleton);
        ImGui::Checkbox("Box", &ESP::Box);
        ImGui::Checkbox("Corner Box", &ESP::CornerBox);
        ImGui::Checkbox("Filled Box", &ESP::FilledBox);
        ImGui::Checkbox("Name", &ESP::Name);
        ImGui::Checkbox("Weapon", &ESP::Weapon);
        ImGui::Checkbox("Distance", &ESP::Distance);
        ImGui::NextColumn();
        ImGui::Checkbox("Snaplines", &ESP::SnapLines);
        ImGui::Checkbox("Head Circle", &ESP::HeadCircle);
        ImGui::Checkbox("Health Bar", &ESP::HealthBar);
        ImGui::Checkbox("OFF Arrows", &ESP::OFFArrows);
        ImGui::Checkbox("Team ID", &ESP::TeamID);
        ImGui::Checkbox("Hotbar Text", &ESP::hotbar_text);
        ImGui::Checkbox("Inventory Panel", &ESP::PlayerInventoryPanel);
        ImGui::Checkbox("Bullet Tracers", &ESP::BulletTracers);
        ImGui::Checkbox("VisCheck", &ESP::VisCheck);
        ImGui::Columns(1);
        EndCard();

        BeginCard("visual_vis", "VISIBILITY COLORS", ImVec2(right, 120.f));
        ImGui::ColorEdit4("Box Visible##bv", (float*)&ESP::color::Visible, ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine();
        ImGui::ColorEdit4("Box Invisible##bi", (float*)&ESP::color::Invisible, ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("Skel Visible##sv", (float*)&ESP::color::SkeletonVisible, ImGuiColorEditFlags_NoInputs);
        ImGui::SameLine();
        ImGui::ColorEdit4("Skel Invisible##si", (float*)&ESP::color::SkeletonInvisible, ImGuiColorEditFlags_NoInputs);
        EndCard();

        BeginCard("visual_style", "DISTANCE & STYLE", ImVec2(right, 172.f));
        ImGui::SliderFloat("Max Distance", &ESP::draw_distance, 50.f, 2000.f, "%.0f m");
        ImGui::SliderInt("Box Thickness", &ESP::BoxThickness, 1, 5);
        ImGui::SliderInt("Skeleton Thickness", &ESP::SkeletonThickness, 1, 5);
        ImGui::SliderFloat("Font Size", &ESP::EspFontSize, 0.5f, 2.0f, "%.1fx");
        EndCard();
    }
    else if (pawnIndex == 1)
    {
        BeginCard("npc_display", "NPC DISPLAY", ImVec2(right, 220.f));
        ImGui::Checkbox("Master Switch", &NPC_ESP::Enabled);
        ImGui::Separator();
        ImGui::Columns(2, "npc_display_cols", false);
        ImGui::Checkbox("Skeleton", &NPC_ESP::Skeleton);
        ImGui::Checkbox("Name", &NPC_ESP::Name);
        ImGui::Checkbox("Distance", &NPC_ESP::Distance);
        ImGui::Checkbox("Held Item", &NPC_ESP::HeldItem);
        ImGui::NextColumn();
        ImGui::Checkbox("Snaplines", &NPC_ESP::SnapLines);
        ImGui::Checkbox("Health Bar", &NPC_ESP::HealthBar);
        ImGui::Columns(1);
        EndCard();

        BeginCard("npc_style", "DISTANCE & STYLE", ImVec2(right, 80.f));
        ImGui::SliderFloat("Max Distance", &NPC_ESP::draw_distance, 50.f, 1000.f, "%.0f m");
        EndCard();
    }
    else
    {
        BeginCard("animal_display", "ANIMAL DISPLAY", ImVec2(right, 200.f));
        ImGui::Checkbox("Master Switch", &ANIMAL_ESP::Enabled);
        ImGui::Separator();
        ImGui::Columns(2, "animal_display_cols", false);
        ImGui::Checkbox("Box", &ANIMAL_ESP::Box);
        ImGui::Checkbox("Name", &ANIMAL_ESP::Name);
        ImGui::Checkbox("Distance", &ANIMAL_ESP::Distance);
        ImGui::NextColumn();
        ImGui::Checkbox("Snaplines", &ANIMAL_ESP::SnapLines);
        ImGui::Checkbox("Health Bar", &ANIMAL_ESP::HealthBar);
        ImGui::Columns(1);
        EndCard();

        BeginCard("animal_types", "ANIMAL TYPES", ImVec2(right, 220.f));
        ImGui::Checkbox("Bear", &ANIMAL_ESP::Bear);
        ImGui::Checkbox("Polar Bear", &ANIMAL_ESP::PolarBear);
        ImGui::Checkbox("Wolf", &ANIMAL_ESP::Wolf);
        ImGui::Checkbox("Boar", &ANIMAL_ESP::Boar);
        ImGui::Checkbox("Stag", &ANIMAL_ESP::Stag);
        ImGui::Checkbox("Chicken", &ANIMAL_ESP::Chicken);
        ImGui::Checkbox("Horse", &ANIMAL_ESP::Horse);
        ImGui::Checkbox("Panther", &ANIMAL_ESP::Panther);
        ImGui::Checkbox("Tiger", &ANIMAL_ESP::Tiger);
        ImGui::Checkbox("Snake", &ANIMAL_ESP::Snake);
        EndCard();

        BeginCard("animal_style", "DISTANCE & STYLE", ImVec2(right, 80.f));
        ImGui::SliderFloat("Max Distance", &ANIMAL_ESP::draw_distance, 50.f, 1000.f, "%.0f m");
        EndCard();
    }
    ImGui::EndGroup();

    ImGui::EndChild();
}

static void DrawResourcesTab()
{
    const float avail = ImGui::GetContentRegionAvail().x;
    const float left = (avail - 12.f) * 0.5f;
    const float right = avail - left - 12.f;

    ImGui::BeginGroup();
    BeginCard("res_ore", "ORE NODES", ImVec2(left, 180.f));
    ImGui::Checkbox("Stone", &WORLD::Stone);
    ImGui::Checkbox("Metal", &WORLD::Metal);
    ImGui::Checkbox("Sulfur", &WORLD::Sulfer);
    ImGui::Checkbox("Hemp", &WORLD::Hemp);
    ImGui::Separator();
    ImGui::SliderFloat("Stone Dist", &WORLD::draw_stone, 50.f, 1000.f);
    ImGui::SliderFloat("Metal Dist", &WORLD::draw_metal, 50.f, 1000.f);
    ImGui::SliderFloat("Sulfur Dist", &WORLD::draw_sulfur, 50.f, 1000.f);
    ImGui::SliderFloat("Hemp Dist", &WORLD::draw_hemp, 50.f, 1000.f);
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("res_pickup", "PICKUPS", ImVec2(right, 180.f));
    ImGui::Checkbox("Stone Pickup", &WORLD::StonePickup);
    ImGui::Checkbox("Metal Pickup", &WORLD::MetalPickup);
    ImGui::Checkbox("Sulfur Pickup", &WORLD::SulfurPickup);
    ImGui::Checkbox("Wood Pickup", &WORLD::WoodPickup);
    EndCard();

    BeginCard("res_world", "WORLD ESP", ImVec2(right, 100.f));
    ImGui::Checkbox("Show Distance", &WORLD::Distance);
    ImGui::SliderFloat("World Draw Distance", &WORLD::draw_distance, 50.f, 5000.f, "%.0f m");
    EndCard();
    ImGui::EndGroup();
}

static void DrawContainersTab()
{
    const float avail = ImGui::GetContentRegionAvail().x;
    const float col = (avail - 12.f) * 0.5f;

    ImGui::BeginGroup();
    BeginCard("cont_loot", "LOOT & STASH", ImVec2(col, 280.f));
    ImGui::Checkbox("Stash", &WORLD::Stash);
    ImGui::Checkbox("Body Bag / Corpse", &WORLD::BodyBag);
    ImGui::Checkbox("Backpack", &WORLD::Backpack);
    ImGui::Checkbox("Dropped Items", &WORLD::DroppedItem);
    ImGui::Separator();
    ImGui::SliderFloat("Stash Dist", &WORLD::draw_stash, 50.f, 1000.f);
    ImGui::SliderFloat("Corpse Dist", &WORLD::draw_corpse, 50.f, 1000.f);
    ImGui::SliderFloat("Backpack Dist", &WORLD::draw_backpack, 50.f, 1000.f);
    ImGui::SliderFloat("Dropped Dist", &WORLD::draw_dropped, 50.f, 1000.f);
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("cont_dep", "DEPLOYABLES", ImVec2(col, 280.f));
    ImGui::Checkbox("Locker", &WORLD::Lockers);
    ImGui::Checkbox("Vending Machine", &WORLD::Vending);
    ImGui::Checkbox("Tool Cupboard", &WORLD::TC);
    ImGui::Checkbox("Sleeping Bag", &WORLD::Bags);
    ImGui::Checkbox("Bed", &WORLD::Beds);
    ImGui::Checkbox("Workbench", &WORLD::Workbench);
    ImGui::Checkbox("Large Storage", &WORLD::LargeStorage);
    ImGui::Checkbox("Ladder", &WORLD::Ladder);
    ImGui::Checkbox("Generator", &WORLD::Generator);
    ImGui::Checkbox("Battery", &WORLD::Battery);
    EndCard();
    ImGui::EndGroup();
}

static void DrawCratesTab()
{
    const float avail = ImGui::GetContentRegionAvail().x;
    const float col = (avail - 12.f) * 0.5f;

    ImGui::BeginGroup();
    BeginCard("crate_crates", "CRATES", ImVec2(col, 240.f));
    ImGui::Checkbox("Normal Crate", &WORLD::CrateNormal);
    ImGui::Checkbox("Military Crate", &WORLD::CrateMilitary);
    ImGui::Checkbox("Elite Crate", &WORLD::CrateElite);
    ImGui::Checkbox("Locked Crate", &WORLD::CrateLocked);
    ImGui::Checkbox("Medical Crate", &WORLD::CrateMedical);
    ImGui::Checkbox("Food Crate", &WORLD::CrateFood);
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("crate_barrels", "BARRELS", ImVec2(col, 240.f));
    ImGui::Checkbox("Beige Barrel", &WORLD::BarrelBeige);
    ImGui::Checkbox("Blue Barrel", &WORLD::BarrelBlue);
    ImGui::Checkbox("Fuel Barrel", &WORLD::BarrelFuel);
    ImGui::Separator();
    ImGui::SliderFloat("Beige Dist", &WORLD::draw_barrelbeige, 50.f, 1000.f);
    ImGui::SliderFloat("Blue Dist", &WORLD::draw_barrelblue, 50.f, 1000.f);
    ImGui::SliderFloat("Fuel Dist", &WORLD::draw_barrelfuel, 50.f, 1000.f);
    EndCard();
    ImGui::EndGroup();
}

static void DrawTrapsVehiclesTab()
{
    const float avail = ImGui::GetContentRegionAvail().x;
    const float col = (avail - 12.f) * 0.5f;

    ImGui::BeginGroup();
    BeginCard("tv_traps", "TRAPS & TURRETS", ImVec2(col, 280.f));
    ImGui::Checkbox("Auto Turret", &WORLD::Turret);
    ImGui::Checkbox("Shotgun Trap", &WORLD::ShotGunTrap);
    ImGui::Checkbox("Flame Turret", &WORLD::FlameTurret);
    ImGui::Checkbox("SAM Site", &WORLD::SamSite);
    ImGui::Checkbox("Bear Trap", &WORLD::BearTrap);
    ImGui::Checkbox("Landmine", &WORLD::Landmine);
    ImGui::Separator();
    ImGui::SliderFloat("Turret Dist", &WORLD::draw_turret, 50.f, 1000.f);
    ImGui::SliderFloat("Shotgun Dist", &WORLD::draw_shotguntrap, 50.f, 1000.f);
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("tv_vehicles", "VEHICLES", ImVec2(col, 280.f));
    ImGui::Checkbox("MiniCopter", &WORLD::MiniCopter);
    ImGui::Checkbox("Bradley APC", &WORLD::BradlyAPC);
    ImGui::Checkbox("Cargo Ship", &WORLD::CargoShip);
    ImGui::Checkbox("Supply Drop", &WORLD::SupplyDrop);
    ImGui::Checkbox("Rowboat", &WORLD::Rowboat);
    ImGui::Checkbox("RHIB", &WORLD::RHIB);
    ImGui::Checkbox("Kayak", &WORLD::Kayak);
    ImGui::Checkbox("Tugboat", &WORLD::Tugboat);
    ImGui::Checkbox("Submarine", &WORLD::Submarine);
    ImGui::Checkbox("Transport Heli", &WORLD::TransportHeli);
    ImGui::Checkbox("Attack Heli", &WORLD::AttackHeli);
    ImGui::Checkbox("Hot Air Balloon", &WORLD::Balloon);
    ImGui::Checkbox("Motorbike", &WORLD::Motorbike);
    ImGui::Checkbox("Snowmobile", &WORLD::Snowmobile);
    EndCard();
    ImGui::EndGroup();
}

inline void Item()
{
    static int item_tab = 0;

    ImGui::BeginChild("ItemsPage", ImVec2(0, 0), false);
    ImGui::PushStyleColor(ImGuiCol_Text, White);
    ImGui::TextUnformatted("ITEMS");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    TextMuted("// world esp, containers, vehicles");
    ImGui::Separator();

    const float tabW = 152.f;
    if (SubTab("RESOURCES", item_tab == 0, ImVec2(tabW, 34.f))) item_tab = 0;
    ImGui::SameLine();
    if (SubTab("CONTAINERS", item_tab == 1, ImVec2(tabW, 34.f))) item_tab = 1;
    ImGui::SameLine();
    if (SubTab("CRATES", item_tab == 2, ImVec2(tabW, 34.f))) item_tab = 2;
    ImGui::SameLine();
    if (SubTab("TRAPS & VEHICLES", item_tab == 3, ImVec2(tabW, 34.f))) item_tab = 3;

    ImGui::Dummy(ImVec2(0, 8.f));
    ImGui::BeginChild("ItemsScroll", ImVec2(0, 0), false);
    if (item_tab == 0) DrawResourcesTab();
    if (item_tab == 1) DrawContainersTab();
    if (item_tab == 2) DrawCratesTab();
    if (item_tab == 3) DrawTrapsVehiclesTab();
    ImGui::EndChild();

    ImGui::EndChild();
}

inline void Settting()
{
    ImGui::BeginChild("SettingsPage", ImVec2(0, 0), false);
    ImGui::PushStyleColor(ImGuiCol_Text, White);
    ImGui::TextUnformatted("SETTINGS");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    TextMuted("// menu and config");
    ImGui::Separator();

    const float avail = ImGui::GetContentRegionAvail().x;
    const float col = (avail - 24.f) / 3.f;

    ImGui::BeginGroup();
    BeginCard("set_battle", "BATTLE MODE", ImVec2(col, 120.f));
    ImGui::Checkbox("Battle Mode", &SETTINGS::BattleMode);
    if (SETTINGS::BattleMode) {
        ImGui::PushStyleColor(ImGuiCol_Text, Yellow);
        ImGui::TextUnformatted("● On - Filters Active");
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
        ImGui::TextUnformatted("● Off");
        ImGui::PopStyleColor();
    }
    EndCard();

    BeginCard("set_battle_filters", "BATTLE MODE FILTERS", ImVec2(col, 180.f));
    ImGui::Checkbox("Players ESP", &BATTLE::Players);
    ImGui::Checkbox("Aimbot", &BATTLE::Aimbot);
    ImGui::Checkbox("World ESP", &BATTLE::World);
    ImGui::Checkbox("NPC ESP", &BATTLE::NPC);
    ImGui::Checkbox("Animal ESP", &BATTLE::Animals);
    ImGui::Checkbox("Radar", &BATTLE::Radar);
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("set_hotkeys", "HOTKEYS", ImVec2(col, 170.f));
    Hotkey("Show/Hide Menu", "global.menu");
    Hotkey("Panic / Unload", "global.shutdown");
    Hotkey("Admin Flags", "UTIL.AdminFlags");
    Hotkey("FOV Changer", "VISUAL.FovChanger");
    EndCard();

    BeginCard("set_config", "CONFIGURATION", ImVec2(col, 190.f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.02f, 0.06f, 0.20f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Blue);
    if (ImGui::Button("Load Config", ImVec2(-1.f, 38.f))) Config::Load();
    if (ImGui::Button("Save Config", ImVec2(-1.f, 38.f))) Config::Save();
    ImGui::PopStyleColor(2);
    if (Config::lastStatusTime > 0) {
        uint64_t now = GetTickCount64();
        if (now - Config::lastStatusTime < 5000) {
            bool isErr = Config::lastStatus.find("fail") != std::string::npos || Config::lastStatus.find("invalid") != std::string::npos;
            ImGui::PushStyleColor(ImGuiCol_Text, isErr ? ImVec4(1.f, 0.3f, 0.43f, 1.f) : Yellow);
            ImGui::TextWrapped("%s", Config::lastStatus.c_str());
            ImGui::PopStyleColor();
        }
    }
    EndCard();
    ImGui::EndGroup();

    SameLineNext();

    ImGui::BeginGroup();
    BeginCard("set_camera", "CAMERA", ImVec2(col, 170.f));
    ImGui::Checkbox("FOV Changer", &MISC::FovChanger);
    if (MISC::FovChanger)
        ImGui::SliderFloat("FOV Amount", &MISC::FovAmount, 30.f, 150.f, "%.0f");
    ImGui::Checkbox("Zoom", &MISC::Zoom);
    if (MISC::Zoom)
        ImGui::SliderFloat("Zoom Amount", &MISC::ZoomAmount, 10.f, 200.f, "%.0f");
    EndCard();

    BeginCard("set_env", "ENVIRONMENT", ImVec2(col, 160.f));
    ImGui::Checkbox("Bright Night", &MISC::BrightNight);
    if (MISC::BrightNight) {
        ImGui::SliderFloat("Ambient Mul", &MISC::ambientMultiplier, 0.f, 20.f);
        ImGui::SliderFloat("Ambient Sat", &MISC::AmbientSaturation, 0.f, 5.f);
    }
    ImGui::Separator();
    ImGui::Checkbox("Time Changer", &MISC::Timechanger);
    if (MISC::Timechanger) {
        ImGui::SliderInt("Time", &MISC::timevalue, 0, 24);
    }
    EndCard();

    BeginCard("set_radar", "RADAR", ImVec2(col, 220.f));
    ImGui::Checkbox("Enable Radar", &RADAR::Enabled);
    if (RADAR::Enabled) {
        ImGui::SliderFloat("Size", &RADAR::Size, 100.f, 500.f, "%.0f");
        ImGui::SliderFloat("Scale", &RADAR::Scale, 0.5f, 5.f, "%.1f");
        ImGui::SliderFloat("Dot Size", &RADAR::DotSize, 2.f, 10.f, "%.0f");
        ImGui::Checkbox("Show Grid", &RADAR::ShowGrid);
        ImGui::Checkbox("Rotate", &RADAR::Rotate);
        ImGui::Checkbox("Show Players", &RADAR::ShowPlayers);
        ImGui::Checkbox("Show NPCs", &RADAR::ShowNPCs);
        ImGui::Checkbox("Show Animals", &RADAR::ShowAnimals);
        ImGui::Checkbox("Hide Sleepers", &RADAR::HideSleepers);
        ImGui::Checkbox("Remove Team", &RADAR::RemoveTeam);
        ImGui::Separator();
        ColorEditSmall("Player", (float*)&RADAR::PlayerColor);
        ImGui::SameLine();
        ColorEditSmall("Teammate", (float*)&RADAR::TeammateColor);
        ColorEditSmall("NPC", (float*)&RADAR::NpcColor);
        ImGui::SameLine();
        ColorEditSmall("Animal", (float*)&RADAR::AnimalColor);
        ColorEditSmall("Border", (float*)&RADAR::BorderColor);
    }
    EndCard();

    BeginCard("set_about", "ABOUT", ImVec2(col, 100.f));
    ImGui::PushStyleColor(ImGuiCol_Text, Yellow);
    ImGui::TextUnformatted("BETTER CHEATS");
    ImGui::PopStyleColor();
    TextMuted("Private Cheat");
    TextMuted("build " __DATE__);
    EndCard();
    ImGui::EndGroup();

    ImGui::EndChild();
}

inline void ApplyTheme() { BetterUI::ApplyStyle(); }

inline void Render() {
    ApplyTheme();

    ImGui::SetNextWindowSize(ImVec2(1200.f, 740.f), ImGuiCond_Once);
    ImGui::Begin("BETTER CHEATS##better", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    DrawTopHeader();
    ImGui::Dummy(ImVec2(0, 8.f));

    ImGui::SetCursorPosX(18.f);
    if (TopTab("AIMBOT", main_tab == 0, ImVec2(145.f, 38.f))) main_tab = 0;
    ImGui::SameLine(0.f, 10.f);
    if (TopTab("VISUALS", main_tab == 1, ImVec2(145.f, 38.f))) main_tab = 1;
    ImGui::SameLine(0.f, 10.f);
    if (TopTab("ITEMS", main_tab == 2, ImVec2(145.f, 38.f))) main_tab = 2;
    ImGui::SameLine(0.f, 10.f);
    if (TopTab("SETTINGS", main_tab == 3, ImVec2(145.f, 38.f))) main_tab = 3;

    ImGui::Dummy(ImVec2(0, 8.f));

    ImGui::BeginChild("MainShell", ImVec2(0, 0), false);

    ImGui::BeginChild("Sidebar", ImVec2(150.f, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::Dummy(ImVec2(0, 4.f));
    if (SideButton("side_aim", "+", "AIMBOT", main_tab == 0, ImVec2(126.f, 42.f))) main_tab = 0;
    if (SideButton("side_visual", "o", "VISUALS", main_tab == 1, ImVec2(126.f, 42.f))) main_tab = 1;
    if (SideButton("side_items", "[]", "ITEMS", main_tab == 2, ImVec2(126.f, 42.f))) main_tab = 2;
    if (SideButton("side_settings", "*", "SETTINGS", main_tab == 3, ImVec2(126.f, 42.f))) main_tab = 3;
    ImGui::EndChild();

    ImGui::SameLine(0.f, 12.f);

    ImGui::BeginChild("Content", ImVec2(0, 0), true);
    if (main_tab == 0) Aim();
    if (main_tab == 1) Pawn();
    if (main_tab == 2) Item();
    if (main_tab == 3) Settting();
    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::End();
}

} // namespace BetterCheatsMenu

#endif // BETTERCHEATS
