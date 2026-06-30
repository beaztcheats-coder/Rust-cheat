#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include "../imgui/imgui.h"
#include <vector>

// ============================================================================
// ESPRenderer — custom DX11 batched renderer for ESP primitives
// Eliminates ImGui draw list overhead for lines/rects/circles
// Supports thick lines via quad generation, alpha blending, no depth test
// ============================================================================
class ESPRenderer {
    ID3D11Device*           dev = nullptr;
    ID3D11DeviceContext*    ctx = nullptr;
    ID3D11VertexShader*     vs = nullptr;
    ID3D11PixelShader*      ps = nullptr;
    ID3D11InputLayout*      layout = nullptr;
    ID3D11Buffer*           vb = nullptr;
    ID3D11Buffer*           constBuf = nullptr;
    ID3D11BlendState*       blendState = nullptr;
    ID3D11DepthStencilState* depthState = nullptr;
    ID3D11RasterizerState*  rasterState = nullptr;

    static constexpr int MAX_VERTS = 65536;
    struct ESPVertex { float x, y, r, g, b, a; };

    ESPVertex lineVerts[MAX_VERTS];
    int lineVertCount = 0;
    ESPVertex triVerts[MAX_VERTS];
    int triVertCount = 0;

    bool initialized = false;

    struct ScreenCB { float w, h, pad0, pad1; };

public:
    bool Init(ID3D11Device* device, ID3D11DeviceContext* context) {
        if (initialized) return true;
        dev = device;
        ctx = context;

        // Shaders — screen-space position + vertex color, no textures
        const char* vsSrc =
            "struct VS_IN { float2 pos : POSITION; float4 col : COLOR; };\n"
            "struct VS_OUT { float4 pos : SV_POSITION; float4 col : COLOR; };\n"
            "cbuffer Screen : register(b0) { float2 screen; float2 pad; };\n"
            "VS_OUT main(VS_IN i) {\n"
            "  VS_OUT o;\n"
            "  o.pos = float4(i.pos.x * 2.0 / screen.x - 1.0, 1.0 - i.pos.y * 2.0 / screen.y, 0.0, 1.0);\n"
            "  o.col = i.col;\n"
            "  return o;\n"
            "}\n";
        const char* psSrc =
            "struct PS_IN { float4 pos : SV_POSITION; float4 col : COLOR; };\n"
            "float4 main(PS_IN i) : SV_TARGET { return i.col; }\n";

        ID3DBlob* vsBlob = nullptr, *psBlob = nullptr;
        D3DCompile(vsSrc, strlen(vsSrc), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsBlob, nullptr);
        D3DCompile(psSrc, strlen(psSrc), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &psBlob, nullptr);
        if (!vsBlob || !psBlob) return false;

        dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vs);
        dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &ps);

        // Input layout: float2 pos + float4 color
        D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        dev->CreateInputLayout(layoutDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &layout);

        if (vsBlob) vsBlob->Release();
        if (psBlob) psBlob->Release();

        // Dynamic vertex buffer (large enough for batched ESP)
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.ByteWidth = MAX_VERTS * sizeof(ESPVertex);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev->CreateBuffer(&bd, nullptr, &vb);

        // Constant buffer for screen dimensions
        bd.ByteWidth = sizeof(ScreenCB);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        dev->CreateBuffer(&bd, nullptr, &constBuf);

        // Blend state — alpha blending
        D3D11_BLEND_DESC bsd = {};
        bsd.RenderTarget[0].BlendEnable = TRUE;
        bsd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        bsd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        bsd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bsd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bsd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        bsd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bsd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        dev->CreateBlendState(&bsd, &blendState);

        // Depth state — disabled
        D3D11_DEPTH_STENCIL_DESC dsd = {};
        dsd.DepthEnable = FALSE;
        dev->CreateDepthStencilState(&dsd, &depthState);

        // Rasterizer — no culling
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;
        rd.ScissorEnable = FALSE;
        dev->CreateRasterizerState(&rd, &rasterState);

        initialized = true;
        return true;
    }

    void Shutdown() {
        if (vs) vs->Release();
        if (ps) ps->Release();
        if (layout) layout->Release();
        if (vb) vb->Release();
        if (constBuf) constBuf->Release();
        if (blendState) blendState->Release();
        if (depthState) depthState->Release();
        if (rasterState) rasterState->Release();
        initialized = false;
    }

    void Begin(float screenW, float screenH) {
        lineVertCount = 0;
        triVertCount = 0;

        // Update constant buffer
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(ctx->Map(constBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            ScreenCB* sc = (ScreenCB*)mapped.pData;
            sc->w = screenW;
            sc->h = screenH;
            ctx->Unmap(constBuf, 0);
        }
    }

    inline void AddLine(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.0f) {
        if (thickness <= 1.0f) {
            // Thin line — 2 vertices
            if (lineVertCount + 2 >= MAX_VERTS) return;
            lineVerts[lineVertCount++] = { x1, y1, ((color>>0)&0xFF)/255.f, ((color>>8)&0xFF)/255.f, ((color>>16)&0xFF)/255.f, ((color>>24)&0xFF)/255.f };
            lineVerts[lineVertCount++] = { x2, y2, ((color>>0)&0xFF)/255.f, ((color>>8)&0xFF)/255.f, ((color>>16)&0xFF)/255.f, ((color>>24)&0xFF)/255.f };
        } else {
            // Thick line — 6 vertices (quad as 2 triangles)
            if (triVertCount + 6 >= MAX_VERTS) return;
            float dx = x2 - x1, dy = y2 - y1;
            float len = sqrtf(dx*dx + dy*dy);
            if (len < 0.001f) return;
            float px = -dy / len * thickness * 0.5f;
            float py = dx / len * thickness * 0.5f;
            float r = ((color>>0)&0xFF)/255.f, g = ((color>>8)&0xFF)/255.f;
            float b = ((color>>16)&0xFF)/255.f, a = ((color>>24)&0xFF)/255.f;
            // Quad: p1+perp, p1-perp, p2+perp, p2-perp
            triVerts[triVertCount++] = { x1+px, y1+py, r, g, b, a };
            triVerts[triVertCount++] = { x1-px, y1-py, r, g, b, a };
            triVerts[triVertCount++] = { x2+px, y2+py, r, g, b, a };
            triVerts[triVertCount++] = { x2+px, y2+py, r, g, b, a };
            triVerts[triVertCount++] = { x1-px, y1-py, r, g, b, a };
            triVerts[triVertCount++] = { x2-px, y2-py, r, g, b, a };
        }
    }

    inline void AddRect(float x, float y, float w, float h, ImU32 color, float thickness = 1.0f) {
        AddLine(x, y, x+w, y, color, thickness);
        AddLine(x+w, y, x+w, y+h, color, thickness);
        AddLine(x+w, y+h, x, y+h, color, thickness);
        AddLine(x, y+h, x, y, color, thickness);
    }

    inline void AddRectFilled(float x, float y, float w, float h, ImU32 color) {
        if (triVertCount + 6 >= MAX_VERTS) return;
        float r = ((color>>0)&0xFF)/255.f, g = ((color>>8)&0xFF)/255.f;
        float b = ((color>>16)&0xFF)/255.f, a = ((color>>24)&0xFF)/255.f;
        triVerts[triVertCount++] = { x, y, r, g, b, a };
        triVerts[triVertCount++] = { x+w, y, r, g, b, a };
        triVerts[triVertCount++] = { x+w, y+h, r, g, b, a };
        triVerts[triVertCount++] = { x, y, r, g, b, a };
        triVerts[triVertCount++] = { x+w, y+h, r, g, b, a };
        triVerts[triVertCount++] = { x, y+h, r, g, b, a };
    }

    inline void AddCircle(float cx, float cy, float radius, ImU32 color, float thickness = 1.0f) {
        int segments = 32;
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * 6.2831853f;
            float a2 = (float)(i+1) / segments * 6.2831853f;
            AddLine(cx + cosf(a1)*radius, cy + sinf(a1)*radius,
                    cx + cosf(a2)*radius, cy + sinf(a2)*radius, color, thickness);
        }
    }

    void Flush() {
        if (!initialized || (lineVertCount == 0 && triVertCount == 0)) return;

        // Save ImGui state
        ID3D11VertexShader* oldVS = nullptr; ID3D11PixelShader* oldPS = nullptr;
        ID3D11InputLayout* oldLayout = nullptr; ID3D11Buffer* oldVB = nullptr;
        ID3D11BlendState* oldBlend = nullptr; ID3D11DepthStencilState* oldDepth = nullptr;
        ID3D11RasterizerState* oldRaster = nullptr; UINT oldStencilRef = 0;
        ID3D11Buffer* oldCB = nullptr;
        float oldBlendFactor[4]; UINT oldSampleMask = 0;
        UINT oldStride = 0, oldOffset = 0;

        ctx->VSGetShader(&oldVS, nullptr, nullptr);
        ctx->PSGetShader(&oldPS, nullptr, nullptr);
        ctx->IAGetInputLayout(&oldLayout);
        ctx->IAGetVertexBuffers(0, 1, &oldVB, &oldStride, &oldOffset);
        ctx->OMGetBlendState(&oldBlend, oldBlendFactor, &oldSampleMask);
        ctx->OMGetDepthStencilState(&oldDepth, &oldStencilRef);
        ctx->RSGetState(&oldRaster);
        ctx->PSGetConstantBuffers(0, 1, &oldCB);

        // Set our pipeline
        ctx->VSSetShader(vs, nullptr, 0);
        ctx->PSSetShader(ps, nullptr, 0);
        ctx->IASetInputLayout(layout);
        UINT stride = sizeof(ESPVertex), offset = 0;
        ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
        ctx->VSSetConstantBuffers(0, 1, &constBuf);
        ctx->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(depthState, 0);
        ctx->RSSetState(rasterState);

        // Render thick lines + filled rects (triangles)
        if (triVertCount > 0) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(ctx->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, triVerts, triVertCount * sizeof(ESPVertex));
                ctx->Unmap(vb, 0);
            }
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ctx->Draw(triVertCount, 0);
        }

        // Render thin lines
        if (lineVertCount > 0) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(ctx->Map(vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, lineVerts, lineVertCount * sizeof(ESPVertex));
                ctx->Unmap(vb, 0);
            }
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
            ctx->Draw(lineVertCount, 0);
        }

        // Restore ImGui state
        ctx->VSSetShader(oldVS, nullptr, 0);
        ctx->PSSetShader(oldPS, nullptr, 0);
        ctx->IASetInputLayout(oldLayout);
        ctx->IASetVertexBuffers(0, 1, &oldVB, &oldStride, &oldOffset);
        ctx->OMSetBlendState(oldBlend, oldBlendFactor, oldSampleMask);
        ctx->OMSetDepthStencilState(oldDepth, oldStencilRef);
        ctx->RSSetState(oldRaster);
        if (oldCB) ctx->PSSetConstantBuffers(0, 1, &oldCB);

        if (oldVS) oldVS->Release();
        if (oldPS) oldPS->Release();
        if (oldLayout) oldLayout->Release();
        if (oldVB) oldVB->Release();
        if (oldBlend) oldBlend->Release();
        if (oldDepth) oldDepth->Release();
        if (oldRaster) oldRaster->Release();
        if (oldCB) oldCB->Release();

        lineVertCount = 0;
        triVertCount = 0;
    }

    // Stats for debug overlay
    int GetLineVertCount() const { return lineVertCount; }
    int GetTriVertCount() const { return triVertCount; }
};

inline ESPRenderer g_ESPRenderer;
