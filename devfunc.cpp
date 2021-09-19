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

    auto groundGeo = std::make_unique<ObjectGeometry>();
    generateGrid(80.0f, 80.0f, 1, 1, groundGeo.get());
    rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, groundGeo.get());
    translateObjectGeometry(0.0f, 0.0f, 40.0f, groundGeo.get());
    auto ground = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, groundGeo.get(), 1, ground.get());
    ground->materials[0] = pCore->materials["checkboard"].get();
    moveNamedRitemToAllRitems(pCore, "ground", std::move(ground));
    bindRitemReferenceWithLayers(pCore, "ground", { {"solid",0}, {"stencil_reflect",0} });

    auto wallGeo = std::make_unique<ObjectGeometry>();
    generateGrid(80.0f, 40.0f, 1, 1, wallGeo.get());
    translateObjectGeometry(0.0f, 20.0f, 0.0f, wallGeo.get());
    auto wall = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, wallGeo.get(), 1, wall.get());
    wall->materials[0] = pCore->materials["brick"].get();
    moveNamedRitemToAllRitems(pCore, "wall", std::move(wall));
    // The wall object should be drawn alone due to it will affect the mirror objects' rendering.
    //findRitemLayerWithName("solid", pCore->ritemLayers).push_back(pCore->ritems["wall"].get());

    auto skullGeo = std::make_unique<ObjectGeometry>();
    std::ifstream fin("models/skull.txt");
    std::string skull_ignore;
    UINT skullVerCount, skullTriCount;
    fin >> skull_ignore >> skullVerCount;
    fin >> skull_ignore >> skullTriCount;
    skullGeo->vertices.resize(skullVerCount);
    skullGeo->indices.resize(skullTriCount * 3);
    fin >> skull_ignore >> skull_ignore >> skull_ignore >> skull_ignore;
    for (UINT i = 0; i < skullVerCount; ++i) {
        Vertex& ver = skullGeo->vertices[i];
        fin >> ver.pos.x >> ver.pos.y >> ver.pos.z;
        fin >> ver.normal.x >> ver.normal.y >> ver.normal.z;
    }
    fin >> skull_ignore >> skull_ignore >> skull_ignore;
    for (UINT i = 0; i < skullTriCount; ++i) {
        fin >> skullGeo->indices[i * 3];
        fin >> skullGeo->indices[i * 3 + 1];
        fin >> skullGeo->indices[i * 3 + 2];
    }
    skullGeo->locationInfo.indexCount = skullGeo->indices.size();
    skullGeo->locationInfo.startIndexLocation = 0;
    skullGeo->locationInfo.baseVertexLocation = 0;
    rotateObjectGeometry(0.0f, 0.6f * XM_PI, 0.0f, skullGeo.get());
    scaleObjectGeometry(0.4f, 0.4f, 0.4f, skullGeo.get());
    translateObjectGeometry(0.0f, 0.0f, 4.0f, skullGeo.get());
    auto skull = std::make_unique<RenderItem>();
    // We need 2 seats in object constants buffer to provide different wolrd matrix.
    // One for the solid skull drawing, and another for the skull planar shadow drawing.
    initRitemWithGeoInfo(pCore, skullGeo.get(), 2, skull.get());
    skull->materials[0] = pCore->materials["skull"].get();
    skull->materials[1] = pCore->materials["shadow"].get();
    moveNamedRitemToAllRitems(pCore, "skull", std::move(skull));
    bindRitemReferenceWithLayers(pCore, "skull", { {"solid",0}, {"stencil_reflect",0}, {"planar_shadow",1} });

    auto mirrorGeo = std::make_unique<ObjectGeometry>();
    generateGrid(8.0f, 4.0f, 1, 1, mirrorGeo.get());
    translateObjectGeometry(0.0f, 2.2f, 0.001f, mirrorGeo.get());
    auto mirror = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, mirrorGeo.get(), 1, mirror.get());
    mirror->materials[0] = pCore->materials["mirror"].get();
    moveNamedRitemToAllRitems(pCore, "mirror", std::move(mirror));
    bindRitemReferenceWithLayers(pCore, "mirror", { {"stencil_mark",0}, {"alpha",0} });

    updateRitemRangeObjConstBuffIdx(pCore->allRitems.data(), pCore->allRitems.size());

    pCore->frameResources.clear();
    createFrameResources(pCore);
}

void dev_updateCoreObjConsts(D3DCore* pCore) {
    XMStoreFloat4x4(&pCore->ritems["ground"]->constData[0].texTrans,
        XMMatrixTranspose(XMMatrixScaling(60.0f, 60.0f, 1.0f)));

    XMStoreFloat4x4(&pCore->ritems["wall"]->constData[0].texTrans,
        XMMatrixTranspose(XMMatrixScaling(20.0f, 20.0f, 1.0f)));

    // Seat No.0 is used to draw the solid entity, and seat No.1 for planar shadow.
    XMMATRIX skullWorld = XMLoadFloat4x4(&pCore->ritems["skull"]->constData[0].worldTrans);
    XMMATRIX skullShadowMat = XMMatrixShadow(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
        -XMLoadFloat3(&pCore->processData.lights[0].direction));
    XMMATRIX skullShadowOffsetMat = XMMatrixTranslation(0.0f, 0.001f, 0.0f);
    // Pay attention to the matrix multiplation order here.
    // The right order should be 1. world 2. shadow 3. offset.
    XMStoreFloat4x4(&pCore->ritems["skull"]->constData[1].worldTrans,
        XMMatrixTranspose(skullWorld * skullShadowMat * skullShadowOffsetMat));
    XMMATRIX normalUnifyMat = XMMatrixSet(
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);
    XMStoreFloat4x4(&pCore->ritems["skull"]->constData[1].invTrWorldTrans,
        XMMatrixTranspose(normalUnifyMat));

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
    ProcConsts& constData = pCore->processData;

    // Reality objects
    XMMATRIX viewMat = XMLoadFloat4x4(&pCore->camera->viewTrans);
    XMMATRIX projMat = XMLoadFloat4x4(&pCore->camera->projTrans);
    XMStoreFloat4x4(&constData.viewTrans, XMMatrixTranspose(viewMat));
    XMStoreFloat4x4(&constData.projTrans, XMMatrixTranspose(projMat));

    XMVECTOR eyePos = sphericalToCartesianDX(
        pCore->camera->radius, pCore->camera->theta, pCore->camera->phi);
    XMStoreFloat3(&constData.eyePosW, eyePos);

    constData.ambientLight = { 0.25f, 0.25f, 0.3f, 1.0f };

    XMVECTOR lightDirection = -sphericalToCartesianDX(1.0f, XM_PIDIV4, XM_PIDIV4);
    XMStoreFloat3(&constData.lights[0].direction, lightDirection);
    constData.lights[0].strength = { 0.9f, 0.9f, 0.92f };

    constData.fogColor = XMFLOAT4(DirectX::Colors::Black);
    constData.fogFallOffStart = 12.0f;
    constData.fogFallOffEnd = 20.0f;

    updateProcConstsWithReflectMat(XMMatrixIdentity(), &constData);    

    // Apply updates.
    memcpy(pCore->currFrameResource->procConstBuffCPU, &constData, sizeof(ProcConsts));

    // Mirror objects
    // Target reflection plane: z = 0 (x-y plane).
    // Note the meaning of the vector parameter passed to XMMatrixReflect:
    // Four components correspond to the four coefficients of Ax + By + Cz + D = 0 each other.
    ProcConsts reflectConstData = constData;
    updateProcConstsWithReflectMat(XMMatrixReflect(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)), &reflectConstData);

    // Apply updates.
    memcpy(pCore->currFrameResource->procConstBuffCPU + sizeof(ProcConsts), &reflectConstData, sizeof(ProcConsts));
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

    // Object constants data should be update after process and material data is updated.
    // Under some special circumstances the updating of object data relies on process data.
    dev_updateCoreProcConsts(pCore);
    dev_updateCoreMatConsts(pCore);
    dev_updateCoreObjConsts(pCore);
}

void dev_drawCoreElems(D3DCore* pCore) {
    checkHR(pCore->currFrameResource->cmdAlloc->Reset());
    checkHR(pCore->cmdList->Reset(pCore->currFrameResource->cmdAlloc.Get(), nullptr));

    pCore->cmdList->RSSetViewports(1, &pCore->camera->screenViewport);
    pCore->cmdList->RSSetScissorRects(1, &pCore->camera->scissorRect);

    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            pCore->swapChainBuffs[pCore->currBackBuffIdx].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    pCore->cmdList->OMSetRenderTargets(1,
        &CD3DX12_CPU_DESCRIPTOR_HANDLE(
            pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            pCore->currBackBuffIdx,
            pCore->rtvDescSize), true,
        &CD3DX12_CPU_DESCRIPTOR_HANDLE(pCore->dsvHeap->GetCPUDescriptorHandleForHeapStart()));

    ID3D12DescriptorHeap* descHeaps[] = { pCore->srvDescHeap.Get() };
    pCore->cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

    pCore->cmdList->SetGraphicsRootSignature(pCore->rootSig.Get());

    auto procConstBuffAddr = pCore->currFrameResource->procConstBuffGPU->GetGPUVirtualAddress();
    pCore->cmdList->SetGraphicsRootConstantBufferView(1, procConstBuffAddr);

    clearBackBuff(Colors::Black, 1.0f, 0, pCore);
    // If the default render item layer drawing order cannot meet the requirements, you should set them manually.
    //for (auto& kv : pCore->ritemLayers) {
    //    // If the related PSO has not been initialized we simply skip drawing the render item layer.
    //    if (pCore->PSOs[kv.first] == nullptr) continue;
    //    pCore->cmdList->SetPipelineState(pCore->PSOs[kv.first].Get());
    //    drawRenderItems(pCore, kv.second.data(), kv.second.size());
    //}
    drawRitemLayerWithName(pCore, "solid");
    pCore->cmdList->OMSetStencilRef(1);
    drawRitemLayerWithName(pCore, "stencil_mark");
    pCore->cmdList->SetGraphicsRootConstantBufferView(1, procConstBuffAddr + sizeof(ProcConsts));
    drawRitemLayerWithName(pCore, "stencil_reflect");
    pCore->cmdList->OMSetStencilRef(0);
    pCore->cmdList->SetGraphicsRootConstantBufferView(1, procConstBuffAddr);
    drawRitemLayerWithName(pCore, "planar_shadow");
    drawRitemLayerWithName(pCore, "alpha");
    // Draw the wall object.
    pCore->cmdList->SetPipelineState(pCore->PSOs["solid"].Get());
    auto pWallRitem = pCore->ritems["wall"].get();
    drawRenderItems(pCore, &pWallRitem, 1, { 0 });

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
