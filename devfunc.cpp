/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <DirectXColors.h>
#include <fstream>

#include "debugger.h"
#include "devfunc.h"
#include "frame-async-utils.h"
#include "render-item-utils.h"
#include "vmesh-utils.h"

void dev_initCoreElems(D3DCore* pCore) {
    // Note the origin render item collection has already included a set of axes (X-Y-Z).
    // However, the collection can still be cleared if the first 3 axes ritems are handled carefully.
    //pCore->ritems.clear();

    auto boxGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(1.0f, 1.0f, 1.0f), boxGeo.get());
    translateObjectGeometry(-4.0f, 0.0f, 0.0f, boxGeo.get());
    auto box = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, boxGeo.get(), 1, box.get());
    box->materials = { pCore->materials["fence"].get() };
    moveNamedRitemToAllRitems(pCore, "box", std::move(box));
    bindRitemReferenceWithLayers(pCore, "box", { {"alpha_test", 0} });

    auto hillGeo = std::make_unique<ObjectGeometry>();
    generateGrid(200.0f, 200.0f, 100, 100, hillGeo.get());
    disturbGridToHill(0.2f, 0.2f, hillGeo.get());
    rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, hillGeo.get());
    auto hill = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, hillGeo.get(), 1, hill.get());
    hill->materials = { pCore->materials["grass"].get() };
    moveNamedRitemToAllRitems(pCore, "hill", std::move(hill));
    bindRitemReferenceWithLayers(pCore, "hill", { {"solid", 0} });

    auto lakeGeo = std::make_unique<ObjectGeometry>();
    generateGrid(200.0f, 200.0f, 100, 100, lakeGeo.get());
    rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, lakeGeo.get());
    translateObjectGeometry(0.0f, -0.2f, 0.0f, lakeGeo.get());
    auto lake = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, lakeGeo.get(), 1, lake.get());
    lake->materials = { pCore->materials["water"].get() };
    moveNamedRitemToAllRitems(pCore, "lake", std::move(lake));
    bindRitemReferenceWithLayers(pCore, "lake", { {"alpha", 0} });

    auto treesGeo = std::make_unique<ObjectGeometry>();
    appendVerticesToObjectGeometry(
        {
            { {3.0f, 7.0f, -6.0f}, {}, {}, { 14.0f, 14.0f } },
            { {8.0f, 9.5f, -18.0f}, {}, {}, { 19.0f, 19.0f } },
            { {-6.0f, 8.8f, 7.0f}, {}, {}, { 17.6f, 17.6f } },
            { { -17.0f, 10.0f, -12.0f}, {}, {}, {20.0f, 20.0f} }
        },
        { 0, 1, 2, 3 }, treesGeo.get());
    auto trees = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, treesGeo.get(), 1, trees.get());
    // Pass the point to geometry shader and then the shader will generate 4 new points from the point passed.
    trees->topologyType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    trees->materials = { pCore->materials["trees"].get() };
    moveNamedRitemToAllRitems(pCore, "trees", std::move(trees));
    bindRitemReferenceWithLayers(pCore, "trees", { {"billboard_gs", 0} });

    updateRitemRangeObjConstBuffIdx(pCore->allRitems.data(), pCore->allRitems.size());

    pCore->frameResources.clear();
    createFrameResources(pCore);
}

void dev_updateCoreObjConsts(D3DCore* pCore) {
    XMStoreFloat4x4(&pCore->ritems["hill"]->constData[0].texTrans,
        XMMatrixTranspose(XMMatrixScaling(40.0f, 40.0f, 1.0f)));

    float timeArg = pCore->timer->elapsedSecs;
    auto lakeTexScaleMat = XMMatrixScaling(6.0f, 6.0f, 1.0f);
    auto lakeTexTransMat = XMMatrixTranslation(timeArg * 0.01f, timeArg * 0.002f, 0.0f);
    auto lakeTexMat = lakeTexTransMat * lakeTexScaleMat;
    XMStoreFloat4x4(&pCore->ritems["lake"]->constData[0].texTrans,
        XMMatrixTranspose(lakeTexMat));
    pCore->ritems["lake"]->numDirtyFrames = NUM_FRAME_RESOURCES;

    // Apply updates.
    auto currObjConstBuff = pCore->currFrameResource->objConstBuffCPU;
    for (auto& kv : pCore->ritems) {
        auto& ritem = kv.second;
        if (ritem->numDirtyFrames > 0) {
            // Every seat should be updated.
            for (UINT i = 0; i < ritem->objConstBuffSeatCount; ++i) {
                memcpy(currObjConstBuff + (ritem->objConstBuffStartIdx + i) * calcConstBuffSize(sizeof(ObjConsts)),
                    &ritem->constData[i], sizeof(ObjConsts));
            }
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

    XMVECTOR eyePos = sphericalToCartesianDX(
        pCore->camera->radius, pCore->camera->theta, pCore->camera->phi);
    XMStoreFloat3(&constData.eyePosW, eyePos);

    constData.ambientLight = { 0.6f, 0.6f, 0.65f, 1.0f };

    XMVECTOR lightDirection = -sphericalToCartesianDX(1.0f, XM_PIDIV4, XM_PIDIV4);
    XMStoreFloat3(&constData.lights[0].direction, lightDirection);
    constData.lights[0].strength = { 1.0f, 1.0f, 0.9f };

    constData.fogColor = XMFLOAT4(DirectX::Colors::SkyBlue);
    constData.fogFallOffStart = 20.0f;
    constData.fogFallOffEnd = 80.0f;

    // Apply updates.
    memcpy(pCore->currFrameResource->procConstBuffCPU, &constData, sizeof(ProcConsts));
}

void dev_updateCoreMatConsts(D3DCore* pCore) {
    // Apply updates.
    auto currMatConstBuff = pCore->currFrameResource->matConstBuffCPU;
    for (auto& mkv : pCore->materials) {
        auto m = mkv.second.get();
        if (m->numDirtyFrames > 0) {
            memcpy(currMatConstBuff + m->matConstBuffIdx * calcConstBuffSize(sizeof(MatConsts)),
                &m->constData, sizeof(MatConsts));
            m->numDirtyFrames--;
        }
    }
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
    dev_updateCoreMatConsts(pCore);
}

void dev_drawCoreElems(D3DCore* pCore) {
    checkHR(pCore->currFrameResource->cmdAlloc->Reset());
    checkHR(pCore->cmdList->Reset(pCore->currFrameResource->cmdAlloc.Get(), nullptr));

    pCore->cmdList->RSSetViewports(1, &pCore->camera->screenViewport);
    pCore->cmdList->RSSetScissorRects(1, &pCore->camera->scissorRect);

    ID3D12DescriptorHeap* descHeaps[] = { pCore->srvDescHeap.Get() };
    pCore->cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

    pCore->cmdList->SetGraphicsRootSignature(pCore->rootSig.Get());

    auto procConstBuffAddr = pCore->currFrameResource->procConstBuffGPU->GetGPUVirtualAddress();
    pCore->cmdList->SetGraphicsRootConstantBufferView(1, procConstBuffAddr);

    // Firstly do all of the rendering works with MSAA back buffer.
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->msaaBackBuff.Get(),
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    auto msaaRtvDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    msaaRtvDescHandle.Offset(2, pCore->rtvDescSize);
    auto dsvDescHanlde = pCore->dsvHeap->GetCPUDescriptorHandleForHeapStart();
    pCore->cmdList->OMSetRenderTargets(1, &msaaRtvDescHandle, TRUE, &dsvDescHanlde);

    clearBackBuff(msaaRtvDescHandle, Colors::SkyBlue, dsvDescHanlde, 1.0f, 0, pCore);
    drawRitemLayerWithName(pCore, "solid");
    drawRitemLayerWithName(pCore, "billboard_gs");
    drawRitemLayerWithName(pCore, "wireframe");
    drawRitemLayerWithName(pCore, "alpha_test");
    drawRitemLayerWithName(pCore, "alpha");

    // Secondly resolve and copy data from MSAA back buffer to swap chain buffer.
    D3D12_RESOURCE_BARRIER resolveBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->msaaBackBuff.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->swapChainBuffs[pCore->currBackBuffIdx].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RESOLVE_DEST) };

    pCore->cmdList->ResourceBarrier(2, resolveBarriers);

    pCore->cmdList->ResolveSubresource(
        pCore->swapChainBuffs[pCore->currBackBuffIdx].Get(), 0,
        pCore->msaaBackBuff.Get(), 0, pCore->swapChainBuffFormat);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->swapChainBuffs[pCore->currBackBuffIdx].Get(),
            D3D12_RESOURCE_STATE_RESOLVE_DEST,
            D3D12_RESOURCE_STATE_PRESENT));

    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);

    // Finally preset swap chain buffer.
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
    else if ((btnState & MK_MBUTTON) != 0) {
        float dx = 0.05f * (x - pCore->camera->lastMouseX);
        float dy = 0.05f * (y - pCore->camera->lastMouseY);
        translateCamera(-dx, dy, 0.0f, pCore->camera.get());
    }
    else if ((btnState & MK_RBUTTON) != 0) {
        float dy = 0.04f * (float)(y - pCore->camera->lastMouseY);
        zoomCamera(-dy, pCore->camera.get());
    }
    pCore->camera->lastMouseX = x;
    pCore->camera->lastMouseY = y;
}
