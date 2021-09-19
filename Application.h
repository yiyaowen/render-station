/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 * 
 * Create fantastic animation and game.
 * 
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d12.h>
#include <DirectXPackedVector.h>
#include <dxgi.h>
#include <Windows.h>
#include <wrl.h>
using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

#include "d3dx12.h"

class Application {
protected:
    // The constructor is protected here to prevent users from disrupting the render application.
    // Any render app defined by users should inherit from this class.
    Application(HINSTANCE hInstance);
    Application(const Application& app) = delete;
    Application& operator=(const Application& app) = delete;
    virtual ~Application();

protected:
    virtual void Initialize();
    virtual void Update() = 0;
    virtual void Draw() = 0;

    void FlushCommandQueue();

    // Use XMVECTORF32 so we can pass in XMVECTOR or float[4].  The color value here should be a
    // normalized float RGBA format (Each component should between 0.0f ~ 1.0f)
    void ClearCurrentRenderTargetView(const XMVECTORF32& color);
    void ClearCurrentDepthStencilView(FLOAT depthValue, UINT8 stencilValue);

public:
    int Run();

private:
    void InitWindow();
    void InitDirectX3D();

protected:
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;

    ComPtr<IDXGIFactory> dxgiFactory;

    ComPtr<IDXGISwapChain> dxgiSwapChain;
    int currentBackBufferIndex = 0;
    ComPtr<ID3D12Resource> swapChainBuffer[2];
    ComPtr<ID3D12Resource> depthStencilBuffer;

    ComPtr<ID3D12Device> d3dDevice;

    ComPtr<ID3D12Fence> d3dFence;
    UINT64 currentFenceValue = 0;

    UINT rtvDescriptorSize = 0; // Render Target View
    UINT dsvDescriptorSize = 0; // Depth Stencil View
    UINT cbvSrvUavDescriptorSize = 0; // Constant Buffer View, Shader Resource View and Unordered Access View

    ComPtr<ID3D12CommandQueue> d3dCommandQueue;
    ComPtr<ID3D12CommandAllocator> d3dCommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> d3dCommandList;

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;

    D3D12_VIEWPORT d3dScreenViewPort;
    D3D12_RECT d3dScissorRect;
};