/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#include "Application.h"
#include "ApplicationException.h"

LRESULT CALLBACK AppWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

Application::Application(HINSTANCE hInstance)
    : hInstance(hInstance) {}

Application::~Application() {}

void Application::Initialize() {
    InitWindow();
    InitDirectX3D();
}

void Application::FlushCommandQueue() {
    currentFenceValue++;

    ThrowIfFailed(d3dCommandQueue->Signal(d3dFence.Get(), currentFenceValue));

    if (d3dFence->GetCompletedValue() < currentFenceValue) {
        HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

        // Fire event when GPU reaches current fence
        ThrowIfFailed(d3dFence->SetEventOnCompletion(currentFenceValue, hEvent));

        // Wait until the event is triggered
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }
}

void Application::ClearCurrentRenderTargetView(const XMVECTORF32& color) {
    d3dCommandList->ClearRenderTargetView(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            currentBackBufferIndex,
            rtvDescriptorSize
        ), color, 0, nullptr);
}

void Application::ClearCurrentDepthStencilView(FLOAT depthValue, UINT8 stencilValue) {
    d3dCommandList->ClearDepthStencilView(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart()),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthValue, stencilValue, 0, nullptr);
}

int Application::Run() {

    Initialize();

    MSG msg;
    // Start render application main loop
    while (true) {
        // If there are window messages then process them
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Otherwise do rendering works
        else {
            Update();
            Draw();
        }
    }

    return msg.wParam;
}

void Application::InitWindow() {

    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = AppWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"AppWnd";

    if (!RegisterClass(&wc)) {
        MessageBox(nullptr, L"Register Application Window Class Failed", L"Initialization Error", MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }

    hWnd = CreateWindow(L"AppWnd", L"Render Station", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) {
        MessageBox(nullptr, L"Create Application Window Failed", L"Initialization Error", MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
}

void Application::InitDirectX3D() {

    ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory)));

    ThrowIfFailed(D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&d3dDevice)));

    ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dFence)));

    rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbvSrvUavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(d3dDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&d3dCommandQueue)));

    ThrowIfFailed(d3dDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&d3dCommandAllocator)));

    ThrowIfFailed(d3dDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        d3dCommandAllocator.Get(),
        nullptr,
        IID_PPV_ARGS(&d3dCommandList)));

    // Start off in a closed state.  This is because the first time we refer
    // to the command list we will Reset it, and it needs to be closed before calling Reset
    d3dCommandList->Close();

    dxgiSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
    dxgiSwapChainDesc.BufferDesc.Width = 800;
    dxgiSwapChainDesc.BufferDesc.Height = 600;
    dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgiSwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    dxgiSwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    dxgiSwapChainDesc.SampleDesc.Count = 1;
    dxgiSwapChainDesc.SampleDesc.Quality = 0;
    dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgiSwapChainDesc.BufferCount = 2;
    dxgiSwapChainDesc.OutputWindow = hWnd;
    dxgiSwapChainDesc.Windowed = true;
    dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Swap chain uses command queue to perform flush
    ThrowIfFailed(dxgiFactory->CreateSwapChain(
        d3dCommandQueue.Get(),
        &dxgiSwapChainDesc,
        &dxgiSwapChain));

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

    // Flush before changing any resources
    FlushCommandQueue();

    ThrowIfFailed(d3dCommandList->Reset(d3dCommandAllocator.Get(), nullptr));

    for (int i = 0; i < 2; ++i) {
        swapChainBuffer[i].Reset();
    }
    depthStencilBuffer.Reset();

    ThrowIfFailed(dxgiSwapChain->ResizeBuffers(
        2, 800, 600,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    currentBackBufferIndex = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < 2; ++i) {
        ThrowIfFailed(dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer[i])));
        d3dDevice->CreateRenderTargetView(swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, rtvDescriptorSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = 800;
    depthStencilDesc.Height = 600;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optimizedClearValue;
    optimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optimizedClearValue.DepthStencil.Depth = 1.0f;
    optimizedClearValue.DepthStencil.Stencil = 0;
    ThrowIfFailed(d3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optimizedClearValue,
        IID_PPV_ARGS(&depthStencilBuffer)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Texture2D.MipSlice = 0;
    d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

    d3dCommandList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            depthStencilBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_DEPTH_WRITE));

    ThrowIfFailed(d3dCommandList->Close());
    ID3D12CommandList* d3dCmdLists[] = { d3dCommandList.Get() };
    d3dCommandQueue->ExecuteCommandLists(1, d3dCmdLists);

    FlushCommandQueue();

    d3dScreenViewPort.TopLeftX = 0;
    d3dScreenViewPort.TopLeftY = 0;
    d3dScreenViewPort.Width = 800;
    d3dScreenViewPort.Height = 600;
    d3dScissorRect.left = 0;
    d3dScissorRect.top = 0;
    d3dScissorRect.right = 800;
    d3dScissorRect.bottom = 600;
}