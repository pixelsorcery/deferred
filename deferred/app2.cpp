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
    GltfModel box = {};
    GltfModel duck = {};
    GltfModel model = {};
    GltfModel lantern = {};
    GltfModel spheres = {};

#define DRAWMODELS 1
#if DRAWMODELS
    result = loadModel(pRenderer.get(), box, "..\\models\\BoxTextured.gltf");
    box.worldScale = glm::vec3(10.0f, 10.0f, 10.0f);
    box.worldPosition = glm::vec3(-25.0f, -15.0f, 0.0f);
    box.worldRotation = glm::vec3(0.0f, 0.0, 0.0f);
    box.rotationAngle = 0;
    models.push_back(box);
    result = loadModel(pRenderer.get(), duck, "..\\models\\duck\\Duck.gltf");
    duck.worldScale = glm::vec3(5.0f, 5.0f, 5.0f);
    duck.worldPosition = glm::vec3(-20.0f, 15.0f, 0.0f);
    duck.worldRotation = glm::vec3(0.0f, 0.0, 0.0f);
    duck.rotationAngle = 0;
    models.push_back(duck);
    result = loadModel(pRenderer.get(), model, "..\\models\\2cylinderengine\\2CylinderEngine.gltf");
    model.worldScale = glm::vec3(0.02f, 0.02f, 0.02f);
    model.worldPosition = glm::vec3(15.0f, 15.0f, 0.0f);
    model.worldRotation = glm::vec3(0.0f, 0.0, 0.0f);
    model.rotationAngle = 0;
    //model.worldScale = glm::vec3(0.07f, 0.07f, 0.07f);
    //model.worldPosition = glm::vec3(0.0f, 10.0f, 0.0f);
    models.push_back(model);
    result = loadModel(pRenderer.get(), lantern, "..\\models\\lantern\\lantern.gltf");
    lantern.worldPosition = glm::vec3(15.0f, -20.0f, 0.0f);
    lantern.worldRotation = glm::vec3(0.0f, 0.0, 1.0f);
    lantern.rotationAngle = 3.14159f;
    models.push_back(lantern);
#else
    result = loadModel(pRenderer.get(), spheres, "..\\models\\MetalRoughSpheresNoTextures.gltf");
    spheres.worldPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    spheres.worldScale = glm::vec3(10000.0f, 10000.0f, 10000.0f);
    models.push_back(spheres);
#endif
    return result;
}

void App2::drawFrame(float time)
{
    // camera
    updateCamera(pRenderer.get(), keys, time);

    // clear
    HRESULT hr = S_OK;

    ID3D12Device* pDevice = pRenderer->pDevice;
    ID3D12GraphicsCommandList* pCmdList = pRenderer->cmdSubmissions[pRenderer->currentSubmission].pGfxCmdList;

    // set rt
    pCmdList->OMSetRenderTargets(1, &pRenderer->backbufDescHandle[pRenderer->currentSubmission], false, &pRenderer->dsDescHandle);

    transitionResource(pRenderer.get(), pRenderer->backbuf[pRenderer->currentSubmission], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // clear rt
    float clearCol[4] = { .8f, .9f, 1.0f, 1.0f };
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

void App2::onMouseMove(int x, int y, int dx, int dy)
{
    // note: this is here because SetCursorPos will generate a WM_MOUSEMOVE message, 
    // so we want to ignore that one
    static bool changed = false;
    if (changed = !changed)
    {
        updateCamera(pRenderer.get(), x, y, dx, dy);
    }
}

void App2::onKey(const uint32_t key, bool pressed)
{
    keys[key] = pressed;
}