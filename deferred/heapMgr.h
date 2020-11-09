#pragma once

#include <atlbase.h>
#include <d3d12.h>
#include "settings.h"

struct HeapMgr
{
    int heapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { 128, 64, 64, 64 };
    int descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    int descriptorIndexes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    D3D12_GPU_DESCRIPTOR_HANDLE heapWritePtrs[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    CComPtr<ID3D12DescriptorHeap> mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    HRESULT initializeHeaps(ID3D12Device* pDevice);

    // Writes descriptors to heap, returns pointer to start
    D3D12_GPU_DESCRIPTOR_HANDLE copyDescriptorsToGpuHeap(ID3D12Device* pDevice,
                                                         D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                         D3D12_CPU_DESCRIPTOR_HANDLE src,
                                                         int numDescriptors);

    int activeHeapEntries[renderer::swapChainBufferCount];
    int totalActive;
};