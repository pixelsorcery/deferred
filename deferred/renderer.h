#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <atlbase.h>
#include <vector>
#include "settings.h"
#include "util.h"
#include "camera.h"
#include "glm/glm.hpp"

struct CmdSubmission
{
    CmdSubmission()
        : completionFenceVal(0)
    {
    }

    CComPtr<ID3D12CommandAllocator>    cmdAlloc;
    CComPtr<ID3D12GraphicsCommandList> pGfxCmdList;

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
    CComPtr<ID3D12Resource> pRes;
    D3D12_RESOURCE_DESC     desc;
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
    CComPtr<IDXGISwapChain3>      pSwapChain;
    CComPtr<ID3D12Device>         pDevice;
    CComPtr<ID3D12CommandQueue>   pCommandQueue;
    CComPtr<ID3D12Fence>          pSubmitFence;
#if defined(_DEBUG)
    CComPtr<ID3D12Debug>          debugController;
    CComPtr<ID3D12DebugDevice>    debugDevice;
#endif
    HANDLE                        fenceEvent;
    CComPtr<ID3D12DescriptorHeap> rtvHeap;
    CComPtr<ID3D12Resource>       backbuf[renderer::swapChainBufferCount];
    CComPtr<ID3D12Resource>       depthStencil;
    D3D12_CPU_DESCRIPTOR_HANDLE   backbufDescHandle[renderer::swapChainBufferCount];
    D3D12_CPU_DESCRIPTOR_HANDLE   dsDescHandle;
    DXGI_FORMAT                   backbufFormat; //TODO set this correctly?
    UINT                          backbufCurrent;

    D3D12_RECT     defaultScissor;
    D3D12_VIEWPORT defaultViewport;

    CmdSubmission cmdSubmissions[renderer::submitQueueDepth];
    UINT          currentSubmission;

    UINT64        submitCount;

    UINT          cbvSrvUavDescriptorSize;
    UINT          rtvDescriptorSize;
    UINT          dsvDescriptorSize;

    static constexpr DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr DXGI_FORMAT depthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // initial heap sizes
    int heapSizes[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { 128, 64, 64, 64 };

    CComPtr<ID3D12DescriptorHeap> mainDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    CComPtr<ID3D12Resource> cbvSrvUavUploadHeaps[renderer::swapChainBufferCount];

    ID3D12GraphicsCommandList* GetCurrentCmdList(){ return cmdSubmissions[currentSubmission].pGfxCmdList; };

    Camera camera;

    glm::mat4 projection;

    ~Dx12Renderer();
};

bool initDevice(Dx12Renderer* pRenderer, HWND hwnd);
CComPtr<ID3D12Resource> createBuffer(const Dx12Renderer* pRenderer, D3D12_HEAP_TYPE heapType, uint size, D3D12_RESOURCE_STATES states);
bool uploadBuffer(Dx12Renderer* pRenderer, ID3D12Resource* pResource, void const* data, uint rowPitch, uint slicePitch);
void transitionResource(Dx12Renderer* pRenderer, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
void waitOnFence(Dx12Renderer* pRenderer, ID3D12Fence* fence, UINT64 targetValue);
void setDefaultPipelineState(Dx12Renderer* pRenderer, D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc);
void submitCmdBuffer(Dx12Renderer* pRenderer);
void present(Dx12Renderer* pRenderer, vsyncType vsync);
Texture createTexture(Dx12Renderer* pRenderer, D3D12_HEAP_TYPE heapType, uint width, uint height, DXGI_FORMAT format);
bool uploadTexture(Dx12Renderer* pRenderer, ID3D12Resource* pResource, void const* data, uint width, uint height, uint comp, DXGI_FORMAT format);