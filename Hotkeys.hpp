#pragma once
#include <Windows.h>
#include <string>
#include <unordered_map>
#include <vector>

enum HotkeyMode : int {
    HK_TOGGLE = 0,
    HK_HOLD = 1,
    HK_ALWAYS = 2,
    HK_DISABLED = 3
};

struct HotkeyBind {
    int key = 0;
    int mode = HK_HOLD;
};

inline std::unordered_map<std::string, HotkeyBind> g_Hotkeys;

struct HotkeyRuntimeState {
    bool downNow = false;
    bool downPrev = false;
    bool pressed = false;
    bool latched = false;
};

inline std::unordered_map<std::string, HotkeyRuntimeState> g_HotkeyRuntime;

inline const char* GetKeyName(int key) {
    if (key == 0) return "None";
    if (key >= 0x01 && key <= 0x06) { static const char* mb[] = {"","LMB","RMB","Cancel","MMB","X1","X2"}; return mb[key]; }
    if (key >= 'A' && key <= 'Z') { static char buf[2]; buf[0]=(char)key; buf[1]=0; return buf; }
    if (key >= '0' && key <= '9') { static char buf[2]; buf[0]=(char)key; buf[1]=0; return buf; }
    if (key >= VK_F1 && key <= VK_F24) { static char buf[4]; wsprintfA(buf,"F%d",key-VK_F1+1); return buf; }
    if (key == VK_SPACE) return "Space";
    if (key == VK_TAB) return "Tab";
    if (key == VK_SHIFT) return "Shift";
    if (key == VK_CONTROL) return "Ctrl";
    if (key == VK_MENU) return "Alt";
    if (key == VK_INSERT) return "Insert";
    if (key == VK_DELETE) return "Del";
    if (key == VK_END) return "End";
    if (key == VK_HOME) return "Home";
    if (key == VK_CAPITAL) return "Caps";
    if (key == VK_RETURN) return "Enter";
    if (key == VK_ESCAPE) return "Esc";
    return "???";
}

inline const char* GetModeName(int mode) {
    switch (mode) {
        case HK_TOGGLE: return "Toggle";
        case HK_HOLD: return "Hold";
        case HK_ALWAYS: return "Always";
        case HK_DISABLED: return "Disabled";
        default: return "???";
    }
}

inline bool IsCriticalHotkeyPair(const std::string& a, const std::string& b) {
    if ((a == "UTIL.DebugCam" && b == "UTIL.AdminFlag") ||
        (a == "UTIL.AdminFlag" && b == "UTIL.DebugCam"))
        return true;
    if ((a == "global.menu" && b == "global.shutdown") ||
        (a == "global.shutdown" && b == "global.menu"))
        return true;
    return false;
}

inline bool CanAssignHotkey(const std::string& featureId, int key, std::string* blockedBy = nullptr) {
    if (key == 0) return true;
    for (const auto& [id, bind] : g_Hotkeys) {
        if (id == featureId) continue;
        if (bind.mode == HK_DISABLED || bind.key == 0) continue;
        if (bind.key != key) continue;
        if (IsCriticalHotkeyPair(featureId, id)) {
            if (blockedBy) *blockedBy = id;
            return false;
        }
    }
    return true;
}

inline void EnsureHotkey(const char* featureId, int defaultKey = 0, int defaultMode = HK_HOLD) {
    auto it = g_Hotkeys.find(featureId);
    if (it == g_Hotkeys.end()) {
        g_Hotkeys[featureId] = { defaultKey, defaultMode };
    }
}

inline bool HasEnabledHotkey(const char* featureId) {
    auto it = g_Hotkeys.find(featureId);
    if (it == g_Hotkeys.end()) return false;
    const auto& bind = it->second;
    return bind.key != 0 && bind.mode != HK_DISABLED;
}

inline void UpdateHotkeysFrame() {
    for (const auto& [id, bind] : g_Hotkeys) {
        auto& st = g_HotkeyRuntime[id];
        st.downPrev = st.downNow;
        st.pressed = false;

        if (bind.key == 0 || bind.mode == HK_DISABLED) {
            st.downNow = false;
            st.latched = false;
            continue;
        }

        st.downNow = (GetAsyncKeyState(bind.key) & 0x8000) != 0;
        st.pressed = st.downNow && !st.downPrev;

        if (bind.mode == HK_TOGGLE && st.pressed)
            st.latched = !st.latched;
        if (bind.mode != HK_TOGGLE)
            st.latched = false;
    }
}

inline bool HotkeyPressed(const char* featureId) {
    auto it = g_Hotkeys.find(featureId);
    if (it == g_Hotkeys.end()) return false;
    const auto& bind = it->second;
    if (bind.key == 0 || bind.mode == HK_DISABLED || bind.mode == HK_ALWAYS) return false;
    auto st = g_HotkeyRuntime.find(featureId);
    if (st == g_HotkeyRuntime.end()) return false;
    return st->second.pressed;
}

// Check if any other feature uses the same key
inline bool HasHotkeyConflict(const std::string& featureId, int key) {
    if (key == 0) return false;
    for (const auto& [id, bind] : g_Hotkeys) {
        if (id != featureId && bind.key == key && bind.mode != HK_DISABLED)
            return true;
    }
    return false;
}

inline std::string GetConflictFeature(const std::string& featureId, int key) {
    for (const auto& [id, bind] : g_Hotkeys) {
        if (id != featureId && bind.key == key && bind.mode != HK_DISABLED)
            return id;
    }
    return "";
}

inline bool IsHotkeyActive(const char* featureId) {
    auto it = g_Hotkeys.find(featureId);
    if (it == g_Hotkeys.end()) return false;
    const auto& bind = it->second;
    if (bind.key == 0 || bind.mode == HK_DISABLED) return false;
    if (bind.mode == HK_ALWAYS) return true;
    auto st = g_HotkeyRuntime.find(featureId);
    if (st == g_HotkeyRuntime.end()) return false;
    if (bind.mode == HK_HOLD) return st->second.downNow;
    if (bind.mode == HK_TOGGLE) return st->second.latched;
    return false;
}
