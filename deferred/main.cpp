/*AMDG*/
#include <windows.h>
#include <chrono>
#include "renderer.h"
#include "settings.h"
#include "app.h"
#include "model.h"
#include "app2.h"

extern App* app;
extern App2* app2;

using namespace std::literals;

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
        break;
    case WM_KEYDOWN:
        break;
    case WM_KEYUP:
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