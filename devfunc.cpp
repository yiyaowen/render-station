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
#include "frame-async-utils.h"
#include "geometry-utils.h"
#include "vmesh-utils.h"

void dev_initCoreElems(D3DCore* pCore) {
    
}

void dev_updateCoreObjConsts(D3DCore* pCore) {
    auto currObjConstBuff = pCore->currFrameResource->objConstBuffCPU;
    for (auto& ritem : pCore->ritems) {
        if (ritem->numDirtyFrames > 0) {
            memcpy(currObjConstBuff + ritem->objConstBuffIdx * calcConstBuffSize(sizeof(ObjConsts)),
                &ritem->constData, sizeof(ObjConsts));
            // After each update progress, we decrease numDirtyFrames of the render item.
            // If numDirtyFrames is still greater than 0, then it will be updated in next update progress.
            ritem->numDirtyFrames--;
        }
    }
}

void dev_updateCoreProcConsts(D3DCore* pCore) {
    ProcConsts constData;
    XMMATRIX viewMat = XMLoadFloat4x4(&pCore->camera->viewTrans);
    XMMATRIX projMat = XMLoadFloat4x4(&pCore->camera->projTrans);
    XMStoreFloat4x4(&constData.viewTrans, XMMatrixTranspose(viewMat));
    XMStoreFloat4x4(&constData.projTrans, XMMatrixTranspose(projMat));
    memcpy(pCore->currFrameResource->procConstBuffCPU, &constData, sizeof(ProcConsts));
}

void dev_updateCoreData(D3DCore* pCore) {
    pCore->currFrameResourceIdx = (pCore->currFrameResourceIdx + 1) % NUM_FRAME_RESOURCES;
    pCore->currFrameResource = pCore->frameResources[pCore->currFrameResourceIdx].get();

    // See end of dev_drawCoreElems, where we update the fence values.
    // When the fence values are updated, the value of frame resource is equal to the value of main fence.
    // With GPU processing the commands pushed by CPU, its flag value becomes closer to the main fence value.
    // If the flag value, i.e. main fence's GetCompletedValue, is less than value of frame resource, it means
    // that the commands pushed during this frame resource processed have not executed completely yet, and it
    // is obviously there is no more disengaged frame resource to use, which means we should wait GPU here.
    if (pCore->currFrameResource->currFenceValue != 0 &&
        pCore->fence->GetCompletedValue() < pCore->currFrameResource->currFenceValue)
    {
        HANDLE hEvent;
        checkNull(hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
        // Fire event when GPU reaches current fence.
        checkHR(pCore->fence->SetEventOnCompletion(pCore->currFenceValue, hEvent));
        // Wait until the event is triggered.
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    dev_updateCoreObjConsts(pCore);
    dev_updateCoreProcConsts(pCore);
}

void dev_drawCoreElems(D3DCore* pCore) {
    checkHR(pCore->currFrameResource->cmdAlloc->Reset());
    // If the key is pressed, GetAsyncKeyState returns a SHORT value whose 15th bit is set (starts at 0).
    if (GetAsyncKeyState('1') & 0x8000) {
        checkHR(pCore->cmdList->Reset(pCore->currFrameResource->cmdAlloc.Get(), pCore->PSOs["wireframe"].Get()));
    }
    else {
        checkHR(pCore->cmdList->Reset(pCore->currFrameResource->cmdAlloc.Get(), pCore->PSOs["solid"].Get()));
    }
    
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

    int procCbvIdx = NUM_FRAME_RESOURCES * pCore->solidModeRitems.size() + pCore->currFrameResourceIdx;
    auto procCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(pCore->cbvHeap->GetGPUDescriptorHandleForHeapStart());
    procCbvHandle.Offset(procCbvIdx, pCore->cbvSrvUavDescSize);
    pCore->cmdList->SetGraphicsRootDescriptorTable(1, procCbvHandle);

    drawRenderItems(pCore, pCore->solidModeRitems.data(), pCore->solidModeRitems.size());

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

    // We use the following frame resource loop array technique to asynchronize CPU and GPU.
    pCore->currFrameResource->currFenceValue = ++pCore->currFenceValue;
    checkHR(pCore->cmdQueue->Signal(pCore->fence.Get(), pCore->currFenceValue));
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
    //else if ((btnState & MK_MBUTTON) != 0) {
    //    float dx = 0.01f * (x - pCore->camera->lastMouseX);
    //    float dy = 0.01f * (y - pCore->camera->lastMouseY);
    //    translateCamera(-dx, dy, 0.0f, pCore->camera.get());
    //}
    else if ((btnState & MK_RBUTTON) != 0) {
        float dy = 0.02f * (float)(y - pCore->camera->lastMouseY);
        zoomCamera(-dy, pCore->camera.get());
    }
    pCore->camera->lastMouseX = x;
    pCore->camera->lastMouseY = y;
}
