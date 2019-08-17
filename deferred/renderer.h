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

struct Dx12Renderer
{
	CComPtr<IDXGISwapChain3>	pSwapChain;
	CComPtr<ID3D12Device>		pDevice;
	CComPtr<ID3D12CommandQueue> pCommandQueue;
	CComPtr<ID3D12Fence>		pSubmitFence;

	CComPtr<ID3D12GraphicsCommandList> pGfxCmdList;

	CComPtr<ID3D12DescriptorHeap> rtvHeap;
	CComPtr<ID3D12Resource>       backbuf[renderer::swapChainBufferCount];
	CComPtr<ID3D12Resource>       dsv;
	D3D12_CPU_DESCRIPTOR_HANDLE   backbufDescHandle[renderer::swapChainBufferCount];
	DXGI_FORMAT                   backbufFormat;
	UINT                          backbufCurrent;

	D3D12_RECT defaultScissor;
	D3D12_VIEWPORT defaultViewport;

	CmdSubmission cmdSubmissions[renderer::submitQueueDepth];
	UINT		  currentSubmission;
	~Dx12Renderer() = default;
};

bool initDevice(Dx12Renderer* pRenderer, HWND hwnd);
ID3D12Resource* createBuffer(Dx12Renderer* pRenderer, D3D12_HEAP_TYPE heapType, UINT64 size, D3D12_RESOURCE_STATES states);
bool uploadBuffer(Dx12Renderer* pRenderer, ID3D12Resource* pResource, void const* data, UINT rowPitch, UINT slicePitch);
void transitionResource(Dx12Renderer* pRenderer, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
bool waitOnFence(Dx12Renderer* pRenderer, ID3D12Fence* fence, UINT64 targetValue);
void setDefaultPipelineState(Dx12Renderer* pRenderer, D3D12_GRAPHICS_PIPELINE_STATE_DESC* decs);
void swapBuffers(Dx12Renderer* pRenderer, vsyncType vsync);