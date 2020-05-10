/*AMDG*/
#include "renderer.h"
#include "util.h"
#include "settings.h"
#include "strings.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

// taken from https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
// see: https://thxforthefish.com/posts/reverse_z/
// also: http://dev.theomader.com/depth-precision/
static glm::mat4 MakeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float zNear)
{
    float f = 1.0f / tan(fovY_radians / 2.0f);
    return glm::mat4(f / aspectWbyH, 0.0f, 0.0f, 0.0f,
                     0.0f, f, 0.0f, 0.0f,
                     0.0f, 0.0f, 0.0f, -1.0f,
                     0.0f, 0.0f, zNear, 0.0f);
}

bool initDevice(Dx12Renderer* pRenderer, HWND hwnd)
{
    HRESULT hr = S_OK;

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
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pRenderer->debugController));
    if (SUCCEEDED(hr))
    {
        pRenderer->debugController->EnableDebugLayer();
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

    pRenderer->pDevice->SetName(L"main_device");

    ID3D12Device* pDevice = pRenderer->pDevice;

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
        D3D12_COMMAND_LIST_TYPE_DIRECT,  // D3D12_COMMAND_LIST_TYPE Type;
        0,                               // INT Priority;
        D3D12_COMMAND_QUEUE_FLAG_NONE,   // D3D12_COMMAND_QUEUE_FLAG Flags;
        0                                // UINT NodeMask;
    };

    hr = pDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&pRenderer->pCommandQueue));
    if (FAILED(hr))
    {
        ErrorMsg("Couldn't create command queue");
        return false;
    }

    for (UINT i = 0; i < renderer::submitQueueDepth; i++)
    {
        pRenderer->currentSubmission = 0;
        CmdSubmission* sub = &pRenderer->cmdSubmissions[i];

        // command allocator
        hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&sub->cmdAlloc));
        if (FAILED(hr)) { ErrorMsg("Couldn't create command allocator"); }
        sub->cmdAlloc->SetName(L"cmd_allocator");
        sub->cmdAlloc->Reset();

        // command list
        hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, sub->cmdAlloc, nullptr, IID_PPV_ARGS(&sub->pGfxCmdList));
        if (FAILED(hr)) { ErrorMsg("Couldn't create command list"); }
        sub->pGfxCmdList->SetName(L"cmd_list");
        sub->pGfxCmdList->Close();
        sub->pGfxCmdList->Reset(sub->cmdAlloc, nullptr);

        if (i > 0)
        {
            sub->pGfxCmdList->Close();
        }
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
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;
    swapChainDesc.BufferDesc.Width = renderer::width;
    swapChainDesc.BufferDesc.Height = renderer::height;
    swapChainDesc.BufferDesc.Format = pRenderer->colorFormat;
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
    uint64 submitCount = 1;
    hr = pDevice->CreateFence(submitCount, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pRenderer->pSubmitFence));
    if (FAILED(hr))
    {
        ErrorMsg("Couldn't create submission fence");
        return 0;
    }

    pRenderer->fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // create render target view descriptor heap for displayable back buffers
    D3D12_DESCRIPTOR_HEAP_DESC desc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                        renderer::swapChainBufferCount,
                                        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
                                        0 };

    hr = pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pRenderer->rtvHeap));
    if (FAILED(hr))
    {
        ErrorMsg("Failed to create render target heap");
        return 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescHandle = pRenderer->rtvHeap->GetCPUDescriptorHandleForHeapStart();
    uint rtvDescHandleIncSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Set up backbuffer heap
    for (uint i = 0; i < renderer::swapChainBufferCount; ++i)
    {
        hr = pRenderer->pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pRenderer->backbuf[i]));
        if (FAILED(hr))
        {
            ErrorMsg("Failed to retrieve back buffer from swapchain");
            return 0;
        }

        pRenderer->backbufDescHandle[i] = rtvDescHandle;

        // Create RTVs in heap
        pDevice->CreateRenderTargetView(pRenderer->backbuf[i], nullptr, rtvDescHandle);
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

    pRenderer->cbvSrvUavDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    pRenderer->rtvDescriptorSize       = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    pRenderer->dsvDescriptorSize       = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // create gpu descriptor heaps
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = pRenderer->heapSizes[i];
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);

        if (heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
            heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
        {
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        }

        hr = pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&pRenderer->mainDescriptorHeaps[i]));
        if (FAILED(hr))
        {
            ErrorMsg("Descriptor heap creation failed.");
            return false;
        }

        pRenderer->mainDescriptorHeaps[i]->SetName(heapTypeStrings[i]);
    }

    // create cb upload heaps // todo make this one ring buffer
    for (int i = 0; i < renderer::swapChainBufferCount; ++i)
    {
        CComPtr<ID3D12Resource> cbvSrvUploadHeap;
        CComPtr<ID3D12Resource> cbvSrvHeap;
        int size = 1024 * 64; // max size is 64k
        cbvSrvUploadHeap = createBuffer(pRenderer, D3D12_HEAP_TYPE_UPLOAD, size, D3D12_RESOURCE_STATE_GENERIC_READ);

        pRenderer->cbvSrvUavUploadHeaps[i] = cbvSrvUploadHeap;
    }

    // create main depth stencil
    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Height    = renderer::height;
    resourceDesc.Width     = renderer::width;
    resourceDesc.Format    = DXGI_FORMAT_R32_TYPELESS;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    D3D12_HEAP_FLAGS flags = D3D12_HEAP_FLAG_NONE;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil.Depth = 0.0;

    hr = pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, resourceState, &clearValue, __uuidof(ID3D12Resource), (void**)&pRenderer->depthStencil);

    if (FAILED(hr))
    {
        ErrorMsg("Failed to create depth stencil.");
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

    pRenderer->dsDescHandle = pRenderer->mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart();
    pDevice->CreateDepthStencilView(pRenderer->depthStencil, &dsvDesc, pRenderer->dsDescHandle);

    // init view
    pRenderer->camera.init(glm::vec3(0.0f, 20.0f, 30.0f), // eye
                           glm::vec3(0.0f, 0.0f, 0.0f),   // center
                           glm::vec3(0.0f, 1.0f, 0.0f));  // up

    // init perspective projection matrix 
    pRenderer->projection = MakeInfReversedZProjRH(glm::radians((float)renderer::fov), 
                                                   (float)renderer::width / (float)renderer::height,
                                                   0.1f);
    return true;
}

CComPtr<ID3D12Resource> createBuffer(const Dx12Renderer* pRenderer, D3D12_HEAP_TYPE heapType, uint size, D3D12_RESOURCE_STATES states)
{
    HRESULT hr = S_OK;

    D3D12_HEAP_PROPERTIES heapProps = {};
    D3D12_RESOURCE_DESC desc = {};
    CComPtr<ID3D12Resource> resource;

    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = pRenderer->pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, states, nullptr, IID_PPV_ARGS(&resource));

    if (FAILED(hr)) 
    { 
        ErrorMsg("Buffer creation failed.");  
        return nullptr;
    }

    return resource;
}

Texture createTexture(Dx12Renderer* pRenderer, D3D12_HEAP_TYPE heapType, uint width, uint height, DXGI_FORMAT format)
{
    Texture tex = {};

    HRESULT hr = S_OK;
    D3D12_HEAP_PROPERTIES heapProps = {};
    D3D12_RESOURCE_DESC desc = {};

    heapProps.Type = heapType;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    hr = pRenderer->pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
        &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&tex.pRes));
    tex.desc = desc;

    return tex;
}

bool uploadTexture(Dx12Renderer* pRenderer, ID3D12Resource* pResource, void const* data, uint width, uint height, uint comp, DXGI_FORMAT format)
{
    HRESULT hr = S_OK;
    ID3D12Device* pDevice = pRenderer->pDevice;

    D3D12_RESOURCE_DESC desc = pResource->GetDesc();

    // create upload buffer
    CComPtr<ID3D12Resource> uploadTemp;
    uploadTemp = createBuffer(pRenderer, D3D12_HEAP_TYPE_UPLOAD, width * height * comp, D3D12_RESOURCE_STATE_GENERIC_READ);
    uploadTemp->SetName(L"Temp Upload Texture");

    unsigned char* pBufData;
    uploadTemp->Map(0, nullptr, reinterpret_cast<void**>(&pBufData));

    // copy data to buffer
    memcpy(pBufData, data, static_cast<size_t>(width) * height * comp);
    uploadTemp->Unmap(0, nullptr);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
    uint64 rowSize, totalBytes;
    pDevice->GetCopyableFootprints(&desc, 0, 1, 0, &fp, nullptr, &rowSize, &totalBytes);

    D3D12_TEXTURE_COPY_LOCATION srcLoc, destLoc;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.pResource = uploadTemp;
    srcLoc.PlacedFootprint = fp;

    destLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    destLoc.pResource = pResource;
    destLoc.SubresourceIndex = 0;

    pRenderer->GetCurrentCmdList()->CopyTextureRegion(&destLoc, 0, 0, 0, &srcLoc, nullptr);
    pRenderer->cmdSubmissions[pRenderer->currentSubmission].deferredFrees.push_back((ID3D12DeviceChild*)uploadTemp);
    uploadTemp.Release();

    return true;
}

void transitionResource(Dx12Renderer* pRenderer, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = res;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    pRenderer->GetCurrentCmdList()->ResourceBarrier(1, &barrier);
}

bool uploadBuffer(Dx12Renderer* pRenderer, ID3D12Resource* pResource, void const* data, uint rowPitch, uint slicePitch)
{
    HRESULT hr = S_OK;
    ID3D12Device* pDevice = pRenderer->pDevice;

    D3D12_RESOURCE_DESC desc = pResource->GetDesc();
    uint totalBytes = static_cast<uint64>(rowPitch) * ((slicePitch > 0) ? slicePitch : 1);

    // create upload buffer (todo: if we have to, otherwise set offset and reuse)
    CComPtr<ID3D12Resource> uploadTemp;
    uploadTemp = createBuffer(pRenderer, D3D12_HEAP_TYPE_UPLOAD, totalBytes, D3D12_RESOURCE_STATE_GENERIC_READ);

    // map the upload resource
    BYTE* pBufData = nullptr;
    hr = uploadTemp->Map(0, nullptr, (void**)& pBufData);
    if (FAILED(hr)) 
    { 
        ErrorMsg("Mapping upload heap failed.");  
        return false;
    }

    // write data to upload resource
    for (UINT z = 0; z < desc.DepthOrArraySize; ++z)
    {
        BYTE const* pSource = (BYTE const*)data + z * slicePitch;
        for (UINT y = 0; y < desc.Height; ++y)
        {
            memcpy(pBufData, pSource, SIZE_T(totalBytes));
        }
    }

    // unmap 
    D3D12_RANGE written = { 0, (SIZE_T)totalBytes };
    uploadTemp->Unmap(0, &written);

    pRenderer->GetCurrentCmdList()->CopyBufferRegion(pResource, 0, uploadTemp, 0, totalBytes);

    pRenderer->cmdSubmissions[pRenderer->currentSubmission].deferredFrees.push_back((ID3D12DeviceChild*)uploadTemp);

    return true;
}

void setDefaultPipelineState(Dx12Renderer* pRenderer, D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc)
{
    ZeroMemory(desc, sizeof(*desc));
    desc->BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc->SampleMask = ~0u;
    desc->RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc->RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc->RasterizerState.DepthClipEnable = TRUE;
    desc->IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    desc->PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc->NumRenderTargets = 1;
    desc->RTVFormats[0] = pRenderer->colorFormat;
    desc->SampleDesc.Count = 1;
}

void waitOnFence(Dx12Renderer* pRenderer, ID3D12Fence* fence, UINT64 targetValue)
{
    HRESULT hr = S_OK;

    pRenderer->pCommandQueue->Signal(fence, targetValue);

    if (fence->GetCompletedValue() < targetValue)
    {
        fence->SetEventOnCompletion(targetValue, pRenderer->fenceEvent);
        WaitForSingleObject(pRenderer->fenceEvent, INFINITE);
    }
}


void submitCmdBuffer(Dx12Renderer* pRenderer)
{
    HRESULT hr = S_OK;
    ID3D12GraphicsCommandList* const pCmdList = pRenderer->GetCurrentCmdList();
    hr = pCmdList->Close();

    if (FAILED(hr))
    {
        ErrorMsg("Failed to close gfx cmd list.");
        return;
    }

    ID3D12CommandList* ppCmds[] = { pRenderer->GetCurrentCmdList() };
    pRenderer->pCommandQueue->ExecuteCommandLists(1, ppCmds);

    // Insert a signal event for current frame so we can wait for it to be done if we have to
    CmdSubmission* pSub = &pRenderer->cmdSubmissions[pRenderer->currentSubmission];
    hr = pRenderer->pCommandQueue->Signal(pRenderer->pSubmitFence, pSub->completionFenceVal);

    if (FAILED(hr))
    {
        ErrorMsg("Failed to signal cmd queue.");
        return;
    }

    // Get next submission and wait if we have to
    UINT nextSubmissionIdx = (pRenderer->currentSubmission + 1) % renderer::submitQueueDepth;

    if (pRenderer->pSubmitFence->GetCompletedValue() < pRenderer->cmdSubmissions[nextSubmissionIdx].completionFenceVal)
    {
        pRenderer->pSubmitFence->SetEventOnCompletion(pRenderer->cmdSubmissions[nextSubmissionIdx].completionFenceVal, pRenderer->fenceEvent);
        WaitForSingleObject(pRenderer->fenceEvent, INFINITE);
    }

    pSub = &pRenderer->cmdSubmissions[nextSubmissionIdx];

    // Clear allocations in new submission
    pSub->deferredFrees.clear();

    // Update current submission idx
    pRenderer->currentSubmission = nextSubmissionIdx;

    // update timestamp
    pRenderer->submitCount++;
    pSub->completionFenceVal = pRenderer->submitCount;

    // switch cmd list over
    pSub->cmdAlloc->Reset();
    hr = pRenderer->GetCurrentCmdList()->Reset(pSub->cmdAlloc, nullptr);

    if (FAILED(hr)) 
    { 
        ErrorMsg("gfxCmdList->Reset(cmdSubmission[next_submission].cmdAlloc, nullptr) failed.\n");
        return; 
    }
}

void present(Dx12Renderer* pRenderer, vsyncType vsync)
{
    HRESULT hr = S_OK;
    // present
    DXGI_PRESENT_PARAMETERS pp = { 0, nullptr, nullptr, nullptr };
    hr = pRenderer->pSwapChain->Present1((vsync == vsyncOn) ? 1 : 0, (vsync == vsyncOn) ? 0 : DXGI_PRESENT_RESTART, &pp);
    if (FAILED(hr)) 
    { 
        ErrorMsg("present failed.\n");
        return;
    }

    pRenderer->backbufCurrent = pRenderer->pSwapChain->GetCurrentBackBufferIndex();
}

Dx12Renderer::~Dx12Renderer()
{
    // make sure everything is done on GPU before destroying device
    for (int i = 0; i < renderer::submitQueueDepth; i++)
    {
        waitOnFence(this, pSubmitFence, cmdSubmissions[i].completionFenceVal);
    }
#if defined(_DEBUG)
    //CComPtr<ID3D12DebugDevice> debugDevice;
    //HRESULT hr = pDevice->QueryInterface(__uuidof(ID3D12DebugDevice1), reinterpret_cast<void**>(&debugDevice));
    //debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
#endif
}
