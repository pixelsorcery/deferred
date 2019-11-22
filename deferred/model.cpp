#include "model.h"
#include "gltfHelper.h"
#include "util.h"

bool loadModel(ID3D12Device* pDevice, GltfModel& model, const char* filename)
{
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model.TinyGltfModel, &err, &warn, filename);

    if (res == false) return false;

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
        resourceDesc.Format = GetFormat(pModel->images[i].component, pModel->images[i].pixel_type); //todo verify this
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.DepthOrArraySize = 1;

        D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COPY_DEST;

        ID3D12Resource* textureResource = nullptr;
        HRESULT hr = pDevice->CreateCommittedResource(&heapProps, flags, &resourceDesc, resourceState, nullptr, __uuidof(ID3D12Resource), (void**)&textureResource);

        if (FAILED(hr))
        {
            ErrorMsg("Failed to create texture for model.");
        }

        model.Textures.push_back(textureResource);
    }

    return res;
}