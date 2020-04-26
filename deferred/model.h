#pragma once

#include <string>
#include <atlbase.h>
#include <memory>
#include "renderer.h"
#include "tiny_gltf.h"
#include "glm/glm.hpp"
#include "util.h"
#include "dynArray.h"

struct GltfModel;

bool loadModel(Dx12Renderer* pRenderer, GltfModel& model, const char* filename);
void drawModel(Dx12Renderer* pRenderer, GltfModel& model, double dt);

struct Prim
{
    D3D12_INDEX_BUFFER_VIEW            indexBufView;
    uint                               indexBufSize;
    DynArray<D3D12_VERTEX_BUFFER_VIEW> bufferViews;
    CComPtr<ID3D12PipelineState>       pPipeline;
    CComPtr<ID3D12RootSignature>       pRootSignature;
};

struct Mesh
{
    DynArray<Prim> prims;
};

struct SceneNode
{
    int                       meshIdx;
    glm::mat4                 transformation;
    D3D12_GPU_VIRTUAL_ADDRESS constantBufferAddr;
};

struct GltfModel
{
    // todo change capitalization for everything here
    std::vector<CComPtr<ID3D12Resource>>      pBuffers;
    std::vector<CComPtr<ID3D12Resource>>      IndexBuffers;
    std::vector<D3D12_INDEX_BUFFER_VIEW>      IndexBufViews;
    std::vector<CComPtr<ID3D12PipelineState>> ModelPipelines;
    std::vector<int>                          IndexBufSizes;
    std::vector<int>                          PrimitiveBufCounts;
    CComPtr<ID3D12RootSignature>              pRootSignature;
    tinygltf::Model                           TinyGltfModel;
    std::vector<Texture>                      Textures;
    std::vector<uint>                         IndexBufSize;
    uint                                      NumPrimitives;
    uint                                      NumTextures;
    CComPtr<ID3D12DescriptorHeap>             TextureDescriptorHeap;
    uint                                      NumDescriptors;
    D3D12_ROOT_DESCRIPTOR_TABLE               DescriptorTable;
    CComPtr<ID3DBlob>                         pModelVs, pModelPs;
    CComPtr<ID3D12Resource>                   ConstantBuffer;
    uint                                      ConstantBufferIncrement;
    DynArray<Mesh>                            meshes;
    DynArray<SceneNode>                       sceneNodes;
    std::unique_ptr<glm::mat4[]>              pCpuConstantBuffer; // todo make this upload heap
    int                                       alignedMatrixSize;
    glm::mat4                                 viewProj;
};