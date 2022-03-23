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
    pMesh->vertexBuffView.SizeInBytes = (UINT)vertexDataSize;

    pMesh->indexBuffView.BufferLocation = pMesh->indexBuffGPU->GetGPUVirtualAddress();
    pMesh->indexBuffView.Format = DXGI_FORMAT_R32_UINT;
    pMesh->indexBuffView.SizeInBytes = (UINT)indexDataSize;
}

void createDefaultBuffs(D3DCore* pCore, const void* initData, UINT64 byteSize,
    ID3DBlob** ppBuffCPU, ID3D12Resource** ppBuffGPU, ID3D12Resource** ppUploadBuff)
{
    pCore->cmdAlloc->Reset();
    pCore->cmdList->Reset(pCore->cmdAlloc.Get(), nullptr);

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

    uploadStatedResource(pCore,
        *ppBuffGPU, D3D12_RESOURCE_STATE_COMMON,
        *ppUploadBuff, D3D12_RESOURCE_STATE_GENERIC_READ,
        initData, byteSize);

    pCore->cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*ppBuffGPU,
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ));

    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);
    flushCmdQueue(pCore);
}

Vmesh* copyVmesh(D3DCore* pCore, const Vmesh* source) {
    std::unique_ptr<Vmesh> mesh = std::make_unique<Vmesh>();

    initVmesh(pCore,
        source->vertexBuffCPU->GetBufferPointer(), source->vertexBuffCPU->GetBufferSize(),
        source->indexBuffCPU->GetBufferPointer(), source->indexBuffCPU->GetBufferSize(),
        mesh.get());

    return mesh.release();
}

void createDefaultTexs(D3DCore* pCore, const void* initData, UINT64 byteSize,
    D3D12_RESOURCE_DESC* texDesc, ID3DBlob** ppTexCPU, ID3D12Resource** ppTexGPU, ID3D12Resource** ppUploadBuff)
{
    pCore->cmdAlloc->Reset();
    pCore->cmdList->Reset(pCore->cmdAlloc.Get(), nullptr);

    // Create CPU data block.
    checkHR(D3DCreateBlob(byteSize, ppTexCPU));
    memcpy((*ppTexCPU)->GetBufferPointer(), initData, byteSize);

    // Create GPU buffer and upload buffer.
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(ppTexGPU)));

    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppUploadBuff)));

    uploadStatedResource(pCore,
        *ppTexGPU, D3D12_RESOURCE_STATE_COMMON,
        *ppUploadBuff, D3D12_RESOURCE_STATE_GENERIC_READ,
        initData, byteSize);

    pCore->cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(*ppTexGPU,
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_GENERIC_READ));

    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);
    flushCmdQueue(pCore);
}
