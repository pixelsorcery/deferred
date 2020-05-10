/*AMDG*/
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "app2.h"
#include "util.h"
#include "shaders.h"
#include "gltfHelper.h"

using namespace std;

App2* app2 = new App2();

App2::App2() : pRenderer(new Dx12Renderer()) 
{};

bool App2::init(HWND hwnd)
{
    bool result = true;
    initDevice(pRenderer.get(), hwnd);

    ID3D12Device* pDevice = pRenderer->pDevice;

    // load model
    GltfModel model = {};
    //GltfModel model2 = model;
    //result = loadModel(pRenderer.get(), boxModel, "..\\models\\BoxTextured.gltf");
    //result = loadModel(pRenderer.get(), duckModel, "..\\models\\duck\\Duck.gltf");
    result = loadModel(pRenderer.get(), model, "..\\models\\2cylinderengine\\2CylinderEngine.gltf");
    models.push_back(model);
    //result = loadModel(pRenderer.get(), lanternModel, "..\\models\\lantern\\lantern.gltf");
    return result;
}

void App2::drawFrame(float time)
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

    // draw models
    for (int i = 0; i < models.size(); i++)
    {
        drawModel(pRenderer.get(), models[i], time);
    }

    // exec cmd buffer
    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->currentSubmission], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    submitCmdBuffer(pRenderer.get());

    // present
    present(pRenderer.get(), vsyncOff);
}