#pragma once
#include <vector>
#include <D3D11.h>
#include "../Hotkeys.hpp"
#include "../Translation.hpp"
#include "../tracers.hpp"
#include <d3d9types.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <Lmcons.h>
#include <tchar.h>

#include "../skcrypt.hpp"
#include "../Imgui/imgui_impl_win32.h"
#include "../Imgui/imgui_impl_dx11.h"
#include "ESPRenderer.hpp"

#ifdef BOMZA
#include "../Bomza/bomza_menu.hpp"
#endif
#if !defined(BOMZA)
#include "../BeaZt/beazt_menu.hpp"
#endif
#include "niglet.hpp"
#include "BadMan.hpp"
#include <time.h>
#include <string>

#include "../Hacks/Cheat/Cheat.hpp"
#include <filesystem>
#include <D3DX11tex.h>
#include "../Config.hpp"
#include <functional>
#include <cctype>
#include <algorithm>
#include <urlmon.h>
#include <Shlwapi.h>

#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shlwapi.lib")


#define IM_PI 3.14159265358979323846
inline bool renderfov;
inline int fov = 25;
inline ImVec4 Color1 = ImVec4(0.0f, 0.90196f, 1.0f, 1.0f);
inline ImVec4 Color2 = ImVec4(0.6f, 0.0f, 1.0f, 1.0f);
extern  ImFont* VisualFont;
extern  ImFont* BeaztFont;
extern  ImFont* BeaztFontTitle;
extern  ImFont* BeaztFontDesc;

inline float Angle = 0.0f;
inline float Speed = 1.0f;

inline std::chrono::high_resolution_clock::time_point lastUpdateTime = std::chrono::high_resolution_clock::now();

inline ImVec4 GetInterpolatedColor(float t)
{
    ImVec4 interpolatedColor;
    interpolatedColor.x = Color1.x * (1 - t) + Color2.x * t;
    interpolatedColor.y = Color1.y * (1 - t) + Color2.y * t;
    interpolatedColor.z = Color1.z * (1 - t) + Color2.z * t;
    interpolatedColor.w = Color1.w * (1 - t) + Color2.w * t;

    return interpolatedColor;
}
inline int currentItem = 0;
inline int ITEM = 0;
inline float FontSize = 16.f;

inline ImDrawList* DrawData;
inline ImFont* ubuntu_bold_font = nullptr;
inline ImFont* ubuntu_medium_font = nullptr;
inline ImFont* ubuntu_regular_font = nullptr;
inline ImFont* expand_font = nullptr;

inline ImFont* SetFont = NULL;

inline ImFont* Arial;
inline ImFont* Calibri;
inline ImFont* Ebrima;
inline ImFont* Tahoma;
inline ImFont* Comic;
enum RENDER_INFORMATION : int {
    RENDER_HIJACK_FAILED = 0,
    RENDER_IMGUI_FAILED = 1,
    RENDER_DRAW_FAILED = 2,
    RENDER_SETUP_SUCCESSFUL = 3,
    RENDER_COD_FAILED = 4,
};


using namespace ImGui;
inline int Segments = 10;
inline HWND window_handle;

inline ID3D11Device* d3d_device;
inline ID3D11DeviceContext* d3d_device_ctx;
inline IDXGISwapChain* d3d_swap_chain;
inline ID3D11RenderTargetView* d3d_render_target;
inline D3DPRESENT_PARAMETERS d3d_present_params;
inline ID3D11ShaderResourceView* g_BeaztLogo = nullptr;
#define vec4( r, g, b, a ) ImColor( r / 255.f, g / 255.f, b / 255.f, a )
#define RGBAs(r, g, b, a) ImVec4((r) / 255.f, (g) / 255.f, (b) / 255.f, (a) / 255.f)
ImVec4 sky_colors = ImVec4(0.941f, 0.957f, 0.980f, 1.0f);
ImVec4 SkyColor = RGBAs(255, 255, 255, 255);

inline std::string loc;

inline const char* items[] = { "Arial", "Calibri", "Ebrima", "Tahoma" };

enum e_fonts : int {

    REGULAR = 0,
    MEDIUM,
    BOLD,
    LOGO
};
inline int FrameRate()
{
    static int iFps, iLastFps;
    static float flLastTickCount, flTickCount;
    flTickCount = clock() * 0.001f;
    iFps++;
    if ((flTickCount - flLastTickCount) >= 1.0f) {
        flLastTickCount = flTickCount;
        iLastFps = iFps;
        iFps = 0;
    }
    return iLastFps;
}
using namespace std;
inline void HelpMarker(const char* desc) {
    ImGui::TextDisabled(("[?]"));
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();

    }
}

inline void draw_text_outline_font_Font(ImFont* Font, float Size, float x, float y, ImColor color, const char* string) {
    char buf[512];
    ImVec2 len = ImGui::CalcTextSize(string);
    ImGui::GetBackgroundDrawList()->AddText(Font, Size, ImVec2(x - 1, y), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);
    ImGui::GetBackgroundDrawList()->AddText(Font, Size, ImVec2(x + 1, y), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);
    ImGui::GetBackgroundDrawList()->AddText(Font, Size, ImVec2(x, y - 1), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);
    ImGui::GetBackgroundDrawList()->AddText(Font, Size, ImVec2(x, y + 1), ImColor(0.0f, 0.0f, 0.0f, 255.f), string);

    ImGui::GetBackgroundDrawList()->AddText(Font, Size, ImVec2(x, y), color, string);

}
inline bool drawmenu = true;

inline bool showMouse = false;
inline int* a;
inline bool weaponScreen = false;
inline bool attachmentsScreen = false;
inline bool armorScreen = false;
inline bool otherScreen = false;
inline ImFont* DefeatureFont;
inline ID3D11ShaderResourceView* HeroTexture = nullptr;
inline ID3D11ShaderResourceView* BadManTexture = nullptr;

inline static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
    const char* const* items = (const char* const*)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}


static const char* keyNames[] =
{

    "Keybind",
    "Left Mouse",
    "Right Mouse",
    "Cancel",
    "Middle Mouse",
    "Mouse 5",
    "Mouse 4",
    "",
    "Backspace",
    "Tab",
    "",
    "",
    "Clear",
    "Enter",
    "",
    "",
    "Shift",
    "Control",
    "Alt",
    "Pause",
    "Caps",
    "",
    "",
    "",
    "",
    "",
    "",
    "Escape",
    "",
    "",
    "",
    "",
    "Space",
    "Page Up",
    "Page Down",
    "End",
    "Home",
    "Left",
    "Up",
    "Right",
    "Down",
    "",
    "",
    "",
    "Print",
    "Insert",
    "Delete",
    "",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "",
    "",
    "",
    "",
    "",
    "Numpad 0",
    "Numpad 1",
    "Numpad 2",
    "Numpad 3",
    "Numpad 4",
    "Numpad 5",
    "Numpad 6",
    "Numpad 7",
    "Numpad 8",
    "Numpad 9",
    "Multiply",
    "Add",
    "",
    "Subtract",
    "Decimal",
    "Divide",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
};

inline static int keystatus = 0;
inline static int realkey = 0;

inline static int keystatus1 = 0;
inline static int realkey1 = 0;

inline static int keystatus2 = 0;
inline static int realkey2 = 0;

inline bool GetKey1(int key)
{
    realkey1 = key;
    return true;
}
inline void ChangeKey1(void* blank)
{
    keystatus1 = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                // AIMBOT::SILENTKEY removed â€” kept for hotkey infrastructure
                keystatus1 = 0;
                return;
            }
        }
    }

}

inline void ChangeKey2(void* blank)
{
    keystatus2 = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                // MISC::SpeedKey removed
                keystatus2 = 0;
                return;
            }
        }
    }

}

inline bool GetKey(int key)
{
    realkey = key;
    return true;
}
inline void ChangeKey(void* blank)
{
    keystatus = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                // old key removed â€” using hotkey system
                keystatus = 0;
                return;
            }
        }
    }

}
inline void HotkeyButton1(int aimkey, void* changekey, int status)
{
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value == NULL)
        aimkeys = "Select Key";
    else
        aimkeys = "[ " + std::string(preview_value) + " ]";

    if (status == 1)
    {
        aimkeys = "Press any key";
    }


    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            std::thread(reinterpret_cast<void(*)()>(changekey)).detach();
        }
    }

    style->Colors[ImGuiCol_Button] = originalButtonColor;
}


void HotkeyButton(int aimkey, void* changekey, int status)
{
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value == NULL)
        aimkeys = "Select Key";
    else
        aimkeys = "[ " + std::string(preview_value) + " ]";

    if (status == 1)
    {
        aimkeys = "Press any key";
    }


    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            std::thread(reinterpret_cast<void(*)()>(changekey)).detach();
        }
    }

    style->Colors[ImGuiCol_Button] = originalButtonColor;
}


void HotkeyButton2(int aimkey, void* changekey, int status)
{
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value == NULL)
        aimkeys = "Select Key";
    else
        aimkeys = "[ " + std::string(preview_value) + " ]";

    if (status == 1)
    {
        aimkeys = "Press any key";
    }


    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            std::thread(reinterpret_cast<void(*)()>(changekey)).detach();
        }
    }

    style->Colors[ImGuiCol_Button] = originalButtonColor;
}

inline static int keystatus3 = 0;
inline static int realkey3 = 0;

inline bool GetKey3(int key)
{
    realkey3 = key;
    return true;
}
inline void ChangeKey3(void* blank)
{
    keystatus3 = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                // old ZoomKey removed â€” using hotkey system
                keystatus3 = 0;
                return;
            }
        }
    }

}

void HotkeyButton4(int aimkey, void* changekey, int status)
{
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value == NULL)
        aimkeys = "Select Key";
    else
        aimkeys = "[ " + std::string(preview_value) + " ]";

    if (status == 1)
    {
        aimkeys = "Press any key";
    }


    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            std::thread(reinterpret_cast<void(*)()>(changekey)).detach();
        }
    }

    style->Colors[ImGuiCol_Button] = originalButtonColor;
}


inline static int keystatus4 = 0;
inline static int realkey4 = 0;

inline bool GetKey4(int key)
{
    realkey4 = key;
    return true;
}
inline void ChangeKey4(void* blank)
{
    keystatus4 = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                // MISC::FlyKey removed
                keystatus4 = 0;
                return;
            }
        }
    }

}

void HotkeyButton3(int aimkey, void* changekey, int status)
{
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value == NULL)
        aimkeys = "Select Key";
    else
        aimkeys = "[ " + std::string(preview_value) + " ]";

    if (status == 1)
    {
        aimkeys = "Press any key";
    }


    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            std::thread(reinterpret_cast<void(*)()>(changekey)).detach();
        }
    }

    style->Colors[ImGuiCol_Button] = originalButtonColor;
}



inline static int keystatus5 = 0;
inline static int realkey5 = 0;

inline bool GetKey5(int key)
{
    realkey5 = key;
    return true;
}
inline void ChangeKey5(void* blank)
{
    keystatus5 = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                // MISC::ThirdPersonKey removed
                keystatus5 = 0;
                return;
            }
        }
    }

}

void HotkeyButton5(int aimkey, void* changekey, int status)
{
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value == NULL)
        aimkeys = "Select Key";
    else
        aimkeys = "[ " + std::string(preview_value) + " ]";

    if (status == 1)
    {
        aimkeys = "Press any key";
    }


    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            std::thread(reinterpret_cast<void(*)()>(changekey)).detach();
        }
    }

    style->Colors[ImGuiCol_Button] = originalButtonColor;
}


inline static int keystatus6 = 0;
inline static int realkey6 = 0;

inline bool GetKey6(int key)
{
    realkey5 = key;
    return true;
}
inline void ChangeKey6(void* blank)
{
    keystatus5 = 1;
    while (true)
    {
        for (int i = 0; i < 0x87; i++)
        {
            if (GetAsyncKeyState(i) & 0x8000)
            {
                // MISC::FlyKey removed
                keystatus6 = 0;
                return;
            }
        }
    }

}

void HotkeyButton6(int aimkey, void* changekey, int status)
{
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames))
        Items_ArrayGetter(keyNames, aimkey, &preview_value);

    std::string aimkeys;
    if (preview_value == NULL)
        aimkeys = "Select Key";
    else
        aimkeys = "[ " + std::string(preview_value) + " ]";

    if (status == 1)
    {
        aimkeys = "Press any key";
    }


    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20)))
    {
        if (status == 0)
        {
            std::thread(reinterpret_cast<void(*)()>(changekey)).detach();
        }
    }

    style->Colors[ImGuiCol_Button] = originalButtonColor;
}

inline static int keystatus7 = 0;
inline void ChangeKey7(void* blank) {
    keystatus7 = 1;
    while (true) {
        for (int i = 0; i < IM_ARRAYSIZE(keyNames); i++) {
            if (GetAsyncKeyState(i) & 0x8000) { /* old key removed */ keystatus7 = 0; return; }
        }
    }
}
void HotkeyButton7(int aimkey, void* changekey, int status) {
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames)) Items_ArrayGetter(keyNames, aimkey, &preview_value);
    std::string aimkeys = preview_value ? "[ " + std::string(preview_value) + " ]" : "Select Key";
    if (status == 1) aimkeys = "Press any key";
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20))) { if (status == 0) std::thread(reinterpret_cast<void(*)()>(changekey)).detach(); }
    style->Colors[ImGuiCol_Button] = originalButtonColor;
}
inline static int keystatus8 = 0;
inline void ChangeKey8(void* blank) {
    keystatus8 = 1;
    while (true) {
        for (int i = 0; i < IM_ARRAYSIZE(keyNames); i++) {
            if (GetAsyncKeyState(i) & 0x8000) { /* old key removed */ keystatus8 = 0; return; }
        }
    }
}
void HotkeyButton8(int aimkey, void* changekey, int status) {
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames)) Items_ArrayGetter(keyNames, aimkey, &preview_value);
    std::string aimkeys = preview_value ? "[ " + std::string(preview_value) + " ]" : "Select Key";
    if (status == 1) aimkeys = "Press any key";
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20))) { if (status == 0) std::thread(reinterpret_cast<void(*)()>(changekey)).detach(); }
    style->Colors[ImGuiCol_Button] = originalButtonColor;
}
inline static int keystatus10 = 0;
inline void ChangeKey10(void* blank) {
    keystatus10 = 1;
    while (true) {
        for (int i = 0; i < IM_ARRAYSIZE(keyNames); i++) {
            if (GetAsyncKeyState(i) & 0x8000) { /* old key removed */ keystatus10 = 0; return; }
        }
    }
}
void HotkeyButton10(int aimkey, void* changekey, int status) {
    const char* preview_value = NULL;
    if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(keyNames)) Items_ArrayGetter(keyNames, aimkey, &preview_value);
    std::string aimkeys = preview_value ? "[ " + std::string(preview_value) + " ]" : "Select Key";
    if (status == 1) aimkeys = "Press any key";
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4 originalButtonColor = style->Colors[ImGuiCol_Button];
    style->Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    if (ImGui::Button(aimkeys.c_str(), ImVec2(125, 20))) { if (status == 0) std::thread(reinterpret_cast<void(*)()>(changekey)).detach(); }
    style->Colors[ImGuiCol_Button] = originalButtonColor;
}


ImVec4 LerpColor(const ImVec4& start, const ImVec4& end, float t) {
    return ImVec4(
        start.x + t * (end.x - start.x),
        start.y + t * (end.y - start.y),
        start.z + t * (end.z - start.z),
        start.w + t * (end.w - start.w)
    );
}

void DrawGradientText(const char* text, const ImVec2& pos, const ImVec4& start_color, const ImVec4& end_color) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 text_size = ImGui::CalcTextSize(text);
    float text_width = text_size.x;
    ImVec2 current_pos = pos;
    for (int i = 0; i < strlen(text); i++) {
        float t = static_cast<float>(i) / (strlen(text) - 1);
        ImVec4 color = LerpColor(start_color, end_color, t);
        char character[2] = { text[i], '\0' };
        draw_list->AddText(current_pos, ImGui::ColorConvertFloat4ToU32(color), character);

        current_pos.x += ImGui::CalcTextSize(character).x;
    }
}
std::string RetrieveUsername() {
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len)) { 
        return std::string(username);
    }
    return "Unknown";
}

void DrawFlags() {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float xOffset = screenSize.x - 10; 
    float yOffset = 10; 
    float lineHeight = ImGui::GetTextLineHeight() + 2; 

    auto DrawOutlinedText = [draw_list](const ImVec2& pos, ImColor color, ImColor outlineColor, const char* text) {
        draw_list->AddText(ImVec2(pos.x - 1, pos.y - 1), outlineColor, text);
        draw_list->AddText(ImVec2(pos.x + 1, pos.y - 1), outlineColor, text);
        draw_list->AddText(ImVec2(pos.x - 1, pos.y + 1), outlineColor, text);
        draw_list->AddText(ImVec2(pos.x + 1, pos.y + 1), outlineColor, text);
        draw_list->AddText(pos, color, text);
        };

    {
        auto& aimBind = g_Hotkeys["COMBAT.Aimbot"];
        std::string aimKeyName = aimBind.key ? GetKeyName(aimBind.key) : "None";
        std::string aimbotText = "Aimbot: " + std::string(AIMBOT::Memory ? "ON" : "OFF") + " [Key: " + aimKeyName + "]";
        DrawOutlinedText(ImVec2(xOffset - ImGui::CalcTextSize(aimbotText.c_str()).x, yOffset),
            AIMBOT::Memory ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255),
            ImColor(0, 0, 0, 255), aimbotText.c_str());
        yOffset += lineHeight;
    }
    if (src->LocalPlayer) {
        auto heldItem = src->LocalPlayer->Get_HeldItem();
        if (heldItem) {
            std::string weaponName = heldItem->get_item_name();
            std::string weaponText = "Held Item -> " + weaponName;
            DrawOutlinedText(ImVec2(xOffset - ImGui::CalcTextSize(weaponText.c_str()).x, yOffset),
                ImColor(255, 255, 255, 255),
                ImColor(0, 0, 0, 255), weaponText.c_str());
            yOffset += lineHeight;
        }
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static float g_RawMouseWheel = 0.0f; // accumulated raw input wheel delta

LRESULT WINAPI WndProcHook(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {  
    case WM_INPUT: {
        UINT size = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
        if (size > 0 && size <= sizeof(RAWINPUT)) {
            RAWINPUT raw = {};
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) > 0) {
                if (raw.header.dwType == RIM_TYPEMOUSE) {
                    if (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                        g_RawMouseWheel += (float)(SHORT)raw.data.mouse.usButtonData / (float)WHEEL_DELTA;
                }
            }
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    case WM_NCHITTEST:
        // When menu is visible, capture all mouse input (including wheel + keyboard focus)
        if (SETTINGS::MenuOpen) return HTCLIENT;
        return HTTRANSPARENT; // pass through to game when menu closed
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return::DefWindowProcW(hWnd, msg, wParam, lParam);
}

inline std::string GenerateRandomString(int length) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::string randomString;
    randomString.reserve(length);

    for (int i = 0; i < length; ++i) {
        randomString += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return randomString;
}

namespace render {
    class c_render {

        HWND fortnite_window = { };

    public:

        auto Setup() -> RENDER_INFORMATION {

            fortnite_window = FindWindowA(skCrypt("UnityWndClass").decrypt(), skCrypt("Rust").decrypt());
            if (!fortnite_window) {
                LOG("Setup: FindWindowA(UnityWndClass, Rust) FAILED — window not found");
                LOG("Setup: Enumerating windows to find Rust...");
                HWND enumResult = nullptr;
                struct EnumData { HWND* target; };
                EnumData ed{ &enumResult };
                EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                    auto* ed = reinterpret_cast<EnumData*>(lParam);
                    char cls[256] = {}, title[256] = {};
                    GetClassNameA(hwnd, cls, sizeof(cls));
                    GetWindowTextA(hwnd, title, sizeof(title));
                    if (IsWindowVisible(hwnd) && (strstr(title, "Rust") || strstr(cls, "Unity"))) {
                        LOG("Setup: Found window: class='%s' title='%s' hwnd=0x%I64X", cls, title, (uint64_t)hwnd);
                        *ed->target = hwnd;
                        return FALSE;
                    }
                    return TRUE;
                }, reinterpret_cast<LPARAM>(&ed));
                if (enumResult) {
                    fortnite_window = enumResult;
                    LOG("Setup: Using fallback window: 0x%I64X", (uint64_t)fortnite_window);
                } else {
                    LOG("Setup: No Rust window found at all — overlay cannot be created");
                    return RENDER_COD_FAILED;
                }
            } else {
                LOG("Setup: FindWindowA(UnityWndClass, Rust) SUCCESS — hwnd=0x%I64X", (uint64_t)fortnite_window);
            }

            if (!render::c_render::Hijack()) {
                LOG("Setup: Hijack() FAILED");
                return RENDER_HIJACK_FAILED;
            }
            LOG("Setup: Hijack() SUCCESS");

            if (!render::c_render::ImGui()) {
                LOG("Setup: ImGui() FAILED");
                return RENDER_IMGUI_FAILED;
            }
            LOG("Setup: ImGui() SUCCESS — overlay fully initialized");
            LOG("Setup: window_handle=0x%I64X fortnite_window=0x%I64X", (uint64_t)window_handle, (uint64_t)fortnite_window);
            return RENDER_SETUP_SUCCESSFUL;

        }

        auto get_screen_status() -> bool
        {
            if (this->fortnite_window == GetForegroundWindow()) {
                return true;
            }

            if (this->fortnite_window == GetActiveWindow()) {
                return true;
            }

            if (GetActiveWindow() == GetForegroundWindow()) {
                return true;
            }

            return false;
        }

        auto Render() -> bool {
            if (!window_handle || !IsWindow(window_handle)) {
                LOG("Render: Invalid window handle (window_handle=0x%I64X) — cannot render", (uint64_t)window_handle);
                return false;
            }
            LOG("Render: Entering render loop (window_handle=0x%I64X)", (uint64_t)window_handle);

            MSG msg = { NULL };
            while (msg.message != WM_QUIT) {
                if (PeekMessageA(&msg, window_handle, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                ImGuiIO& io = ImGui::GetIO();
                io.ImeWindowHandle = window_handle;
                io.DeltaTime = 1.0f / 60.0f;

                POINT p_cursor;
                if (!GetCursorPos(&p_cursor)) {
                    printf("Error: Failed to get cursor position\n");
                    continue;
                }

                io.MousePos.x = p_cursor.x;
                io.MousePos.y = p_cursor.y;

                if (GetAsyncKeyState(VK_LBUTTON)) {
                    io.MouseDown[0] = true;
                    io.MouseClicked[0] = true;
                    io.MouseClickedPos[0].x = io.MousePos.x;
                    io.MouseClickedPos[0].y = io.MousePos.y;
                }
                else {
                    io.MouseDown[0] = false;
                }

                render::c_render::Draw();
            }
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            DestroyWindow(window_handle);
            printf("[+] Render Setup\n");
            return true;
        }

        auto ImGui() -> bool {
            DXGI_SWAP_CHAIN_DESC swap_chain_description;
            ZeroMemory(&swap_chain_description, sizeof(swap_chain_description));
            swap_chain_description.BufferCount = 2;
            swap_chain_description.BufferDesc.Width = 0;
            swap_chain_description.BufferDesc.Height = 0;
            swap_chain_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            swap_chain_description.BufferDesc.RefreshRate.Numerator = 60;
            swap_chain_description.BufferDesc.RefreshRate.Denominator = 1;
            swap_chain_description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
            swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swap_chain_description.OutputWindow = window_handle;
            swap_chain_description.SampleDesc.Count = 1;
            swap_chain_description.SampleDesc.Quality = 0;
            swap_chain_description.Windowed = 1;
            swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            D3D_FEATURE_LEVEL d3d_feature_lvl;

            const D3D_FEATURE_LEVEL d3d_feature_array[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

            D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, d3d_feature_array, 2, D3D11_SDK_VERSION, &swap_chain_description, &d3d_swap_chain, &d3d_device, &d3d_feature_lvl, &d3d_device_ctx);
            ID3D11Texture2D* pBackBuffer;
            d3d_swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

            d3d_device->CreateRenderTargetView(pBackBuffer, NULL, &d3d_render_target);

            pBackBuffer->Release();

#ifdef BOMZA
            BomzaMenu::LoadBomzaLogo(d3d_device);
#else
            {
#include "../Beazt/beazt_logo.h"
                if (beazt_logo_size > 0 && d3d_device) {
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
                    D3DX11CreateShaderResourceViewFromMemory(
                        d3d_device, beazt_logo_data, beazt_logo_size,
                        &info, pump, &g_BeaztLogo, nullptr);
                }
            }
#endif

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();

            // Our state
            ImVec4 clear_color = GetStyleColorVec4(ImGuiCol_WindowBg);
            io.IniFilename = NULL;


            ImFontConfig font_config;
            font_config.PixelSnapH = false;
            font_config.OversampleH = 5;
            font_config.OversampleV = 5;
            font_config.RasterizerMultiply = 1.2f;

            D3DX11_IMAGE_LOAD_INFO info;
            ID3DX11ThreadPump* pmp{ nullptr };

            //if (!HeroTexture) //be saving white lives since 2020
            //    D3DX11CreateShaderResourceViewFromMemory(d3d_device, HeroData, sizeof(HeroData), &info, pmp, &HeroTexture, 0);

            //if (!BadManTexture) //be saving white lives since 2020
            //    D3DX11CreateShaderResourceViewFromMemory(d3d_device, BadManData, sizeof(BadManData), &info, pmp, &BadManTexture, 0);

            ImGui_ImplWin32_Init(window_handle);
            ImGui_ImplDX11_Init(d3d_device, d3d_device_ctx);
            g_ESPRenderer.Init(d3d_device, d3d_device_ctx);
            g_ItemIcons.Init(d3d_device);
            d3d_device->Release();

            VisualFont = io.Fonts->AddFontFromFileTTF(skCrypt("C:\\Windows\\Fonts\\ariblk.ttf"), 13, nullptr, io.Fonts->GetGlyphRangesDefault());
            // BeaZt V5.0: Segoe UI Semibold 13px (body) + 16px (titles) + Segoe UI Regular 11px (meta)
            {
                BeaztFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisb.ttf", 13, &font_config, io.Fonts->GetGlyphRangesCyrillic());
                if (!BeaztFont) BeaztFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 13, &font_config, io.Fonts->GetGlyphRangesCyrillic());
                BeaztFontTitle = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguisb.ttf", 14, &font_config, io.Fonts->GetGlyphRangesCyrillic());
                if (!BeaztFontTitle) BeaztFontTitle = BeaztFont;
                BeaztFontDesc = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 11, &font_config, io.Fonts->GetGlyphRangesCyrillic());
                if (!BeaztFontDesc) BeaztFontDesc = BeaztFont;
            }

            Sleep(1000);

            return true;
        }

        //auto Hijack() -> bool
        //{
        //    //            window_handle = FindWindowA(skCrypt("MedalOverlayClass").decrypt(), skCrypt("MedalOverlay").decrypt());
        //    //window_handle = FindWindowA(skCrypt("MedalOverlayClass").decrypt(), skCrypt("MedalOverlay").decrypt());
        //    window_handle = FindWindowA(skCrypt("OOPO_WINDOWS_CLASS").decrypt(), skCrypt("ow overlay").decrypt());
        //    if (!window_handle) {
        //        printf("Failed To Find Nividia");
        //        return false;
        //    }
        //    MARGINS Margin = { -1 };
        //    DwmExtendFrameIntoClientArea(window_handle, &Margin);
        //    SetWindowPos(window_handle, 0, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
        //    ShowWindow(window_handle, SW_SHOW);
        //    UpdateWindow(window_handle);
        //    return true;
        //}

        auto Hijack() -> bool {
            // Get screen size
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            WNDCLASSEXW wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProcHook, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("onGuiClass"), NULL };
            ::RegisterClassEx(&wc);

            window_handle = CreateWindowEx(
                WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
                wc.lpszClassName,
                L"ongaUI",
                WS_POPUP,
                0, 0, screenWidth, screenHeight,
                NULL, NULL, wc.hInstance, NULL
            );

            //driver::HideWindow((uintptr_t)window_handle, WDA_EXCLUDEFROMCAPTURE);

            // Extend frame for visual transparency
            MARGINS margins = { -1 };
            DwmExtendFrameIntoClientArea(window_handle, &margins);

            // Set alpha to 255 (fully opaque)
            SetLayeredWindowAttributes(window_handle, 0, 255, LWA_ALPHA);

            // Show the window
            ::ShowWindow(window_handle, SW_SHOWDEFAULT);
            ::UpdateWindow(window_handle);

            // Register raw input for mouse wheel (works even without window focus)
            RAWINPUTDEVICE rid = {};
            rid.usUsagePage = 0x01;   // HID_USAGE_PAGE_GENERIC
            rid.usUsage     = 0x02;   // HID_USAGE_GENERIC_MOUSE
            rid.dwFlags     = RIDEV_INPUTSINK; // receive even when not foreground
            rid.hwndTarget  = window_handle;
            RegisterRawInputDevices(&rid, 1, sizeof(rid));

            return true;
        }
        RECT rect;
        bool StartCheat = false;
        int Slec = 0;

        enum class Page : int { Dashboard, Combat, PlayerESP, NPCESP, AnimalESP, WorldESP, Radar, Visuals, Utility, Settings };

        struct MenuRuntimeState {
            char search[96];
            char keybindSearch[96];
            char activeConfig[64];
            char newConfigName[64];
            char renameConfigName[64];
            std::vector<std::string> configNames;
            int selectedConfigIndex;
            bool configListDirty;
            uint64_t lastConfigListRefreshMs;
            bool dirty;
            bool showOnlyConflicts;
            int keybindPageFilter;
            float saveFlash;
            float loadFlash;
            float resetFlash;

            MenuRuntimeState() {
                memset(search, 0, sizeof(search));
                memset(keybindSearch, 0, sizeof(keybindSearch));
                memset(activeConfig, 0, sizeof(activeConfig));
                strcpy_s(activeConfig, sizeof(activeConfig), "default");
                memset(newConfigName, 0, sizeof(newConfigName));
                strcpy_s(newConfigName, sizeof(newConfigName), "default");
                memset(renameConfigName, 0, sizeof(renameConfigName));
                selectedConfigIndex = -1;
                configListDirty = true;
                lastConfigListRefreshMs = 0;
                dirty = false;
                showOnlyConflicts = false;
                keybindPageFilter = 0;
                saveFlash = 0.0f;
                loadFlash = 0.0f;
                resetFlash = 0.0f;
            }
        };

        inline static MenuRuntimeState g_MenuState;
        inline static float g_MenuAnim = 1.0f;

        void MarkMenuDirty() {
            g_MenuState.dirty = true;
        }

        void ResetAllSettingsDefaults() {
            ESP::Box = true;
            ESP::CornerBox = false;
            ESP::FilledBox = false;
            ESP::Distance = true;
            ESP::SnapLines = true;
            ESP::HeadCircle = false;
            ESP::Weapon = true;
            ESP::Name = true;
            ESP::Skeleton = true;
            ESP::HealthBar = true;
            ESP::OFFArrows = false;
            ESP::VisCheck = false;
            ESP::TeamID = false;
            ESP::hotbar_text = false;
            ESP::DrawTextBackground = false;
            ESP::draw_distance = 300.f;

            NPC_ESP::Enabled = false;
            NPC_ESP::Box = false;
            NPC_ESP::Name = true;
            NPC_ESP::Distance = true;
            NPC_ESP::HealthBar = true;
            NPC_ESP::Skeleton = false;
            NPC_ESP::HeldItem = false;
            NPC_ESP::SnapLines = false;
            NPC_ESP::draw_distance = 300.f;

            ANIMAL_ESP::Enabled = false;
            ANIMAL_ESP::Box = false;
            ANIMAL_ESP::Name = false;
            ANIMAL_ESP::Distance = false;
            ANIMAL_ESP::HealthBar = false;
            ANIMAL_ESP::SnapLines = false;
            ANIMAL_ESP::draw_distance = 200.f;

            AIMBOT::Memory = false;
            AIMBOT::FovCircle = false;
            AIMBOT::TargetLine = false;
            AIMBOT::TargetText = false;
            AIMBOT::FovSize = 100;
            AIMBOT::SMOOTHING = 5.f;
            AIMBOT::KeepTarget = false;
            AIMBOT::BoneSelector = 53;
            AIMBOT::VisibleStrict = false; // hidden feature â€” force off in presets
            AIMBOT::BonePriority = 0;
            AIMBOT::PredictionScale = 0.f;
            AIMBOT::SpinWrites = true;

            MISC::FovChanger = false;
            MISC::Zoom = false;
            MISC::DrawFlags = false;

            RADAR::Enabled = false;
            RADAR::Size = 250.f;
            RADAR::Scale = 1.f;
            RADAR::DotSize = 4.f;
            RADAR::ShowPlayers = true;
            RADAR::ShowNPCs = true;
            RADAR::ShowAnimals = true;
            RADAR::HideSleepers = true;
            RADAR::RemoveTeam = false;

            MENU_UI::UiScale = 100;
            MENU_UI::MenuOpacity = 0.96f;
            MENU_UI::BorderGlow = 0.50f;
            MENU_UI::AnimationSpeed = 1.00f;
            MENU_UI::CompactMode = false;
            MENU_UI::AdvancedMode = false;
            MENU_UI::Tooltips = true;
            MENU_UI::ShowWatermark = true;
            MENU_UI::ShowFps = true;
            MENU_UI::PerformanceMode = false;
            MENU_UI::DisableAnimations = false;
            MENU_UI::Theme = 0;

            MENU_FX::AnimatedBackground = true;
            MENU_FX::MatrixParticles = true;
            MENU_FX::LightningFx = false;
            MENU_FX::FxIntensity = 0.75f;
            MENU_FX::MatrixDensity = 0.65f;
            MENU_FX::BackgroundOpacity = 0.55f;
        }

        std::string GetConfigPathForName(const char* name) {
            return Config::GetConfigPathForName(name);
        }

        bool ConfigExists(const char* name) {
            auto p = GetConfigPathForName(name);
            return GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES;
        }

        bool LoadNamedConfig(const char* name) {
            auto p = GetConfigPathForName(name);
            if (GetFileAttributesA(p.c_str()) == INVALID_FILE_ATTRIBUTES) return false;
            const char* activeCfg = Config::GetDefaultConfigPath();
            if (lstrcmpiA(p.c_str(), activeCfg) != 0) {
                CopyFileA(p.c_str(), activeCfg, FALSE);
            }
            Config::Load();
            lstrcpynA(g_MenuState.activeConfig, (name && name[0]) ? name : "default", 63);
            g_MenuState.activeConfig[63] = 0;
            lstrcpynA(g_MenuState.newConfigName, g_MenuState.activeConfig, 63);
            g_MenuState.newConfigName[63] = 0;
            g_MenuState.configListDirty = true;
            return true;
        }

        bool SaveNamedConfig(const char* name) {
            Config::Save();
            auto p = GetConfigPathForName(name);
            const char* activeCfg = Config::GetDefaultConfigPath();
            if (lstrcmpiA(p.c_str(), activeCfg) != 0) {
                if (!CopyFileA(activeCfg, p.c_str(), FALSE)) return false;
            }
            lstrcpynA(g_MenuState.activeConfig, (name && name[0]) ? name : "default", 63);
            g_MenuState.activeConfig[63] = 0;
            lstrcpynA(g_MenuState.newConfigName, g_MenuState.activeConfig, 63);
            g_MenuState.newConfigName[63] = 0;
            g_MenuState.configListDirty = true;
            return true;
        }

        void RefreshConfigList(bool force = false) {
            const uint64_t now = GetTickCount64();
            if (!force && !g_MenuState.configListDirty && (now - g_MenuState.lastConfigListRefreshMs) < 1000) return;

            std::vector<std::string> names;
            names.push_back("default");

            const std::string prefix = std::string(RuntimePaths::ConfigPrefix()) + "_";
            const std::string pattern = "C:\\" + prefix + "*.dat";
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
            g_MenuState.configNames.swap(names);

            g_MenuState.selectedConfigIndex = 0;
            for (int i = 0; i < (int)g_MenuState.configNames.size(); i++) {
                if (lstrcmpiA(g_MenuState.configNames[i].c_str(), g_MenuState.activeConfig) == 0) {
                    g_MenuState.selectedConfigIndex = i;
                    break;
                }
            }

            g_MenuState.configListDirty = false;
            g_MenuState.lastConfigListRefreshMs = now;
        }


        void EnsureHotkeyRegistry() {
            static const std::pair<const char*, int> entries[] = {
                { "global.menu", HK_HOLD }, { "global.shutdown", HK_HOLD },
                { "COMBAT.Aimbot", HK_HOLD },

        { "UTIL.AdminFlags", HK_TOGGLE },
        { "UTIL.DebugCam", HK_TOGGLE },
        { "UTIL.BattleMode", HK_TOGGLE },
        { "UTIL.CombatMode", HK_TOGGLE },
        { "VISUAL.FovChanger", HK_TOGGLE },
        { "VISUAL.Zoom", HK_TOGGLE },
        { "VISUAL.RemoveLayers", HK_TOGGLE },
            };

            EnsureHotkey("global.menu", VK_INSERT, HK_HOLD);
            EnsureHotkey("global.shutdown", VK_END, HK_HOLD);
            for (const auto& [id, mode] : entries) EnsureHotkey(id, 0, mode);
            EnsureHotkey("UTIL.AdminFlags", VK_F6, HK_TOGGLE);
            EnsureHotkey("UTIL.DebugCam", VK_F7, HK_TOGGLE);
            EnsureHotkey("UTIL.BattleMode", VK_F2, HK_TOGGLE);
            EnsureHotkey("UTIL.CombatMode", VK_F3, HK_TOGGLE);
            EnsureHotkey("VISUAL.RemoveLayers", VK_F8, HK_TOGGLE);
            EnsureHotkey("VISUAL.FovChanger", VK_NUMPAD1, HK_TOGGLE);
            EnsureHotkey("VISUAL.Zoom", VK_NUMPAD2, HK_TOGGLE);
            g_Hotkeys.erase("COMBAT.Recoil");
        }

        void ProcessHotkeys() {
            auto& hk = g_Hotkeys;
            auto applyHoldOrAlways = [&](const char* id, bool& out) {
                auto it = hk.find(id);
                if (it == hk.end()) return;
                const auto& bind = it->second;
                if (bind.key == 0 || bind.mode == HK_DISABLED) return;
                out = IsHotkeyActive(id);
            };

            applyHoldOrAlways("COMBAT.Aimbot", AIMBOT::Memory);
        }


        bool  render_Menu = false;

        static ImGuiKey VkToImGuiKey(int vk) {
            switch (vk) {
            case VK_TAB: return ImGuiKey_Tab;
            case VK_LEFT: return ImGuiKey_LeftArrow;
            case VK_RIGHT: return ImGuiKey_RightArrow;
            case VK_UP: return ImGuiKey_UpArrow;
            case VK_DOWN: return ImGuiKey_DownArrow;
            case VK_PRIOR: return ImGuiKey_PageUp;
            case VK_NEXT: return ImGuiKey_PageDown;
            case VK_HOME: return ImGuiKey_Home;
            case VK_END: return ImGuiKey_End;
            case VK_INSERT: return ImGuiKey_Insert;
            case VK_DELETE: return ImGuiKey_Delete;
            case VK_BACK: return ImGuiKey_Backspace;
            case VK_SPACE: return ImGuiKey_Space;
            case VK_RETURN: return ImGuiKey_Enter;
            case VK_ESCAPE: return ImGuiKey_Escape;
            case VK_OEM_COMMA: return ImGuiKey_Comma;
            case VK_OEM_MINUS: return ImGuiKey_Minus;
            case VK_OEM_PERIOD: return ImGuiKey_Period;
            case VK_OEM_2: return ImGuiKey_Slash;
            case VK_OEM_1: return ImGuiKey_Semicolon;
            case VK_OEM_PLUS: return ImGuiKey_Equal;
            case VK_OEM_4: return ImGuiKey_LeftBracket;
            case VK_OEM_5: return ImGuiKey_Backslash;
            case VK_OEM_6: return ImGuiKey_RightBracket;
            case VK_OEM_3: return ImGuiKey_GraveAccent;
            case VK_CAPITAL: return ImGuiKey_CapsLock;
            case VK_SCROLL: return ImGuiKey_ScrollLock;
            case VK_NUMLOCK: return ImGuiKey_NumLock;
            case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
            case VK_PAUSE: return ImGuiKey_Pause;
            case VK_NUMPAD0: return ImGuiKey_Keypad0;
            case VK_NUMPAD1: return ImGuiKey_Keypad1;
            case VK_NUMPAD2: return ImGuiKey_Keypad2;
            case VK_NUMPAD3: return ImGuiKey_Keypad3;
            case VK_NUMPAD4: return ImGuiKey_Keypad4;
            case VK_NUMPAD5: return ImGuiKey_Keypad5;
            case VK_NUMPAD6: return ImGuiKey_Keypad6;
            case VK_NUMPAD7: return ImGuiKey_Keypad7;
            case VK_NUMPAD8: return ImGuiKey_Keypad8;
            case VK_NUMPAD9: return ImGuiKey_Keypad9;
            case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
            case VK_DIVIDE: return ImGuiKey_KeypadDivide;
            case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
            case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
            case VK_ADD: return ImGuiKey_KeypadAdd;
            case VK_LSHIFT: return ImGuiKey_LeftShift;
            case VK_RSHIFT: return ImGuiKey_RightShift;
            case VK_LCONTROL: return ImGuiKey_LeftCtrl;
            case VK_RCONTROL: return ImGuiKey_RightCtrl;
            case VK_LMENU: return ImGuiKey_LeftAlt;
            case VK_RMENU: return ImGuiKey_RightAlt;
            case VK_LWIN: return ImGuiKey_LeftSuper;
            case VK_RWIN: return ImGuiKey_RightSuper;
            case VK_APPS: return ImGuiKey_Menu;
            case '0': return ImGuiKey_0;
            case '1': return ImGuiKey_1;
            case '2': return ImGuiKey_2;
            case '3': return ImGuiKey_3;
            case '4': return ImGuiKey_4;
            case '5': return ImGuiKey_5;
            case '6': return ImGuiKey_6;
            case '7': return ImGuiKey_7;
            case '8': return ImGuiKey_8;
            case '9': return ImGuiKey_9;
            case 'A': return ImGuiKey_A;
            case 'B': return ImGuiKey_B;
            case 'C': return ImGuiKey_C;
            case 'D': return ImGuiKey_D;
            case 'E': return ImGuiKey_E;
            case 'F': return ImGuiKey_F;
            case 'G': return ImGuiKey_G;
            case 'H': return ImGuiKey_H;
            case 'I': return ImGuiKey_I;
            case 'J': return ImGuiKey_J;
            case 'K': return ImGuiKey_K;
            case 'L': return ImGuiKey_L;
            case 'M': return ImGuiKey_M;
            case 'N': return ImGuiKey_N;
            case 'O': return ImGuiKey_O;
            case 'P': return ImGuiKey_P;
            case 'Q': return ImGuiKey_Q;
            case 'R': return ImGuiKey_R;
            case 'S': return ImGuiKey_S;
            case 'T': return ImGuiKey_T;
            case 'U': return ImGuiKey_U;
            case 'V': return ImGuiKey_V;
            case 'W': return ImGuiKey_W;
            case 'X': return ImGuiKey_X;
            case 'Y': return ImGuiKey_Y;
            case 'Z': return ImGuiKey_Z;
            case VK_F1: return ImGuiKey_F1;
            case VK_F2: return ImGuiKey_F2;
            case VK_F3: return ImGuiKey_F3;
            case VK_F4: return ImGuiKey_F4;
            case VK_F5: return ImGuiKey_F5;
            case VK_F6: return ImGuiKey_F6;
            case VK_F7: return ImGuiKey_F7;
            case VK_F8: return ImGuiKey_F8;
            case VK_F9: return ImGuiKey_F9;
            case VK_F10: return ImGuiKey_F10;
            case VK_F11: return ImGuiKey_F11;
            case VK_F12: return ImGuiKey_F12;
            default: return ImGuiKey_None;
            }
        }

        auto Draw() -> void {
            ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.f);

            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();

            // Manual input sync (overlay window doesn't receive keyboard/mouse messages)
            {
                ImGuiIO& io = ImGui::GetIO();
                // Mouse buttons
                io.MouseDown[0] = (GetAsyncKeyState(VK_LBUTTON)  & 0x8000) != 0;
                io.MouseDown[1] = (GetAsyncKeyState(VK_RBUTTON)  & 0x8000) != 0;
                io.MouseDown[2] = (GetAsyncKeyState(VK_MBUTTON)  & 0x8000) != 0;
                io.MouseDown[3] = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
                io.MouseDown[4] = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;

                // Overlay windows don't have OS-level focus â€” force keyboard capture
                // when menu is open so ImGui widgets (InputText etc.) process keys
                io.WantCaptureKeyboard = render_Menu;
                io.WantTextInput = render_Menu;

                // Keyboard: sync all keys + detect transitions for character input
                static bool prevKeys[256] = {};
                const bool allowTextInput = render_Menu;
                for (int k = 1; k < 256; k++) {
                    bool down = (GetAsyncKeyState(k) & 0x8000) != 0;
                    io.KeysDown[k] = down;

                    ImGuiKey imguiKey = VkToImGuiKey(k);
                    if (imguiKey != ImGuiKey_None && down != prevKeys[k]) {
                        io.AddKeyEvent(imguiKey, down);
                    }

                    // On rising edge: translate to character for typing (InputText etc.)
                    if (allowTextInput && down && !prevKeys[k]) {
                        // Only translate printable characters via ToAscii
                        // Control keys (backspace, enter, etc) are handled via io.KeysDown[] natively
                        SHORT ks[256] = {};
                        for (int i = 0; i < 256; i++) {
                            if (io.KeysDown[i]) ks[i] = (SHORT)0x8000;
                        }
                        if (io.KeyCtrl)  ks[VK_CONTROL] = (SHORT)0x8000;
                        if (io.KeyShift) ks[VK_SHIFT]   = (SHORT)0x8000;
                        if (io.KeyAlt)   ks[VK_MENU]    = (SHORT)0x8000;

                        BYTE keyState[256] = {};
                        for (int i = 0; i < 256; i++)
                            keyState[i] = (ks[i] & 0x8000) ? 0x80 : 0;
                        if (io.KeyCtrl)  keyState[VK_CONTROL] = 0x80;
                        if (io.KeyShift) keyState[VK_SHIFT] = 0x80;

                        WORD ch = 0;
                        UINT scan = MapVirtualKeyA(k, MAPVK_VK_TO_VSC);
                        int ret = ToAscii(k, scan, keyState, &ch, 0);
                        if (ret == 1 && ch >= 32) {
                            io.AddInputCharacter(ch);
                        } else if (ret == 2) {
                            io.AddInputCharacter(ch);
                            io.AddInputCharacter(ch);
                        }
                    }
                    prevKeys[k] = down;
                }

                io.AddKeyEvent(ImGuiMod_Ctrl, (io.KeysDown[VK_LCONTROL] || io.KeysDown[VK_RCONTROL] || io.KeysDown[VK_CONTROL]));
                io.AddKeyEvent(ImGuiMod_Shift, (io.KeysDown[VK_LSHIFT] || io.KeysDown[VK_RSHIFT] || io.KeysDown[VK_SHIFT]));
                io.AddKeyEvent(ImGuiMod_Alt, (io.KeysDown[VK_LMENU] || io.KeysDown[VK_RMENU] || io.KeysDown[VK_MENU]));
                io.KeyCtrl = io.KeysDown[VK_CONTROL];
                io.KeyShift = io.KeysDown[VK_SHIFT];
                io.KeyAlt = io.KeysDown[VK_MENU];

                // Mouse wheel: accumulated from raw input (WM_INPUT handler)
                io.MouseWheel += g_RawMouseWheel;
                g_RawMouseWheel = 0.0f;
            }

            ImGui::NewFrame();

            // Global hotkey defaults (first frame)
            static bool hkInit = false;
            if (!hkInit) {
                EnsureHotkeyRegistry();
                EnsureHotkey("COMBAT.Aimbot", VK_F1, HK_HOLD);
                hkInit = true;
            }

            EnsureHotkeyRegistry();
            UpdateHotkeysFrame();

            // Menu: toggle on rising edge
            {
                if (HotkeyPressed("global.menu")) {
                    render_Menu = !render_Menu;
                }
            }

            // Shutdown: fire once on rising edge
            {
                if (HotkeyPressed("global.shutdown")) {
                    MISC::ShutdownRequested = true;
                    if (MISC::RecoilEnabled) {
                        MISC::RecoilEnabled = false;
                        if (hx) hx->do_misc();
                    }
                    PostMessageA(window_handle, WM_QUIT, 0, 0);
                }
            }

            SETTINGS::MenuOpen = render_Menu;
            float targetAnim = render_Menu ? 1.0f : 0.0f;
            float speed = MENU_UI::DisableAnimations ? 1.0f : (ImGui::GetIO().DeltaTime * 14.0f * MENU_UI::AnimationSpeed);
            if (speed > 1.0f) speed = 1.0f;
            g_MenuAnim += (targetAnim - g_MenuAnim) * speed;

            if (g_MenuAnim > 0.01f) {
#ifdef BOMZA
                BomzaMenu::Render();
#else
                BeaztMenu::Render();
#endif
            }

            // Process hotkey toggles/holds for all bound features
            ProcessHotkeys();

            if(MISC::DrawFlags)
            DrawFlags();

            if (AIMBOT::FovCircle)
            {
                auto draw = ImGui::GetBackgroundDrawList();
                float fovSize = (float)AIMBOT::FovSize;
                // Dynamic FOV â€” scale circle based on camera vs reference FOV
                if (AIMBOT::DynamicFov && src && src->LocalPlayer_Matrix._22 != 0.f) {
                    float cameraFov = 2.f * atanf(1.f / fabsf(src->LocalPlayer_Matrix._22)) * 57.2957795f;
                    if (cameraFov > 10.f && cameraFov < 180.f) {
                        fovSize = fovSize * (90.f / cameraFov);
                    }
                }

                ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);

                // FOV outline â€” black circle behind segments
                if (AIMBOT::FovOutline) {
                    draw->AddCircle(center, fovSize + 1.5f, IM_COL32(0, 0, 0, 180), 0, 2.5f);
                }

                float angle = 2.0f * IM_PI / 200;

                std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
                float deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();
                Angle += Speed * deltaTime;
                lastUpdateTime = currentTime;

                for (int i = 0; i < 200; ++i)
                {
                    float start = i * angle + Angle;
                    float end = (i + 1) * angle + Angle;

                    ImVec2 startPoint(center.x + fovSize * cos(start), center.y + fovSize * sin(start));
                    ImVec2 endPoint(center.x + fovSize * cos(end), center.y + fovSize * sin(end));

                    float t = cos(start - Angle);

                    // Use FovColor when outline is on, rainbow when off
                    ImVec4 segmentColor;
                    if (AIMBOT::FovOutline) {
                        segmentColor = ImVec4(AIMBOT::FovColor.Value.x, AIMBOT::FovColor.Value.y, AIMBOT::FovColor.Value.z, AIMBOT::FovColor.Value.w);
                    } else {
                        segmentColor = GetInterpolatedColor(0.5f + 0.5f * t);
                    }

                    draw->AddLine(startPoint, endPoint, IM_COL32(segmentColor.x * 255, segmentColor.y * 255, segmentColor.z * 255, segmentColor.w * 255), 1.0f);
                }
            }
            if (MISC::CrosshairStyle > 0) {
                auto* cdraw = ImGui::GetBackgroundDrawList();
                ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
                float csz = MISC::CrosshairSize;
                ImColor ccol = MISC::CrosshairColor;
                if (MISC::CrosshairStyle == 1) {
                    cdraw->AddCircleFilled(center, csz, ccol);
                } else if (MISC::CrosshairStyle == 2) {
                    cdraw->AddLine(ImVec2(center.x - csz * 3, center.y), ImVec2(center.x - csz, center.y), ccol, 1.5f);
                    cdraw->AddLine(ImVec2(center.x + csz, center.y), ImVec2(center.x + csz * 3, center.y), ccol, 1.5f);
                    cdraw->AddLine(ImVec2(center.x, center.y - csz * 3), ImVec2(center.x, center.y - csz), ccol, 1.5f);
                    cdraw->AddLine(ImVec2(center.x, center.y + csz), ImVec2(center.x, center.y + csz * 3), ccol, 1.5f);
                } else if (MISC::CrosshairStyle == 3) {
                    cdraw->AddCircle(center, csz * 3, ccol, 24, 1.5f);
                    cdraw->AddCircleFilled(center, 1.5f, ccol);
                }
            }
            // Run Do_Cheat every frame â€” with stability gate for server join
            {
                static int stableFrames = 0;
                static uintptr_t lastLP = 0;
                uintptr_t currentLP = src && src->LocalPlayer ? (uintptr_t)src->LocalPlayer : 0;
                if (currentLP != lastLP) {
                    stableFrames = 0;
                    lastLP = currentLP;
                }
                if (currentLP && stableFrames < 90) {
                    stableFrames++;
                } else {
                    g_ESPRenderer.Begin(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);
                    Xheat::Do_Cheat();
                    g_ESPRenderer.Flush();
                }
            }

            // Hit indicator / crosshair rendering continues

            if (RADAR::Enabled && src && src->LocalPlayer && (!SETTINGS::BattleMode || BATTLE::Radar)) {
                auto* rdraw = ImGui::GetBackgroundDrawList();
                float rSize = RADAR::Size;
                float rX = 10, rY = 10;
                if (RADAR::Position == 1) { rX = ImGui::GetIO().DisplaySize.x - rSize - 10; rY = 10; }
                else if (RADAR::Position == 2) { rX = 10; rY = ImGui::GetIO().DisplaySize.y - rSize - 10; }
                else if (RADAR::Position == 3) { rX = ImGui::GetIO().DisplaySize.x - rSize - 10; rY = ImGui::GetIO().DisplaySize.y - rSize - 10; }
                float cx = rX + rSize * 0.5f, cy = rY + rSize * 0.5f;

                rdraw->AddRectFilled(ImVec2(rX, rY), ImVec2(rX + rSize, rY + rSize), ImColor(10, 10, 10, (int)(255 * RADAR::BackgroundOpacity)));
                rdraw->AddRect(ImVec2(rX, rY), ImVec2(rX + rSize, rY + rSize), RADAR::BorderColor);

                ImColor gc(60, 70, 90, 60);
                if (RADAR::ShowGrid) {
                    float g3 = rSize / 3.f;
                    rdraw->AddLine(ImVec2(rX, rY + g3), ImVec2(rX + rSize, rY + g3), gc);
                    rdraw->AddLine(ImVec2(rX, rY + g3 * 2), ImVec2(rX + rSize, rY + g3 * 2), gc);
                    rdraw->AddLine(ImVec2(rX + g3, rY), ImVec2(rX + g3, rY + rSize), gc);
                    rdraw->AddLine(ImVec2(rX + g3 * 2, rY), ImVec2(rX + g3 * 2, rY + rSize), gc);
                }

                // LocalPlayer position + yaw from globals (zero IOCTLs â€” written by fast thread)
                Vector3 lp;
                float yawDeg = 0.f;
                {
                    std::lock_guard<std::mutex> lk(g_LocalPlayerDataMutex);
                    lp = g_LocalPlayerPos;
                    yawDeg = g_LocalBodyAngles.y;
                }
                if (!lp.Empty()) {
                    float yawRad = yawDeg * 3.14159f / 180.f;
                    if (!RADAR::Rotate) yawRad = 0.f;
                    float halfR = rSize * 0.5f - 5.f;
                    float rScale = RADAR::Scale > 0.1f ? RADAR::Scale : 1.f;
                    float dotSz = RADAR::DotSize;

                    auto drawDot = [&](Vector3 ep, ImColor col) {
                        float dx = (ep.x - lp.x) / rScale;
                        float dz = (ep.z - lp.z) / rScale;
                        float rx = dx * cosf(yawRad) - dz * sinf(yawRad);
                        float rz = dx * sinf(yawRad) + dz * cosf(yawRad);
                        if (rx < -halfR || rx > halfR || rz < -halfR || rz > halfR) return;
                        rdraw->AddCircleFilled(ImVec2(cx + rx, cy - rz), dotSz, col);
                    };

                    rdraw->AddCircleFilled(ImVec2(cx, cy), dotSz + 1.f, ImColor(0, 200, 255, 255));

                    // Copy entity lists outside the cache lock
                    std::vector<Rust::BaseEntity*> npcCopy, animCopy;
                    if (RADAR::ShowNPCs) { std::lock_guard<std::mutex> nlk(cac->npc_mutex); npcCopy = npc_List; }
                    if (RADAR::ShowAnimals) { std::lock_guard<std::mutex> alk(cac->animal_mutex); animCopy = animal_List; }

                    uint64_t myTeam = 0;
                    { std::lock_guard<std::mutex> tlk(g_LocalPlayerDataMutex); myTeam = g_LocalPlayerTeam; }

                    // Single lock for all radar entity rendering
                    {
                        std::lock_guard<std::mutex> lk(g_EspCacheMutex);
                        if (RADAR::ShowPlayers) {
                            for (const auto& entry : g_EspCache) {
                                if (entry.second.isDead) continue;
                                if (RADAR::HideSleepers && entry.second.isSleeping) continue;
                                if (RADAR::RemoveTeam && myTeam != 0 && entry.second.teamId == myTeam) continue;
                                Vector3 pos = entry.second.spine1Pos;
                                if (pos.Empty()) pos = entry.second.headPos;
                                if (pos.Empty()) continue;
                                ImColor dotCol = (myTeam != 0 && entry.second.teamId == myTeam)
                                    ? RADAR::TeammateColor : RADAR::PlayerColor;
                                drawDot(pos, dotCol);
                            }
                        }
                        if (RADAR::ShowNPCs) {
                            for (auto* n : npcCopy) {
                                if (!n || !is_valid((uintptr_t)n)) continue;
                                auto it = g_EspCache.find((uintptr_t)n);
                                if (it == g_EspCache.end() || it->second.isDead) continue;
                                Vector3 pos = it->second.headPos;
                                if (!pos.Empty()) drawDot(pos, RADAR::NpcColor);
                            }
                        }
                        if (RADAR::ShowAnimals) {
                            for (auto* a : animCopy) {
                                if (!a || !is_valid((uintptr_t)a)) continue;
                                auto it = g_EspCache.find((uintptr_t)a);
                                if (it == g_EspCache.end() || it->second.isDead) continue;
                                Vector3 pos = it->second.headPos;
                                if (!pos.Empty()) drawDot(pos, RADAR::AnimalColor);
                            }
                        }
                    }
                }
            }

            //if (ESP::hotbar_text)
            //{
            //    aim->hotbar_mutex.lock();
            //    auto list = hotbar_list;
            //    aim->hotbar_mutex.unlock();

            //    //std::string playerName = Player->GetName() + "'s Inventory";

            //    ImGui::SetNextWindowSize(ImVec2(255, 160));
            //    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
            //    ImGui::Begin("", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

            ////    ImGui::TextColored(ImColor(255, 255, 255, 255), playerName.c_str());

            //    ImGui::Separator(); // Adds the line under the title

            //    for (int i = 0; i < list.size(); i++)
            //    {
            //        auto text = list[i];
            //        if (text.empty())
            //            text = "none";

            //        if (i == 0)  // First item = Active Item
            //        {
            //            ImGui::TextColored(ImColor(255, 165, 0, 255), " Active Item");

            //        }
            //        else
            //        {
            //            ImGui::TextColored(ImColor(255, 255, 255, 255), text.c_str());
            //        }
            //    }

            //    ImGui::End();
            //    ImGui::PopStyleColor();
            //}
            if (false) {
                float boxWidth = 100.0f;
                float boxHeight = 100.0f;
                float spacing = 10.0f;
                float boxPadding = 10.0f;
                static TextureLoader loader("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Rust\\Bundles\\items", d3d_device);

                ImGui::Begin("Hotbar", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);

                static std::vector<std::string> oldList;
                static std::vector<ID3D11ShaderResourceView*> cachedTextures;

                aim->hotbar_mutex.lock();
                auto list = hotbar_list;
                aim->hotbar_mutex.unlock();

                if (oldList != hotbar_list) {
                    oldList = hotbar_list;
                    cachedTextures.clear();
                    for (const auto& item : hotbar_list) {
                        if (!item.empty()) {
                            ID3D11ShaderResourceView* texture = nullptr;
                            if (loader.LoadSimilarTexture(item, &texture)) {
                                cachedTextures.push_back(texture);
                            }
                            else {
                                cachedTextures.push_back(nullptr);
                            }
                        }
                        else {
                            cachedTextures.push_back(nullptr);
                        }
                    }
                }

                for (size_t i = 0; i < 6; ++i) {
                    if (i > 0) ImGui::SameLine();

                    // Draw a slightly larger box around each image
                    ImGui::BeginGroup();
                    ImGui::Dummy(ImVec2(boxWidth + 2 * boxPadding, boxHeight + 2 * boxPadding));
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() - boxPadding);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - boxPadding);

                    if (i < cachedTextures.size() && cachedTextures[i]) {
                        ImGui::Image((void*)cachedTextures[i], ImVec2(boxWidth, boxHeight));
                    }
                    else {
                        ImGui::Dummy(ImVec2(boxWidth, boxHeight));
                    }

                    ImGui::EndGroup();
                }

                ImGui::End();
            }
            hotbar_list.clear();

            ImGui::Render();

            // Frame limiter â€” cap overlay to ~120 FPS without VSync (no input lag)
            {
                static ULONGLONG lastFrame = 0;
                ULONGLONG now = GetTickCount64();
                if (lastFrame > 0) {
                    ULONGLONG elapsed = now - lastFrame;
                    if (elapsed < 8) Sleep(8 - elapsed); // ~120 FPS cap
                }
                lastFrame = now;
            }

            const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
            d3d_device_ctx->OMSetRenderTargets(1, &d3d_render_target, nullptr);
            d3d_device_ctx->ClearRenderTargetView(d3d_render_target, clear_color_with_alpha);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            d3d_swap_chain->Present(0, 0); // No VSync â€” zero input lag
        }

    };
} static render::c_render* Renderer = new render::c_render();
