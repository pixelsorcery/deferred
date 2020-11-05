/*AMDG*/
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "app3.h"
#include "util.h"
#include "shaders.h"
#include "gltfHelper.h"

using namespace std;

App3* app3 = new App3();

App3::App3() : pRenderer(new Dx12Renderer())
{};

bool App3::init(HWND hwnd)
{
    bool result = true;
    initDevice(pRenderer.get(), hwnd);

    ID3D12Device* pDevice = pRenderer->pDevice;

    result = initEffect(pRenderer.get(), &sunset, "fullscreentri.hlsl", "sunset.hlsl");
    
    if (result == true)
    {
        result = initEffect(pRenderer.get(), &road, "fullscreentri.hlsl", "roadGrid.hlsl");
    }

    return result;
}

void App3::drawFrame(float time)
{
    // clear
    HRESULT hr = S_OK;

    ID3D12Device* pDevice = pRenderer->pDevice;
    ID3D12GraphicsCommandList* pCmdList = pRenderer->cmdSubmissions[pRenderer->currentSubmission].pGfxCmdList;

    // set rt
    pCmdList->OMSetRenderTargets(1, &pRenderer->backbufDescHandle[pRenderer->currentSubmission], false, &pRenderer->dsDescHandle);

    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->currentSubmission], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // clear rt
    float clearCol[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    pCmdList->ClearRenderTargetView(pRenderer->backbufDescHandle[pRenderer->currentSubmission], clearCol, 0, nullptr);
    D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH;
    pCmdList->ClearDepthStencilView(pRenderer->dsDescHandle, clearFlags, 0.0, 0, 0, nullptr);

    pCmdList->RSSetViewports(1, &pRenderer->defaultViewport);
    pCmdList->RSSetScissorRects(1, &pRenderer->defaultScissor);

    // draw sunset
    renderEffect(pRenderer.get(), &sunset);
    renderEffect(pRenderer.get(), &road);
    // exec cmd buffer
    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->currentSubmission], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    submitCmdBuffer(pRenderer.get());

    // present
    present(pRenderer.get(), vsyncOff);
}