#include "heapMgr.h"
#include "util.h"
#include "strings.h"
#include "renderer.h"

HRESULT HeapMgr::initializeHeaps(ID3D12Device* pDevice)
{
    HRESULT hr = S_OK;
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = heapSizes[i];
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);

        if (heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
            heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
        {
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }

        D3D12_DESCRIPTOR_HEAP_DESC cpuHeapDesc = heapDesc;
        cpuHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeaps[i]));
        if (FAILED(hr))
        {
            ErrorMsg("GPU descriptor heap creation failed.");
            return hr;
        }

        hr = pDevice->CreateDescriptorHeap(&cpuHeapDesc, IID_PPV_ARGS(&cpuDescriptorHeaps[i]));
        if (FAILED(hr))
        {
            ErrorMsg("CPU descriptor heap creation failed.");
            return hr;
        }

        descriptorSizes[i] = pDevice->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

        mainDescriptorHeaps[i]->SetName(heapTypeStrings[i]);
        cpuDescriptorHeaps[i]->SetName(heapTypeStrings[i]);
    }

    return hr;
}

int HeapMgr::createSRVDescriptor(ID3D12Device* pDevice, const Texture& tex)
{
    D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    int idx = cpuDescriptorIndexes[heapType];

    // unlike the GPU heap, the CPU heap always grows
    if (cpuDescriptorIndexes[heapType] >= cpuHeapSizes[heapType])
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        cpuHeapSizes[heapType] *= 2;
        heapDesc.NumDescriptors = cpuHeapSizes[heapType];
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.Type = heapType;

        CComPtr<ID3D12DescriptorHeap> newHeapPtr;
        // TODO: implement the rest of this, copy old heap to new heap, etc
        HRESULT hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&newHeapPtr));
        if (FAILED(hr))
        {
            ErrorMsg("CPU descriptor heap re-size failed.");
            return -1;
        }
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = tex.desc.Format;
    srvDesc.Texture2D.MipLevels = tex.desc.MipLevels;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    D3D12_CPU_DESCRIPTOR_HANDLE descHandle = cpuDescriptorHeaps[heapType]->GetCPUDescriptorHandleForHeapStart();
    descHandle.ptr += descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * static_cast<int64>(idx);

    pDevice->CreateShaderResourceView(tex.pRes, &srvDesc, descHandle);

    cpuDescriptorIndexes[heapType]++;

    return idx;
}

int HeapMgr::createUAVDescriptor(ID3D12Device* pDevice, const Texture& tex, int mipSlice)
{
    D3D12_DESCRIPTOR_HEAP_TYPE heapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    int idx = cpuDescriptorIndexes[heapType];

    // unlike the GPU heap, the CPU heap always grows
    if (cpuDescriptorIndexes[heapType] >= cpuHeapSizes[heapType])
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        cpuHeapSizes[heapType] *= 2;
        heapDesc.NumDescriptors = cpuHeapSizes[heapType];
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.Type = heapType;

        HRESULT hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&cpuDescriptorHeaps[heapType]));
        if (FAILED(hr))
        {
            ErrorMsg("CPU descriptor heap re-size failed.");
            return -1;
        }
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = tex.desc.Format;
    uavDesc.Texture2D.MipSlice = mipSlice;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    D3D12_CPU_DESCRIPTOR_HANDLE descHandle = cpuDescriptorHeaps[heapType]->GetCPUDescriptorHandleForHeapStart();
    descHandle.ptr += descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] * static_cast<int64>(idx);

    pDevice->CreateUnorderedAccessView(tex.pRes, nullptr, &uavDesc, descHandle);

    cpuDescriptorIndexes[heapType]++;

    return idx;
}

D3D12_CPU_DESCRIPTOR_HANDLE HeapMgr::getCPUDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, int heapIdx)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = cpuDescriptorHeaps[heapType]->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += descriptorSizes[heapType] * static_cast<int64>(heapIdx);
    return handle;
}

// TODO: keep track of which entries are in flight and assert if we overwrite entries still in use.
D3D12_GPU_DESCRIPTOR_HANDLE HeapMgr::copyDescriptorsToGpuHeap(ID3D12Device* pDevice,
                                                              D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                              D3D12_CPU_DESCRIPTOR_HANDLE src,
                                                              int numDescriptors)
{
    D3D12_CPU_DESCRIPTOR_HANDLE dest    = mainDescriptorHeaps[heapType]->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE gpuDest = mainDescriptorHeaps[heapType]->GetGPUDescriptorHandleForHeapStart();

    if (numDescriptors + descriptorIndexes[heapType] > heapSizes[heapType])
    {
        descriptorIndexes[heapType] = 0;
    }
    
    dest.ptr    += static_cast<size_t>(descriptorSizes[heapType]) * descriptorIndexes[heapType];
    gpuDest.ptr += static_cast<size_t>(descriptorSizes[heapType]) * descriptorIndexes[heapType];

    pDevice->CopyDescriptorsSimple(numDescriptors, dest, src, heapType);
    descriptorIndexes[heapType] += numDescriptors;

    return gpuDest;
}