/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "basic-process.h"
#include "d3dcore/d3dcore.h"
#include "utils/debugger.h"

BasicProcess::BasicProcess(D3DCore* pCore) : pCore(pCore) {
    auto wh = getWndSize(pCore->hWnd);
    texWidth = wh.first;
    texHeight = wh.second;
}

void BasicProcess::init() {
    _isPrepared = true;
    createOffscreenTextureResources();
}

void BasicProcess::onResize(UINT w, UINT h) {
    texWidth = w;
    texHeight = h;
    createOffscreenTextureResources();
}

ID3D12Resource* BasicProcess::process(ID3D12Resource* msaaOrigin) {
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            msaaOrigin,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["main"].Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_RESOLVE_DEST));

    // We simply resolve and copy the MSAA origin buffer to the flat output buffer without postprocessing.
    pCore->cmdList->ResolveSubresource(textures["main"].Get(), 0, msaaOrigin, 0, pCore->swapChainBuffFormat);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            msaaOrigin,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET));
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            textures["main"].Get(),
            D3D12_RESOURCE_STATE_RESOLVE_DEST,
            D3D12_RESOURCE_STATE_COMMON));

    return textures["main"].Get();
}

void BasicProcess::createOffscreenTextureResources() {
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

    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&textures["main"])));
}
