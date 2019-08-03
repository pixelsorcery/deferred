#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <atlbase.h>
#include "settings.h"

struct Dx12Renderer
{
	CComPtr<IDXGISwapChain3>	pSwapChain;
	CComPtr<ID3D12Device>		pDevice;
	CComPtr<ID3D12CommandQueue> pCommandQueue;
	CComPtr<ID3D12Fence>		pSubmitFence;

	CComPtr<ID3D12DescriptorHeap> rtvHeap;
	CComPtr<ID3D12Resource>       backbuf[renderer::swapChainBufferCount];
	CComPtr<ID3D12Resource>       dsv;
	D3D12_CPU_DESCRIPTOR_HANDLE   backbufDescHandle[renderer::swapChainBufferCount];
	DXGI_FORMAT                   backbufFormat;
	UINT                          backbufCurrent;

	D3D12_RECT defaultScissor;
	D3D12_VIEWPORT defaultViewport;

	~Dx12Renderer();
};

bool initDevice(Dx12Renderer* pDevice, HWND hwnd);