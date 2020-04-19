#pragma once

#include <d3d12.h>

const wchar_t* heapTypeStrings[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
{
    L"CBV_SRV_UAV_Heap",
    L"Sampler_Heap",
    L"RTV_Heap",
    L"DSV_Heap",
};
