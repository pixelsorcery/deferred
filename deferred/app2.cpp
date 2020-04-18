/*AMDG*/
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "app2.h"
#include "util.h"
#include "shaders.h"
#include "gltfHelper.h"

using namespace std;

App2* app2 = new App2();

App2::App2() : pRenderer(new Dx12Renderer()) {};

bool App2::init(HWND hwnd)
{
    initDevice(pRenderer.get(), hwnd);

    ID3D12Device* pDevice = pRenderer->pDevice;

    // load model
    bool result = loadModel(pRenderer.get(), Model, "..\\models\\BoxTextured.gltf");

    return result;
}

void App2::drawFrame()
{
    // clear
    HRESULT hr = S_OK;

    ID3D12Device* pDevice = pRenderer->pDevice;
    ID3D12GraphicsCommandList* pCmdList = pRenderer->pGfxCmdList;

    // set rt
    pCmdList->OMSetRenderTargets(1, &pRenderer->backbufDescHandle[pRenderer->backbufCurrent], false, 0);

    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // clear rt
    float clearCol[4] = { 0.4f, 0.4f, 0.6f, 1.0f };
    pCmdList->ClearRenderTargetView(pRenderer->backbufDescHandle[pRenderer->backbufCurrent], clearCol, 0, nullptr);
    pCmdList->RSSetViewports(1, &pRenderer->defaultViewport);
    pCmdList->RSSetScissorRects(1, &pRenderer->defaultScissor);

    // draw model
    drawModel(pRenderer.get(), Model);

    // exec cmd buffer
    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->backbufCurrent], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    submitCmdBuffer(pRenderer.get());

    // present
    present(pRenderer.get(), vsyncOff);
}

App2::~App2()
{
}