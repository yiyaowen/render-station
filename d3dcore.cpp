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
#include "DDSTextureLoader.h"

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

void createD3DCore(HWND hWnd, XMFLOAT4 clearColor, D3DCore** ppCore) {
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

    checkFeatureSupports(pCore);

    createCmdObjs(pCore);
    
    createSwapChain(hWnd, pCore);

    createRtvDsvHeaps(pCore);

    loadBasicTextures(pCore);
    createDescHeaps(pCore);

    createRootSig(pCore);

    createShaders(pCore);

    createInputLayout(pCore);

    createPSOs(pCore);

    createBasicMaterials(pCore);

    createRenderItemLayers(pCore);
    createRenderItems(pCore);

    createFrameResources(pCore);

    pCore->clearColor = clearColor;
    resizeSwapBuffs(wndW, wndH, clearColor, pCore);
    pCore->camera = std::make_unique<Camera>();
    initCamera(wndW, wndH, pCore->camera.get());

    pCore->timer = std::make_unique<Timer>();
    initTimer(pCore->timer.get());
}

void checkFeatureSupports(D3DCore* pCore) {
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
    qualityLevels.Format = pCore->swapChainBuffFormat;
    qualityLevels.SampleCount = 4; // 4xMSAA
    qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    qualityLevels.NumQualityLevels = 0;
    pCore->device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &qualityLevels,
        sizeof(qualityLevels));
    UINT _4xMsaaQuality = qualityLevels.NumQualityLevels;
    assert(_4xMsaaQuality > 0);

    // Note the true supported quality level is not equal to the queried quality level.
    pCore->_4xMsaaQuality = _4xMsaaQuality - 1;
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
    // DirectX 12 3D does NOT support creating MSAA swap chain.
    // In classic method, a MSAA swap chain will be resolved automatically during presenting,
    // which is NOT supported in UWP program. It is recommended to create MSAA render target instead.
    //swapChainDesc.SampleDesc.Count = 4; // 4xMSAA
    //swapChainDesc.SampleDesc.Quality = pCore->_4xMsaaQuality;
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
    rtvHeapDesc.NumDescriptors = 3; // 1st and 2nd are swap chain buffers. 3rd is MSAA back buffer.
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
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);
    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

    auto samplers = generateStaticSamplers();

    // A root signature is an array of root parameters.
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, samplers.size(), samplers.data(),
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
    pCore->s_default = std::make_unique<Shader>(
        "default",
        L"shaders/default.hlsl",
        Shader::VS | Shader::PS);

    pCore->s_subdivisionGs = std::make_unique<Shader>(
        "subdivision",
        L"shaders/subdivision-gs.hlsl",
        Shader::VS | Shader::GS | Shader::PS);

    pCore->s_billboardGs = std::make_unique<Shader>(
        "billboard",
        L"shaders/billboard-gs.hlsl",
        Shader::VS | Shader::GS | Shader::PS);

    pCore->s_cylinderGeneratorGs = std::make_unique<Shader>(
        "cylinder_generator",
        L"shaders/cylinder-generator-gs.hlsl",
        Shader::VS | Shader::GS | Shader::PS);

    pCore->s_explosionGs = std::make_unique<Shader>(
        "explosion",
        L"shaders/explosion-gs.hlsl",
        Shader::VS | Shader::GS | Shader::PS);

    pCore->s_verNormalVisibleGs = std::make_unique<Shader>(
        "ver_normal_visible",
        L"shaders/ver-normal-visible-gs.hlsl",
        Shader::VS | Shader::GS | Shader::PS);

    pCore->s_triNormalVisibleGs = std::make_unique<Shader>(
        "tri_normal_visible",
        L"shaders/tri-normal-visible-gs.hlsl",
        Shader::VS | Shader::GS | Shader::PS);
}

void createInputLayout(D3DCore* pCore) {
    // SemanticName, SemanticIndex, Format, InputSlot, AlignedByteOffset, InputSlotClass, InstanceDataStepRate
    pCore->defaultInputLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } };
}

void createPSOs(D3DCore* pCore) {
    // Solid
    D3D12_GRAPHICS_PIPELINE_STATE_DESC solidPsoDesc = {};
    solidPsoDesc.InputLayout = { pCore->defaultInputLayout.data(), (UINT)pCore->defaultInputLayout.size() };
    solidPsoDesc.pRootSignature = pCore->rootSig.Get();
    bindShaderToPSO(&solidPsoDesc, pCore->s_default.get());
    solidPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    solidPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    solidPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    solidPsoDesc.SampleMask = UINT_MAX;
    solidPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    solidPsoDesc.NumRenderTargets = 1;
    solidPsoDesc.RTVFormats[0] = pCore->swapChainBuffFormat;
    solidPsoDesc.SampleDesc.Count = 4; // 4xMSAA
    solidPsoDesc.SampleDesc.Quality = pCore->_4xMsaaQuality;
    solidPsoDesc.DSVFormat = pCore->depthStencilBuffFormat;
    checkHR(pCore->device->CreateGraphicsPipelineState(&solidPsoDesc, IID_PPV_ARGS(&pCore->PSOs["solid"])));

    // Wireframe
    D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframePsoDesc = solidPsoDesc;
    wireframePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    checkHR(pCore->device->CreateGraphicsPipelineState(&wireframePsoDesc, IID_PPV_ARGS(&pCore->PSOs["wireframe"])));

    // Alpha Test
    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestPsoDesc = solidPsoDesc;
    alphaTestPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    checkHR(pCore->device->CreateGraphicsPipelineState(&alphaTestPsoDesc, IID_PPV_ARGS(&pCore->PSOs["alpha_test"])));

    // Alpha
    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaPsoDesc = solidPsoDesc;
    D3D12_RENDER_TARGET_BLEND_DESC alphaRTBD = alphaPsoDesc.BlendState.RenderTarget[0];
    alphaRTBD.BlendEnable = true;
    alphaRTBD.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    alphaRTBD.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    alphaRTBD.BlendOp = D3D12_BLEND_OP_ADD;
    alphaPsoDesc.BlendState.RenderTarget[0] = alphaRTBD;
    checkHR(pCore->device->CreateGraphicsPipelineState(&alphaPsoDesc, IID_PPV_ARGS(&pCore->PSOs["alpha"])));

    // Stencil Mark:
    // Keep the render target buffer and depth buffer intact and always mark the stencil buffer (stencil test will always passes).
    // (Note: This setting also takes into account the depth test. When depth test fails the stencil test will not pass.)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC stencilMarkPsoDesc = solidPsoDesc;
    D3D12_RENDER_TARGET_BLEND_DESC stencilMarkRTBD = stencilMarkPsoDesc.BlendState.RenderTarget[0];
    alphaRTBD.RenderTargetWriteMask = 0;
    stencilMarkPsoDesc.BlendState.RenderTarget[0] = alphaRTBD;
    D3D12_DEPTH_STENCIL_DESC stencilMarkDSD = stencilMarkPsoDesc.DepthStencilState;
    stencilMarkDSD.DepthEnable = true;
    stencilMarkDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    stencilMarkDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    stencilMarkDSD.StencilEnable = true;
    stencilMarkDSD.StencilReadMask = 0xff;
    stencilMarkDSD.StencilWriteMask = 0xff;
    stencilMarkDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    stencilMarkDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    stencilMarkDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
    stencilMarkDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    stencilMarkDSD.BackFace = stencilMarkDSD.FrontFace;
    stencilMarkPsoDesc.DepthStencilState = stencilMarkDSD;
    checkHR(pCore->device->CreateGraphicsPipelineState(&stencilMarkPsoDesc, IID_PPV_ARGS(&pCore->PSOs["stencil_mark"])));

    // Stencil Reflect:
    // Write into the render target buffer and depth buffer if and only if the stencil test value equals to reference value.
    // (Note: This is usually used to achieve an effect of mirror reflect and so is called Stencil Reflect PSO.)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC stencilReflectPsoDesc = solidPsoDesc;
    D3D12_DEPTH_STENCIL_DESC stencilReflectDSD = stencilReflectPsoDesc.DepthStencilState;
    stencilReflectDSD.DepthEnable = true;
    stencilReflectDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    stencilReflectDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    stencilReflectDSD.StencilEnable = true;
    stencilReflectDSD.StencilReadMask = 0xff;
    stencilReflectDSD.StencilWriteMask = 0xff;
    stencilReflectDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    stencilReflectDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    stencilReflectDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    stencilReflectDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    stencilReflectDSD.BackFace = stencilReflectDSD.FrontFace;
    stencilReflectPsoDesc.DepthStencilState = stencilReflectDSD;
    // The reflection operation actually changes the target object's space coordinate system.
    // For example, in DirectX the default coordinate system is LEFT-handed. After reflected,
    // the target object jumps into a RIGHT-handed space. Thus an anticlockwise winding order
    // should be applied here instead of the default front-clockwise winding order in DirectX.
    stencilReflectPsoDesc.RasterizerState.FrontCounterClockwise = true;
    checkHR(pCore->device->CreateGraphicsPipelineState(&stencilReflectPsoDesc, IID_PPV_ARGS(&pCore->PSOs["stencil_reflect"])));

    // Planar Shadow
    D3D12_GRAPHICS_PIPELINE_STATE_DESC planarShadowPsoDesc = alphaPsoDesc;
    D3D12_DEPTH_STENCIL_DESC planarShadowDSD = planarShadowPsoDesc.DepthStencilState;
    planarShadowDSD.DepthEnable = true;
    planarShadowDSD.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    planarShadowDSD.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    planarShadowDSD.StencilEnable = true;
    planarShadowDSD.StencilReadMask = 0xff;
    planarShadowDSD.StencilWriteMask = 0xff;
    planarShadowDSD.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    planarShadowDSD.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    planarShadowDSD.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
    planarShadowDSD.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
    planarShadowDSD.BackFace = planarShadowDSD.FrontFace;
    planarShadowPsoDesc.DepthStencilState = planarShadowDSD;
    checkHR(pCore->device->CreateGraphicsPipelineState(&planarShadowPsoDesc, IID_PPV_ARGS(&pCore->PSOs["planar_shadow"])));

    // Simple Geometry Shader
    D3D12_GRAPHICS_PIPELINE_STATE_DESC simpleGsPsoDesc = wireframePsoDesc;
    bindShaderToPSO(&simpleGsPsoDesc, pCore->s_subdivisionGs.get());
    checkHR(pCore->device->CreateGraphicsPipelineState(&simpleGsPsoDesc, IID_PPV_ARGS(&pCore->PSOs["simple_gs"])));

    // Billboard Geometry Shader
    D3D12_GRAPHICS_PIPELINE_STATE_DESC billboardGsPsoDesc = solidPsoDesc;
    bindShaderToPSO(&billboardGsPsoDesc, pCore->s_billboardGs.get());
    billboardGsPsoDesc.BlendState.AlphaToCoverageEnable = true;
    billboardGsPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    checkHR(pCore->device->CreateGraphicsPipelineState(&billboardGsPsoDesc, IID_PPV_ARGS(&pCore->PSOs["billboard_gs"])));

    // Cylinder Generator Geometry Shader
    D3D12_GRAPHICS_PIPELINE_STATE_DESC cylinderGeneratorGsPsoDesc = solidPsoDesc;
    bindShaderToPSO(&cylinderGeneratorGsPsoDesc, pCore->s_cylinderGeneratorGs.get());
    cylinderGeneratorGsPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    cylinderGeneratorGsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    checkHR(pCore->device->CreateGraphicsPipelineState(&cylinderGeneratorGsPsoDesc, IID_PPV_ARGS(&pCore->PSOs["cylinder_generator_gs"])));

    // Explosion Animation Geometry Shader
    D3D12_GRAPHICS_PIPELINE_STATE_DESC explosionAnimationGsPsoDesc = solidPsoDesc;
    bindShaderToPSO(&explosionAnimationGsPsoDesc, pCore->s_explosionGs.get());
    checkHR(pCore->device->CreateGraphicsPipelineState(&explosionAnimationGsPsoDesc, IID_PPV_ARGS(&pCore->PSOs["explosion_animation_gs"])));

    // Normal Visible Geometry Shader
    // Vertex Normal
    D3D12_GRAPHICS_PIPELINE_STATE_DESC verNormalVisibleGsPsoDesc = solidPsoDesc;
    bindShaderToPSO(&verNormalVisibleGsPsoDesc, pCore->s_verNormalVisibleGs.get());
    verNormalVisibleGsPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    checkHR(pCore->device->CreateGraphicsPipelineState(&verNormalVisibleGsPsoDesc, IID_PPV_ARGS(&pCore->PSOs["ver_normal_visible_gs"])));
    // Triangle Normal
    D3D12_GRAPHICS_PIPELINE_STATE_DESC triNormalVisibleGsPsoDesc = solidPsoDesc;
    bindShaderToPSO(&triNormalVisibleGsPsoDesc, pCore->s_triNormalVisibleGs.get());
    checkHR(pCore->device->CreateGraphicsPipelineState(&triNormalVisibleGsPsoDesc, IID_PPV_ARGS(&pCore->PSOs["tri_normal_visible_gs"])));
}

void createBasicMaterials(D3DCore* pCore) {
    auto red = std::make_unique<Material>();
    red->name = "red";
    red->matConstBuffIdx = 0;
    red->texSrvHeapIdx = pCore->textures2d["default"]->srvHeapIdx;
    red->constData.diffuseAlbedo = XMFLOAT4(Colors::Red);
    red->constData.fresnelR0 = { 1.0f, 0.0f, 0.0f };
    red->constData.roughness = 0.0f;
    pCore->materials[red->name] = std::move(red);

    auto green = std::make_unique<Material>();
    green->name = "green";
    green->matConstBuffIdx = 1;
    green->texSrvHeapIdx = pCore->textures2d["default"]->srvHeapIdx;
    green->constData.diffuseAlbedo = XMFLOAT4(Colors::Green);
    green->constData.fresnelR0 = { 0.0f, 1.0f, 0.0f };
    green->constData.roughness = 0.0f;
    pCore->materials[green->name] = std::move(green);

    auto blue = std::make_unique<Material>();
    blue->name = "blue";
    blue->matConstBuffIdx = 2;
    blue->texSrvHeapIdx = pCore->textures2d["default"]->srvHeapIdx;
    blue->constData.diffuseAlbedo = XMFLOAT4(Colors::Blue);
    blue->constData.fresnelR0 = { 0.0f, 0.0f, 1.0f };
    blue->constData.roughness = 0.0f;
    pCore->materials[blue->name] = std::move(blue);

    auto grass = std::make_unique<Material>();
    grass->name = "grass";
    grass->matConstBuffIdx = 3;
    grass->texSrvHeapIdx = pCore->textures2d["grass"]->srvHeapIdx;
    grass->constData.diffuseAlbedo = { 0.2f, 0.4f, 0.2f, 1.0f };
    grass->constData.fresnelR0 = { 0.001f, 0.001f, 0.001f };
    grass->constData.roughness = 0.8f;
    pCore->materials[grass->name] = std::move(grass);

    auto water = std::make_unique<Material>();
    water->name = "water";
    water->matConstBuffIdx = 4;
    water->texSrvHeapIdx = pCore->textures2d["water"]->srvHeapIdx;
    water->constData.diffuseAlbedo = { 0.4f, 0.4f, 0.6f, 0.6f };
    water->constData.fresnelR0 = { 0.1f, 0.1f, 0.1f };
    water->constData.roughness = 0.0f;
    pCore->materials[water->name] = std::move(water);

    auto crate = std::make_unique<Material>();
    crate->name = "crate";
    crate->matConstBuffIdx = 5;
    crate->texSrvHeapIdx = pCore->textures2d["crate"]->srvHeapIdx;
    crate->constData.diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    crate->constData.fresnelR0 = { 0.01f, 0.01f, 0.01f };
    crate->constData.roughness = 0.4f;
    pCore->materials[crate->name] = std::move(crate);

    auto fence = std::make_unique<Material>();
    fence->name = "fence";
    fence->matConstBuffIdx = 6;
    fence->texSrvHeapIdx = pCore->textures2d["fence"]->srvHeapIdx;
    fence->constData.diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    fence->constData.fresnelR0 = { 0.06f, 0.06f, 0.06f };
    fence->constData.roughness = 0.1f;
    pCore->materials[fence->name] = std::move(fence);

    auto brick = std::make_unique<Material>();
    brick->name = "brick";
    brick->matConstBuffIdx = 7;
    brick->texSrvHeapIdx = pCore->textures2d["brick"]->srvHeapIdx;
    brick->constData.diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    brick->constData.fresnelR0 = { 0.002f, 0.002f, 0.002f };
    brick->constData.roughness = 0.9f;
    pCore->materials[brick->name] = std::move(brick);

    auto checkboard = std::make_unique<Material>();
    checkboard->name = "checkboard";
    checkboard->matConstBuffIdx = 8;
    checkboard->texSrvHeapIdx = pCore->textures2d["checkboard"]->srvHeapIdx;
    checkboard->constData.diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    checkboard->constData.fresnelR0 = { 0.008f, 0.008f, 0.008f };
    checkboard->constData.roughness = 0.6f;
    pCore->materials[checkboard->name] = std::move(checkboard);

    auto skull = std::make_unique<Material>();
    skull->name = "skull";
    skull->matConstBuffIdx = 9;
    skull->texSrvHeapIdx = pCore->textures2d["default"]->srvHeapIdx;
    skull->constData.diffuseAlbedo = { 0.8f, 0.8f, 0.8f, 1.0f };
    skull->constData.fresnelR0 = { 0.003f, 0.003f, 0.003f };
    skull->constData.roughness = 0.7f;
    pCore->materials[skull->name] = std::move(skull);

    auto mirror = std::make_unique<Material>();
    mirror->name = "mirror";
    mirror->matConstBuffIdx = 10;
    mirror->texSrvHeapIdx = pCore->textures2d["ice"]->srvHeapIdx;
    mirror->constData.diffuseAlbedo = { 0.75f, 0.75f, 0.8f, 0.3f };
    mirror->constData.fresnelR0 = { 0.5f, 0.5f, 0.5f };
    mirror->constData.roughness = 0.0f;
    pCore->materials[mirror->name] = std::move(mirror);

    auto shadow = std::make_unique<Material>();
    shadow->name = "shadow";
    shadow->matConstBuffIdx = 11;
    shadow->texSrvHeapIdx = pCore->textures2d["default"]->srvHeapIdx;
    shadow->constData.diffuseAlbedo = { 0.0f, 0.0f, 0.0f, 0.5f };
    shadow->constData.fresnelR0 = { 0.001f, 0.001f, 0.001f };
    shadow->constData.roughness = 0.0f;
    pCore->materials[shadow->name] = std::move(shadow);

    auto trees = std::make_unique<Material>();
    trees->name = "trees";
    trees->matConstBuffIdx = 12;
    // Note this is a 2D texture array!
    trees->texSrvHeapIdx = pCore->textures2darray["tree_array"]->srvHeapIdx;
    trees->constData.diffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
    trees->constData.fresnelR0 = { 0.002f, 0.002f, 0.002f };
    trees->constData.roughness = 0.6f;
    pCore->materials[trees->name] = std::move(trees);
}

void loadBasicTextures(D3DCore* pCore) {
    checkHR(pCore->cmdAlloc->Reset());
    checkHR(pCore->cmdList->Reset(pCore->cmdAlloc.Get(), nullptr));

    auto defaultTex = std::make_unique<Texture>();
    defaultTex->name = "default";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/DefaultWhite.dds", defaultTex->resource, defaultTex->uploadHeap));
    pCore->textures2d[defaultTex->name] = std::move(defaultTex);

    auto grassTex = std::make_unique<Texture>();
    grassTex->name = "grass";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/grass.dds", grassTex->resource, grassTex->uploadHeap));
    pCore->textures2d[grassTex->name] = std::move(grassTex);

    auto waterTex = std::make_unique<Texture>();
    waterTex->name = "water";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/water1.dds", waterTex->resource, waterTex->uploadHeap));
    pCore->textures2d[waterTex->name] = std::move(waterTex);

    auto crateTex = std::make_unique<Texture>();
    crateTex->name = "crate";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/WoodCrate01.dds", crateTex->resource, crateTex->uploadHeap));
    pCore->textures2d[crateTex->name] = std::move(crateTex);

    auto fenceTex = std::make_unique<Texture>();
    fenceTex->name = "fence";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/WireFence.dds", fenceTex->resource, fenceTex->uploadHeap));
    pCore->textures2d[fenceTex->name] = std::move(fenceTex);

    auto brickTex = std::make_unique<Texture>();
    brickTex->name = "brick";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/bricks3.dds", brickTex->resource, brickTex->uploadHeap));
    pCore->textures2d[brickTex->name] = std::move(brickTex);

    auto checkboardTex = std::make_unique<Texture>();
    checkboardTex->name = "checkboard";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/checkboard.dds", checkboardTex->resource, checkboardTex->uploadHeap));
    pCore->textures2d[checkboardTex->name] = std::move(checkboardTex);

    auto iceTex = std::make_unique<Texture>();
    iceTex->name = "ice";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/ice.dds", iceTex->resource, iceTex->uploadHeap));
    pCore->textures2d[iceTex->name] = std::move(iceTex);

    auto treeArrayTex = std::make_unique<Texture>();
    treeArrayTex->name = "tree_array";
    checkHR(CreateDDSTextureFromFile12(pCore->device.Get(), pCore->cmdList.Get(),
        L"textures/treearray.dds", treeArrayTex->resource, treeArrayTex->uploadHeap));
    // Note this is a 2D texture array!
    pCore->textures2darray[treeArrayTex->name] = std::move(treeArrayTex);

    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);
    flushCmdQueue(pCore);
}

void createDescHeaps(D3DCore* pCore) {
    // SRV heap
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = pCore->textures2d.size() + pCore->textures2darray.size();
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    checkHR(pCore->device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&pCore->srvDescHeap)));

    int srvHeapIdx = 0;
    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(pCore->srvDescHeap->GetCPUDescriptorHandleForHeapStart());
    // 2D Texture
    for (auto& kv : pCore->textures2d) {
        kv.second->srvHeapIdx = srvHeapIdx;
        auto tex = kv.second->resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = tex->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = tex->GetDesc().MipLevels;
        //srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // LOD: Level of Detail
        pCore->device->CreateShaderResourceView(tex.Get(), &srvDesc, handle);
        ++srvHeapIdx;
        handle.Offset(1, pCore->cbvSrvUavDescSize);
    }
    // 2D Texture Array
    for (auto& kv : pCore->textures2darray) {
        kv.second->srvHeapIdx = srvHeapIdx;
        auto tex = kv.second->resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = tex->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = tex->GetDesc().MipLevels;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = tex->GetDesc().DepthOrArraySize;
        pCore->device->CreateShaderResourceView(tex.Get(), &srvDesc, handle);
        ++srvHeapIdx;
        handle.Offset(1, pCore->cbvSrvUavDescSize);
    }
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> generateStaticSamplers() {
    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f, 8);

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0.0f, 8);

    return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp };
}

void createRenderItemLayers(D3DCore* pCore) {
    // Note the layer names' order cannot be disorganized due to it decides the drawing priority.
    // Simply put, the name in front is drawn first. For example, solid layer is the first to draw.
    std::string ritemLayerNames[] = {
        // General
        "solid", "wireframe",
        // Geometry Shader
        "simple_gs", "billboard_gs", "cylinder_generator_gs", "explosion_animation_gs",
        "ver_normal_visible_gs", "tri_normal_visible_gs",
        // Alpha Blend, Stencil
        "alpha_test", "stencil_mark", "stencil_reflect", "planar_shadow", "alpha" };
    for (auto& name : ritemLayerNames) {
        pCore->ritemLayers.push_back({ name, std::vector<RenderItem*>() });
    }
}

void createRenderItems(D3DCore* pCore) {
    auto xAxisGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(100.0f, 0.01f, 0.01f), xAxisGeo.get());
    auto xAxis = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, xAxisGeo.get(), 1, xAxis.get());
    xAxis->materials[0] = pCore->materials["red"].get();
    moveNamedRitemToAllRitems(pCore, "X", std::move(xAxis));
    bindRitemReferenceWithLayers(pCore, "X", { {"solid",0} });

    auto yAxisGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(0.01f, 100.0f, 0.01f), yAxisGeo.get());
    auto yAxis = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, yAxisGeo.get(), 1, yAxis.get());
    yAxis->materials[0] = pCore->materials["green"].get();
    moveNamedRitemToAllRitems(pCore, "Y", std::move(yAxis));
    bindRitemReferenceWithLayers(pCore, "Y", { {"solid",0} });

    auto zAxisGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(0.01f, 0.01f, 100.0f), zAxisGeo.get());
    auto zAxis = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, zAxisGeo.get(), 1, zAxis.get());
    zAxis->materials[0] = pCore->materials["blue"].get();
    moveNamedRitemToAllRitems(pCore, "Z", std::move(zAxis));
    bindRitemReferenceWithLayers(pCore, "Z", { {"solid",0} });
}

void createFrameResources(D3DCore* pCore) {
    for (int i = 0; i < NUM_FRAME_RESOURCES; ++i) {
        auto resource = std::make_unique<FrameResource>();
        initEmptyFrameResource(pCore, resource.get());
        // Note [Total Object Constants Buffer Count] != [Total Render Item Count]
        // One render item can hold more than one object constants buffer seat.
        UINT totalObjBuffCount = calcRitemRangeTotalObjConstBuffSeatCount(
            pCore->allRitems.data(), pCore->allRitems.size());
        initFResourceObjConstBuff(pCore, totalObjBuffCount, resource.get());
        // Due to we use the stencil technique to achieve an effect of plane mirror,
        // another reflected process constant buffer is needed to draw the mirror objects.
        initFResourceProcConstBuff(pCore, 2, resource.get());
        initFResourceMatConstBuff(pCore, pCore->materials.size(), resource.get());
        pCore->frameResources.push_back(std::move(resource));
    }
}

void resizeSwapBuffs(int w, int h, XMFLOAT4 clearColor, D3DCore* pCore) {
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

    // Create swap chain buffers.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < 2; ++i) {
        checkHR(pCore->swapChain->GetBuffer(i, IID_PPV_ARGS(&pCore->swapChainBuffs[i])));
        pCore->device->CreateRenderTargetView(pCore->swapChainBuffs[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, pCore->rtvDescSize);
    }

    // Create MSAA buffer. It will be resolved and copyed to swap chain buffer to present.
    D3D12_RESOURCE_DESC renderTargetViewDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        pCore->swapChainBuffFormat, w, h, 1, 1, 4, pCore->_4xMsaaQuality); // 4xMSAA
    renderTargetViewDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE msaaBuffOptimizedClearValue = {};
    msaaBuffOptimizedClearValue.Format = pCore->swapChainBuffFormat;
    memcpy(msaaBuffOptimizedClearValue.Color, &clearColor, sizeof(FLOAT) * 4);
    pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &renderTargetViewDesc,
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        &msaaBuffOptimizedClearValue,
        IID_PPV_ARGS(&pCore->msaaBackBuff));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS; // MSAA
    rtvDesc.Format = pCore->swapChainBuffFormat;
    pCore->device->CreateRenderTargetView(pCore->msaaBackBuff.Get(), &rtvDesc, rtvHeapHandle);

    // Create depth stencil buffer.
    D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        pCore->depthStencilBuffFormat, w, h, 1, 1, 4, pCore->_4xMsaaQuality); // 4xMSAA
    depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE dsvOptimizedClearValue;
    dsvOptimizedClearValue.Format = pCore->depthStencilBuffFormat;
    dsvOptimizedClearValue.DepthStencil.Depth = 1.0f;
    dsvOptimizedClearValue.DepthStencil.Stencil = 0;
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &dsvOptimizedClearValue,
        IID_PPV_ARGS(&pCore->depthStencilBuff)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS; // MSAAs
    dsvDesc.Format = pCore->depthStencilBuffFormat;
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

void clearBackBuff(D3D12_CPU_DESCRIPTOR_HANDLE msaaRtvDescHandle, XMVECTORF32 color,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescHandle, FLOAT depth, UINT8 stencil, D3DCore* pCore)
{
    pCore->cmdList->ClearRenderTargetView(
        msaaRtvDescHandle, color, 0, nullptr);
    pCore->cmdList->ClearDepthStencilView(
        dsvDescHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}