/*AMDG*/
#include <windows.h>
#include <chrono>
#include "renderer.h"
#include "settings.h"
#include "app.h"
#include "model.h"
#include "app2.h"
#include "app3.h"

extern App*  app;
extern App2* app2;
extern App3* app3;

using namespace std::literals;

#define GETX(l) (int(l & 0xFFFF))
#define GETY(l) (int(l) >> 16)

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CREATE:
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        ValidateRect(hwnd, NULL);
        return 0;

    case WM_CHAR:
        if (wparam == 27) // escape
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_MOUSEMOVE:
        static int lastX, lastY;
        int x, y;
        x = GETX(lparam);
        y = GETY(lparam);
        app2->onMouseMove(x, y, x - lastX, y - lastY);
        lastX = x;
        lastY = y;
        break;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else
        {
            app2->onKey((unsigned int)wparam, true);
        }
        break;
    case WM_KEYUP:
        app2->onKey((unsigned int)wparam, false);
        break;
    case WM_SYSKEYDOWN:
        break;
    case WM_SYSKEYUP:
        break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow)
{
    WNDCLASSA wc = {};
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hInstance = hThisInst;
    wc.lpfnWndProc = WinProc;
    wc.lpszClassName = "window";
    RegisterClassA(&wc);

    RECT rc = { 0, 0, renderer::width, renderer::height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindow("window", "renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hThisInst,
        NULL);

    ShowWindow(hwnd, nCmdShow);

    if (false == app2->init(hwnd))
    {
        return 0;
    }

    // loop
    MSG msg = {};

    std::chrono::steady_clock::time_point oldTime = std::chrono::high_resolution_clock::now();
    while (msg.message != WM_QUIT)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        std::chrono::steady_clock::time_point newTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = newTime - oldTime;

        // render frame
        app2->drawFrame(deltaTime/1s);
        oldTime = newTime;
    }

    DestroyWindow(hwnd);
    delete(app2);

    return (int)msg.wParam;
}