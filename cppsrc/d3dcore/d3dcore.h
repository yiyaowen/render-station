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

#include <array>
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

#include "frame-async.h"
#include "graphics/material.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/vmesh.h"
#include "postprocessing/basic-process.h"
#include "toolbox/d3dx12.h"
#include "utils/math-utils.h"
#include "widgets/camera.h"
#include "widgets/timer.h"

struct D3DCore {
    HWND hWnd = nullptr;

    XMFLOAT4 clearColor = {};

    UINT _4xMsaaQuality = 0;

    // Basic Components
    ComPtr<IDXGIFactory> factory = nullptr;

    ComPtr<IDXGISwapChain> swapChain = nullptr;
    int currBackBuffIdx = 0;
    DXGI_FORMAT swapChainBuffFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    ComPtr<ID3D12Resource> swapChainBuffs[2] = {};
    ComPtr<ID3D12Resource> msaaBackBuff = nullptr;
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

    // Shader Objects
    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> rootSigs = {};

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders = {};

    std::vector<D3D12_INPUT_ELEMENT_DESC> defaultInputLayout = {};

    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs = {};

    // Frame Asyncronization
    int currFrameResourceIdx = 0;
    FrameResource* currFrameResource = nullptr;
    std::vector<std::unique_ptr<FrameResource>> frameResources;

    ProcConsts processData = {};

    std::unordered_map<std::string, std::unique_ptr<RenderItem>> ritems = {};// ritems: render items
    std::vector<RenderItem*> allRitems = {};
    std::vector<std::pair<std::string, std::vector<RenderItem*>>> ritemLayers = {};

    // Graphics
    std::unordered_map<std::string, std::unique_ptr<Material>> materials = {};
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures2d = {};
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures2darray = {};
    ComPtr<ID3D12DescriptorHeap> srvUavHeap = nullptr;

    // Widgets
    std::unique_ptr<Camera> camera = nullptr;
    std::unique_ptr<Timer> timer = nullptr;

    // Postprocessing
    std::unordered_map<std::string, std::unique_ptr<BasicProcess>> postprocessors = {};
};

std::pair<int, int> getWndSize(HWND hWnd);

void flushCmdQueue(D3DCore* pCore);

void createD3DCore(HWND hWnd, XMFLOAT4 clearColor, D3DCore** ppCore);

void checkFeatureSupports(D3DCore* pCore);

void createCmdObjs(D3DCore* pCore);
void createSwapChain(HWND hWnd, D3DCore* pCore);
void createRtvDsvHeaps(D3DCore* pCore);

void createRootSig(D3DCore* pCore, const std::string& name, D3D12_ROOT_SIGNATURE_DESC* desc);

void createRootSigs(D3DCore* pCore);
void createShaders(D3DCore* pCore);
void createInputLayout(D3DCore* pCore);
void createPSOs(D3DCore* pCore);

void createBasicMaterials(D3DCore* pCore);
// Due to root signature includes a descriptor table of textures, and materials depend on initialized textures,
// all texture initialization funcs should be called before the root signature and the materials are created.
void loadBasicTextures(D3DCore* pCore);
void createDescHeaps(D3DCore* pCore);
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> generateStaticSamplers();

void createRenderItemLayers(D3DCore* pCore);
void createRenderItems(D3DCore* pCore);

// Note frame resource depends on initialized render items, materials.
// This func should be called only after these components created.
void createFrameResources(D3DCore* pCore);

void createBasicPostprocessor(D3DCore* pCore);

void resizeSwapBuffs(int w, int h, XMFLOAT4 clearColor, D3DCore* pCore);

void clearBackBuff(D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvDescHandle, XMVECTORF32 color,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescHandle, FLOAT depth, UINT8 stencil, D3DCore* pCore);

// Note this func will not reset the command list and flush the command queue.
// It should be invoked in a reset-flush closure written by users.
void copyStatedResource(D3DCore* pCore,
    ID3D12Resource* dest, D3D12_RESOURCE_STATES destState,
    ID3D12Resource* src, D3D12_RESOURCE_STATES srcState);

void uploadStatedResource(D3DCore* pCore,
    ID3D12Resource* resource, D3D12_RESOURCE_STATES resourceState,
    ID3D12Resource* intermidiate, D3D12_RESOURCE_STATES intermidiateState,
    const void* data, UINT64 byteSize);
