/*AMDG*/
#include "renderer.h"
#include "util.h"
#include "settings.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

bool initDevice(Dx12Renderer* pRenderer, HWND hwnd)
{
    HRESULT hr = S_OK;

    DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    CComPtr<IDXGIFactory1> dxgiFactory;
    hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr))
    {
        ErrorMsg("Failed to create DXGI Factory");
        return false;
    }

    CComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiFactory->EnumAdapters(0, &dxgiAdapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        ErrorMsg("No adapters found");
        return false;
    }

#if defined(_DEBUG)
	CComPtr<ID3D12Debug> debugController;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
    if (SUCCEEDED(hr))
    {
		debugController->EnableDebugLayer();
    }
    else
    {
        WarningMsg("Failed to create debug interface.");
    }
#endif

    // create device
    hr = D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pRenderer->pDevice));
    if (FAILED(hr))
    {
        ErrorMsg("D3D12CreateDevice failed");
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
        D3D12_COMMAND_LIST_TYPE_DIRECT,  // D3D12_COMMAND_LIST_TYPE Type;
        0,                               // INT Priority;
        D3D12_COMMAND_QUEUE_FLAG_NONE,   // D3D12_COMMAND_QUEUE_FLAG Flags;
        0                                // UINT NodeMask;
    };

    hr = pRenderer->pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&pRenderer->pCommandQueue));
    if (FAILED(hr))
    {
        ErrorMsg("Couldn't create command queue");
        return false;
    }

    // create swapchain
    CComPtr<IDXGISwapChain> pSwapChain;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = renderer::swapChainBufferCount;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;
    swapChainDesc.BufferDesc.Width = renderer::width;
    swapChainDesc.BufferDesc.Height = renderer::height;
    swapChainDesc.BufferDesc.Format = colorFormat;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    hr = dxgiFactory->CreateSwapChain(pRenderer->pCommandQueue, &swapChainDesc, &pSwapChain);
    if (FAILED(hr))
    {
        ErrorMsg("Couldn't create swapchain");
        return false;
    }

    // Query for swapchain3 swapchain
    hr = pSwapChain->QueryInterface(&pRenderer->pSwapChain);
    if (FAILED(hr))
    {
		ErrorMsg("Couldn't create swapchain3");
        return 0;
    }

	// Submission fence
	uint64 submitCount = 0;
	hr = pRenderer->pDevice->CreateFence(submitCount, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pRenderer->pSubmitFence));
	if (FAILED(hr))
	{
		ErrorMsg("Couldn't create submission fence");
		return 0;
	}

	// create render target view descriptor heap for displayable back buffers
	D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
										renderer::swapChainBufferCount,
										D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
										0 };

	hr = pRenderer->pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRenderer->rtvHeap));
	if (FAILED(hr))
	{
		ErrorMsg("Failed to create render target heap");
		return 0;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescHandle = pRenderer->rtvHeap->GetCPUDescriptorHandleForHeapStart();
	uint rtvDescHandleIncSize = pRenderer->pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Set up backbuffer heap
	for (uint i = 0; i < renderer::swapChainBufferCount; ++i)
	{
		hr = pRenderer->pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pRenderer->backbuf[i]));
		if (FAILED(hr))
		{
			ErrorMsg("Failed to retrieve back buffer from swapchain");
			return 0;
		}

		// Create RTVs in heap
		pRenderer->pDevice->CreateRenderTargetView(pRenderer->backbuf[i], nullptr, rtvDescHandle);
		rtvDescHandle.ptr += rtvDescHandleIncSize;
	}

	pRenderer->backbufCurrent = pRenderer->pSwapChain->GetCurrentBackBufferIndex();

	// default scissor rect and viewport
	pRenderer->defaultScissor = {};
	pRenderer->defaultScissor.left   = 0;
	pRenderer->defaultScissor.top    = 0;
	pRenderer->defaultScissor.right  = renderer::width;
	pRenderer->defaultScissor.bottom = renderer::height;

	pRenderer->defaultViewport = {};
	pRenderer->defaultViewport.Height   = static_cast<float>(renderer::height);
	pRenderer->defaultViewport.Width    = static_cast<float>(renderer::width);
	pRenderer->defaultViewport.TopLeftX = 0;
	pRenderer->defaultViewport.TopLeftY = 0;
	pRenderer->defaultViewport.MaxDepth = 1.0f;
	pRenderer->defaultViewport.MinDepth = 0.0f;

    return true;
}

Dx12Renderer::~Dx12Renderer()
{
	return;
}
