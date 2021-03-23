#pragma once

/**************************************************

This app draws a simple triangle.

**************************************************/
#include <memory>
#include <atlbase.h>
#include "renderer.h"

class App
{
public:
    App();
    bool init(HWND hwnd);
    void drawFrame();
    void onMouseMove(int x, int y, int dx, int dy);
    void onKey(const uint32_t key, bool pressed) {};
private:
    std::unique_ptr<Dx12Renderer> pRenderer;
    CComPtr<ID3D12Resource>       vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW      vertexBufferView;
    CComPtr<ID3DBlob>             triangleVs,trianglePs;
    CComPtr<ID3D12PipelineState>  triPipeline;
    CComPtr<ID3D12RootSignature>  triRootSignature;
};