#pragma once

/**************************************************

This app draws an unanimated gltf model.

**************************************************/

#include <memory>
#include <atlbase.h>
#include <vector>
#include "renderer.h"
#include "model.h"
#include "fsEffect.h"

class App2
{
public:
    App2();
    bool init(HWND hwnd);
    void drawFrame(float time);
    void onMouseMove(int x, int y, int dx, int dy);
    void onKey(const uint32_t key, bool pressed);
private:
    std::unique_ptr<Dx12Renderer> pRenderer;
    std::vector<GltfModel>        models;
    FsEffect                      sunset;
    FsEffect                      road;
    bool                          keys[256];
};