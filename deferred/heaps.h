#pragma once
#include <d3d12.h>
#include <atlbase.h>

class Dx12UploadHeap
{
public:
    Dx12UploadHeap(D3D12_HEAP_TYPE heapType, int size);
    void Reset();

    // Copy data to heap
    //void uploadData(ID3D12Resource* dest, int size, )
    // Mapped pointer
private:
    CComPtr<ID3D12Resource>
};

struct Dx12GpuHeap
{

};