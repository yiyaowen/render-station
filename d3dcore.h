/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3d12.h>
#include <DirectXPackedVector.h>
#include <dxgi.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <Windows.h>
#include <wrl.h>
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

#include "camera.h"
#include "d3dx12.h"
#include "frame-async.h"
#include "math-utils.h"
#include "shader.h"
#include "vmesh.h"

struct D3DCore {
    HWND hWnd = nullptr;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // Basic D3D components for creating a render window:
    // Factory, Swap Chain, Render Target Buffer (Swap Chain Buffer), Depth Stencil Buffer,
    // Device, Fence, Command Queue, Command Allocator, Command List, RTV Heap, DSV Heap.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    ComPtr<IDXGIFactory> factory = nullptr;

    ComPtr<IDXGISwapChain> swapChain = nullptr;
    int currBackBuffIdx = 0;
    DXGI_FORMAT swapChainBuffFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    ComPtr<ID3D12Resource> swapChainBuffs[2] = {};
    DXGI_FORMAT depthStencilBuffFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ComPtr<ID3D12Resource> depthStencilBuff = nullptr;

    ComPtr<ID3D12Device> device = nullptr;

    ComPtr<ID3D12Fence> fence = nullptr;
    UINT64 currFenceValue = 0;

    // RTV: Render Target View
    UINT rtvDescSize = 0;
    // DSV: Depth Stencil View
    UINT dsvDescSize = 0;
    // CBV: Constant Buffer View
    // SRV: Shader Resource View
    // UAV: Unordered Access View
    UINT cbvSrvUavDescSize = 0;

    ComPtr<ID3D12CommandQueue> cmdQueue = nullptr;
    ComPtr<ID3D12CommandAllocator> cmdAlloc = nullptr;
    ComPtr<ID3D12GraphicsCommandList> cmdList = nullptr;

    ComPtr<ID3D12DescriptorHeap> rtvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // To render some objects in our scene, these components are needed:
    // CBV: manage constant data passed between program and shaders.
    // Constant Buffer, and its mapped buffer in CPU side.
    // Root Signature, and we will create it with some root parameters.
    // Shader Byte Code, including VS and PS etc.
    // Input Layout: works with Root Signature together.
    // PSO: Pipeline State Object.
    // At last, we create some data structures to help managing object data,
    // and a camera to store the space transformation data.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;

    // Note these 2 buffers are deprecated as we have used the render item technique.
    // All constant buffer data is stored in the collection of render items.
    BYTE* objConstBuffCPU = nullptr;
    ComPtr<ID3D12Resource> objConstBuffGPU = nullptr;

    ComPtr<ID3D12RootSignature> rootSig = nullptr;

    ComPtr<ID3DBlob> vsByteCode = nullptr;
    ComPtr<ID3DBlob> psByteCode = nullptr;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout = {};

    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs = {};

    // Note the main meshes here are deprecated as we have used the render item technique.
    // All mesh data is stored in the collection of render items.
    std::unordered_map<std::string, std::unique_ptr<Vmesh>> meshes = {};

    std::unique_ptr<Camera> camera = nullptr;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // We originally use flushCmdQueue to force the CPU to wait for GPU executing
    // all of the command pushed, since the CPU must update draw-data after
    // last frame rendered. To avoid this time-consuming sychronization from
    // happending, a generally used technique is implemented, i.e. Frame Resource.
    // We also create a new struct to store the data needed to draw a object,
    // i.e. Render Item, which includes the vertex positions and world matrix etc.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    int currFrameResourceIdx = 0;
    FrameResource* currFrameResource = nullptr;
    std::vector<std::unique_ptr<FrameResource>> frameResources;

    std::vector<std::unique_ptr<RenderItem>> ritems; // ritems: render items
    std::vector<RenderItem*> solidModeRitems;
    std::vector<RenderItem*> wireframeModeRitems;
};

std::pair<int, int> getWndSize(HWND hWnd);

void flushCmdQueue(D3DCore* pCore);

void createD3DCore(HWND hWnd, D3DCore** ppCore);

void createCmdObjs(D3DCore* pCore);
void createSwapChain(HWND hWnd, D3DCore* pCore);
void createRtvDsvHeaps(D3DCore* pCore);

// Note descriptor heap depends on initialized render items.
// This func should be called only after render items created.
void createDescHeap(D3DCore* pCore);
// Note constant buffer views depends on initialized frame resources.
// This func should be called only after frame resources created.
void createConstBuffViews(D3DCore* pCore);

void createRootSig(D3DCore* pCore);
void createShaders(D3DCore* pCore);
void createInputLayout(D3DCore* pCore);
void createPSOs(D3DCore* pCore);

void createRenderItems(D3DCore* pCore);
// Note frame resource depends on initialized render items.
// This func should be called only after render items created.
void createFrameResources(D3DCore* pCore);

void resizeSwapBuffs(int w, int h, D3DCore* pCore);

void clearBackBuff(XMVECTORF32 color, FLOAT depth, UINT8 stencil, D3DCore* pCore);