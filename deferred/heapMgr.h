#pragma once

#include <atlbase.h>
#include <d3d12.h>
#include "settings.h"
#include "texture.h"

enum class resourceViewType
{
    SRV,
    RTV,
    UAV,
    CBV,
    DSV
};

struct HeapMgr
{
    int heapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { 2048, 256, 256, 256 };
    int cpuHeapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { 2048, 256, 256, 256 };
    int descriptorSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    int descriptorIndexes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    int cpuDescriptorIndexes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    D3D12_GPU_DESCRIPTOR_HANDLE heapWritePtrs[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    CComPtr<ID3D12DescriptorHeap> mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    CComPtr<ID3D12DescriptorHeap> cpuDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    HRESULT initializeHeaps(ID3D12Device* pDevice);

    // Writes descriptors to heap, returns pointer to start
    D3D12_GPU_DESCRIPTOR_HANDLE copyDescriptorsToGpuHeap(ID3D12Device* pDevice,
                                                         D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                         D3D12_CPU_DESCRIPTOR_HANDLE src,
                                                         int numDescriptors);

    int createSRVDescriptor(ID3D12Device* pDevice, const Texture& tex);
    int createUAVDescriptor(ID3D12Device* pDevice, const Texture& tex, int mipSlice);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE heapType, int heapIdx);
};