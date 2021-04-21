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
void drawModel(Dx12Renderer* pRenderer, GltfModel& model, float dt);

struct ModelConstants
{
    glm::mat4x4 worldPos;
    glm::mat4x4 normalMatrix;
    glm::vec4   baseColor;
    glm::vec3   emissiveFactor;
    int pad;
    glm::vec3   cameraPos;
    float       metallicFactor;
    float       roughnessFactor;
};

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
    int       meshIdx;
    glm::mat4 transformation;
};

struct ShaderParams
{
    glm::vec4 baseColor;
    glm::vec3 emissiveFactor;
    float     roughnessFactor;
    float     metallicFactor;
    bool      hasBaseTex;
    bool      hasRoughnessTex;
    bool      hasNormalTex;
    bool      hasEmissiveTex;
    bool      hasOcclusion;
    bool      hasTangent;
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
    CComPtr<ID3D12DescriptorHeap>             TextureDescriptorHeap;
    uint                                      NumDescriptors;
    D3D12_ROOT_DESCRIPTOR_TABLE               DescriptorTable;
    CComPtr<ID3D12Resource>                   ConstantBuffer;
    CComPtr<ID3D12Resource>                   ConstantBuffer2;
    uint                                      ConstantBufferIncrement;
    DynArray<Mesh>                            meshes;
    DynArray<SceneNode>                       sceneNodes;
    std::shared_ptr<char[]>                   pCpuConstantBuffer; // todo make this upload heap
    std::shared_ptr<char[]>                   pCpuConstantBuffer2; // inverse transpose normal matrix todo make this upload heap
    int                                       alignedMatrixSize;
    int                                       alignedConstantSize;
    DynArray<D3D12_GPU_VIRTUAL_ADDRESS>       cb0Ptrs; // todo refactor this, use buffer manager
    DynArray<D3D12_GPU_VIRTUAL_ADDRESS>       cb1Ptrs;
    glm::vec3                                 worldScale;
    glm::vec3                                 worldRotation;
    glm::vec3                                 worldPosition;
    DynArray<ShaderParams>                    shaderParams;
    DynArray<int>                             paramIdxs;
    int                                       totalPrimCount;
};