/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "d3dcore/d3dcore.h"
#include "gaussian-blur.h"
#include "utils/debugger.h"
#include "utils/math-utils.h"

GaussianBlur::GaussianBlur(D3DCore* pCore, int blurRadius, float blurGrade, int blurCount)
    : BasicProcess(pCore), _blurRadius(blurRadius), _blurGrade(blurGrade), _blurCount(blurCount)
{
    // Reserved
}

void GaussianBlur::init() {
    _isPrepared = true;

    // Create descriptor heap, descriptors and off-screen texture resources.
    createTextureDescriptorHeap();
    createOffscreenTextureResources();
    createResourceDescriptors();

    // Create blur filter shader root signature.
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];

    slotRootParameter[0].InitAsConstants(12, 0);
    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE);
    createRootSig(pCore, "gaussian_blur", &rootSigDesc);

    // Compile blur filter shaders.
    ShaderFuncEntryPoints csEntryPoint = {};
    csEntryPoint.cs = "HorzBlurCS";
    s_gaussianBlurHorz = std::make_unique<Shader>(
        "gaussian_blur_horz", L"shaders/postprocessing/gaussian-blur.hlsl", Shader::CS, csEntryPoint);
    csEntryPoint.cs = "VertBlurCS";
    s_gaussianBlurVert = std::make_unique<Shader>(
        "gaussian_blur_horz", L"shaders/postprocessing/gaussian-blur.hlsl", Shader::CS, csEntryPoint);

    // Create blur filter compute shader PSOs.
    D3D12_COMPUTE_PIPELINE_STATE_DESC horzPsoDesc = {};
    horzPsoDesc.pRootSignature = pCore->rootSigs["gaussian_blur"].Get();
    bindShaderToCPSO(&horzPsoDesc, s_gaussianBlurHorz.get());
    horzPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    checkHR(pCore->device->CreateComputePipelineState(&horzPsoDesc, IID_PPV_ARGS(&pCore->PSOs["gaussian_blur_horz"])));

    D3D12_COMPUTE_PIPELINE_STATE_DESC vertPsoDesc = {};
    vertPsoDesc.pRootSignature = pCore->rootSigs["gaussian_blur"].Get();
    bindShaderToCPSO(&vertPsoDesc, s_gaussianBlurVert.get());
    vertPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    checkHR(pCore->device->CreateComputePipelineState(&vertPsoDesc, IID_PPV_ARGS(&pCore->PSOs["gaussian_blur_vert"])));
}

void GaussianBlur::onResize(UINT w, UINT h) {
    texWidth = w;
    texHeight = h;
    createOffscreenTextureResources();
    createResourceDescriptors();
}

ID3D12Resource* GaussianBlur::process(ID3D12Resource* flatOrigin) {
    auto weights = calcGaussianBlurWeight(_blurRadius, _blurGrade);

    ID3D12DescriptorHeap* descHeaps[] = { texDescHeap.Get() };
    pCore->cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

    pCore->cmdList->SetComputeRootSignature(pCore->rootSigs["gaussian_blur"].Get());

    pCore->cmdList->SetComputeRoot32BitConstants(0, 1, &_blurRadius, 0);
    pCore->cmdList->SetComputeRoot32BitConstants(0, weights.size(), weights.data(), 1);

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

    for (int i = 0; i < _blurCount; ++i) {
        // Horizontal Blur
        pCore->cmdList->SetPipelineState(pCore->PSOs["gaussian_blur_horz"].Get());
        pCore->cmdList->SetComputeRootDescriptorTable(1, texA_SrvGPU);
        pCore->cmdList->SetComputeRootDescriptorTable(2, texB_UavGPU);

        UINT numGroupX = (UINT)ceilf(texWidth / 256.0f); // N = 256 in gaussian-blur.hlsl
        pCore->cmdList->Dispatch(numGroupX, texHeight, 1);

        pCore->cmdList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                textures["A"].Get(),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
        pCore->cmdList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                textures["B"].Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_GENERIC_READ));

        // Vertical Blur
        pCore->cmdList->SetPipelineState(pCore->PSOs["gaussian_blur_vert"].Get());
        pCore->cmdList->SetComputeRootDescriptorTable(1, texB_SrvGPU);
        pCore->cmdList->SetComputeRootDescriptorTable(2, texA_UavGPU);

        UINT numGroupY = (UINT)ceilf(texHeight / 256.0f); // N = 256 in gaussian-blur.hlsl
        pCore->cmdList->Dispatch(texWidth, numGroupY, 1);

        pCore->cmdList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                textures["A"].Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_GENERIC_READ));
        pCore->cmdList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                textures["B"].Get(),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    }

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

    return textures["A"].Get();
}

void GaussianBlur::createOffscreenTextureResources() {
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
    // Enable changing to UAV state.
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

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
}

void GaussianBlur::createTextureDescriptorHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC tdhDesc = {}; // tdh: Texture Descriptor Heap
    tdhDesc.NumDescriptors = 4; // SRV, UAV for texA, texB, 2*2 = 4
    tdhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    tdhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    checkHR(pCore->device->CreateDescriptorHeap(&tdhDesc, IID_PPV_ARGS(&texDescHeap)));
}

void GaussianBlur::createResourceDescriptors() {
    auto cbvSrvUavDescSize = pCore->cbvSrvUavDescSize;

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(texDescHeap->GetCPUDescriptorHandleForHeapStart());
    texA_SrvCPU = handle;
    texA_UavCPU = handle.Offset(1, cbvSrvUavDescSize);
    texB_SrvCPU = handle.Offset(1, cbvSrvUavDescSize);
    texB_UavCPU = handle.Offset(1, cbvSrvUavDescSize);

    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(texDescHeap->GetGPUDescriptorHandleForHeapStart());
    texA_SrvGPU = gpuHandle;
    texA_UavGPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
    texB_SrvGPU = gpuHandle.Offset(1, cbvSrvUavDescSize);
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
    pCore->device->CreateShaderResourceView(textures["B"].Get(), &srvDesc, texB_SrvCPU);

    pCore->device->CreateUnorderedAccessView(textures["A"].Get(), nullptr, &uavDesc, texA_UavCPU);
    pCore->device->CreateUnorderedAccessView(textures["B"].Get(), nullptr, &uavDesc, texB_UavCPU);
}