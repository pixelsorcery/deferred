#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <atlbase.h>
#include <vector>
#include "settings.h"

struct CmdSubmission
{
	CmdSubmission()
		: completionFenceVal(0)
	{
	}

	CComPtr<ID3D12CommandAllocator> cmdAlloc;

	// note 0 = not in flight
	UINT64 completionFenceVal;

	// things to free once submission retires
	std::vector<CComPtr<ID3D12DeviceChild>> deferredFrees;
};

enum vsyncType
{
	vsyncOn,
	vsyncOff
};

struct Texture
{
    ID3D12Resource* pRes;
    UINT            width;
    UINT            height;
};

struct DescriptorHeap
{
    CComPtr<ID3D12DescriptorHeap> pDescHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE   cpuDescHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE   gpuDescHandle;
    UINT                          descSize;

    void Init(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT size, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = heapType;
        desc.Flags = flags;
        desc.NumDescriptors = size;

        pDevice->CreateDescriptorHeap(&desc, __uuidof(ID3D12DescriptorHeap), (void**)&pDescHeap);
        cpuDescHandle = pDescHeap->GetCPUDescriptorHandleForHeapStart();
        gpuDescHandle = pDescHeap->GetGPUDescriptorHandleForHeapStart();
        descSize = pDevice->GetDescriptorHandleIncrementSize(desc.Type);
    }
};

struct Dx12Renderer
{
	CComPtr<IDXGISwapChain3>	       pSwapChain;
	CComPtr<ID3D12Device>		       pDevice;
	CComPtr<ID3D12CommandQueue>        pCommandQueue;
	CComPtr<ID3D12Fence>		       pSubmitFence;
	CComPtr<ID3D12GraphicsCommandList> pGfxCmdList;
#if defined(_DEBUG)
	CComPtr<ID3D12Debug>               debugController;
#endif
	HANDLE                        fenceEvent;
	CComPtr<ID3D12DescriptorHeap> rtvHeap;
	CComPtr<ID3D12Resource>       backbuf[renderer::swapChainBufferCount];
	CComPtr<ID3D12Resource>       dsv;
	D3D12_CPU_DESCRIPTOR_HANDLE   backbufDescHandle[renderer::swapChainBufferCount];
	DXGI_FORMAT                   backbufFormat; //TODO set this correctly?
	UINT                          backbufCurrent;

	D3D12_RECT     defaultScissor;
	D3D12_VIEWPORT defaultViewport;

	CmdSubmission cmdSubmissions[renderer::submitQueueDepth];
	UINT		  currentSubmission;

	UINT64        submitCount;

	static constexpr DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	~Dx12Renderer();
};

bool initDevice(Dx12Renderer* pRenderer, HWND hwnd);
ID3D12Resource* createBuffer(Dx12Renderer* pRenderer, D3D12_HEAP_TYPE heapType, UINT64 size, D3D12_RESOURCE_STATES states);
bool uploadBuffer(Dx12Renderer* pRenderer, ID3D12Resource* pResource, void const* data, UINT rowPitch, UINT slicePitch);
void transitionResource(Dx12Renderer* pRenderer, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
void waitOnFence(Dx12Renderer* pRenderer, ID3D12Fence* fence, UINT64 targetValue);
void setDefaultPipelineState(Dx12Renderer* pRenderer, D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);
void submitCmdBuffer(Dx12Renderer* pRenderer);
void present(Dx12Renderer* pRenderer, vsyncType vsync);
Texture createTexture(ID3D12Device* pDevice);