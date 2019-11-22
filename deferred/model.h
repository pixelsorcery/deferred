#pragma once

#include<string>
#include <atlbase.h>
#include "renderer.h"
#include "tiny_gltf.h"

struct GltfModel;

bool loadModel(ID3D12Device* pDevice, GltfModel& model, const char* filename);

struct GltfModel
{
    std::vector<CComPtr<ID3D12PipelineState>> PSOs;
    std::vector<ID3D12Resource*>              Buffers;
    std::vector<D3D12_VERTEX_BUFFER_VIEW>     BufferViews;
    std::vector<CComPtr<ID3D12Resource>>      IndexBuffers;
    std::vector<D3D12_INDEX_BUFFER_VIEW>      IndexBufViews;
    std::vector<CComPtr<ID3D12RootSignature>> RootSignatures;
    tinygltf::Model                           TinyGltfModel;
    std::vector<CComPtr<ID3D12Resource>>      Textures;
    std::vector<UINT>                         IndexBufSize;
    UINT                                      NumPrimitives;
    UINT                                      NumTextures;
};