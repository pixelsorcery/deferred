#include "heapMgr.h"
#include "util.h"
#include "strings.h"

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

        hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeaps[i]));
        if (FAILED(hr))
        {
            ErrorMsg("Descriptor heap creation failed.");
            return hr;
        }

        descriptorSizes[i] = pDevice->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

        mainDescriptorHeaps[i]->SetName(heapTypeStrings[i]);
    }

    return hr;
}

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