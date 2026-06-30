#pragma once
#include <vector>
#include <mutex>
#include <cmath>
#include "sdk.hpp"

struct Tracer {
    Vector3 start;
    Vector3 hit;
    ULONGLONG timestamp;
};

inline std::vector<Tracer> g_Tracers;
inline std::mutex g_TracersMutex;
inline int g_LastMagazineCount = -1;

inline bool TracerScreenPointValid(const Vector2& p, int w, int h) {
    if (!std::isfinite(p.x) || !std::isfinite(p.y)) return false;
    if (p.x <= 1.0f || p.y <= 1.0f) return false;
    if (p.x >= (float)w - 1.0f || p.y >= (float)h - 1.0f) return false;
    return true;
}

// Called every frame — detects shot via magazine count decrease
inline void TracerDetectShot() {
    if (!src || !src->LocalPlayer) { g_LastMagazineCount = -1; return; }
    // Use children-based weapon discovery (bypasses broken inventory)
    uintptr_t bp = src->LocalPlayer->Get_HeldWeapon();
    if (!bp || !is_valid(bp)) { g_LastMagazineCount = -1; return; }

    // Read magazine via BaseProjectile→magazine→contents
    uintptr_t mag = read<uintptr_t>(bp + offsets::BaseProjectile::primaryMagazine);
    if (!mag || !is_valid(mag)) return;
    int current = read<int>(mag + offsets::ItemMagazine::contents);
    if (current < 0 || current > 255) return;

    if (g_LastMagazineCount >= 0 && current < g_LastMagazineCount) {
        // Shot fired — create tracer
        // Read hit position from EffectNetwork
        Vector3 hitPos;
        uintptr_t en = read_chain<uintptr_t>(GameAssembly, { offsets::EffectNetwork_Pointer, offsets::EffectNetwork::static_fields, offsets::EffectNetwork::instance });
        if (en && is_valid(en)) {
            hitPos = read<Vector3>(en + offsets::EffectNetwork::hitPosition);
        }
        // Start position: local player eyes + forward * 2 (component-based resolution)
        Vector3 startPos;
        uintptr_t eyes = src->LocalPlayer->GetPlayerEyes();
        if (eyes) {
                Vector4 rot = read<Vector4>(eyes + offsets::PlayerEyes::bodyRotation);
                Vector3 headPos = read<Vector3>(eyes + offsets::PlayerEyes::headPosition);
                startPos = headPos;
                float qx=rot.x, qy=rot.y, qz=rot.z, qw=rot.w;
                Vector3 fwd = {2*(qx*qz+qw*qy), 2*(qy*qz-qw*qx), 1-2*(qx*qx+qy*qy)};
                startPos.x += fwd.x * 2.f;
                startPos.y += fwd.y * 2.f;
                startPos.z += fwd.z * 2.f;
        }
        if (startPos.Zero()) {
            auto t = src->LocalPlayer->Get_Transformation(BoneList::head);
            if (t) startPos = t->Get_Position();
        }
        if (!startPos.Zero()) {
            if (hitPos.Zero()) {
                uintptr_t input = read<uintptr_t>((uintptr_t)src->LocalPlayer + offsets::BasePlayer::PlayerInput);
                float yawDeg = input ? read<float>(input + offsets::PlayerInput::yaw) : 0.0f;
                float yaw = yawDeg * 3.1415926f / 180.0f;
                Vector3 fwd = { sinf(yaw), 0.0f, cosf(yaw) };
                hitPos = startPos + (fwd * 120.0f);
            }
            std::lock_guard<std::mutex> lk(g_TracersMutex);
            g_Tracers.push_back({startPos, hitPos, GetTickCount64()});
        }
    }
    g_LastMagazineCount = current;
}

// Render all tracers with fade
inline void TracerRender(Matrix4x4& viewMatrix, int screenW, int screenH) {
    std::lock_guard<std::mutex> lk(g_TracersMutex);
    ULONGLONG now = GetTickCount64();
    auto* dl = ImGui::GetBackgroundDrawList();
    auto it = g_Tracers.begin();
    while (it != g_Tracers.end()) {
        float age = (float)(now - it->timestamp) / 1000.f;
        if (age > 2.0f) { it = g_Tracers.erase(it); continue; }
        if (ESP::BulletTracers) {
            Vector2 a = WorldToScreen(it->start, viewMatrix);
            Vector2 b = WorldToScreen(it->hit, viewMatrix);
            if (TracerScreenPointValid(a, screenW, screenH) && TracerScreenPointValid(b, screenW, screenH)) {
                float alpha = 1.f - (age / 2.f);
                ImU32 col = IM_COL32((int)(ESP::TracerColor.Value.x*255),(int)(ESP::TracerColor.Value.y*255),(int)(ESP::TracerColor.Value.z*255),(int)(alpha*255));
                if (ESP::TracerOutline) {
                    dl->AddLine(ImVec2(a.x-1,a.y-1), ImVec2(b.x-1,b.y-1), IM_COL32(0,0,0,(int)(alpha*128)), ESP::TracerThickness+1.f);
                    dl->AddLine(ImVec2(a.x+1,a.y+1), ImVec2(b.x+1,b.y+1), IM_COL32(0,0,0,(int)(alpha*128)), ESP::TracerThickness+1.f);
                }
                dl->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), col, ESP::TracerThickness);
            }
        }
        ++it;
    }
}
