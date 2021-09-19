/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <DirectXColors.h>

#include "d3dcore.h"
#include "debugger.h"
#include "frame-async-utils.h"
#include "render-item-utils.h"
#include "timer-utils.h"
#include "vmesh-utils.h"

std::pair<int, int> getWndSize(HWND hWnd) {
    RECT rect;
    GetWindowRect(hWnd, &rect);
    return std::make_pair<int, int>(rect.right - rect.left, rect.bottom - rect.top);
}

void flushCmdQueue(D3DCore* pCore) {
    pCore->currFenceValue++;
    checkHR(pCore->cmdQueue->Signal(pCore->fence.Get(), pCore->currFenceValue));
    if (pCore->fence->GetCompletedValue() < pCore->currFenceValue) {
        HANDLE hEvent;
        checkNull(hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
        // Fire event when GPU reaches current fence.
        checkHR(pCore->fence->SetEventOnCompletion(pCore->currFenceValue, hEvent));
        // Wait until the event is triggered.
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }
}

void createD3DCore(HWND hWnd, D3DCore** ppCore) {
    checkNull((*ppCore) = new D3DCore);

    D3DCore* pCore = *ppCore;

    pCore->hWnd = hWnd;
    auto wndSize = getWndSize(hWnd);
    int wndW = wndSize.first;
    int wndH = wndSize.second;

    checkHR(CreateDXGIFactory(IID_PPV_ARGS(&pCore->factory)));

    checkHR(D3D12CreateDevice(
        nullptr,
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&pCore->device)));

    checkHR(pCore->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pCore->fence)));

    pCore->rtvDescSize = pCore->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    pCore->dsvDescSize = pCore->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    pCore->cbvSrvUavDescSize = pCore->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    createCmdObjs(pCore);
    
    createSwapChain(hWnd, pCore);

    createRtvDsvHeaps(pCore);

    createRootSig(pCore);

    createShaders(pCore);

    createInputLayout(pCore);

    createPSOs(pCore);

    createBasicMaterials(pCore);

    createRenderItems(pCore);

    createFrameResources(pCore);

    resizeSwapBuffs(wndW, wndH, pCore);
    pCore->camera = std::make_unique<Camera>();
    initCamera(wndW, wndH, pCore->camera.get());

    pCore->timer = std::make_unique<Timer>();
    initTimer(pCore->timer.get());
}

void createCmdObjs(D3DCore* pCore) {
    D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
    cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    checkHR(pCore->device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&pCore->cmdQueue)));

    checkHR(pCore->device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&pCore->cmdAlloc)));

    checkHR(pCore->device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        pCore->cmdAlloc.Get(),
        nullptr,
        IID_PPV_ARGS(&pCore->cmdList)));

    // Start off in a closed state.  This is because the first time we refer
    // to the command list we will Reset it, and it needs to be closed before calling Reset
    pCore->cmdList->Close();
}

void createSwapChain(HWND hWnd, D3DCore* pCore) {
    auto wndSize = getWndSize(hWnd);
    int wndW = wndSize.first;
    int wndH = wndSize.second;

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    swapChainDesc.BufferDesc.Width = wndW;
    swapChainDesc.BufferDesc.Height = wndH;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = pCore->swapChainBuffFormat;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // Swap chain uses command queue to perform flush
    checkHR(pCore->factory->CreateSwapChain(
        pCore->cmdQueue.Get(),
        &swapChainDesc,
        &pCore->swapChain));
}

void createRtvDsvHeaps(D3DCore* pCore) {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
    rtvHeapDesc.NumDescriptors = 2;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    checkHR(pCore->device->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(&pCore->rtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    checkHR(pCore->device->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(&pCore->dsvHeap)));
}

void createRootSig(D3DCore* pCore) {
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if (errorBlob != nullptr) {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    checkHR(hr);

    checkHR(pCore->device->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&pCore->rootSig)));
}

void createShaders(D3DCore* pCore) {
    pCore->vsByteCode = compileShader(L"shaders/rs.hlsl", nullptr, "VS", "vs_5_0");
    pCore->psByteCode = compileShader(L"shaders/rs.hlsl", nullptr, "PS", "ps_5_0");
}

void createInputLayout(D3DCore* pCore) {
    pCore->inputLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } };
}

void createPSOs(D3DCore* pCore) {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { pCore->inputLayout.data(), (UINT)pCore->inputLayout.size() };
    psoDesc.pRootSignature = pCore->rootSig.Get();
    psoDesc.VS = {
        reinterpret_cast<BYTE*>(pCore->vsByteCode->GetBufferPointer()),
        pCore->vsByteCode->GetBufferSize() };
    psoDesc.PS = {
        reinterpret_cast<BYTE*>(pCore->psByteCode->GetBufferPointer()),
        pCore->psByteCode->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = pCore->swapChainBuffFormat;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.DSVFormat = pCore->depthStencilBuffFormat;
    checkHR(pCore->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pCore->PSOs["solid"])));

    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    checkHR(pCore->device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pCore->PSOs["wireframe"])));
}

void createBasicMaterials(D3DCore* pCore) {
    auto red = std::make_unique<Material>();
    red->name = "red";
    red->matConstBuffIdx = 0;
    red->constData.diffuseAlbedo = XMFLOAT4(Colors::Red);
    red->constData.fresnelR0 = { 1.0f, 0.0f, 0.0f };
    red->constData.roughness = 0.0f;
    pCore->materials["red"] = std::move(red);

    auto green = std::make_unique<Material>();
    green->name = "green";
    green->matConstBuffIdx = 1;
    green->constData.diffuseAlbedo = XMFLOAT4(Colors::Green);
    green->constData.fresnelR0 = { 0.0f, 1.0f, 0.0f };
    green->constData.roughness = 0.0f;
    pCore->materials["green"] = std::move(green);

    auto blue = std::make_unique<Material>();
    blue->name = "blue";
    blue->matConstBuffIdx = 2;
    blue->constData.diffuseAlbedo = XMFLOAT4(Colors::Blue);
    blue->constData.fresnelR0 = { 0.0f, 0.0f, 1.0f };
    blue->constData.roughness = 0.0f;
    pCore->materials["blue"] = std::move(blue);

    auto grass = std::make_unique<Material>();
    grass->name = "grass";
    grass->matConstBuffIdx = 3;
    grass->constData.diffuseAlbedo = { 0.2f, 0.6f, 0.2f, 1.0f };
    grass->constData.fresnelR0 = { 0.01f, 0.01f, 0.01f };
    grass->constData.roughness = 0.125f;
    pCore->materials["grass"] = std::move(grass);

    auto water = std::make_unique<Material>();
    water->name = "water";
    water->matConstBuffIdx = 4;
    water->constData.diffuseAlbedo = { 0.0f, 0.2f, 0.6f, 1.0f };
    water->constData.fresnelR0 = { 0.1f, 0.1f, 0.1f };
    water->constData.roughness = 0.0f;
    pCore->materials["water"] = std::move(water);
}

void createRenderItems(D3DCore* pCore) {
    auto xAxisGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(100.0f, 0.01f, 0.01f), XMFLOAT4(DirectX::Colors::Red), xAxisGeo.get());
    auto xAxis = std::make_unique<RenderItem>();
    generateCubeEx(pCore, xAxisGeo.get(), xAxis.get());
    xAxis->material = pCore->materials["red"].get();
    pCore->ritems.push_back(std::move(xAxis));

    auto yAxisGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(0.01f, 100.0f, 0.01f), XMFLOAT4(DirectX::Colors::Green), yAxisGeo.get());
    auto yAxis = std::make_unique<RenderItem>();
    generateCubeEx(pCore, yAxisGeo.get(), yAxis.get());
    yAxis->material = pCore->materials["green"].get();
    pCore->ritems.push_back(std::move(yAxis));

    auto zAxisGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(0.01f, 0.01f, 100.0f), XMFLOAT4(DirectX::Colors::Blue), zAxisGeo.get());
    auto zAxis = std::make_unique<RenderItem>();
    generateCubeEx(pCore, zAxisGeo.get(), zAxis.get());
    zAxis->material = pCore->materials["blue"].get();
    pCore->ritems.push_back(std::move(zAxis));

    for (auto& ritem : pCore->ritems) {
        pCore->solidModeRitems.push_back(ritem.get());
    }
}

void createFrameResources(D3DCore* pCore) {
    for (int i = 0; i < NUM_FRAME_RESOURCES; ++i) {
        auto resource = std::make_unique<FrameResource>();
        initEmptyFrameResource(pCore, resource.get());
        initFResourceObjConstBuff(pCore, pCore->ritems.size(), resource.get());
        initFResourceProcConstBuff(pCore, 1, resource.get());
        initFResourceMatConstBuff(pCore, pCore->materials.size(), resource.get());
        pCore->frameResources.push_back(std::move(resource));
    }
}

void resizeSwapBuffs(int w, int h, D3DCore* pCore) {
    // Flush before changing any resources.
    flushCmdQueue(pCore);
    checkHR(pCore->cmdList->Reset(pCore->cmdAlloc.Get(), nullptr));

    for (int i = 0; i < 2; ++i) {
        pCore->swapChainBuffs[i].Reset();
    }
    pCore->depthStencilBuff.Reset();

    checkHR(pCore->swapChain->ResizeBuffers(
        2, w, h,
        pCore->swapChainBuffFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    pCore->currBackBuffIdx = 0;

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < 2; ++i) {
        checkHR(pCore->swapChain->GetBuffer(i, IID_PPV_ARGS(&pCore->swapChainBuffs[i])));
        pCore->device->CreateRenderTargetView(pCore->swapChainBuffs[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, pCore->rtvDescSize);
    }

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = w;
    depthStencilDesc.Height = h;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = pCore->depthStencilBuffFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optimizedClearValue;
    optimizedClearValue.Format = pCore->depthStencilBuffFormat;
    optimizedClearValue.DepthStencil.Depth = 1.0f;
    optimizedClearValue.DepthStencil.Stencil = 0;
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optimizedClearValue,
        IID_PPV_ARGS(&pCore->depthStencilBuff)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = pCore->depthStencilBuffFormat;
    dsvDesc.Texture2D.MipSlice = 0;
    pCore->device->CreateDepthStencilView(
        pCore->depthStencilBuff.Get(), &dsvDesc, pCore->dsvHeap->GetCPUDescriptorHandleForHeapStart());

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->depthStencilBuff.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_DEPTH_WRITE));

    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);

    flushCmdQueue(pCore);
}

void clearBackBuff(XMVECTORF32 color, FLOAT depth, UINT8 stencil, D3DCore* pCore) {
    pCore->cmdList->ClearRenderTargetView(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(
            pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            pCore->currBackBuffIdx,
            pCore->rtvDescSize
        ), color, 0, nullptr);
    pCore->cmdList->ClearDepthStencilView(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(pCore->dsvHeap->GetCPUDescriptorHandleForHeapStart()),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}