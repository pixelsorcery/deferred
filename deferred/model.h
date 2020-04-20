#pragma once

#include<string>
#include <atlbase.h>
#include "renderer.h"
#include "tiny_gltf.h"

struct GltfModel;

bool loadModel(Dx12Renderer* pRenderer, GltfModel& model, const char* filename);
void drawModel(Dx12Renderer* pRenderer, GltfModel& model, double dt);

struct GltfModel
{
    std::vector<CComPtr<ID3D12Resource>>      pBuffers;
    std::vector<D3D12_VERTEX_BUFFER_VIEW>     BufferViews;
    std::vector<CComPtr<ID3D12Resource>>      IndexBuffers;
    std::vector<D3D12_INDEX_BUFFER_VIEW>      IndexBufViews;
    std::vector<int>                          IndexBufSizes;
    CComPtr<ID3D12RootSignature>              pRootSignature;
    tinygltf::Model                           TinyGltfModel;
    std::vector<Texture>                      Textures;
    std::vector<UINT>                         IndexBufSize;
    UINT                                      NumPrimitives;
    UINT                                      NumTextures;
    CComPtr<ID3D12DescriptorHeap>             TextureDescriptorHeap;
    UINT                                      NumDescriptors;
    D3D12_ROOT_DESCRIPTOR_TABLE               DescriptorTable;
    CComPtr<ID3D12PipelineState>              pModelPipeline;
    CComPtr<ID3DBlob>                         pModelVs, pModelPs;

    //~GltfModel()
    //{
    //}
};