/*AMDG*/
#include <windows.h>
#include "renderer.h"

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
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

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

    RECT rc = { 0, 0, 640, 480 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hwnd = CreateWindow("window", "renderer", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hThisInst,
        NULL);

    ShowWindow(hwnd, nCmdShow);

	Dx12Renderer device = {};
    initDevice(&device, hwnd);

    // loop
    MSG msg;

    while (true)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (msg.message == WM_QUIT)
        {
            break;
        }

        // call render function here
    }

    DestroyWindow(hwnd);

    return (int)msg.wParam;
}