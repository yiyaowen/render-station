/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "debugger.h"
#include "frame-async-utils.h"

void initFrameResource(D3DCore* pCore, UINT objBuffCount, UINT procBuffCount, FrameResource* pResource) {
    checkHR(pCore->device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&pResource->cmdAlloc)));
    createConstBuffPair(pCore, sizeof(ObjConsts), objBuffCount,
        &pResource->objConstBuffCPU, &pResource->objConstBuffGPU);
    createConstBuffPair(pCore, sizeof(ProcConsts), procBuffCount,
        &pResource->procConstBuffCPU, &pResource->procConstBuffGPU);
}

void drawRenderItems(D3DCore* pCore, RenderItem** ppRitem, UINT ritemCount) {
    for (UINT i = 0; i < ritemCount; ++i) {
        pCore->cmdList->IASetVertexBuffers(0, 1, &ppRitem[i]->mesh->vertexBuffView);
        pCore->cmdList->IASetIndexBuffer(&ppRitem[i]->mesh->indexBuffView);
        pCore->cmdList->IASetPrimitiveTopology(ppRitem[i]->topologyType);

        UINT cbvIdx = pCore->currFrameResourceIdx * pCore->solidModeRitems.size() + ppRitem[i]->objConstBuffIdx;
        auto cbvHanlde = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCore->cbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHanlde.Offset(cbvIdx, pCore->cbvSrvUavDescSize);
        pCore->cmdList->SetGraphicsRootDescriptorTable(0, cbvHanlde);

        Vsubmesh ritemMain = ppRitem[i]->mesh->objects["main"];
        pCore->cmdList->DrawIndexedInstanced(ritemMain.indexCount, 1, ritemMain.startIndexLocation, ritemMain.baseVertexLocation, 0);
    }
}

UINT calcConstBuffSize(UINT byteSize)
{
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose byteSize = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (byteSize + 255) & ~255;
}

void createConstBuffPair(D3DCore* pCore, size_t elemSize, UINT elemCount,
    BYTE** ppBuffCPU, ID3D12Resource** ppBuffGPU)
{
    // Create GPU buffer & CPU mapped data block
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(calcConstBuffSize(elemSize) * elemCount),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppBuffGPU)));
    checkHR((*ppBuffGPU)->Map(0, nullptr, reinterpret_cast<void**>(ppBuffCPU)));
}
