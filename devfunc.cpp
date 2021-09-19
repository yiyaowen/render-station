/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <DirectXColors.h>

#include "debugger.h"
#include "devfunc.h"
#include "vmesh-utils.h"

static UINT calcConstBuffSize(UINT byteSize)
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

void dev_initCoreElems(D3DCore* pCore) {
    // Flush command queue and reset command list before change any resource.
    flushCmdQueue(pCore);
    pCore->cmdList->Reset(pCore->cmdAlloc.Get(), nullptr);

    // Create constant buffers and mapped data blocks for target objects.
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(calcConstBuffSize(sizeof(ObjConsts))),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&pCore->objConstBuffGPU)));
    checkHR(pCore->objConstBuffGPU->Map(0, nullptr, reinterpret_cast<void**>(&pCore->objConstBuffCPU)));

    // Create CBV.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = pCore->objConstBuffGPU->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = calcConstBuffSize(sizeof(ObjConsts));
    pCore->device->CreateConstantBufferView(
        &cbvDesc,
        pCore->cbvHeap->GetCPUDescriptorHandleForHeapStart());

    // Create target objects.
    Vertex vertices[8] =
    {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
        Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
    };

    UINT16 indices[36] =
    {
        // front face
        0, 1, 2,
        0, 2, 3,

        // back face
        4, 6, 5,
        4, 7, 6,

        // left face
        4, 5, 1,
        4, 1, 0,

        // right face
        3, 2, 6,
        3, 6, 7,

        // top face
        1, 5, 6,
        1, 6, 2,

        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    pCore->meshes["main"] = std::make_unique<Vmesh>();
    initVmesh(pCore, vertices, sizeof(vertices), indices, sizeof(indices), pCore->meshes["main"].get());

    Vsubmesh cube;
    cube.indexCount = 36;
    cube.startIndexLocation = 0;
    cube.baseVertexLocation = 0;
    pCore->meshes["main"]->objects["cube"] = cube;

    // Wait for all commands to be executed.
    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);
    flushCmdQueue(pCore);
}

void dev_updateCoreData(D3DCore* pCore) {
    XMMATRIX viewMat = XMLoadFloat4x4(&pCore->camera->viewTrans);
    XMMATRIX projMat = XMLoadFloat4x4(&pCore->camera->projTrans);
    XMMATRIX worldViewProjMat = viewMat * projMat;

    ObjConsts cubeConsts;
    XMStoreFloat4x4(&cubeConsts.worldTrans, XMMatrixTranspose(worldViewProjMat));
    memcpy(pCore->objConstBuffCPU, &cubeConsts, sizeof(cubeConsts));
}

void dev_drawCoreElems(D3DCore* pCore) {
    checkHR(pCore->cmdAlloc->Reset());
    checkHR(pCore->cmdList->Reset(pCore->cmdAlloc.Get(), pCore->PSOs["solid"].Get()));
    
    pCore->cmdList->RSSetViewports(1, &pCore->camera->screenViewport);
    pCore->cmdList->RSSetScissorRects(1, &pCore->camera->scissorRect);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->swapChainBuffs[pCore->currBackBuffIdx].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    clearBackBuff(Colors::SteelBlue, 1.0f, 0, pCore);

    pCore->cmdList->OMSetRenderTargets(1,
        &CD3DX12_CPU_DESCRIPTOR_HANDLE(
            pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            pCore->currBackBuffIdx,
            pCore->rtvDescSize), true,
        &CD3DX12_CPU_DESCRIPTOR_HANDLE(pCore->dsvHeap->GetCPUDescriptorHandleForHeapStart()));

    ID3D12DescriptorHeap* descHeaps[] = { pCore->cbvHeap.Get() };
    pCore->cmdList->SetDescriptorHeaps(1, descHeaps);

    pCore->cmdList->SetGraphicsRootSignature(pCore->rootSig.Get());
    pCore->cmdList->SetGraphicsRootDescriptorTable(0, pCore->cbvHeap->GetGPUDescriptorHandleForHeapStart());

    pCore->cmdList->IASetVertexBuffers(0, 1, &pCore->meshes["main"]->vertexBuffView);
    pCore->cmdList->IASetIndexBuffer(&pCore->meshes["main"]->indexBuffView);
    pCore->cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    Vsubmesh cube = pCore->meshes["main"]->objects["cube"];
    pCore->cmdList->DrawIndexedInstanced(cube.indexCount, 1, cube.startIndexLocation, cube.baseVertexLocation, 0);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->swapChainBuffs[pCore->currBackBuffIdx].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT));

    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);

    checkHR(pCore->swapChain->Present(0, 0));
    pCore->currBackBuffIdx = (pCore->currBackBuffIdx + 1) % 2;

    flushCmdQueue(pCore);
}

void dev_onMouseDown(WPARAM btnState, int x, int y, D3DCore* pCore) {
    pCore->camera->lastMouseX = x;
    pCore->camera->lastMouseY = y;
    SetCapture(pCore->hWnd);
}

void dev_onMouseUp(WPARAM btnState, int x, int y, D3DCore* pCore) {
    ReleaseCapture();
}

void dev_onMouseMove(WPARAM btnState, int x, int y, D3DCore* pCore) {
    if ((btnState & MK_LBUTTON) != 0) {
        float dx = XMConvertToRadians(0.25f * (x - pCore->camera->lastMouseX));
        float dy = XMConvertToRadians(0.25f * (y - pCore->camera->lastMouseY));
        // Theta = Angle<radial-proj, X-axis>, Phi = Angle<radial, Y-axis>.
        rotateCamera(-dy, -dx, pCore->camera.get());
    }
    else if ((btnState & MK_MBUTTON) != 0) {
        //float dx = 0.0001 * (x - pCore->camera->lastMouseX);
        //float dy = 0.0001 * (y - pCore->camera->lastMouseY);
        //translateCamera(-dx, dy, 0.0f, pCore->camera.get());
    }
    else if ((btnState & MK_RBUTTON) != 0) {
        float dy = 0.02f * (float)(y - pCore->camera->lastMouseY);
        zoomCamera(-dy, pCore->camera.get());
    }
    pCore->camera->lastMouseX = x;
    pCore->camera->lastMouseY = y;
}
