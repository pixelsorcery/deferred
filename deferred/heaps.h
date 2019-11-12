#pragma once
#include <d3d12.h>
#include <memory>

class HeapMgr
{
public:
    HeapMgr* GetHeapMgr();

private:
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeap;
    HeapMgr();
    HeapMgr(const HeapMgr&);
    HeapMgr& operator=(const HeapMgr&);
};