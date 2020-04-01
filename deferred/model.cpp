#include "model.h"
#include "gltfHelper.h"
#include "util.h"
#include "dynArray.h"

bool loadModel(Dx12Renderer* pRenderer, GltfModel& model, const char* filename)
{
    ID3D12Device* pDevice = pRenderer->pDevice;
    HRESULT hr = S_OK;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model.TinyGltfModel, &err, &warn, filename);

    if (res == false) return false;

    DynArray<D3D12_RESOURCE_DESC> arr;

    tinygltf::Model* pModel = &model.TinyGltfModel;

    // create textures (TODO: add this to renderer and generalize this to create RTs and DBs)
    for (size_t i = 0; i < pModel->images.size(); i++)
    {
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Height = pModel->images[i].height;
        resourceDesc.Width = pModel->images[i].width;
        resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;// GetFormat(pModel->images[i].component, pModel->images[i].pixel_type); //todo verify this
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.DepthOrArraySize = 1;

        D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COPY_DEST;

        ID3D12Resource* textureResource = nullptr;
        hr = pDevice->CreateCommittedResource(&heapProps, flags, &resourceDesc, resourceState, nullptr, __uuidof(ID3D12Resource), (void**)&textureResource);

        if (FAILED(hr))
        {
            ErrorMsg("Failed to create texture for model.");
        }

        // todo: upload texture data here
        for (auto tex : pModel->images)
        {
            uploadTexture(pRenderer,
                textureResource,
                &tex.image[0],
                tex.width,
                tex.height,
                tex.component,
                resourceDesc.Format);
        }

        Texture tex = {};
        tex.pRes = textureResource;
        tex.desc = resourceDesc;
        model.Textures.push_back(tex);

        arr.push(resourceDesc);
    }

    // Create cpu descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = arr.size;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags;

    hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&model.TextureDescriptorHeap));
    if (FAILED(hr))
    {
        ErrorMsg("Descriptor heap creation for model failed.");
        return false;
    }

    model.TextureDescriptorHeap->SetName(L"Texture Descriptor Heap for Model");

    uint heapIncrementSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   
    // Create srvs
    for (uint i = 0; i < arr.size; i++)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = arr[i].Format;
        srvDesc.Texture2D.MipLevels = arr[i].MipLevels;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        D3D12_CPU_DESCRIPTOR_HANDLE descHandle = model.TextureDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        pDevice->CreateShaderResourceView(model.Textures[i].pRes, &srvDesc, descHandle);

        descHandle.ptr += pRenderer->cbvSrvUavDescriptorSize;
    }

    return res;
}