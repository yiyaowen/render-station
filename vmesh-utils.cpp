/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <d3dcompiler.h>

#include "debugger.h"
#include "vmesh-utils.h"

void initVmesh(D3DCore* pCore, const void* vertexData, UINT64 vertexDataSize,
    const void* indexData, UINT64 indexDataSize, Vmesh* pMesh)
{
    createDefaultBuffs(pCore, vertexData, vertexDataSize,
        &pMesh->vertexBuffCPU, &pMesh->vertexBuffGPU, &pMesh->vertexUploadBuff);
    createDefaultBuffs(pCore, indexData, indexDataSize,
        &pMesh->indexBuffCPU, &pMesh->indexBuffGPU, &pMesh->indexUploadBuff);

    pMesh->vertexBuffView.BufferLocation = pMesh->vertexBuffGPU->GetGPUVirtualAddress();
    pMesh->vertexBuffView.StrideInBytes = sizeof(Vertex);
    pMesh->vertexBuffView.SizeInBytes = vertexDataSize;

    pMesh->indexBuffView.BufferLocation = pMesh->indexBuffGPU->GetGPUVirtualAddress();
    pMesh->indexBuffView.Format = DXGI_FORMAT_R16_UINT;
    pMesh->indexBuffView.SizeInBytes = indexDataSize;
}

void createDefaultBuffs(D3DCore* pCore, const void* initData, UINT64 byteSize,
    ID3DBlob** ppBuffCPU, ID3D12Resource** ppBuffGPU, ID3D12Resource** ppUploadBuff)
{
    // Create CPU data block.
    checkHR(D3DCreateBlob(byteSize, ppBuffCPU));
    memcpy((*ppBuffCPU)->GetBufferPointer(), initData, byteSize);

    // Create GPU buffer and upload buffer.
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(ppBuffGPU)));

    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppUploadBuff)));

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Note UpdateSubresources func is essentially a GPU operation, which must be invoked
    // after reset the command list. We also need to close the command list to finish the
    // command pushing and flush the command queue to wait for all data to be uploaded.
    pCore->cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*ppBuffGPU,
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    UpdateSubresources<1>(pCore->cmdList.Get(), *ppBuffGPU, *ppUploadBuff, 0, 0, 1, &subResourceData);
    pCore->cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*ppBuffGPU,
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}
