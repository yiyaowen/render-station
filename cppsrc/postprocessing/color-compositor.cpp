/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "d3dcore/d3dcore.h"
#include "color-compositor.h"
#include "utils/debugger.h"

ColorCompositor::ColorCompositor(D3DCore* pCore)
	: BasicProcess(pCore)
{
	// Reserved
}

void ColorCompositor::init() {
    _isPrepared = true;

    // Create descriptor heap, descriptors and off-screen texture resources.
    createTextureDescriptorHeap();
    createOffscreenTextureResources();
    createResourceDescriptors();

    // Create sobel operator root signature.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    slotRootParameter[0].InitAsConstants(2, 0);
    CD3DX12_DESCRIPTOR_RANGE srvTable[2] = {};
    srvTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable[0]);
    srvTable[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    slotRootParameter[2].InitAsDescriptorTable(1, &srvTable[1]);
    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    slotRootParameter[3].InitAsDescriptorTable(1, &uavTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE);
    createRootSig(pCore, "color_compositor", &rootSigDesc);

    // Compile sobel operator shaders.
    ShaderFuncEntryPoints csEntryPoint = {};
    csEntryPoint.cs = "MixerCS";
    s_mixer = std::make_unique<Shader>(
        "color_compositor", L"shaders/postprocessing/color-compositor.hlsl", Shader::CS, csEntryPoint);

    // Create sobel operator compute shader PSOs.
    D3D12_COMPUTE_PIPELINE_STATE_DESC mixerPsoDesc = {};
    mixerPsoDesc.pRootSignature = pCore->rootSigs["color_compositor"].Get();
    bindShaderToCPSO(&mixerPsoDesc, s_mixer.get());
    mixerPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    checkHR(pCore->device->CreateComputePipelineState(&mixerPsoDesc, IID_PPV_ARGS(&pCore->PSOs["color_compositor"])));
}

void ColorCompositor::onResize(UINT w, UINT h) {
    texWidth = w;
    texHeight = h;
    createOffscreenTextureResources();
    createResourceDescriptors();
}

ID3D12Resource* ColorCompositor::process(ID3D12Resource* flatOrigin) {
    pCore->cmdList->SetComputeRootSignature(pCore->rootSigs["color_compositor"].Get());

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            flatOrigin,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_SOURCE));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["A"].Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST));

    pCore->cmdList->CopyResource(textures["A"].Get(), flatOrigin);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            flatOrigin,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_COMMON));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["A"].Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ));

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["C"].Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    pCore->cmdList->SetPipelineState(pCore->PSOs["color_compositor"].Get());
    pCore->cmdList->SetComputeRoot32BitConstant(0, _mixType, 0);
    pCore->cmdList->SetComputeRoot32BitConstant(0, _weight, 1);
    pCore->cmdList->SetComputeRootDescriptorTable(1, texA_SrvGPU);
    pCore->cmdList->SetComputeRootDescriptorTable(2, texB_SrvGPU);
    pCore->cmdList->SetComputeRootDescriptorTable(3, texC_UavGPU);

    UINT numGroupX = (UINT)ceilf(texWidth / 32.0f); // M = 32 in multiplication-compositor.hlsl
    UINT numGroupY = (UINT)ceilf(texHeight / 32.0f); // N = 32 in multiplication-compositor.hlsl
    pCore->cmdList->Dispatch(numGroupX, numGroupY, 1);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["A"].Get(),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            D3D12_RESOURCE_STATE_COMMON));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["C"].Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON));

    return textures["C"].Get();
}

void ColorCompositor::bindBackgroundPlate(ID3D12Resource* bkgn, int mixType, float weight) {

    _mixType = mixType;
    _weight = weight;

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            bkgn,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_SOURCE));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["B"].Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_COPY_DEST));

    pCore->cmdList->CopyResource(textures["B"].Get(), bkgn);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            bkgn,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_COMMON));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["B"].Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_GENERIC_READ));
}

void ColorCompositor::createOffscreenTextureResources() {
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = texWidth;
    texDesc.Height = texHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = pCore->swapChainBuffFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_RESOURCE_DESC uavTexDesc = texDesc;
    // Enable changing to UAV state.
    uavTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&textures["A"])));

    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&textures["B"])));

    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &uavTexDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&textures["C"])));
}

void ColorCompositor::createTextureDescriptorHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC tdhDesc = {}; // tdh: Texture Descriptor Heap
    tdhDesc.NumDescriptors = 3; // texA SRV, texB SRV, texC UAV.
    tdhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    tdhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    checkHR(pCore->device->CreateDescriptorHeap(&tdhDesc, IID_PPV_ARGS(&texDescHeap)));
}

void ColorCompositor::createResourceDescriptors() {
    auto cbvSrvUavDescSize = pCore->cbvSrvUavDescSize;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(texDescHeap->GetCPUDescriptorHandleForHeapStart());
    texA_SrvCPU = handle;
    texB_SrvCPU = handle.Offset(1, cbvSrvUavDescSize);
    texC_UavCPU = handle.Offset(1, cbvSrvUavDescSize);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(texDescHeap->GetGPUDescriptorHandleForHeapStart());
    texA_SrvGPU = gpuHandle;
    texB_SrvGPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
    texC_UavGPU = gpuHandle.Offset(1, cbvSrvUavDescSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = pCore->swapChainBuffFormat;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = pCore->swapChainBuffFormat;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    pCore->device->CreateShaderResourceView(textures["A"].Get(), &srvDesc, texA_SrvCPU);
    pCore->device->CreateShaderResourceView(textures["B"].Get(), &srvDesc, texB_SrvCPU);
    pCore->device->CreateUnorderedAccessView(textures["C"].Get(), nullptr, &uavDesc, texC_UavCPU);
}
