#pragma once

/**************************************************

This app draws an unanimated gltf model.

**************************************************/

#include <memory>
#include <atlbase.h>
#include <vector>
#include "renderer.h"
#include "model.h"

class App2
{
public:
    App2();
    bool init(HWND hwnd);
    void drawFrame();
    ~App2();
private:
    std::unique_ptr<Dx12Renderer> pRenderer;
    std::vector<ID3D12Resource*>  boxBuffers;
    std::vector<D3D12_VERTEX_BUFFER_VIEW> boxBufferViews;
    CComPtr<ID3D12Resource>       pIndexBuffer;
    D3D12_INDEX_BUFFER_VIEW       indexBufView;
    CComPtr<ID3DBlob>             boxVs, boxPs;
    CComPtr<ID3D12PipelineState>  boxPipeline;
    CComPtr<ID3D12RootSignature>  boxRootSignature;
    tinygltf::Model               boxModel;
    std::vector<ID3D12Resource*>  boxTextures;
};