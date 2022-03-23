/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "d3dcore/d3dcore.h"
#include "sobel-operator.h"
#include "utils/debugger.h"

SobelOperator::SobelOperator(D3DCore* pCore)
	: BasicProcess(pCore)
{
	// Reserved
}

void SobelOperator::init() {
    _isPrepared = true;

    // Create descriptor heap, descriptors and off-screen texture resources.
    createTextureDescriptorHeap();
    createOffscreenTextureResources();
    createResourceDescriptors();

    // Create sobel operator root signature.
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[0].InitAsDescriptorTable(1, &srvTable);
    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    slotRootParameter[1].InitAsDescriptorTable(1, &uavTable);
    slotRootParameter[2].InitAsConstants(1, 0);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE);
    createRootSig(pCore, "sobel_operator", &rootSigDesc);

    // Compile sobel operator shaders.
    ShaderFuncEntryPoints csEntryPoint = {};
    csEntryPoint.cs = "SobelCS";
    s_sobelOperator = std::make_unique<Shader>(
        "sobel_operator", L"shaders/postprocessing/sobel-operator.hlsl", Shader::CS, csEntryPoint);

    // Create sobel operator compute shader PSOs.
    D3D12_COMPUTE_PIPELINE_STATE_DESC sobelPsoDesc = {};
    sobelPsoDesc.pRootSignature = pCore->rootSigs["sobel_operator"].Get();
    bindShaderToCPSO(&sobelPsoDesc, s_sobelOperator.get());
    sobelPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    checkHR(pCore->device->CreateComputePipelineState(&sobelPsoDesc, IID_PPV_ARGS(&pCore->PSOs["sobel_operator"])));
}

void SobelOperator::onResize(UINT w, UINT h) {
    texWidth = w;
    texHeight = h;
    createOffscreenTextureResources();
    createResourceDescriptors();
}

ID3D12Resource* SobelOperator::process(ID3D12Resource* flatOrigin) {

    ID3D12DescriptorHeap* descHeaps[] = { texDescHeap.Get() };
    pCore->cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

    pCore->cmdList->SetComputeRootSignature(pCore->rootSigs["sobel_operator"].Get());

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
            textures["B"].Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    pCore->cmdList->SetPipelineState(pCore->PSOs["sobel_operator"].Get());
    pCore->cmdList->SetComputeRootDescriptorTable(0, texA_SrvGPU);
    pCore->cmdList->SetComputeRootDescriptorTable(1, texB_UavGPU);
    int colorMode = BLACK_ON_WHITE;
    if (GetAsyncKeyState(VK_SPACE)) colorMode = WHITE_ON_BLACK;
    pCore->cmdList->SetComputeRoot32BitConstant(2, colorMode, 0);

    UINT numGroupX = (UINT)ceilf(texWidth / 32.0f); // M = 32 in sobel-operator.hlsl
    UINT numGroupY = (UINT)ceilf(texHeight / 32.0f); // N = 32 in sobel-operator.hlsl
    pCore->cmdList->Dispatch(numGroupX, numGroupY, 1);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["A"].Get(),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            D3D12_RESOURCE_STATE_COMMON));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["B"].Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON));

    return textures["B"].Get();
}

void SobelOperator::createOffscreenTextureResources() {
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
        &uavTexDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&textures["B"])));
}

void SobelOperator::createTextureDescriptorHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC tdhDesc = {}; // tdh: Texture Descriptor Heap
    tdhDesc.NumDescriptors = 2; // texA SRV, texB UAV.
    tdhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    tdhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    checkHR(pCore->device->CreateDescriptorHeap(&tdhDesc, IID_PPV_ARGS(&texDescHeap)));
}

void SobelOperator::createResourceDescriptors() {
    auto cbvSrvUavDescSize = pCore->cbvSrvUavDescSize;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(texDescHeap->GetCPUDescriptorHandleForHeapStart());
    texA_SrvCPU = handle;
    texB_UavCPU = handle.Offset(1, cbvSrvUavDescSize);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(texDescHeap->GetGPUDescriptorHandleForHeapStart());
    texA_SrvGPU = gpuHandle;
    texB_UavGPU = gpuHandle.Offset(1, cbvSrvUavDescSize);

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
    pCore->device->CreateUnorderedAccessView(textures["B"].Get(), nullptr, &uavDesc, texB_UavCPU);
}
