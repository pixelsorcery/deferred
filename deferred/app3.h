#pragma once

/**************************************************

This app draws some stuff.

**************************************************/

#include <memory>
#include <atlbase.h>
#include <vector>
#include "renderer.h"
#include "model.h"
#include "fsEffect.h"

class App3
{
public:
    App3();
    bool init(HWND hwnd);
    void drawFrame(float time);
    void onMouseMove(int x, int y, int dx, int dy) {};
    void onKey(const uint32_t key, bool pressed) {};
private:
    std::unique_ptr<Dx12Renderer> pRenderer;
    FsEffect                      sunset;
    FsEffect                      road;
};