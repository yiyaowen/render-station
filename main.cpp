/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <DirectXColors.h>
#include <windows.h>
#include <windowsx.h>

#include "d3dcore.h"
#include "debugger.h"
#include "devfunc.h"
#include "timer-utils.h"

struct RenderWindow {
    HINSTANCE hInstance;
    WNDCLASS wndclass;
    HWND hMainWnd;
};

#define WNDPROC_SIGNATURE(FUNC_NAME) \
LRESULT CALLBACK FUNC_NAME(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)

#define DECLARE_WNDPROC(FUNC_NAME) \
WNDPROC_SIGNATURE(FUNC_NAME);

RenderWindow* pRwnd = nullptr;
D3DCore* pRcore = nullptr;

WNDPROC_SIGNATURE(renderWindowProc) {
    static int wndW = 800, wndH = 600;
    switch (msg) {
    case WM_SIZE:
        wndW = LOWORD(lParam);
        wndH = HIWORD(lParam);
        if (pRcore != nullptr) {
            if (wParam != SIZE_MINIMIZED) {
                resizeSwapBuffs(wndW, wndH, pRcore->clearColor, pRcore);
                resizeCameraView(wndW, wndH, pRcore->camera.get());
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        dev_onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), pRcore);
        return 0;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        dev_onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), pRcore);
        return 0;
    case WM_MOUSEMOVE:
        dev_onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), pRcore);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void createRenderWindow(HINSTANCE hInst, RenderWindow** ppWnd) {
    checkNull((*ppWnd) = new RenderWindow);

    RenderWindow* pWnd = *ppWnd;

    pWnd->hInstance = hInst;

    pWnd->wndclass.cbClsExtra = 0;
    pWnd->wndclass.cbWndExtra = 0;
    pWnd->wndclass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    pWnd->wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    pWnd->wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    pWnd->wndclass.hInstance = hInst;
    pWnd->wndclass.lpfnWndProc = renderWindowProc;
    pWnd->wndclass.lpszClassName = L"RenderWindow";
    pWnd->wndclass.lpszMenuName = nullptr;
    pWnd->wndclass.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClass(&pWnd->wndclass)) {
        MessageBox(nullptr, L"Register Render Window Class Failed", L"Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    pWnd->hMainWnd = CreateWindow(L"RenderWindow", L"Render Station", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInst, nullptr);
    if (!pWnd->hMainWnd) {
        MessageBox(nullptr, L"Create Render Window Failed", L"Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    ShowWindow(pWnd->hMainWnd, SW_SHOW);
    UpdateWindow(pWnd->hMainWnd);
}

int startRenderWindowMsgLoop(RenderWindow* pWnd) {
    MSG msg;
    dev_initCoreElems(pRcore);
    while (1) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            tickTimer(pRcore->timer.get());
            updateRenderWindowCaptionTimerInfo(pRcore->hWnd, pRcore->timer.get());
            dev_updateCoreData(pRcore);
            dev_drawCoreElems(pRcore);
        }
    }
    return msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
#if defined(DEBUG) || defined(_DEBUG) 
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        checkHR(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif
    createRenderWindow(hInstance, &pRwnd);
    createD3DCore(pRwnd->hMainWnd, XMFLOAT4(DirectX::Colors::SkyBlue), &pRcore);
    startRenderWindowMsgLoop(pRwnd);
    return 0;
}