#pragma once
#include <d3d12.h>

class HeapMgr
{
public:
	HeapMgr* GetHeapMgr();

private:
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeap;
	HeapMgr();
	HeapMgr& HeapMgr(const& heapMgr);
	HeapMgr& operator=(const& heapMgr);
};