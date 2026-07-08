#pragma once
#include "../imgui/imgui.h"
#include "../offsets.hpp"
#include "../Config.hpp"
#include "../tracers.hpp"

namespace vischeck {
    extern bool g_VisCheckLoaded;
    extern std::string g_VisCheckStatus;
}
#include "../Render/icons.cpp"
#include "../Hotkeys.hpp"
#include "../Translation.hpp"
#include "../RuntimePaths.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

#if !defined(BOMZA) && !defined(BETTERCHEATS)

extern ID3D11ShaderResourceView* g_BeaztLogo;
extern ImFont* BeaztFont;
extern ImFont* BeaztFontTitle;
extern ImFont* BeaztFontDesc;

namespace BeaztMenu {

// ============================================================================
// COMPACT TECH PALETTE — black + blue + white
// ============================================================================
static const ImVec4 C_BG_APP      (0.039f, 0.043f, 0.051f, 1.0f);
static const ImVec4 C_BG_MENU     (0.039f, 0.043f, 0.051f, 0.98f);
static const ImVec4 C_BG_SIDEBAR  (0.051f, 0.055f, 0.063f, 0.95f);
static const ImVec4 C_BG_CARD     (0.059f, 0.063f, 0.075f, 0.95f);
static const ImVec4 C_BG_HEADER   (0.047f, 0.051f, 0.059f, 0.92f);
static const ImVec4 C_BG_INPUT    (0.043f, 0.047f, 0.055f, 0.95f);
static const ImVec4 C_BG_HOVER    (0.075f, 0.082f, 0.098f, 0.95f);
static const ImVec4 C_BG_ACTIVE   (0.063f, 0.071f, 0.090f, 0.95f);
static const ImVec4 C_BG_ROW_HOVER(0.035f, 0.043f, 0.059f, 0.50f);
static const ImVec4 C_BG_PILL     (0.059f, 0.063f, 0.075f, 0.86f);
static const ImVec4 C_BORDER      (1.0f,   1.0f,   1.0f,   0.06f);
static const ImVec4 C_BORDER_MED  (1.0f,   1.0f,   1.0f,   0.10f);
static const ImVec4 C_BORDER_SOFT  (1.0f,   1.0f,   1.0f,   0.04f);
static const ImVec4 C_BORDER_ACTIVE(0.231f, 0.510f, 0.965f, 0.60f);
static const ImVec4 C_ROW_DIV     (1.0f,   1.0f,   1.0f,   0.03f);
static const ImVec4 C_TEXT        (0.910f, 0.925f, 0.945f, 1.0f);
static const ImVec4 C_TEXT_LABEL  (0.788f, 0.816f, 0.855f, 1.0f);
static const ImVec4 C_TEXT_LIST   (0.733f, 0.769f, 0.820f, 1.0f);
static const ImVec4 C_TEXT_SEC    (0.545f, 0.588f, 0.659f, 1.0f);
static const ImVec4 C_TEXT_MUTED  (0.290f, 0.329f, 0.412f, 1.0f);
static const ImVec4 C_TEXT_OFF    (0.545f, 0.588f, 0.659f, 0.45f);
static const ImVec4 C_DANGER      (0.945f, 0.302f, 0.302f, 1.0f);
static const ImVec4 C_ACCENT      (0.231f, 0.510f, 0.965f, 1.0f); // #3B82F6
static const ImVec4 C_ACCENT_HOVER(0.275f, 0.549f, 0.980f, 1.0f);
static const ImVec4 C_ACCENT_DIM  (0.145f, 0.337f, 0.620f, 1.0f);
static const ImVec4 C_GLOW        (0.231f, 0.510f, 0.965f, 0.12f);
static const ImVec4 C_SW_OFF_TRK  (0.075f, 0.082f, 0.098f, 0.90f);
static const ImVec4 C_SW_OFF_TH   (0.545f, 0.588f, 0.659f, 1.0f);
static const ImVec4 C_SW_ON_TRK   (0.231f, 0.510f, 0.965f, 1.0f);
static const ImVec4 C_SW_ON_TH    (1.0f,   1.0f,   1.0f,   1.0f);
static const ImU32  U_ACCENT       = IM_COL32(0x3B, 0x82, 0xF6, 255);
static const ImU32  U_ACCENT_DIM   = IM_COL32(0x25, 0x56, 0x9E, 255);

// Layout — compact tech spacing
static const float MENU_W = 820.0f;
static const float MENU_H = 520.0f;
static const float HEADER_H = 32.0f;
static const float FOOTER_H = 20.0f;
static const float SIDEBAR_W = 120.0f;
static const float OUTER_PAD = 8.0f;
static const float SB_GAP = 8.0f;
static const float CARD_PAD = 10.0f;
static const float CARD_PAD_R = 10.0f;
static const float CARD_GAP = 8.0f;
static const float CARD_RADIUS = 8.0f;
static const float ROW_H = 28.0f;
static const float SB_ITEM_H = 30.0f;
static const float SB_ITEM_GAP = 4.0f;
static const float SB_ITEM_RADIUS = 6.0f;
static const float SB_RADIUS = 8.0f;
static const float PANEL_HEADER_H = 24.0f;
static const float SHELL_RADIUS = 10.0f;
static const float ACCORDION_RADIUS = 6.0f;

// Control positions
static const float COL_CONTROL  = 130.0f;
static const float SLIDER_DEF_W = 120.0f;
static const float VALUE_PILL_W = 48.0f;
static const float VALUE_PILL_H = 18.0f;
static const float COLOR_SIZE   = 18.0f;
static const float HOTKEY_W     = 56.0f;
static const float HOTKEY_H     = 20.0f;
static const float COMBO_DEF_W  = 100.0f;

// ============================================================================
// SEARCH (always returns true — search bar removed, kept for call-site compat)
// ============================================================================
inline bool SearchMatch(const char*) {
    return true;
}

// ============================================================================
// SWITCH — 32x16 modern toggle, pill track, circular knob, subtle glow
// ============================================================================
inline bool Switch(const char* id, bool* v) {
    const ImVec2 size(32.0f, 16.0f);
    ImGui::PushID(id);
    bool pressed = ImGui::InvisibleButton("##sw", size);
    if (pressed) *v = !*v;

    static std::unordered_map<ImGuiID, float>* pAnim = nullptr;
    if (!pAnim) pAnim = new std::unordered_map<ImGuiID, float>();
    auto& anim = *pAnim;
    ImGuiID iid = ImGui::GetID("##sw");
    float& t = anim[iid];
    float target = *v ? 1.0f : 0.0f;
    float dt = ImGui::GetIO().DeltaTime;
    t += (target - t) * (dt * 18.0f > 1.0f ? 1.0f : dt * 18.0f);

    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    auto* dl = ImGui::GetWindowDrawList();
    bool hovered = ImGui::IsItemHovered();

    auto lerpCol = [](ImU32 a, ImU32 b, float f) -> ImU32 {
        int ar=(a>>0)&0xFF, ag=(a>>8)&0xFF, ab=(a>>16)&0xFF, aa=(a>>24)&0xFF;
        int br=(b>>0)&0xFF, bg=(b>>8)&0xFF, bb=(b>>16)&0xFF, ba=(b>>24)&0xFF;
        return IM_COL32(ar+(br-ar)*f, ag+(bg-ag)*f, ab+(bb-ab)*f, aa+(ba-aa)*f);
    };

    ImU32 offTrack = hovered ? IM_COL32(0x1E, 0x23, 0x30, 255) : IM_COL32(0x15, 0x19, 0x22, 255);
    ImU32 onTrack  = hovered ? IM_COL32(0x46, 0x8C, 0xFA, 255) : U_ACCENT;
    ImU32 offKnob  = IM_COL32(0x6B, 0x73, 0x85, 255);
    ImU32 onKnob   = IM_COL32(0xFF, 0xFF, 0xFF, 255);

    ImU32 trkCol = lerpCol(offTrack, onTrack, t);
    ImU32 knobCol = lerpCol(offKnob, onKnob, t);

    // Track — fully rounded pill
    dl->AddRectFilled(pMin, pMax, trkCol, 8.0f);

    // Subtle inner glow when ON
    if (t > 0.01f) {
        dl->AddRect(pMin, pMax, IM_COL32(0x3B, 0x82, 0xF6, (int)(60 * t)), 8.0f, 0, 1.0f);
    } else {
        dl->AddRect(pMin, pMax, IM_COL32(255, 255, 255, 8), 8.0f, 0, 1.0f);
    }

    // Knob — circle with shadow
    const float knobR = 6.0f;
    float knobX = pMin.x + 8.0f + t * (size.x - 16.0f);
    float knobY = pMin.y + size.y * 0.5f;

    if (t > 0.01f) {
        dl->AddCircleFilled(ImVec2(knobX, knobY), knobR + 2.0f, IM_COL32(0x3B, 0x82, 0xF6, (int)(50 * t)), 16);
    }
    dl->AddCircleFilled(ImVec2(knobX, knobY), knobR, knobCol, 16);

    ImGui::PopID();
    return pressed;
}

// ============================================================================
// CUSTOM SLIDER — modern thin track, circular knob, smooth drag
// ============================================================================
inline bool CustomSlider(const char* id, float* valF, int* valI, float minV, float maxV, float width) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    auto* dl = ImGui::GetWindowDrawList();

    float trackY = pos.y + ROW_H * 0.5f;
    float trackH = 4.0f;
    float knobR = 6.0f;
    float usableW = width - 2 * knobR;
    if (usableW < 20.0f) usableW = 20.0f;

    float curVal = valF ? *valF : (float)*valI;
    float range = maxV - minV;
    if (range <= 0) range = 1;
    float pct = (curVal - minV) / range;
    if (pct < 0) pct = 0;
    if (pct > 1) pct = 1;

    float knobX = pos.x + knobR + pct * usableW;

    ImGui::PushID(id);
    ImGui::InvisibleButton("##slider", ImVec2(width, ROW_H));
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    bool changed = false;

    if (active && ImGui::GetIO().MouseDown[0]) {
        float mouseX = ImGui::GetIO().MousePos.x;
        float relX = (mouseX - pos.x - knobR) / usableW;
        if (relX < 0) relX = 0;
        if (relX > 1) relX = 1;
        float newVal = minV + relX * range;
        if (valF) { *valF = newVal; changed = true; }
        else if (valI) { *valI = (int)newVal; changed = true; }
        curVal = newVal;
        pct = relX;
        knobX = pos.x + knobR + pct * usableW;
    }

    // Track background
    ImU32 trackCol = hovered || active ? IM_COL32(0x1E, 0x23, 0x30, 255) : IM_COL32(0x12, 0x15, 0x1C, 255);
    dl->AddRectFilled(ImVec2(pos.x, trackY - trackH * 0.5f), ImVec2(pos.x + width, trackY + trackH * 0.5f), trackCol, trackH * 0.5f);

    // Fill
    ImU32 fillCol = active ? IM_COL32(0x46, 0x8C, 0xFA, 255) : U_ACCENT;
    dl->AddRectFilled(ImVec2(pos.x, trackY - trackH * 0.5f), ImVec2(knobX, trackY + trackH * 0.5f), fillCol, trackH * 0.5f);

    // Knob shadow
    dl->AddCircleFilled(ImVec2(knobX, trackY + 1.0f), knobR + 1.0f, IM_COL32(0, 0, 0, 50), 12);

    // Knob
    ImU32 knobCol = active ? IM_COL32(0xFF, 0xFF, 0xFF, 255) : (hovered ? IM_COL32(0xE8, 0xEC, 0xF1, 255) : IM_COL32(0xC8, 0xD1, 0xDA, 255));
    dl->AddCircleFilled(ImVec2(knobX, trackY), knobR, knobCol, 12);

    // Blue ring around knob when active
    if (active) {
        dl->AddCircle(ImVec2(knobX, trackY), knobR + 2.0f, IM_COL32(0x3B, 0x82, 0xF6, 100), 12, 1.0f);
    }

    ImGui::PopID();
    return changed;
}

// ============================================================================
// COLOR BUTTON — 18x18, 4px radius
// ============================================================================
inline bool ColorButton(const char* id, ImColor& color) {
    const ImVec2 size(COLOR_SIZE, COLOR_SIZE);
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("##cb", size);

    ImVec2 pMin = ImGui::GetItemRectMin();
    ImVec2 pMax = ImGui::GetItemRectMax();
    auto* dl = ImGui::GetWindowDrawList();

    ImU32 fillCol = ImGui::ColorConvertFloat4ToU32(color.Value);
    bool cbHovered = ImGui::IsItemHovered();
    if (cbHovered) {
        dl->AddRectFilled(ImVec2(pMin.x - 2, pMin.y - 2), ImVec2(pMax.x + 2, pMax.y + 2), U_ACCENT_DIM, 6.0f);
    }
    dl->AddRectFilled(pMin, pMax, fillCol, 4.0f);
    dl->AddRect(pMin, pMax, IM_COL32(255, 255, 255, 25), 4.0f, 0, 1.0f);

    if (clicked) ImGui::OpenPopup("##colorpop");
    if (ImGui::BeginPopup("##colorpop")) {
        float cf[4] = { color.Value.x, color.Value.y, color.Value.z, color.Value.w };
        if (ImGui::ColorEdit4("##cedit", cf, ImGuiColorEditFlags_AlphaPreview))
            color = ImColor(cf[0], cf[1], cf[2], cf[3]);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return clicked;
}

// ============================================================================
// SETTING ROW GRID — relative columns, right-aligned controls, symmetric padding
// ============================================================================
inline void SettingRowGrid(const char* label,
    bool* toggle = nullptr,
    float* sliderF = nullptr, float slMinF = 0, float slMaxF = 0, const char* slFmtF = "",
    int* sliderI = nullptr, int slMinI = 0, int slMaxI = 0,
    ImColor* color = nullptr,
    int* combo = nullptr, const char* const* comboItems = nullptr, int comboCount = 0,
    const char* hkFeatureId = nullptr,
    const char* tooltip = nullptr,
    bool isDanger = false
) {
    if (!SearchMatch(label)) return;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    // availW minus right padding so rows don't touch card border
    float availW = ImGui::GetContentRegionAvail().x - CARD_PAD_R;
    auto* dl = ImGui::GetWindowDrawList();

    // Row hover background
    bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + availW, pos.y + ROW_H));
    if (hovered)
        dl->AddRectFilled(pos, ImVec2(pos.x + availW, pos.y + ROW_H), ImGui::ColorConvertFloat4ToU32(C_BG_ROW_HOVER), 4.0f);

    // Row divider (bottom)
    dl->AddLine(ImVec2(pos.x, pos.y + ROW_H - 1), ImVec2(pos.x + availW, pos.y + ROW_H - 1), ImGui::ColorConvertFloat4ToU32(C_ROW_DIV), 1.0f);

    // ── Label at x=0 — color depends on state
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::AlignTextToFramePadding();
    if (isDanger) {
        ImGui::PushStyleColor(ImGuiCol_Text, C_DANGER);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
    } else if (toggle && !*toggle) {
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_OFF);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_LABEL);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();
    }
    if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

    // ── Toggle at COL_CONTROL
    if (toggle) {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + COL_CONTROL, pos.y + (ROW_H - 16.0f) * 0.5f));
        ImGui::PushID(label);
        Switch("##sw", toggle);
        ImGui::PopID();
    }

    // ── Slider — custom modern slider at COL_CONTROL, value pill right after
    if (sliderF || sliderI) {
        float slW = (std::min)(SLIDER_DEF_W, availW - COL_CONTROL - VALUE_PILL_W - 8.0f);
        if (slW < 50.0f) slW = 50.0f;
        ImGui::SetCursorScreenPos(ImVec2(pos.x + COL_CONTROL, pos.y));
        ImGui::PushID(label);
        float minV = sliderF ? slMinF : (float)slMinI;
        float maxV = sliderF ? slMaxF : (float)slMaxI;
        CustomSlider("##sr", sliderF, sliderI, minV, maxV, slW);
        ImGui::PopID();

        // Value pill right after slider
        char valStr[32];
        if (sliderF) {
            if (strstr(slFmtF, "%")) snprintf(valStr, sizeof(valStr), slFmtF, *sliderF);
            else snprintf(valStr, sizeof(valStr), "%.1f", *sliderF);
        } else {
            snprintf(valStr, sizeof(valStr), "%d", *sliderI);
        }
        float pillX = pos.x + COL_CONTROL + slW + 6.0f;
        float pillY = pos.y + (ROW_H - VALUE_PILL_H) * 0.5f;
        dl->AddRectFilled(ImVec2(pillX, pillY), ImVec2(pillX + VALUE_PILL_W, pillY + VALUE_PILL_H),
            ImGui::ColorConvertFloat4ToU32(C_BG_CARD), 4.0f);
        dl->AddRect(ImVec2(pillX, pillY), ImVec2(pillX + VALUE_PILL_W, pillY + VALUE_PILL_H),
            ImGui::ColorConvertFloat4ToU32(C_BORDER_SOFT), 4.0f, 0, 1.0f);
        ImVec2 ts = ImGui::CalcTextSize(valStr);
        dl->AddText(ImVec2(pillX + (VALUE_PILL_W - ts.x) * 0.5f, pillY + (VALUE_PILL_H - ts.y) * 0.5f),
            ImGui::ColorConvertFloat4ToU32(C_TEXT_SEC), valStr);
    }

    // ── Combo at COL_CONTROL, fixed width
    if (combo && comboItems && comboCount > 0) {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + COL_CONTROL, pos.y + (ROW_H - 18.0f) * 0.5f));
        ImGui::PushID(label);
        ImGui::PushItemWidth(COMBO_DEF_W);
        ImGui::Combo("##sr", combo, comboItems, comboCount);
        ImGui::PopItemWidth();
        ImGui::PopID();
    }

    // ── Color — right-aligned
    if (color) {
        float cx = pos.x + availW - COLOR_SIZE;
        ImGui::SetCursorScreenPos(ImVec2(cx, pos.y + (ROW_H - COLOR_SIZE) * 0.5f));
        ImGui::PushID(label);
        ColorButton("##cb", *color);
        ImGui::PopID();
    }

    // ── Hotkey chip — right-aligned
    if (hkFeatureId) {
        float hkX = pos.x + availW - HOTKEY_W;
        ImGui::SetCursorScreenPos(ImVec2(hkX, pos.y + (ROW_H - HOTKEY_H) * 0.5f));
        ImGui::PushID(label);
        auto& bind = g_Hotkeys[hkFeatureId];
        bool hasKey = (bind.key != 0 && bind.mode != HK_DISABLED);
        const char* chipText = hasKey ? GetKeyName(bind.key) : "None";
        ImVec2 cp = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##hkc", ImVec2(HOTKEY_W, HOTKEY_H));
        bool chipHovered = ImGui::IsItemHovered();
        bool chipClicked = ImGui::IsItemClicked();
        auto* cdl = ImGui::GetWindowDrawList();
        ImU32 chipBg = chipHovered ? ImGui::ColorConvertFloat4ToU32(C_BG_HOVER) : ImGui::ColorConvertFloat4ToU32(C_BG_CARD);
        cdl->AddRectFilled(cp, ImVec2(cp.x + HOTKEY_W, cp.y + HOTKEY_H), chipBg, 4.0f);
        cdl->AddRect(cp, ImVec2(cp.x + HOTKEY_W, cp.y + HOTKEY_H), ImGui::ColorConvertFloat4ToU32(C_BORDER_SOFT), 4.0f, 0, 1.0f);
        ImVec2 cts = ImGui::CalcTextSize(chipText);
        cdl->AddText(ImVec2(cp.x + (HOTKEY_W - cts.x) * 0.5f, cp.y + (HOTKEY_H - cts.y) * 0.5f), ImGui::ColorConvertFloat4ToU32(C_TEXT), chipText);
        if (chipClicked) ImGui::OpenPopup("hk_bind");
        if (ImGui::BeginPopup("hk_bind")) {
            ImGui::Text("Bind: %s", hasKey ? GetKeyName(bind.key) : "None");
            ImGui::Text("Mode: %s", GetModeName(bind.mode));
            ImGui::TextColored(C_ACCENT, "Press any key or mouse button...");
            ImGuiIO& io = ImGui::GetIO();
            bool captured = false;
            auto tryAssignKey = [&](int key) { std::string b; if (!CanAssignHotkey(hkFeatureId, key, &b)) return false; bind.key = key; return true; };
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
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ROW_H));
}

// World entity row — relative columns: Name(flex) | Toggle | Distance | Slider | Color(right)
inline void WorldEntityRow(const char* label, bool* t, float* dist, ImColor& col) {
    if (!SearchMatch(label)) return;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    auto* dl = ImGui::GetWindowDrawList();
    float availW = ImGui::GetContentRegionAvail().x - CARD_PAD_R;

    // Row hover + divider
    bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + availW, pos.y + ROW_H));
    if (hovered) dl->AddRectFilled(pos, ImVec2(pos.x + availW, pos.y + ROW_H), ImGui::ColorConvertFloat4ToU32(C_BG_ROW_HOVER), 4.0f);
    dl->AddLine(ImVec2(pos.x, pos.y + ROW_H - 1), ImVec2(pos.x + availW, pos.y + ROW_H - 1), ImGui::ColorConvertFloat4ToU32(C_ROW_DIV), 1.0f);

    // Name at x=0
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, *t ? C_TEXT_LIST : C_TEXT_OFF);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();

    // Toggle at COL_CONTROL
    ImGui::SetCursorScreenPos(ImVec2(pos.x + COL_CONTROL, pos.y + (ROW_H - 16.0f) * 0.5f));
    ImGui::PushID(label);
    Switch("##wsw", t);

    // Distance badge at COL_CONTROL + 40 — 40x18
    ImGui::SetCursorScreenPos(ImVec2(pos.x + COL_CONTROL + 40.0f, pos.y + (ROW_H - 18.0f) * 0.5f));
    ImGui::InvisibleButton("##vbg", ImVec2(40, 18));
    ImVec2 vbMin = ImGui::GetItemRectMin(), vbMax = ImGui::GetItemRectMax();
    dl->AddRectFilled(vbMin, vbMax, IM_COL32(3, 7, 17, 255), 4.0f);
    dl->AddRect(vbMin, vbMax, IM_COL32(255, 255, 255, 20), 4.0f, 0, 1.0f);
    char valStr[16]; snprintf(valStr, sizeof(valStr), "%.0fm", *dist);
    ImVec2 vSize = ImGui::CalcTextSize(valStr);
    dl->AddText(ImVec2(vbMin.x + (40 - vSize.x) * 0.5f, vbMin.y + (18 - vSize.y) * 0.5f), ImGui::ColorConvertFloat4ToU32(C_TEXT_SEC), valStr);

    // Slider at COL_CONTROL + 80
    float slW = availW - (COL_CONTROL + 80.0f) - COLOR_SIZE - 6.0f;
    if (slW < 50.0f) slW = 50.0f;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + COL_CONTROL + 80.0f, pos.y));
    ImGui::PushItemWidth(slW);
    CustomSlider("##wd", dist, nullptr, 0.f, 500.f, slW);
    ImGui::PopItemWidth();

    // Color right-aligned
    float cx = pos.x + availW - COLOR_SIZE;
    ImGui::SetCursorScreenPos(ImVec2(cx, pos.y + (ROW_H - COLOR_SIZE) * 0.5f));
    ColorButton("##wc", col);
    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ROW_H));
}

inline void WorldEntitySimple(const char* label, bool* t, ImColor& col) {
    if (!SearchMatch(label)) return;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    auto* dl = ImGui::GetWindowDrawList();
    float availW = ImGui::GetContentRegionAvail().x - CARD_PAD_R;

    bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + availW, pos.y + ROW_H));
    if (hovered) dl->AddRectFilled(pos, ImVec2(pos.x + availW, pos.y + ROW_H), ImGui::ColorConvertFloat4ToU32(C_BG_ROW_HOVER), 4.0f);
    dl->AddLine(ImVec2(pos.x, pos.y + ROW_H - 1), ImVec2(pos.x + availW, pos.y + ROW_H - 1), ImGui::ColorConvertFloat4ToU32(C_ROW_DIV), 1.0f);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + (ROW_H - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::AlignTextToFramePadding();
    ImGui::PushStyleColor(ImGuiCol_Text, *t ? C_TEXT_LIST : C_TEXT_OFF);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + COL_CONTROL, pos.y + (ROW_H - 16.0f) * 0.5f));
    ImGui::PushID(label);
    Switch("##wsw", t);

    float cx = pos.x + availW - COLOR_SIZE;
    ImGui::SetCursorScreenPos(ImVec2(cx, pos.y + (ROW_H - COLOR_SIZE) * 0.5f));
    ColorButton("##wc", col);
    ImGui::PopID();

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ROW_H));
}

// Section header
inline void SectionHeader(const char* text) {
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
    if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
    ImGui::TextUnformatted(text);
    if (BeaztFontDesc) ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 4));
}

// Card — compact, no shadow, simple header
inline void Card(const char* title, std::function<void()> body, float height = 0.0f, int enabledCount = -1, const char* helperText = nullptr) {
    float availW = ImGui::GetContentRegionAvail().x;
    char id[64]; wsprintfA(id, "card_%p", (void*)title);
    ImVec2 childSize(availW, height > 0 ? height : 0);

    ImVec2 cardPos = ImGui::GetCursorScreenPos();
    auto* sdl = ImGui::GetWindowDrawList();
    float drawH = height > 0 ? height : 0;

    // Simple bg fill — no shadow
    if (drawH > 0) {
        sdl->AddRectFilled(cardPos, ImVec2(cardPos.x + availW, cardPos.y + drawH),
            IM_COL32(0x0F, 0x11, 0x15, 240), CARD_RADIUS);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border, C_BORDER);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, CARD_RADIUS);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild(id, childSize, true);

    if (title && title[0]) {
        // Header bar
        ImVec2 hPos = ImGui::GetCursorScreenPos();
        float hW = ImGui::GetContentRegionAvail().x;
        sdl->AddRectFilled(hPos, ImVec2(hPos.x + hW, hPos.y + PANEL_HEADER_H), ImGui::ColorConvertFloat4ToU32(C_BG_HEADER), CARD_RADIUS, ImDrawFlags_RoundCornersTop);
        sdl->AddLine(ImVec2(hPos.x, hPos.y + PANEL_HEADER_H), ImVec2(hPos.x + hW, hPos.y + PANEL_HEADER_H), ImGui::ColorConvertFloat4ToU32(C_BORDER_SOFT), 1.0f);

        // Title
        ImGui::SetCursorScreenPos(ImVec2(hPos.x + CARD_PAD, hPos.y + (PANEL_HEADER_H - ImGui::GetTextLineHeight()) * 0.5f));
        if (BeaztFontTitle) ImGui::PushFont(BeaztFontTitle);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        if (BeaztFontTitle) ImGui::PopFont();

        // Advance past header
        float bodyY = hPos.y + PANEL_HEADER_H + 6.0f;

        if (helperText) {
            ImGui::SetCursorScreenPos(ImVec2(hPos.x + CARD_PAD, bodyY));
            if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
            ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
            ImGui::TextWrapped("%s", helperText);
            ImGui::PopStyleColor();
            if (BeaztFontDesc) ImGui::PopFont();
            bodyY = ImGui::GetCursorScreenPos().y + 4.0f;
        }

        ImGui::SetCursorScreenPos(ImVec2(hPos.x + CARD_PAD, bodyY));
    }
    if (body) body();
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
    ImGui::Dummy(ImVec2(0, CARD_GAP));
}

// Collapsible section — compact accordion
inline bool CollapsibleSection(const char* title, bool defaultOpen = false) {
    static std::unordered_map<ImGuiID, bool>* pState = nullptr;
    if (!pState) pState = new std::unordered_map<ImGuiID, bool>();
    auto& state = *pState;
    ImGuiID id = ImGui::GetID(title);
    if (state.find(id) == state.end()) state[id] = defaultOpen;
    bool& open = state[id];

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = 30.0f;

    ImGui::InvisibleButton(title, ImVec2(width, height));
    bool hovered = ImGui::IsItemHovered();
    bool clicked = ImGui::IsItemClicked();
    if (clicked) open = !open;

    auto* dl = ImGui::GetWindowDrawList();
    ImU32 bgCol = hovered ? IM_COL32(0x14, 0x17, 0x22, 244) : IM_COL32(0x0F, 0x11, 0x15, 240);
    if (open) bgCol = hovered ? IM_COL32(0x14, 0x17, 0x22, 248) : IM_COL32(0x0F, 0x11, 0x15, 244);
    dl->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), bgCol, ACCORDION_RADIUS);
    ImU32 bordCol = open ? IM_COL32(0x3B, 0x82, 0xF6, 80) : (hovered ? IM_COL32(0x3B, 0x82, 0xF6, 60) : IM_COL32(255, 255, 255, 12));
    dl->AddRect(pos, ImVec2(pos.x + width, pos.y + height), bordCol, ACCORDION_RADIUS, 0, 1.0f);

    // Title
    if (BeaztFontTitle) ImGui::PushFont(BeaztFontTitle);
    float textX = pos.x + 10;
    dl->AddText(ImVec2(textX, pos.y + (height - ImGui::GetTextLineHeight()) * 0.5f), ImGui::ColorConvertFloat4ToU32(C_TEXT), title);
    if (BeaztFontTitle) ImGui::PopFont();

    // Chevron
    const char* chevron = open ? "v" : ">";
    ImVec2 cs = ImGui::CalcTextSize(chevron);
    ImU32 chevCol = open ? ImGui::ColorConvertFloat4ToU32(C_ACCENT) : ImGui::ColorConvertFloat4ToU32(C_TEXT_SEC);
    dl->AddText(ImVec2(pos.x + width - 20.0f, pos.y + (height - cs.y) * 0.5f), chevCol, chevron);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + height + 6.0f));
    return open;
}

// Metric card — compact 48px
inline void MetricCard(const char* label, const char* value) {
    float cardW = ImGui::GetContentRegionAvail().x;
    char id[64]; wsprintfA(id, "mc_%p", (void*)label);
    ImVec2 cardSize(cardW, 48.0f);

    ImVec2 cardPos = ImGui::GetCursorScreenPos();
    auto* sdl = ImGui::GetWindowDrawList();
    sdl->AddRectFilled(cardPos, ImVec2(cardPos.x + cardW, cardPos.y + 48.0f),
        IM_COL32(0x0F, 0x11, 0x15, 240), CARD_RADIUS);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border, C_BORDER);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, CARD_RADIUS);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    ImGui::BeginChild(id, cardSize, true);
    if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    if (BeaztFontDesc) ImGui::PopFont();
    if (BeaztFontTitle) ImGui::PushFont(BeaztFontTitle);
    ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT);
    ImGui::TextUnformatted(value);
    ImGui::PopStyleColor();
    if (BeaztFontTitle) ImGui::PopFont();
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

// Two-column helper — supports custom left ratio
struct TwoCol { float leftW, rightW, gap; };
inline TwoCol BeginTwoCol(float gap = CARD_GAP, float leftRatio = 0.55f) {
    float totalW = ImGui::GetContentRegionAvail().x;
    float leftW = totalW * leftRatio - gap * 0.5f;
    float rightW = totalW - leftW - gap;
    return { leftW, rightW, gap };
}

// Auto-height card calculator — pass row count, get pixel height
inline float CardHeight(int rows, bool hasHeader = true, bool hasHelper = false) {
    float h = hasHeader ? PANEL_HEADER_H : 0;
    h += 6.0f;           // top body padding
    h += rows * ROW_H;   // row content
    h += 4.0f;           // bottom body padding
    if (hasHelper) h += 18.0f;
    return h;
}

// ============================================================================
// CONFIG / STATE
// ============================================================================
struct MenuState {
    char newConfigName[64] = "default";
    char activeConfig[64] = "default";
    std::vector<std::string> configNames;
    int selectedConfigIndex = 0;
    bool configListDirty = true;
    uint64_t lastConfigListRefreshMs = 0;
    char keybindSearch[96] = {};
    int keybindPageFilter = 0;
    bool showOnlyConflicts = false;
    int espSubPage = 0;
    bool autoSave = false;
};
inline MenuState g_State;

inline void RefreshConfigList(bool force = false) {
    uint64_t now = GetTickCount64();
    if (!force && !g_State.configListDirty && (now - g_State.lastConfigListRefreshMs) < 1000) return;
    std::vector<std::string> names; names.push_back("default");
    std::string prefix = std::string(RuntimePaths::ConfigPrefix()) + "_";
    std::string pattern = RuntimePaths::DllDirectory() + prefix + "*.dat";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::string file = fd.cFileName;
            size_t dotPos = file.rfind('.');
            if (dotPos == std::string::npos) continue;
            std::string name = file.substr(prefix.size(), dotPos - prefix.size());
            if (name.empty() || name == "default") continue;
            names.push_back(name);
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    g_State.configNames.swap(names);
    g_State.selectedConfigIndex = 0;
    for (int i = 0; i < (int)g_State.configNames.size(); i++)
        if (g_State.configNames[i] == g_State.activeConfig) { g_State.selectedConfigIndex = i; break; }
    g_State.configListDirty = false;
    g_State.lastConfigListRefreshMs = now;
}

inline bool SaveNamedConfig(const char* name) {
    Config::Save();
    auto p = Config::GetConfigPathForName(name);
    const char* activeCfg = Config::GetDefaultConfigPath();
    if (lstrcmpiA(p.c_str(), activeCfg) != 0)
        if (!CopyFileA(activeCfg, p.c_str(), FALSE)) return false;
    lstrcpynA(g_State.activeConfig, (name && name[0]) ? name : "default", 63);
    g_State.activeConfig[63] = 0;
    lstrcpynA(g_State.newConfigName, g_State.activeConfig, 63);
    g_State.newConfigName[63] = 0;
    g_State.configListDirty = true;
    return true;
}

inline bool LoadNamedConfig(const char* name) {
    auto p = Config::GetConfigPathForName(name);
    if (GetFileAttributesA(p.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    const char* activeCfg = Config::GetDefaultConfigPath();
    if (lstrcmpiA(p.c_str(), activeCfg) != 0) CopyFileA(p.c_str(), activeCfg, FALSE);
    Config::Load();
    lstrcpynA(g_State.activeConfig, (name && name[0]) ? name : "default", 63);
    g_State.activeConfig[63] = 0;
    lstrcpynA(g_State.newConfigName, g_State.activeConfig, 63);
    g_State.newConfigName[63] = 0;
    g_State.configListDirty = true;
    return true;
}

inline bool DeleteNamedConfig(const char* name) {
    auto p = Config::GetConfigPathForName(name);
    if (GetFileAttributesA(p.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
    return DeleteFileA(p.c_str()) != 0;
}

inline bool ImportConfigDialog() {
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Config Files (*.dat)\0*.dat\0All Files (*.*)\0*.*\0";
    ofn.lpstrDefExt = "dat";
    char file[260] = {};
    ofn.lpstrFile = file;
    ofn.nMaxFile = 260;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameA(&ofn)) return false;
    const char* activeCfg = Config::GetDefaultConfigPath();
    if (!CopyFileA(file, activeCfg, FALSE)) return false;
    Config::Load();
    g_State.configListDirty = true;
    return true;
}

inline bool ExportConfigDialog() {
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Config Files (*.dat)\0*.dat\0All Files (*.*)\0*.*\0";
    ofn.lpstrDefExt = "dat";
    char file[260] = "beazt_config.dat";
    ofn.lpstrFile = file;
    ofn.nMaxFile = 260;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!GetSaveFileNameA(&ofn)) return false;
    Config::Save();
    const char* activeCfg = Config::GetDefaultConfigPath();
    return CopyFileA(activeCfg, file, FALSE) != 0;
}

inline void ResetAllSettingsDefaults() {
    ESP::Box = true; ESP::ESPEnabled = true; ESP::CornerBox = false; ESP::FilledBox = false;
    ESP::Name = true; ESP::Distance = true; ESP::Skeleton = true; ESP::Weapon = true;
    ESP::SnapLines = true; ESP::HeadCircle = false; ESP::HealthBar = false; ESP::OFFArrows = false;
    ESP::hotbar_text = false; ESP::TeamID = false; ESP::AmmoBar = false; ESP::ReloadBar = false;
    ESP::RemoveSleepers = false; ESP::RemoveWounded = false; ESP::RemoveTeam = false;
    ESP::draw_distance = 300.f; ESP::BoxThickness = 1; ESP::SkeletonThickness = 1;
    AIMBOT::Memory = false; AIMBOT::FovSize = 100; AIMBOT::SMOOTHING = 5.f;
    MISC::RecoilEnabled = true; MISC::RecoilModifier = 100.f;
    NPC_ESP::Enabled = true; ANIMAL_ESP::Enabled = true;
    RADAR::Enabled = false; MISC::FovChanger = false; MISC::Zoom = false;
    SETTINGS::BattleMode = false;
}

// Presets
inline void PresetLegit() {
    ESP::Box = true; ESP::CornerBox = false; ESP::FilledBox = false;
    ESP::Name = true; ESP::Distance = true; ESP::HealthBar = true;
    ESP::Weapon = false; ESP::Skeleton = false; ESP::HeadCircle = false;
    ESP::SnapLines = false; ESP::OFFArrows = false;
    ESP::TeamID = true; ESP::hotbar_text = false;
    ESP::AmmoBar = false; ESP::ReloadBar = false;
    ESP::RemoveSleepers = true; ESP::RemoveWounded = true;
    ESP::draw_distance = 200.f;
    AIMBOT::Memory = false; MISC::RecoilModifier = 0.f;
    MISC::NoSpread = false; MISC::Automatic = false;
    AIMBOT::HumanizeEnabled = true; AIMBOT::JitterAmount = 1.5f; AIMBOT::OvershootAmount = 3.f;
    AIMBOT::SmoothingVariance = 0.15f; AIMBOT::MissProbability = 0.02f;
    MISC::RecoilVariance = true; MISC::RecoilFloor = 0.25f; MISC::AntiAnybrain = true;
    RADAR::Enabled = true;
    WORLD::Stone = true; WORLD::Metal = true; WORLD::Sulfer = true; WORLD::Hemp = true;
    WORLD::Stash = true; WORLD::DroppedItem = true; WORLD::Backpack = true;
    WORLD::Turret = true; WORLD::ShotGunTrap = true;
    WORLD::draw_distance = 250.f;
    NPC_ESP::Enabled = true; NPC_ESP::Name = true; NPC_ESP::Distance = true;
    NPC_ESP::HealthBar = false; NPC_ESP::Skeleton = false;
    NPC_ESP::HeldItem = false; NPC_ESP::SnapLines = false;
    NPC_ESP::draw_distance = 200.f;
    ANIMAL_ESP::Enabled = true;
    ANIMAL_ESP::Box = false; ANIMAL_ESP::Name = true; ANIMAL_ESP::Distance = true;
    ANIMAL_ESP::draw_distance = 150.f;
    MISC::FovChanger = false; MISC::Zoom = false;
    SETTINGS::BattleMode = false;
}

inline void PresetAggressive() {
    ESP::Box = true; ESP::CornerBox = true; ESP::FilledBox = false;
    ESP::Name = true; ESP::Distance = true; ESP::HealthBar = true;
    ESP::Weapon = true; ESP::Skeleton = true; ESP::HeadCircle = true;
    ESP::SnapLines = true; ESP::OFFArrows = true;
    ESP::TeamID = true; ESP::hotbar_text = true;
    ESP::AmmoBar = true; ESP::ReloadBar = true;
    ESP::RemoveSleepers = false; ESP::RemoveWounded = false;
    ESP::draw_distance = 500.f;
    AIMBOT::Memory = true; AIMBOT::VisibleOnly = false;
    AIMBOT::IgnoreSleepers = true; AIMBOT::IgnoreWounded = true;
    AIMBOT::SMOOTHING = 3.f; AIMBOT::FovSize = 120;
    AIMBOT::FovCircle = true; AIMBOT::TargetLine = true;
    MISC::RecoilModifier = 100.f; MISC::NoSpread = true;
    AIMBOT::HumanizeEnabled = false; MISC::RecoilVariance = false; MISC::AntiAnybrain = true;
    RADAR::Enabled = true;
    WORLD::Stone = true; WORLD::Metal = true; WORLD::Sulfer = true; WORLD::Hemp = true;
    WORLD::Stash = true; WORLD::DroppedItem = true; WORLD::Backpack = true;
    WORLD::Turret = true; WORLD::ShotGunTrap = true; WORLD::MiniCopter = true;
    WORLD::BradlyAPC = true; WORLD::BodyBag = true;
    WORLD::draw_distance = 500.f;
    NPC_ESP::Enabled = true; NPC_ESP::Name = true; NPC_ESP::Distance = true;
    NPC_ESP::HealthBar = true;
    ANIMAL_ESP::Enabled = true;
    MISC::FovChanger = true; MISC::FovAmount = 90.f;
    SETTINGS::BattleMode = false;
}

inline void PresetFarmer() {
    ESP::Box = false; ESP::Name = false; ESP::Distance = false;
    ESP::Skeleton = false; ESP::HealthBar = false; ESP::SnapLines = false;
    ESP::ESPEnabled = false;
    AIMBOT::Memory = false; MISC::RecoilModifier = 0.f; MISC::RecoilEnabled = false;
    RADAR::Enabled = false;
    WORLD::Stone = true; WORLD::Metal = true; WORLD::Sulfer = true; WORLD::Hemp = true;
    WORLD::Stash = true; WORLD::Backpack = true; WORLD::DroppedItem = true;
    WORLD::BarrelBeige = true; WORLD::BarrelBlue = true; WORLD::BarrelFuel = true;
    WORLD::CrateNormal = true; WORLD::CrateMilitary = true; WORLD::CrateElite = true;
    WORLD::CrateFood = true; WORLD::CrateMedical = true;
    WORLD::draw_distance = 400.f; WORLD::Distance = true;
    NPC_ESP::Enabled = false; ANIMAL_ESP::Enabled = false;
    MISC::FovChanger = false; MISC::Zoom = false;
    SETTINGS::BattleMode = false;
}

inline void PresetESPOnly() {
    ESP::Box = true; ESP::Name = true; ESP::Distance = true; ESP::Skeleton = true;
    ESP::HealthBar = true; ESP::Weapon = true;
    ESP::ESPEnabled = true; ESP::draw_distance = 300.f;
    ESP::SnapLines = false; ESP::HeadCircle = false; ESP::OFFArrows = false;
    AIMBOT::Memory = false; MISC::RecoilModifier = 0.f; MISC::RecoilEnabled = false;
    RADAR::Enabled = false;
    WORLD::Stone = false; WORLD::Metal = false; WORLD::Sulfer = false; WORLD::Hemp = false;
    WORLD::Stash = false; WORLD::DroppedItem = false; WORLD::Backpack = false;
    WORLD::draw_distance = 0.f; WORLD::Distance = false;
    NPC_ESP::Enabled = false; ANIMAL_ESP::Enabled = false;
    MISC::FovChanger = false; MISC::Zoom = false;
    SETTINGS::BattleMode = false;
}

// ============================================================================
// PAGES
// ============================================================================
enum class Page : int { Combat, ESP, Radar, Visuals, Utility, Config };
static Page currentPage = Page::Combat;

// ── Dashboard — Template A: Overview ──
inline void PageDashboard() {
    float contentH = ImGui::GetContentRegionAvail().y;

    // Top row: 4 compact metric cards
    float metricW = (ImGui::GetContentRegionAvail().x - 8 * 3) / 4.0f;
    char buf[32];
    ImGui::BeginGroup();
    ImGui::PushItemWidth(metricW);
    snprintf(buf, sizeof(buf), "%.0f", ImGui::GetIO().Framerate);
    MetricCard("FPS", buf);
    ImGui::PopItemWidth();
    ImGui::SameLine(0, 8);
    ImGui::PushItemWidth(metricW);
    wsprintfA(buf, "%d", (int)entity_List.size());
    MetricCard("Players", buf);
    ImGui::PopItemWidth();
    ImGui::SameLine(0, 8);
    ImGui::PushItemWidth(metricW);
    wsprintfA(buf, "%d", (int)npc_List.size());
    MetricCard("NPCs", buf);
    ImGui::PopItemWidth();
    ImGui::SameLine(0, 8);
    ImGui::PushItemWidth(metricW);
    wsprintfA(buf, "%d", (int)animal_List.size());
    MetricCard("Animals", buf);
    ImGui::PopItemWidth();
    ImGui::EndGroup();

    ImGui::Dummy(ImVec2(0, CARD_GAP));
    float remaining = contentH - 80 - CARD_GAP;

    // Grid: 1fr 420px
    auto cols = BeginTwoCol(CARD_GAP, 0.62f);
    // Left: Quick Presets + System Info (compact, auto-height)
    ImGui::BeginChild("##dashL", ImVec2(cols.leftW, remaining), false);
    Card("Quick Presets", [&]() {
        float bw = (ImGui::GetContentRegionAvail().x - CARD_PAD_R - 9) / 4.0f;
        if (ImGui::Button("Legit", ImVec2(bw, 30))) PresetLegit();
        ImGui::SameLine(0, 3);
        if (ImGui::Button("Aggressive", ImVec2(bw, 30))) PresetAggressive();
        ImGui::SameLine(0, 3);
        if (ImGui::Button("Farmer", ImVec2(bw, 30))) PresetFarmer();
        ImGui::SameLine(0, 3);
        if (ImGui::Button("ESP Only", ImVec2(bw, 30))) PresetESPOnly();
    }, CardHeight(1, true));
    Card("System Information", [&]() {
        auto InfoRow = [](const char* k, const char* v) {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float avail = ImGui::GetContentRegionAvail().x - CARD_PAD_R;
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
            ImGui::TextUnformatted(k);
            ImGui::PopStyleColor();
            ImGui::SetCursorScreenPos(ImVec2(pos.x + 140, pos.y));
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_SEC);
            ImGui::TextUnformatted(v);
            ImGui::PopStyleColor();
            ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x, pos.y + ROW_H - 1), ImVec2(pos.x + avail, pos.y + ROW_H - 1), ImGui::ColorConvertFloat4ToU32(C_ROW_DIV), 1.0f);
            ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ROW_H));
        };
        InfoRow("Version", "v9.0");
        InfoRow("Build Date", __DATE__);
        InfoRow("Menu Key", "INSERT");
        InfoRow("Exit Key", "END");
    }, CardHeight(4, true));
    ImGui::EndChild();

    ImGui::SameLine(0, cols.gap);
    // Right: Active Features (fills remaining)
    ImGui::BeginChild("##dashR", ImVec2(cols.rightW, remaining), false);
    int feat = 0;
    if(ESP::Box) feat++; if(ESP::Skeleton) feat++; if(ESP::Name) feat++; if(ESP::Distance) feat++;
    if(ESP::Weapon) feat++; if(ESP::HealthBar) feat++; if(ESP::HeadCircle) feat++; if(ESP::SnapLines) feat++;
    if(ESP::OFFArrows) feat++; if(NPC_ESP::Enabled) feat++; if(ANIMAL_ESP::Enabled) feat++;
    if(AIMBOT::Memory) feat++; if(MISC::FovChanger) feat++; if(MISC::RecoilModifier > 0.f) feat++;
    if(RADAR::Enabled) feat++;

    auto StatusBadge = [](float x, float y, bool on) {
        auto* bdl = ImGui::GetWindowDrawList();
        const char* txt = on ? "ON" : "OFF";
        ImVec2 ts = ImGui::CalcTextSize(txt);
        float bw = ts.x + 16.0f, bh = 22.0f;
        ImU32 bg = on ? IM_COL32(0x48, 0xF5, 0x8A, 31) : IM_COL32(0x8D, 0x9B, 0xB3, 26);
        ImU32 bd = on ? IM_COL32(0x48, 0xF5, 0x8A, 64) : IM_COL32(0x8D, 0x9B, 0xB3, 51);
        ImU32 tx = on ? IM_COL32(0x48, 0xF5, 0x8A, 255) : IM_COL32(0x8D, 0x9B, 0xB3, 255);
        bdl->AddRectFilled(ImVec2(x, y), ImVec2(x + bw, y + bh), bg, 999.0f);
        bdl->AddRect(ImVec2(x, y), ImVec2(x + bw, y + bh), bd, 999.0f, 0, 1.0f);
        bdl->AddText(ImVec2(x + 8, y + (bh - ts.y) * 0.5f), tx, txt);
        return bw;
    };
    auto KVRow = [&](const char* k, const char* v, ImVec4 vCol) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float avail = ImGui::GetContentRegionAvail().x - CARD_PAD_R;
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
        ImGui::TextUnformatted(k);
        ImGui::PopStyleColor();
        ImGui::SetCursorScreenPos(ImVec2(pos.x + 160, pos.y));
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, vCol);
        ImGui::TextUnformatted(v);
        ImGui::PopStyleColor();
        ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x, pos.y + ROW_H - 1), ImVec2(pos.x + avail, pos.y + ROW_H - 1), ImGui::ColorConvertFloat4ToU32(C_ROW_DIV), 1.0f);
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ROW_H));
    };
    auto KVBadge = [&](const char* k, bool on) {
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float avail = ImGui::GetContentRegionAvail().x - CARD_PAD_R;
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
        ImGui::TextUnformatted(k);
        ImGui::PopStyleColor();
        StatusBadge(pos.x + 160, pos.y - 2, on);
        ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x, pos.y + ROW_H - 1), ImVec2(pos.x + avail, pos.y + ROW_H - 1), ImGui::ColorConvertFloat4ToU32(C_ROW_DIV), 1.0f);
        ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + ROW_H));
    };
    Card("Active Features", [&]() {
        char fb[16]; wsprintfA(fb, "%d", feat);
        KVRow("Total Active", fb, C_TEXT);
        KVBadge("Player ESP", ESP::ESPEnabled);
        KVBadge("Aimbot", AIMBOT::Memory);
        KVBadge("Recoil Mod", MISC::RecoilEnabled);
        KVBadge("Radar", RADAR::Enabled);
        KVBadge("World ESP", WORLD::Stone);
        KVRow("Menu Key", "INSERT", C_TEXT_SEC);
        KVRow("Exit Key", "END", C_TEXT_SEC);
        KVRow("Build Date", __DATE__, C_TEXT_SEC);
        KVRow("Config", g_State.activeConfig, C_TEXT_SEC);
    }, remaining);
    ImGui::EndChild();
}

// ── Combat — Template B: 5 separate cards, 2-col grid ──
inline void PageCombat() {
    float contentH = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("##combatScroll", ImVec2(0, contentH), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    auto cols = BeginTwoCol(CARD_GAP, 0.5f);

    // Left: Aim Core + Prediction + Recoil
    ImGui::BeginChild("##combatL", ImVec2(cols.leftW, 0), false);
    Card("Aim Core", [&]() {
        SettingRowGrid("Memory Aimbot", &AIMBOT::Memory, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, "COMBAT.Aimbot", "Uses memory writes. Higher detection risk.", true);
        SettingRowGrid("Silent Aim", &AIMBOT::Silent, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Uses memory writes. Higher detection risk.", true);
        SettingRowGrid("Smoothing", nullptr, &AIMBOT::SMOOTHING, 1.f, 20.f, "%.1f");
        SettingRowGrid("FOV Size", nullptr, nullptr,0.f,0.f,"", &AIMBOT::FovSize, 0, 200);
        SettingRowGrid("Keep Target", &AIMBOT::KeepTarget);
    }, CardHeight(5, true));
    Card("Prediction", [&]() {
        SettingRowGrid("Prediction Scale", nullptr, &AIMBOT::PredictionScale, 0.f, 2.f, "%.2fx");
        SettingRowGrid("Spin Writes", &AIMBOT::SpinWrites, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Multiple angle writes per frame. Very high risk.", true);
    }, CardHeight(2, true));
    Card("Recoil", [&]() {
        SettingRowGrid("Recoil Mod", &MISC::RecoilEnabled, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Writes recoil properties in weapon memory.", true);
        SettingRowGrid("Recoil Mod %", nullptr, &MISC::RecoilModifier, 0.f, 100.f, "%.0f%%");
        SettingRowGrid("Recoil Variance", &MISC::RecoilVariance, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Randomizes recoil scale +-10% per write.", true);
        SettingRowGrid("Recoil Floor", nullptr, &MISC::RecoilFloor, 0.10f, 0.50f, "%.2f");
    }, CardHeight(4, true));
    Card("Humanization", [&]() {
        SettingRowGrid("Humanize Aim", &AIMBOT::HumanizeEnabled, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Adds jitter/overshoot to aimbot for anti-AI detection.", true);
        SettingRowGrid("Jitter Amount", nullptr, &AIMBOT::JitterAmount, 0.f, 5.f, "%.1f deg");
        SettingRowGrid("Overshoot", nullptr, &AIMBOT::OvershootAmount, 0.f, 10.f, "%.1f");
        SettingRowGrid("Smooth Variance", nullptr, &AIMBOT::SmoothingVariance, 0.f, 0.50f, "%.2f");
        SettingRowGrid("Miss Probability", nullptr, &AIMBOT::MissProbability, 0.f, 0.10f, "%.2f");
    }, CardHeight(5, true));
    Card("Anti-Cheat", [&]() {
        SettingRowGrid("Anti Anybrain", &MISC::AntiAnybrain, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Disables Anybrain SDK data collection. Requires Frida RVA.", true);
    }, CardHeight(1, true));
    ImGui::EndChild();

    ImGui::SameLine(0, cols.gap);
    // Right: Targeting + Visual Indicators
    ImGui::BeginChild("##combatR", ImVec2(cols.rightW, 0), false);
    int tgtRows = 6;
    if (AIMBOT::BonePriority == 0) tgtRows += 2;
    Card("Targeting", [&]() {
        SettingRowGrid("Ignore Team", &AIMBOT::IgnoreTeam);
        SettingRowGrid("Ignore Wounded", &AIMBOT::IgnoreWounded);
        SettingRowGrid("Ignore Sleepers", &AIMBOT::IgnoreSleepers);
        SettingRowGrid("Weapon Check", &AIMBOT::WeaponCheck, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Only targets players holding weapons.");
        SettingRowGrid("Max Aim Distance", nullptr, &AIMBOT::MaxDistanceAim, 0.f, 500.f, "%.0f m");
        const char* bonePrio[] = { "Head", "Neck", "Chest", "Pelvis", "Closest", "Smart" };
        SettingRowGrid("Bone Priority", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &AIMBOT::BonePriority, bonePrio, 6);
        if (AIMBOT::BonePriority == 0) {
            const char* bones[] = { "Head", "Neck", "Spine", "Pelvis" };
            int boneVals[] = { 53, 52, 23, 0 };
            static int bIdx = 0;
            SettingRowGrid("Aim Bone", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &bIdx, bones, 4);
            // Apply Bone button row
            ImVec2 bp = ImGui::GetCursorScreenPos();
            float avail = ImGui::GetContentRegionAvail().x - CARD_PAD_R;
            ImGui::GetWindowDrawList()->AddLine(ImVec2(bp.x, bp.y + ROW_H - 1), ImVec2(bp.x + avail, bp.y + ROW_H - 1), ImGui::ColorConvertFloat4ToU32(C_ROW_DIV), 1.0f);
            ImGui::SetCursorScreenPos(ImVec2(bp.x + COL_CONTROL, bp.y + (ROW_H - 28) * 0.5f));
            if (ImGui::Button("Apply Bone", ImVec2(120, 28))) AIMBOT::BoneSelector = boneVals[bIdx];
            ImGui::SetCursorScreenPos(ImVec2(bp.x, bp.y + ROW_H));
        }
    }, CardHeight(tgtRows, true));
    int viRows = 6;
    if (AIMBOT::FovCircle) viRows++;
    if (AIMBOT::TargetLine || AIMBOT::TargetText) viRows += 3;
    Card("Visual Indicators", [&]() {
        SettingRowGrid("FOV Circle", &AIMBOT::FovCircle);
        SettingRowGrid("FOV Outline", &AIMBOT::FovOutline);
        if (AIMBOT::FovCircle) SettingRowGrid("FOV Color", nullptr, nullptr,0,0,"", nullptr,0,0, &AIMBOT::FovColor);
        SettingRowGrid("Prediction Indicator", &AIMBOT::PredictionIndicator);
        SettingRowGrid("Target Line", &AIMBOT::TargetLine);
        SettingRowGrid("Target Icon", &AIMBOT::TargetText);
        if (AIMBOT::TargetLine || AIMBOT::TargetText) {
            SettingRowGrid("Line Thickness", nullptr, &AIMBOT::TargetLineThickness, 1.0f, 4.0f, "%.1f");
            SettingRowGrid("Line Color", nullptr, nullptr,0,0,"", nullptr,0,0, &AIMBOT::color::TargetLine);
            SettingRowGrid("Target Icon Color", nullptr, nullptr,0,0,"", nullptr,0,0, &AIMBOT::color::TargetText);
        }
    }, CardHeight(viRows, true));
    ImGui::EndChild();
    ImGui::EndChild();
}

// ── ESP — tabs + 2-col cards with auto-height ──
inline void PageESP() {
    const char* espTabs[] = { "Player", "NPC", "Animal", "World" };
    // Segmented control
    float segW = 4 * 70.0f + 3 * 4.0f + 8.0f;
    ImVec2 segPos = ImGui::GetCursorScreenPos();
    auto* segDl = ImGui::GetWindowDrawList();
    segDl->AddRectFilled(segPos, ImVec2(segPos.x + segW, segPos.y + 24.0f), ImGui::ColorConvertFloat4ToU32(C_BG_CARD), 6.0f);
    segDl->AddRect(segPos, ImVec2(segPos.x + segW, segPos.y + 24.0f), ImGui::ColorConvertFloat4ToU32(C_BORDER_SOFT), 6.0f, 0, 1.0f);
    for (int i = 0; i < 4; i++) {
        bool sel = (g_State.espSubPage == i);
        float tabX = segPos.x + 4 + i * 70.0f;
        if (sel) {
            segDl->AddRectFilled(ImVec2(tabX, segPos.y + 3), ImVec2(tabX + 66, segPos.y + 21), U_ACCENT, 4.0f);
            segDl->AddRect(ImVec2(tabX, segPos.y + 3), ImVec2(tabX + 66, segPos.y + 21), IM_COL32(0x3B, 0x82, 0xF6, 120), 4.0f, 0, 1.0f);
        }
        ImVec2 ts = ImGui::CalcTextSize(espTabs[i]);
        ImU32 tCol = sel ? IM_COL32(0xFF, 0xFF, 0xFF, 255) : IM_COL32(0x8B, 0x95, 0xA7, 255);
        segDl->AddText(ImVec2(tabX + (66 - ts.x) * 0.5f, segPos.y + 3 + (18 - ts.y) * 0.5f), tCol, espTabs[i]);
        ImGui::SetCursorScreenPos(ImVec2(tabX, segPos.y + 3));
        char tabId[16]; wsprintfA(tabId, "##esptab%d", i);
        if (ImGui::InvisibleButton(tabId, ImVec2(66, 18))) g_State.espSubPage = i;
    }
    ImGui::SetCursorScreenPos(ImVec2(segPos.x, segPos.y + 30));
    ImGui::Dummy(ImVec2(0, 4));

    float contentH = ImGui::GetContentRegionAvail().y;

    if (g_State.espSubPage == 0) {
        // ── Player ESP — scrollable ──
        ImGui::BeginChild("##espPlayerScroll", ImVec2(0, contentH - 34), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        auto cols = BeginTwoCol(CARD_GAP, 0.52f);
        ImGui::BeginChild("##pesL", ImVec2(cols.leftW, 0), false);
        Card("General", [&]() {
            SettingRowGrid("Enable Player ESP", &ESP::ESPEnabled);
            SettingRowGrid("Draw Distance", nullptr, &ESP::draw_distance, 0.f, 500.f, "%.0f m");
        }, CardHeight(2, true));
        int boxRows = 9;
        Card("Box & Skeleton", [&]() {
            SettingRowGrid("Box", &ESP::Box, nullptr,0,0,"", nullptr,0,0, &ESP::color::Box);
            SettingRowGrid("Box Outline", &ESP::BoxOutline);
            SettingRowGrid("Box Thickness", nullptr, nullptr,0.f,0.f,"", &ESP::BoxThickness, 1, 5);
            SettingRowGrid("Corner Box", &ESP::CornerBox);
            SettingRowGrid("Filled Box", &ESP::FilledBox, nullptr,0,0,"", nullptr,0,0, &ESP::color::FilledBox);
            SettingRowGrid("Skeleton", &ESP::Skeleton, nullptr,0,0,"", nullptr,0,0, &ESP::color::Skeleton);
            SettingRowGrid("Skeleton Outline", &ESP::SkeletonOutline);
            SettingRowGrid("Skeleton Thickness", nullptr, nullptr,0.f,0.f,"", &ESP::SkeletonThickness, 1, 5);
            SettingRowGrid("Head Circle", &ESP::HeadCircle, nullptr,0,0,"", nullptr,0,0, &ESP::color::HeadCircle);
        }, CardHeight(boxRows, true));
        int advRows = 4 + (ESP::ESPAdvanced ? 7 : 0);
        Card("Advanced", [&]() {
            SettingRowGrid("Ammo Bar", &ESP::AmmoBar, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Shows ammo count at crosshair.");
            SettingRowGrid("Movement Trails", &ESP::MovementTrails, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, nullptr, "Shows historical position trace.");
            SettingRowGrid("Advanced Distances", &ESP::ESPAdvanced);
            SettingRowGrid("ESP Font Size", nullptr, &ESP::EspFontSize, 0.5f, 2.0f, "%.1fx");
            const char* bracketStyles[] = { "(100m)", "[100m]", "{100m}", "100m" };
            SettingRowGrid("Distance Style", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &ESP::DistanceBracketStyle, bracketStyles, 4);
            if (ESP::ESPAdvanced) {
                SettingRowGrid("Box Dist", nullptr, &ESP::BoxDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Skeleton Dist", nullptr, &ESP::SkeletonDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Name Dist", nullptr, &ESP::NameDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Weapon Dist", nullptr, &ESP::WeaponDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Snapline Dist", nullptr, &ESP::SnaplineDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("HeadCircle Dist", nullptr, &ESP::HeadCircleDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("OFF Arrow Dist", nullptr, &ESP::OFFArrowDist, 10.f, 300.f, "%.0f m");
            }
        }, CardHeight(advRows, true));
        ImGui::EndChild();
        ImGui::SameLine(0, cols.gap);
        ImGui::BeginChild("##pesR", ImVec2(cols.rightW, 0), false);
        Card("Visibility", [&]() {
            SettingRowGrid("VisCheck", &ESP::VisCheck);
            SettingRowGrid("Box Visible", nullptr, nullptr,0,0,"", nullptr,0,0, &ESP::color::Visible);
            SettingRowGrid("Box Invisible", nullptr, nullptr,0,0,"", nullptr,0,0, &ESP::color::Invisible);
            SettingRowGrid("Skel Visible", nullptr, nullptr,0,0,"", nullptr,0,0, &ESP::color::SkeletonVisible);
            SettingRowGrid("Skel Invisible", nullptr, nullptr,0,0,"", nullptr,0,0, &ESP::color::SkeletonInvisible);
        }, CardHeight(5, true));
        Card("Filters", [&]() {
            SettingRowGrid("Hide Wounded", &ESP::RemoveWounded);
            SettingRowGrid("Hide Sleepers", &ESP::RemoveSleepers);
            SettingRowGrid("Hide Teammates", &ESP::RemoveTeam);
            SettingRowGrid("Teammate Color", nullptr, nullptr,0,0,"", nullptr,0,0, &ESP::color::Teammate);
            SettingRowGrid("Highlight Wounded", &ESP::HighlightWounded, nullptr,0,0,"", nullptr,0,0, &ESP::color::Wounded);
            SettingRowGrid("Highlight Sleepers", &ESP::HighlightSleepers, nullptr,0,0,"", nullptr,0,0, &ESP::color::Sleepers);
        }, CardHeight(6, true));
        Card("Info & Text", [&]() {
            SettingRowGrid("Snaplines", &ESP::SnapLines, nullptr,0,0,"", nullptr,0,0, &ESP::color::Snaplines);
            SettingRowGrid("Snapline Outline", &ESP::SnaplineOutline);
            SettingRowGrid("Snapline Thickness", nullptr, nullptr,0.f,0.f,"", &ESP::SnaplineThickness, 1, 5);
            const char* sm[] = { "Off", "Bottom", "Middle", "Top" };
            SettingRowGrid("Snapline Mode", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &ESP::SnaplineMode, sm, 4);
            SettingRowGrid("OFF Arrows", &ESP::OFFArrows);
            SettingRowGrid("Name", &ESP::Name, nullptr,0,0,"", nullptr,0,0, &ESP::color::Name);
            SettingRowGrid("Distance", &ESP::Distance, nullptr,0,0,"", nullptr,0,0, &ESP::color::Distance);
            SettingRowGrid("Held Item", &ESP::Weapon, nullptr,0,0,"", nullptr,0,0, &ESP::color::Weapon);
            SettingRowGrid("Team ID", &ESP::TeamID);
            SettingRowGrid("Hotbar Text", &ESP::hotbar_text);
            SettingRowGrid("Inventory Panel", &ESP::PlayerInventoryPanel);
        }, CardHeight(10, true));
        ImGui::EndChild();
        ImGui::EndChild(); // close ##espPlayerScroll
    } else if (g_State.espSubPage == 1) {
        // ── NPC ESP ──
        auto cols = BeginTwoCol(CARD_GAP, 0.6f);
        int npcRows = 9 + (NPC_ESP::ESPAdvanced ? 5 : 0);
        ImGui::BeginChild("##npcL", ImVec2(cols.leftW, 0), false);
        Card("NPC Visuals", [&]() {
            SettingRowGrid("Enable NPC ESP", &NPC_ESP::Enabled);
            SettingRowGrid("Name", &NPC_ESP::Name, nullptr,0,0,"", nullptr,0,0, &NPC_ESP::color::Name);
            SettingRowGrid("Distance", &NPC_ESP::Distance, nullptr,0,0,"", nullptr,0,0, &NPC_ESP::color::Distance);
            SettingRowGrid("Skeleton", &NPC_ESP::Skeleton);
            SettingRowGrid("Held Item", &NPC_ESP::HeldItem);
            SettingRowGrid("Snaplines", &NPC_ESP::SnapLines);
            const char* sm[] = { "Off", "Bottom", "Middle", "Top" };
            SettingRowGrid("Snapline Mode", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &NPC_ESP::SnaplineMode, sm, 4);
            SettingRowGrid("Draw Distance", nullptr, &NPC_ESP::draw_distance, 0.f, 300.f, "%.0f m");
            SettingRowGrid("Advanced Distances", &NPC_ESP::ESPAdvanced);
            if (NPC_ESP::ESPAdvanced) {
                SettingRowGrid("NPC Name Dist", nullptr, &NPC_ESP::NPCNameDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("NPC DistText Dist", nullptr, &NPC_ESP::NPCDistTextDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("NPC Skeleton Dist", nullptr, &NPC_ESP::NPCSkeletonDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("NPC Held Dist", nullptr, &NPC_ESP::NPCHeldItemDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("NPC Snapline Dist", nullptr, &NPC_ESP::NPCSnaplineDist, 10.f, 300.f, "%.0f m");
            }
        }, CardHeight(npcRows, true));
        ImGui::EndChild();
        ImGui::SameLine(0, cols.gap);
        ImGui::BeginChild("##npcR", ImVec2(cols.rightW, 0), false);
        Card("NPC Types", [&]() {
            SettingRowGrid("Scientist", &NPC_ESP::Scientists);
            SettingRowGrid("Underwater Dweller", &NPC_ESP::UnderwaterDweller);
            SettingRowGrid("Tunnel Dweller", &NPC_ESP::TunnelDweller);
        }, CardHeight(3, true));
        ImGui::EndChild();
    } else if (g_State.espSubPage == 2) {
        // ── Animal ESP ──
        auto cols = BeginTwoCol(CARD_GAP, 0.6f);
        int aniRows = 9 + (ANIMAL_ESP::ESPAdvanced ? 5 : 0);
        ImGui::BeginChild("##aniL", ImVec2(cols.leftW, 0), false);
        Card("Animal Visuals", [&]() {
            SettingRowGrid("Enable Animal ESP", &ANIMAL_ESP::Enabled);
            SettingRowGrid("Box", &ANIMAL_ESP::Box, nullptr,0,0,"", nullptr,0,0, &ANIMAL_ESP::color::Box);
            SettingRowGrid("Name", &ANIMAL_ESP::Name, nullptr,0,0,"", nullptr,0,0, &ANIMAL_ESP::color::Name);
            SettingRowGrid("Distance", &ANIMAL_ESP::Distance, nullptr,0,0,"", nullptr,0,0, &ANIMAL_ESP::color::Distance);
            SettingRowGrid("Health Bar", &ANIMAL_ESP::HealthBar);
            SettingRowGrid("Snaplines", &ANIMAL_ESP::SnapLines);
            const char* sm[] = { "Off", "Bottom", "Middle", "Top" };
            SettingRowGrid("Snapline Mode", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &ANIMAL_ESP::SnaplineMode, sm, 4);
            SettingRowGrid("Draw Distance", nullptr, &ANIMAL_ESP::draw_distance, 0.f, 300.f, "%.0f m");
            SettingRowGrid("Advanced Distances", &ANIMAL_ESP::ESPAdvanced);
            if (ANIMAL_ESP::ESPAdvanced) {
                SettingRowGrid("Animal Box Dist", nullptr, &ANIMAL_ESP::AnimalBoxDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Animal Name Dist", nullptr, &ANIMAL_ESP::AnimalNameDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Animal DistText Dist", nullptr, &ANIMAL_ESP::AnimalDistTextDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Animal HP Dist", nullptr, &ANIMAL_ESP::AnimalHealthBarDist, 10.f, 300.f, "%.0f m");
                SettingRowGrid("Animal Snapline Dist", nullptr, &ANIMAL_ESP::AnimalSnaplineDist, 10.f, 300.f, "%.0f m");
            }
        }, CardHeight(aniRows, true));
        ImGui::EndChild();
        ImGui::SameLine(0, cols.gap);
        ImGui::BeginChild("##aniR", ImVec2(cols.rightW, 0), false);
        Card("Animal Types", [&]() {
            SettingRowGrid("Bear", &ANIMAL_ESP::Bear);
            SettingRowGrid("Wolf", &ANIMAL_ESP::Wolf);
            SettingRowGrid("Boar", &ANIMAL_ESP::Boar);
            SettingRowGrid("Stag", &ANIMAL_ESP::Stag);
            SettingRowGrid("Chicken", &ANIMAL_ESP::Chicken);
            SettingRowGrid("Horse", &ANIMAL_ESP::Horse);
            SettingRowGrid("Polar Bear", &ANIMAL_ESP::PolarBear);
            SettingRowGrid("Panther", &ANIMAL_ESP::Panther);
            SettingRowGrid("Tiger", &ANIMAL_ESP::Tiger);
            SettingRowGrid("Snake", &ANIMAL_ESP::Snake);
        }, CardHeight(10, true));
        ImGui::EndChild();
    } else {
        // ── World ESP ──
        float scrollH = contentH;
        ImGui::BeginChild("##worldScroll", ImVec2(0, scrollH), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        if (ImGui::Button("Disable All World ESP", ImVec2(180, 28))) {
            WORLD::Hemp = false; WORLD::Distance = false; WORLD::Stash = false; WORLD::BodyBag = false;
            WORLD::Turret = false; WORLD::Stone = false; WORLD::Metal = false; WORLD::Sulfer = false;
            WORLD::DroppedItem = false; WORLD::ShotGunTrap = false; WORLD::MiniCopter = false;
            WORLD::Backpack = false; WORLD::BradlyAPC = false; WORLD::StonePickup = false;
            WORLD::MetalPickup = false; WORLD::SulfurPickup = false; WORLD::WoodPickup = false;
            WORLD::SamSite = false; WORLD::BearTrap = false; WORLD::Landmine = false;
            WORLD::FlameTurret = false; WORLD::Lockers = false; WORLD::Bags = false;
            WORLD::Beds = false; WORLD::TC = false; WORLD::Vending = false; WORLD::Workbench = false;
            WORLD::LargeStorage = false; WORLD::Ladder = false; WORLD::Generator = false;
            WORLD::Battery = false; WORLD::BarrelBeige = false; WORLD::BarrelBlue = false;
            WORLD::BarrelFuel = false; WORLD::CrateNormal = false; WORLD::CrateMilitary = false;
            WORLD::CrateElite = false; WORLD::CrateLocked = false; WORLD::CrateMedical = false;
            WORLD::CrateFood = false; WORLD::Rowboat = false; WORLD::RHIB = false;
            WORLD::Kayak = false; WORLD::Tugboat = false; WORLD::Submarine = false;
            WORLD::TransportHeli = false; WORLD::AttackHeli = false; WORLD::Balloon = false;
            WORLD::Motorbike = false; WORLD::MotorbikeSidecar = false; WORLD::Trike = false;
            WORLD::Bicycle = false; WORLD::Snowmobile = false; WORLD::Shark = false;
            WORLD::CargoShip = false; WORLD::SupplyDrop = false;
        }
        ImGui::Dummy(ImVec2(0, CARD_GAP));

        // Calculate column heights
        float leftColH = CardHeight(4, true, true) + CardHeight(4, true) + CardHeight(4, true) + CardHeight(6, true) + CardHeight(2, true) + 5 * CARD_GAP;
        float vehCardH = 380.0f;
        float rightColH = vehCardH + CardHeight(9, true) + CardHeight(10, true) + 3 * CARD_GAP;
        float colH = (std::max)(leftColH, rightColH);

        float availW = ImGui::GetContentRegionAvail().x;
        float cardW = (availW - CARD_GAP) * 0.5f;
        ImGui::BeginChild("##worldL", ImVec2(cardW, colH), false);
        Card("Resources", [&]() {
            WorldEntityRow("Stone Ore", &WORLD::Stone, &WORLD::draw_stone, WORLD::color::Stone_Color);
            WorldEntityRow("Metal Ore", &WORLD::Metal, &WORLD::draw_metal, WORLD::color::Metal_Color);
            WorldEntityRow("Sulfur Ore", &WORLD::Sulfer, &WORLD::draw_sulfur, WORLD::color::Sulfer_Color);
            WorldEntityRow("Hemp", &WORLD::Hemp, &WORLD::draw_hemp, WORLD::color::Hemp_Color);
        }, CardHeight(4, true, true), (WORLD::Stone?1:0)+(WORLD::Metal?1:0)+(WORLD::Sulfer?1:0)+(WORLD::Hemp?1:0), "Resource ESP visibility, range, and color.");
        Card("Pickups", [&]() {
            WorldEntitySimple("Stone Pkup", &WORLD::StonePickup, WORLD::color::StonePickup);
            WorldEntitySimple("Metal Pkup", &WORLD::MetalPickup, WORLD::color::MetalPickup);
            WorldEntitySimple("Sulfur Pkup", &WORLD::SulfurPickup, WORLD::color::SulfurPickup);
            WorldEntitySimple("Wood Pkup", &WORLD::WoodPickup, WORLD::color::WoodPickup);
        }, CardHeight(4, true), (WORLD::StonePickup?1:0)+(WORLD::MetalPickup?1:0)+(WORLD::SulfurPickup?1:0)+(WORLD::WoodPickup?1:0));
        Card("Loot", [&]() {
            WorldEntityRow("Stash", &WORLD::Stash, &WORLD::draw_stash, WORLD::color::Stash_Color);
            WorldEntityRow("Corpse", &WORLD::BodyBag, &WORLD::draw_corpse, WORLD::color::BodyBag_Color);
            WorldEntityRow("Backpack", &WORLD::Backpack, &WORLD::draw_backpack, WORLD::color::Backpack_Color);
            WorldEntityRow("Dropped Items", &WORLD::DroppedItem, &WORLD::draw_dropped, WORLD::color::DroppedItem_Color);
        }, CardHeight(4, true), (WORLD::Stash?1:0)+(WORLD::BodyBag?1:0)+(WORLD::Backpack?1:0)+(WORLD::DroppedItem?1:0));
        Card("Traps", [&]() {
            WorldEntityRow("Auto Turret", &WORLD::Turret, &WORLD::draw_turret, WORLD::color::Turret_Color);
            WorldEntityRow("Shotgun Trap", &WORLD::ShotGunTrap, &WORLD::draw_shotguntrap, WORLD::color::ShotGunTrap_Color);
            WorldEntityRow("Sam Site", &WORLD::SamSite, &WORLD::draw_samsite, WORLD::color::SamSite);
            WorldEntityRow("Bear Trap", &WORLD::BearTrap, &WORLD::draw_beartrap, WORLD::color::BearTrap);
            WorldEntityRow("Landmine", &WORLD::Landmine, &WORLD::draw_landmine, WORLD::color::Landmine);
            WorldEntityRow("Flame Turret", &WORLD::FlameTurret, &WORLD::draw_flameturret, WORLD::color::FlameTurret);
        }, CardHeight(6, true), (WORLD::Turret?1:0)+(WORLD::ShotGunTrap?1:0)+(WORLD::SamSite?1:0)+(WORLD::BearTrap?1:0)+(WORLD::Landmine?1:0)+(WORLD::FlameTurret?1:0));
        Card("Display", [&]() {
            SettingRowGrid("Show Distance", &WORLD::Distance);
            SettingRowGrid("Global World Draw", nullptr, &WORLD::draw_distance, 0.f, 500.f, "%.0f m");
        }, CardHeight(2, true));
        ImGui::EndChild();

        ImGui::SameLine(0, CARD_GAP);
        ImGui::BeginChild("##worldR", ImVec2(0, colH), false);
        Card("Vehicles", [&]() {
            float innerH = vehCardH - PANEL_HEADER_H - 20;
            ImGui::BeginChild("##vehInner", ImVec2(0, innerH), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            WorldEntityRow("MiniCopter", &WORLD::MiniCopter, &WORLD::draw_minicopter, WORLD::color::MiniCopter_Color);
            WorldEntityRow("Bradley APC", &WORLD::BradlyAPC, &WORLD::draw_bradley, WORLD::color::BradlyAPC);
            WorldEntityRow("Rowboat", &WORLD::Rowboat, &WORLD::draw_rowboat, WORLD::color::Rowboat);
            WorldEntityRow("RHIB", &WORLD::RHIB, &WORLD::draw_rhib, WORLD::color::RHIB);
            WorldEntityRow("Kayak", &WORLD::Kayak, &WORLD::draw_kayak, WORLD::color::Kayak);
            WorldEntityRow("Submarine", &WORLD::Submarine, &WORLD::draw_submarine, WORLD::color::Submarine);
            WorldEntityRow("Tugboat", &WORLD::Tugboat, &WORLD::draw_tugboat, WORLD::color::Tugboat);
            WorldEntityRow("Transport Heli", &WORLD::TransportHeli, &WORLD::draw_transportheli, WORLD::color::TransportHeli);
            WorldEntityRow("Attack Heli", &WORLD::AttackHeli, &WORLD::draw_attackheli, WORLD::color::AttackHeli);
            WorldEntityRow("Balloon", &WORLD::Balloon, &WORLD::draw_balloon, WORLD::color::Balloon);
            WorldEntityRow("Motorbike", &WORLD::Motorbike, &WORLD::draw_motorbike, WORLD::color::Motorbike);
            WorldEntityRow("Snowmobile", &WORLD::Snowmobile, &WORLD::draw_snowmobile, WORLD::color::Snowmobile);
            WorldEntitySimple("Sidecar", &WORLD::MotorbikeSidecar, WORLD::color::Sidecar);
            WorldEntitySimple("Trike", &WORLD::Trike, WORLD::color::Trike);
            WorldEntitySimple("Bicycle", &WORLD::Bicycle, WORLD::color::Bicycle);
            WorldEntitySimple("Shark", &WORLD::Shark, WORLD::color::Shark);
            WorldEntitySimple("Cargo Ship", &WORLD::CargoShip, WORLD::color::CargoShip);
            WorldEntitySimple("Supply Drop", &WORLD::SupplyDrop, WORLD::color::SupplyDrop);
            ImGui::EndChild();
        }, vehCardH, (WORLD::MiniCopter?1:0)+(WORLD::BradlyAPC?1:0)+(WORLD::Rowboat?1:0)+(WORLD::RHIB?1:0)+(WORLD::Kayak?1:0)+(WORLD::Submarine?1:0)+(WORLD::Tugboat?1:0)+(WORLD::TransportHeli?1:0)+(WORLD::AttackHeli?1:0)+(WORLD::Balloon?1:0)+(WORLD::Motorbike?1:0)+(WORLD::Snowmobile?1:0), "Vehicle ESP visibility, range, and color.");
        Card("Barrels & Crates", [&]() {
            WorldEntityRow("Beige Barrel", &WORLD::BarrelBeige, &WORLD::draw_barrelbeige, WORLD::color::BarrelBeige);
            WorldEntityRow("Blue Barrel", &WORLD::BarrelBlue, &WORLD::draw_barrelblue, WORLD::color::BarrelBlue);
            WorldEntityRow("Fuel Barrel", &WORLD::BarrelFuel, &WORLD::draw_barrelfuel, WORLD::color::BarrelFuel);
            WorldEntityRow("Normal Crate", &WORLD::CrateNormal, &WORLD::draw_crate_normal, WORLD::color::NormalCrate);
            WorldEntityRow("Military Crate", &WORLD::CrateMilitary, &WORLD::draw_crate_military, WORLD::color::MilitaryCrate);
            WorldEntityRow("Elite Crate", &WORLD::CrateElite, &WORLD::draw_crate_elite, WORLD::color::EliteCrate);
            WorldEntityRow("Locked Crate", &WORLD::CrateLocked, &WORLD::draw_crate_locked, WORLD::color::LockedCrate);
            WorldEntitySimple("Med Crate", &WORLD::CrateMedical, WORLD::color::MedicalCrate);
            WorldEntitySimple("Food Crate", &WORLD::CrateFood, WORLD::color::FoodCrate);
        }, CardHeight(9, true), (WORLD::BarrelBeige?1:0)+(WORLD::BarrelBlue?1:0)+(WORLD::BarrelFuel?1:0)+(WORLD::CrateNormal?1:0)+(WORLD::CrateMilitary?1:0)+(WORLD::CrateElite?1:0)+(WORLD::CrateLocked?1:0)+(WORLD::CrateMedical?1:0)+(WORLD::CrateFood?1:0));
        Card("Deployables", [&]() {
            WorldEntityRow("Lockers", &WORLD::Lockers, &WORLD::draw_lockers, WORLD::color::Lockers);
            WorldEntityRow("Sleeping Bags", &WORLD::Bags, &WORLD::draw_bags, WORLD::color::Bags);
            WorldEntityRow("Beds", &WORLD::Beds, &WORLD::draw_beds, WORLD::color::Beds);
            WorldEntityRow("Tool Cupboard", &WORLD::TC, &WORLD::draw_tc, WORLD::color::TC);
            WorldEntityRow("Vending Machine", &WORLD::Vending, &WORLD::draw_vending, WORLD::color::Vending);
            WorldEntitySimple("Workbench", &WORLD::Workbench, WORLD::color::Workbench);
            WorldEntitySimple("Large Storage", &WORLD::LargeStorage, WORLD::color::LargeStorage);
            WorldEntitySimple("Ladder", &WORLD::Ladder, WORLD::color::Ladder);
            WorldEntitySimple("Generator", &WORLD::Generator, WORLD::color::Generator);
            WorldEntitySimple("Battery", &WORLD::Battery, WORLD::color::Battery);
        }, CardHeight(10, true), (WORLD::Lockers?1:0)+(WORLD::Bags?1:0)+(WORLD::Beds?1:0)+(WORLD::TC?1:0)+(WORLD::Vending?1:0)+(WORLD::Workbench?1:0)+(WORLD::LargeStorage?1:0)+(WORLD::Ladder?1:0)+(WORLD::Generator?1:0)+(WORLD::Battery?1:0));
        ImGui::EndChild();
        ImGui::EndChild();
    }
}

// ── Radar — 4 cards, 2-col ──
inline void PageRadar() {
    auto cols = BeginTwoCol(CARD_GAP, 0.5f);
    ImGui::BeginChild("##radarL", ImVec2(cols.leftW, 0), false);
    Card("Radar Display", [&]() {
        SettingRowGrid("Enable Radar", &RADAR::Enabled);
        SettingRowGrid("Size", nullptr, &RADAR::Size, 50.f, 400.f, "%.0f");
        SettingRowGrid("Scale", nullptr, &RADAR::Scale, 0.5f, 5.f, "%.1f");
        SettingRowGrid("Dot Size", nullptr, &RADAR::DotSize, 2.f, 10.f, "%.1f");
        const char* rpos[] = { "Top Left", "Top Right", "Bottom Left", "Bottom Right" };
        SettingRowGrid("Position", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &RADAR::Position, rpos, 4);
    }, CardHeight(5, true));
    Card("Radar Grid", [&]() {
        SettingRowGrid("Grid", &RADAR::ShowGrid);
        SettingRowGrid("Rotate", &RADAR::Rotate);
        SettingRowGrid("Background Opacity", nullptr, &RADAR::BackgroundOpacity, 0.1f, 1.0f, "%.2f");
    }, CardHeight(3, true));
    ImGui::EndChild();
    ImGui::SameLine(0, cols.gap);
    ImGui::BeginChild("##radarR", ImVec2(cols.rightW, 0), false);
    Card("Categories", [&]() {
        SettingRowGrid("Players", &RADAR::ShowPlayers);
        SettingRowGrid("NPCs", &RADAR::ShowNPCs);
        SettingRowGrid("Animals", &RADAR::ShowAnimals);
        SettingRowGrid("Hide Sleepers", &RADAR::HideSleepers);
        SettingRowGrid("Remove Team", &RADAR::RemoveTeam);
    }, CardHeight(5, true));
    Card("Colors", [&]() {
        SettingRowGrid("Player Color", nullptr, nullptr,0,0,"", nullptr,0,0, &RADAR::PlayerColor);
        SettingRowGrid("Teammate Color", nullptr, nullptr,0,0,"", nullptr,0,0, &RADAR::TeammateColor);
        SettingRowGrid("NPC Color", nullptr, nullptr,0,0,"", nullptr,0,0, &RADAR::NpcColor);
        SettingRowGrid("Animal Color", nullptr, nullptr,0,0,"", nullptr,0,0, &RADAR::AnimalColor);
        SettingRowGrid("Border Color", nullptr, nullptr,0,0,"", nullptr,0,0, &RADAR::BorderColor);
    }, CardHeight(5, true));
    ImGui::EndChild();
}

// ── Visuals — 3 auto-height cards, 2-col ──
inline void PageVisuals() {
    auto cols = BeginTwoCol(CARD_GAP, 0.55f);
    ImGui::BeginChild("##visL", ImVec2(cols.leftW, 0), false);
    Card("Camera", [&]() {
        SettingRowGrid("FOV Changer", &MISC::FovChanger, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, "VISUAL.FovChanger", "Writes camera/FOV values. Higher detection risk.", true);
        SettingRowGrid("FOV", nullptr, &MISC::FovAmount, 1.f, 200.f, "%.0f");
        SettingRowGrid("Zoom", &MISC::Zoom, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, "VISUAL.Zoom", "FOV manipulation writes while active.", true);
        SettingRowGrid("Zoom Amount", nullptr, &MISC::ZoomAmount, 1.f, 200.f, "%.0f");
    }, CardHeight(4, true));
    Card("Crosshair & Display", [&]() {
        const char* cross[] = { "Off", "Dot", "Cross", "Circle" };
        SettingRowGrid("Crosshair", nullptr, nullptr,0,0,"", nullptr,0,0, nullptr, &MISC::CrosshairStyle, cross, 4);
        SettingRowGrid("Crosshair Size", nullptr, &MISC::CrosshairSize, 1.f, 10.f, "%.1f");
        SettingRowGrid("Crosshair Color", nullptr, nullptr,0,0,"", nullptr,0,0, &MISC::CrosshairColor);
        SettingRowGrid("Screen Info", &MISC::DrawFlags);
    }, CardHeight(4, true));
    ImGui::EndChild();
    ImGui::SameLine(0, cols.gap);
    ImGui::BeginChild("##visR", ImVec2(cols.rightW, 0), false);
    Card("Ambient", [&]() {
        SettingRowGrid("Bright Night", &MISC::BrightNight);
        SettingRowGrid("Ambient Multiplier", nullptr, &MISC::ambientMultiplier, 0.0f, 20.0f, "%.2f");
        SettingRowGrid("Ambient Saturation", nullptr, &MISC::AmbientSaturation, 0.0f, 5.0f, "%.2f");
        SettingRowGrid("Time Changer", &MISC::Timechanger);
        SettingRowGrid("Time Value", nullptr, nullptr, 0.f, 0.f, "", &MISC::timevalue, 0, 24);
    }, CardHeight(5, true));
    ImGui::EndChild();
}

// ── Utility — 3 compact cards, 2-col ──
inline void PageUtility() {
    auto cols = BeginTwoCol(CARD_GAP, 0.5f);
    ImGui::BeginChild("##utiL", ImVec2(cols.leftW, 0), false);
    Card("Tools", [&]() {
        SettingRowGrid("Admin Flags", &MISC::AdminFlags, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, "UTIL.AdminFlags", "Writes IsAdmin flag. Very high risk.", true);
        SettingRowGrid("Battle Mode", &SETTINGS::BattleMode, nullptr,0,0,"", nullptr,0,0, nullptr, nullptr,nullptr,0, "UTIL.BattleMode");
    }, CardHeight(2, true));
    Card("Debug Camera", [&]() {
        if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_SEC);
        ImGui::TextWrapped("Bind UTIL.DebugCam in Config > Keybinds.");
        ImGui::Dummy(ImVec2(0, 2));
        ImGui::TextWrapped("- Requires Admin Flags");
        ImGui::TextWrapped("- 50s timeout, auto-disables");
        ImGui::PopStyleColor();
        if (BeaztFontDesc) ImGui::PopFont();
    }, 130.0f);
    ImGui::EndChild();
    ImGui::SameLine(0, cols.gap);
    ImGui::BeginChild("##utiR", ImVec2(cols.rightW, 0), false);
    Card("Battle Mode Filters", [&]() {
        SettingRowGrid("Players ESP", &BATTLE::Players);
        SettingRowGrid("Aimbot", &BATTLE::Aimbot);
        SettingRowGrid("World ESP", &BATTLE::World);
        SettingRowGrid("NPC ESP", &BATTLE::NPC);
        SettingRowGrid("Animal ESP", &BATTLE::Animals);
        SettingRowGrid("Radar", &BATTLE::Radar);
    }, CardHeight(6, true));
    ImGui::EndChild();
}

// ── Config — compact About, button grid, 2-col ──
inline void PageConfig() {
    RefreshConfigList();
    float contentH = ImGui::GetContentRegionAvail().y;
    auto cols = BeginTwoCol(CARD_GAP, 0.6f);
    float cfgMgmtH = 300.0f;
    float kbH = contentH - cfgMgmtH - CARD_GAP;
    ImGui::BeginChild("##cfgL", ImVec2(cols.leftW, contentH), false);
    Card("Config Management", [&]() {
        const char* cfgPreview = "default";
        if (g_State.selectedConfigIndex >= 0 && g_State.selectedConfigIndex < (int)g_State.configNames.size())
            cfgPreview = g_State.configNames[g_State.selectedConfigIndex].c_str();
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - CARD_PAD_R);
        if (ImGui::BeginCombo("##cfg_pick", cfgPreview)) {
            for (int i = 0; i < (int)g_State.configNames.size(); i++) {
                bool sel = (i == g_State.selectedConfigIndex);
                if (ImGui::Selectable(g_State.configNames[i].c_str(), sel)) {
                    g_State.selectedConfigIndex = i;
                    lstrcpynA(g_State.newConfigName, g_State.configNames[i].c_str(), 63);
                    g_State.newConfigName[63] = 0;
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::Dummy(ImVec2(0, 6));
        float btnW = (ImGui::GetContentRegionAvail().x - CARD_PAD_R - 8) * 0.5f;
        ImGui::PushItemWidth(btnW);
        ImGui::InputTextWithHint("##cfg_name", "Config name...", g_State.newConfigName, 64);
        ImGui::PopItemWidth();
        ImGui::SameLine(0, 8);
        if (ImGui::Button("Create", ImVec2(btnW, 28))) { if (SaveNamedConfig(g_State.newConfigName)) RefreshConfigList(true); }
        ImGui::Dummy(ImVec2(0, 6));
        if (ImGui::Button("Save", ImVec2(btnW, 28))) { if (SaveNamedConfig(g_State.newConfigName)) RefreshConfigList(true); }
        ImGui::SameLine(0, 8);
        if (ImGui::Button("Load", ImVec2(btnW, 28))) {
            const char* t = g_State.newConfigName;
            if (!t[0] && g_State.selectedConfigIndex >= 0 && g_State.selectedConfigIndex < (int)g_State.configNames.size())
                t = g_State.configNames[g_State.selectedConfigIndex].c_str();
            if (LoadNamedConfig(t)) RefreshConfigList(true);
        }
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0xFF, 0x4D, 0x6D, 51));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0xFF, 0x4D, 0x6D, 82));
        if (ImGui::Button("Delete", ImVec2(btnW, 28))) {
            const char* t = g_State.newConfigName;
            if (!t[0] && g_State.selectedConfigIndex >= 0 && g_State.selectedConfigIndex < (int)g_State.configNames.size())
                t = g_State.configNames[g_State.selectedConfigIndex].c_str();
            DeleteNamedConfig(t); RefreshConfigList(true);
        }
        ImGui::PopStyleColor(2);
        ImGui::Dummy(ImVec2(0, 6));
        SettingRowGrid("Auto Save", &g_State.autoSave);
        ImGui::Dummy(ImVec2(0, 4));
        if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
        ImGui::TextWrapped("Path: %s", Config::GetConfigPathForName(g_State.activeConfig).c_str());
        ImGui::PopStyleColor();
        if (BeaztFontDesc) ImGui::PopFont();
        // Config status confirmation
        if (Config::lastStatusTime > 0) {
            uint64_t now = GetTickCount64();
            if (now - Config::lastStatusTime < 5000) {
                if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
                bool isErr = Config::lastStatus.find("fail") != std::string::npos || Config::lastStatus.find("invalid") != std::string::npos;
                ImGui::PushStyleColor(ImGuiCol_Text, isErr ? IM_COL32(0xFF, 0x4D, 0x6D, 255) : IM_COL32(0x4C, 0xAF, 0x50, 255));
                ImGui::TextWrapped("%s", Config::lastStatus.c_str());
                ImGui::PopStyleColor();
                if (BeaztFontDesc) ImGui::PopFont();
            }
        }
    }, cfgMgmtH);
    Card("Keybinds", [&]() {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.4f);
        ImGui::InputTextWithHint("##kbs", "Search binds...", g_State.keybindSearch, 96);
        ImGui::PopItemWidth();
        ImGui::SameLine(0, 6);
        const char* pf[] = { "All", "Global", "Combat", "ESP", "NPC", "Animal", "World", "Visuals", "Utility", "Radar" };
        ImGui::PushItemWidth(80);
        ImGui::Combo("##kbpf", &g_State.keybindPageFilter, pf, 10);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Conflicts", ImVec2(64, 0))) g_State.showOnlyConflicts = !g_State.showOnlyConflicts;
        ImGui::SameLine();
        if (ImGui::Button("Clear All", ImVec2(64, 0))) {
            for (auto& [id, bind] : g_Hotkeys) { bind.key = 0; bind.mode = HK_DISABLED; }
            g_Hotkeys["global.menu"] = { VK_INSERT, HK_HOLD };
            g_Hotkeys["global.shutdown"] = { VK_END, HK_HOLD };
        }
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::BeginChild("##kbtbl", ImVec2(0, 0), true);
        ImGui::Columns(5, "kbcols");
        ImGui::SetColumnWidth(0, 140); ImGui::Text("Feature"); ImGui::NextColumn();
        ImGui::SetColumnWidth(1, 70); ImGui::Text("Page"); ImGui::NextColumn();
        ImGui::SetColumnWidth(2, 60); ImGui::Text("Key"); ImGui::NextColumn();
        ImGui::SetColumnWidth(3, 55); ImGui::Text("Mode"); ImGui::NextColumn();
        ImGui::SetColumnWidth(4, 50); ImGui::Text(""); ImGui::NextColumn();
        auto GetPage = [](const std::string& id) -> const char* {
            if (id.find("global") == 0) return "Global";
            if (id.find("COMBAT") == 0) return "Combat";
            if (id.find("ESP") == 0) return "ESP";
            if (id.find("NPC") == 0) return "NPC";
            if (id.find("ANIMAL") == 0) return "Animal";
            if (id.find("WORLD") == 0) return "World";
            if (id.find("VISUAL") == 0) return "Visuals";
            if (id.find("UTIL") == 0) return "Utility";
            if (id.find("RADAR") == 0) return "Radar";
            return "Other";
        };
        for (auto& [id, bind] : g_Hotkeys) {
            if (bind.key == 0) continue;
            bool match = (g_State.keybindSearch[0] == 0 || id.find(g_State.keybindSearch) != std::string::npos);
            if (!match) continue;
            bool conflict = HasHotkeyConflict(id, bind.key);
            if (g_State.showOnlyConflicts && !conflict) continue;
            const char* pageName = GetPage(id);
            if (g_State.keybindPageFilter > 0 && lstrcmpiA(pageName, pf[g_State.keybindPageFilter]) != 0) continue;
            ImGui::Text("%s", id.c_str()); ImGui::NextColumn();
            ImGui::Text("%s", pageName); ImGui::NextColumn();
            if (conflict) ImGui::PushStyleColor(ImGuiCol_Text, C_DANGER);
            ImGui::Text("%s", GetKeyName(bind.key));
            if (conflict) ImGui::PopStyleColor();
            ImGui::NextColumn();
            ImGui::PushID((id + "_mode").c_str());
            if (ImGui::SmallButton(GetModeName(bind.mode))) { int n = bind.mode + 1; if (n > HK_ALWAYS) n = HK_TOGGLE; bind.mode = n; }
            ImGui::PopID(); ImGui::NextColumn();
            ImGui::PushID(id.c_str());
            if (ImGui::SmallButton("X")) { bind.key = 0; bind.mode = HK_DISABLED; }
            ImGui::PopID(); ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::EndChild();
    }, kbH);
    ImGui::EndChild();
    ImGui::SameLine(0, cols.gap);
    ImGui::BeginChild("##cfgR", ImVec2(cols.rightW, contentH), false);
    Card("About", [&]() {
        if (BeaztFontTitle) ImGui::PushFont(BeaztFontTitle);
        ImGui::PushStyleColor(ImGuiCol_Text, C_ACCENT);
        ImGui::TextUnformatted("THE BEAZT");
        ImGui::PopStyleColor();
        if (BeaztFontTitle) ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 4));
        if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_SEC);
        ImGui::TextUnformatted("Private Legit");
        ImGui::TextUnformatted("Version: v9.0");
        ImGui::TextUnformatted("Build Date:");
        ImGui::SameLine(0, 4);
        ImGui::TextUnformatted(__DATE__);
        ImGui::Dummy(ImVec2(0, 6));
        ImGui::PushStyleColor(ImGuiCol_Text, C_TEXT_MUTED);
        ImGui::TextUnformatted("Menu Key: INSERT");
        ImGui::TextUnformatted("Exit Key: END");
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        if (BeaztFontDesc) ImGui::PopFont();
    }, 160.0f);
    ImGui::EndChild();
}

// ============================================================================
// THEME
// ============================================================================
inline void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = SHELL_RADIUS; s.ChildRounding = CARD_RADIUS; s.FrameRounding = 6.f;
    s.ScrollbarRounding = 999.f; s.GrabRounding = 999.f; s.PopupRounding = 8.f; s.TabRounding = 6.f;
    s.WindowPadding = ImVec2(OUTER_PAD, OUTER_PAD);
    s.FramePadding = ImVec2(6, 4);
    s.ItemSpacing = ImVec2(6, 4); s.ItemInnerSpacing = ImVec2(4, 3);
    s.FrameBorderSize = 0.f; s.WindowBorderSize = 1.f; s.ChildBorderSize = 0.f;
    s.ScrollbarSize = 6.f; s.GrabMinSize = 16.f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = C_BG_MENU; c[ImGuiCol_ChildBg] = ImVec4(0,0,0,0); c[ImGuiCol_PopupBg] = C_BG_CARD;
    c[ImGuiCol_Border] = C_BORDER; c[ImGuiCol_BorderShadow] = ImVec4(0,0,0,0);
    c[ImGuiCol_FrameBg] = C_BG_INPUT; c[ImGuiCol_FrameBgHovered] = C_BG_HOVER; c[ImGuiCol_FrameBgActive] = C_BG_ACTIVE;
    c[ImGuiCol_Button] = C_BG_CARD; c[ImGuiCol_ButtonHovered] = C_BG_HOVER; c[ImGuiCol_ButtonActive] = C_ACCENT;
    c[ImGuiCol_Header] = C_BG_CARD; c[ImGuiCol_HeaderHovered] = C_BG_HOVER; c[ImGuiCol_HeaderActive] = C_BG_ACTIVE;
    c[ImGuiCol_CheckMark] = C_ACCENT;
    c[ImGuiCol_SliderGrab] = C_ACCENT; c[ImGuiCol_SliderGrabActive] = C_ACCENT_HOVER;
    c[ImGuiCol_Tab] = C_BG_CARD; c[ImGuiCol_TabHovered] = C_BG_HOVER; c[ImGuiCol_TabActive] = C_BG_ACTIVE;
    c[ImGuiCol_TabUnfocused] = C_BG_CARD; c[ImGuiCol_TabUnfocusedActive] = C_BG_HOVER;
    c[ImGuiCol_Separator] = C_BORDER_SOFT; c[ImGuiCol_SeparatorHovered] = C_ACCENT; c[ImGuiCol_SeparatorActive] = C_ACCENT;
    c[ImGuiCol_ScrollbarBg] = ImVec4(0,0,0,0); c[ImGuiCol_ScrollbarGrab] = C_BORDER_MED;
    c[ImGuiCol_ScrollbarGrabHovered] = C_ACCENT; c[ImGuiCol_ScrollbarGrabActive] = C_ACCENT_HOVER;
    c[ImGuiCol_Text] = C_TEXT; c[ImGuiCol_TextDisabled] = C_TEXT_MUTED; c[ImGuiCol_TextSelectedBg] = C_BG_ACTIVE;
    c[ImGuiCol_NavHighlight] = ImVec4(0,0,0,0); c[ImGuiCol_NavWindowingHighlight] = ImVec4(0,0,0,0);
    c[ImGuiCol_TitleBg] = C_BG_APP; c[ImGuiCol_TitleBgActive] = C_BG_APP; c[ImGuiCol_TitleBgCollapsed] = C_BG_APP;
    c[ImGuiCol_ResizeGrip] = ImVec4(0,0,0,0); c[ImGuiCol_ResizeGripHovered] = C_ACCENT; c[ImGuiCol_ResizeGripActive] = C_ACCENT;
}

// ============================================================================
// MAIN RENDER
// ============================================================================
inline void Render() {
    ApplyTheme();
    if (BeaztFont) ImGui::PushFont(BeaztFont);

    ImGui::SetNextWindowSize(ImVec2(MENU_W, MENU_H), ImGuiCond_Once);
    ImGui::Begin("The BeaZt##beazt", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

    ImVec2 wPos = ImGui::GetWindowPos();
    ImVec2 wSize = ImGui::GetWindowSize();
    float wW = wSize.x;
    auto* dl = ImGui::GetWindowDrawList();

    // ===== SHELL — simple dark overlay, no glows =====
    dl->AddRectFilled(wPos, ImVec2(wPos.x + wW, wPos.y + wSize.y), IM_COL32(0x02, 0x05, 0x0C, 56), SHELL_RADIUS);
    dl->AddLine(ImVec2(wPos.x + 6, wPos.y + 2), ImVec2(wPos.x + wW - 6, wPos.y + 2), IM_COL32(255, 255, 255, 10), 1.0f);

    // ===== HEADER (32px) =====
    float headerY = wPos.y + 4.0f;
    float curX = wPos.x + OUTER_PAD;
    if (g_BeaztLogo) {
        dl->AddImage((ImTextureID)g_BeaztLogo, ImVec2(curX, headerY + 4), ImVec2(curX + 16, headerY + 20));
        curX += 20;
    }
    if (BeaztFontTitle) ImGui::PushFont(BeaztFontTitle);
    dl->AddText(ImVec2(curX, headerY + 5), ImGui::ColorConvertFloat4ToU32(C_TEXT), "BEAZT");
    if (BeaztFontTitle) ImGui::PopFont();

    // Build date right-aligned
    if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
    const char* buildText = __DATE__;
    ImVec2 buildSize = ImGui::CalcTextSize(buildText);
    dl->AddText(ImVec2(wPos.x + wW - OUTER_PAD - buildSize.x, headerY + 6), ImGui::ColorConvertFloat4ToU32(C_TEXT_MUTED), buildText);
    if (BeaztFontDesc) ImGui::PopFont();

    float headerBottom = wPos.y + HEADER_H;
    dl->AddLine(ImVec2(wPos.x + OUTER_PAD, headerBottom), ImVec2(wPos.x + wW - OUTER_PAD, headerBottom), ImGui::ColorConvertFloat4ToU32(C_BORDER), 1.0f);

    // ===== SIDEBAR (120px, compact) =====
    float sidebarX = wPos.x + OUTER_PAD;
    float sidebarY = headerBottom + SB_ITEM_GAP;
    float sidebarH = wSize.y - HEADER_H - FOOTER_H - 2 * OUTER_PAD;

    dl->AddRectFilled(ImVec2(sidebarX, sidebarY), ImVec2(sidebarX + SIDEBAR_W, sidebarY + sidebarH),
        ImGui::ColorConvertFloat4ToU32(C_BG_SIDEBAR), SB_RADIUS);
    dl->AddRect(ImVec2(sidebarX, sidebarY), ImVec2(sidebarX + SIDEBAR_W, sidebarY + sidebarH),
        ImGui::ColorConvertFloat4ToU32(C_BORDER), SB_RADIUS, 0, 1.0f);

    ImGui::SetCursorScreenPos(ImVec2(sidebarX + 6, sidebarY + 6));
    ImGui::BeginChild("##sb", ImVec2(SIDEBAR_W - 12, sidebarH - 12), false);

    const char* nav[] = { "Combat", "ESP", "Radar", "Visuals", "Utility", "Config" };
    float btnW = ImGui::GetContentRegionAvail().x;
    for (int i = 0; i < 6; i++) {
        bool sel = ((int)currentPage == i);
        ImVec2 bp = ImGui::GetCursorScreenPos();
        bool hover = ImGui::IsMouseHoveringRect(bp, ImVec2(bp.x + btnW, bp.y + SB_ITEM_H));

        if (sel) {
            dl->AddRectFilled(bp, ImVec2(bp.x + btnW, bp.y + SB_ITEM_H), IM_COL32(0x3B, 0x82, 0xF6, 200), SB_ITEM_RADIUS);
        } else if (hover) {
            dl->AddRectFilled(bp, ImVec2(bp.x + btnW, bp.y + SB_ITEM_H), IM_COL32(255, 255, 255, 10), SB_ITEM_RADIUS);
        }

        ImU32 textCol = sel ? IM_COL32(0xFF, 0xFF, 0xFF, 255) : (hover ? IM_COL32(0xE8, 0xEC, 0xF1, 255) : IM_COL32(0x8B, 0x95, 0xA7, 255));
        ImVec2 labelSize = ImGui::CalcTextSize(nav[i]);
        dl->AddText(ImVec2(bp.x + 8, bp.y + (SB_ITEM_H - labelSize.y) * 0.5f), textCol, nav[i]);

        char btnId[16]; wsprintfA(btnId, "##nav%d", i);
        if (ImGui::InvisibleButton(btnId, ImVec2(btnW, SB_ITEM_H))) currentPage = (Page)i;

        ImGui::Dummy(ImVec2(0, SB_ITEM_GAP));
    }
    ImGui::EndChild();

    // ===== MAIN CONTENT =====
    float contentX = sidebarX + SIDEBAR_W + SB_GAP;
    float contentY = sidebarY;
    float contentW = wW - OUTER_PAD - (contentX - wPos.x);
    float contentH = sidebarH;

    ImGui::SetCursorScreenPos(ImVec2(contentX, contentY));
    ImGui::BeginChild("##content", ImVec2(contentW, contentH), false);

    switch (currentPage) {
        case Page::Combat:     PageCombat();     break;
        case Page::ESP:        PageESP();        break;
        case Page::Radar:      PageRadar();      break;
        case Page::Visuals:    PageVisuals();    break;
        case Page::Utility:    PageUtility();    break;
        case Page::Config:     PageConfig();     break;
    }

    ImGui::EndChild();

    // ===== FOOTER (20px) =====
    float footerY = wPos.y + wSize.y - FOOTER_H;
    char sbText[192];
    if (MENU_UI::ShowFps) snprintf(sbText, sizeof(sbText), "INSERT: Menu  |  END: Exit  |  FPS: %.0f", ImGui::GetIO().Framerate);
    else wsprintfA(sbText, "INSERT: Menu  |  END: Exit");
    if (BeaztFontDesc) ImGui::PushFont(BeaztFontDesc);
    dl->AddText(ImVec2(wPos.x + OUTER_PAD + 4, footerY + 3), ImGui::ColorConvertFloat4ToU32(C_TEXT_MUTED), sbText);
    const char* versionText = "BEAZT";
    ImVec2 vSize = ImGui::CalcTextSize(versionText);
    dl->AddText(ImVec2(wPos.x + wW - OUTER_PAD - vSize.x - 4, footerY + 3), ImGui::ColorConvertFloat4ToU32(C_TEXT_MUTED), versionText);
    if (BeaztFontDesc) ImGui::PopFont();

    ImGui::End();
    if (BeaztFont) ImGui::PopFont();

    if (g_State.autoSave) {
        static uint64_t lastSave = 0;
        uint64_t now = GetTickCount64();
        if (now - lastSave > 30000) { Config::Save(); lastSave = now; }
    }
}

} // namespace BeaztMenu

#endif // BOMZA && BETTERCHEATS
