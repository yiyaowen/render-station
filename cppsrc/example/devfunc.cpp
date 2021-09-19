/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <DirectXColors.h>
#include <fstream>

#include "devfunc.h"
#include "modifier/wave-simulator.h"
#include "postprocessing/bilateral-blur.h"
#include "postprocessing/gaussian-blur.h"
#include "utils/debugger.h"
#include "utils/frame-async-utils.h"
#include "utils/render-item-utils.h"
#include "utils/vmesh-utils.h"

void dev_initCoreElems(D3DCore* pCore) {
     //Note the origin render item collection has already included a set of axes (X-Y-Z).
     //However, the collection can still be cleared if the first 3 axes ritems are handled carefully.
    findRitemLayerWithName("solid", pCore->ritemLayers).clear();

    auto boxGeo = std::make_unique<ObjectGeometry>();
    generateCube(XMFLOAT3(1.0f, 1.0f, 1.0f), boxGeo.get());
    translateObjectGeometry(-4.0f, 0.0f, 0.0f, boxGeo.get());
    auto box = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, boxGeo.get(), 1, box.get());
    box->materials = { pCore->materials["fence"].get() };
    moveNamedRitemToAllRitems(pCore, "box", std::move(box));
    bindRitemReferenceWithLayers(pCore, "box", { {"alpha_test", 0} });

    // Hill 1
    auto hillGeo = std::make_unique<ObjectGeometry>();
    generateGrid(100.0f, 100.0f, 100, 100, hillGeo.get());
    disturbGridToHill(0.2f, 0.2f, hillGeo.get());
    rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, hillGeo.get());
    auto hill = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, hillGeo.get(), 1, hill.get());
    hill->materials = { pCore->materials["grass"].get() };
    moveNamedRitemToAllRitems(pCore, "hill", std::move(hill));
    bindRitemReferenceWithLayers(pCore, "hill", { {"solid", 0} });
    // Hill 2
    rotateObjectGeometry(0.0f, 130.0f * XM_2PI / 360.0f, 0.0f, hillGeo.get());
    translateObjectGeometry(0.0f, -5.0f, 0.0f, hillGeo.get());
    auto hill2 = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, hillGeo.get(), 1, hill2.get());
    hill2->materials = { pCore->materials["grass"].get() };
    moveNamedRitemToAllRitems(pCore, "hill2", std::move(hill2));
    bindRitemReferenceWithLayers(pCore, "hill2", { {"solid", 0} });

    auto lakeGeo = std::make_unique<ObjectGeometry>();
    generateGrid(100.0f, 100.0f, 100, 100, lakeGeo.get());
    rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, lakeGeo.get());
    translateObjectGeometry(0.0f, -0.2f, 0.0f, lakeGeo.get());
    auto lake = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, lakeGeo.get(), 1, lake.get());
    lake->materials = { pCore->materials["water"].get() };
    // Lake ritem is marked as dynamic to simulate wave animation.
    lake->isDynamic = true;
    lake->modifiers["wave"] = std::make_unique<WaveSimulator>(pCore,
        lake->mesh.get(),
        100.0f / (2 * 100), // The distance between two adjacent vertices in the grid.
        100, // Half horizontal node count.
        100, // Half vertical node count.
        lakeGeo.get(),
        8.0f, // The spread velocity of wave.
        0.2f, // The damping grade with velocity dimension.
        0.1f, // The min disturbance height.
        0.2f, // The max disturbance height.
        0.1f); // The interval seconds between 2 random disturbance.
    //lake->modifiers["wave"]->setActived(false);
    moveNamedRitemToAllRitems(pCore, "lake", std::move(lake));
    bindRitemReferenceWithLayers(pCore, "lake", { {"alpha", 0} });

    auto treesGeo = std::make_unique<ObjectGeometry>();
    std::vector<Vertex> treeVertices;
    std::vector<UINT16> treeIndicies;
    for (int i = 0; i < 200; ++i) {
        float randX = randfloat(-40.0f, 40.0f);
        float randZ = 0.0f;
        if (-30.0f < randX && randX < 30.0f) { randZ = randfloatEx({ {-40.0f, -30.0f}, {30.0f, 40.0f} }); }
        else { randZ = randfloat(-40.0f, 40.0f); }
        treeVertices.push_back({{ randX, randfloat(3.5f, 7.5f), randZ }, {}, {}, { randfloat(15.0f, 17.0f), randfloat(16.0f, 18.0f) }});
        treeIndicies.push_back(i);
    }
    appendVerticesToObjectGeometry(treeVertices, treeIndicies, treesGeo.get());
    auto trees = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, treesGeo.get(), 1, trees.get());
    // Pass the point to geometry shader and then the shader will generate 4 new points from the point passed.
    trees->topologyType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    trees->materials = { pCore->materials["trees"].get() };
    moveNamedRitemToAllRitems(pCore, "trees", std::move(trees));
    bindRitemReferenceWithLayers(pCore, "trees", { {"billboard", 0} });

    updateRitemRangeObjConstBuffIdx(pCore->allRitems.data(), pCore->allRitems.size());

    pCore->frameResources.clear();
    createFrameResources(pCore);

    // Initialiez all postprocess effects.
    pCore->postprocessors["gaussian_blur"] = std::make_unique<GaussianBlur>(pCore, 5, 256.0f, 1);
    pCore->postprocessors["bilateral_blur"] = std::make_unique<BilateralBlur>(pCore, 5, 256.0f, 0.1f, 1);

    // Init all created postprocessors.
    for (auto p = pCore->postprocessors.begin(); p != pCore->postprocessors.end(); ++p)
        if (!p->second->isPrepared()) p->second->init();
}

void dev_updateCoreObjConsts(D3DCore* pCore) {
    float timeArg = pCore->timer->elapsedSecs;
    auto lakeTexScaleMat = XMMatrixScaling(6.0f, 6.0f, 1.0f);
    auto lakeTexTransMat = XMMatrixTranslation(timeArg * 0.005f, timeArg * 0.005f, 0.0f);
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
    constData.elapsedSecs = pCore->timer->elapsedSecs;

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
    XMStoreFloat4x4(&pCore->materials["grass"]->constData.matTrans,
        XMMatrixTranspose(XMMatrixScaling(8.0f, 8.0f, 1.0f)));

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

void dev_updateCoreDynamicMesh(D3DCore* pCore) {
    // Apply updates.
    auto& currDynamicMeshes = pCore->currFrameResource->dynamicMeshes;
    for (auto& kv : currDynamicMeshes) {
        auto& name = kv.first;
        auto& mesh = kv.second;

        // Bind current dynamic mesh to render item.
        pCore->ritems[name]->dynamicMesh = mesh.get();

        auto& modifiers = pCore->ritems[name]->modifiers;
        for (auto& mod : modifiers) {
            auto& modifier = mod.second;
            // Change target mesh to current dynamic mesh and update it.
            modifier->changeMesh(mesh.get());
            modifier->update();
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

    checkHR(pCore->currFrameResource->cmdAlloc->Reset());
    checkHR(pCore->cmdList->Reset(pCore->currFrameResource->cmdAlloc.Get(), nullptr));

    dev_updateCoreObjConsts(pCore);
    dev_updateCoreProcConsts(pCore);
    dev_updateCoreMatConsts(pCore);

    dev_updateCoreDynamicMesh(pCore); // The update will be executed with command queue.
}

void dev_drawCoreElems(D3DCore* pCore) {
    pCore->cmdList->RSSetViewports(1, &pCore->camera->screenViewport);
    pCore->cmdList->RSSetScissorRects(1, &pCore->camera->scissorRect);

    ID3D12DescriptorHeap* descHeaps[] = { pCore->srvUavHeap.Get() };
    pCore->cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

    pCore->cmdList->SetGraphicsRootSignature(pCore->rootSigs["main"].Get());

    auto procConstBuffAddr = pCore->currFrameResource->procConstBuffGPU->GetGPUVirtualAddress();
    pCore->cmdList->SetGraphicsRootConstantBufferView(1, procConstBuffAddr);

    auto msaaRtvDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    msaaRtvDescHandle.Offset(2, pCore->rtvDescSize);
    auto dsvDescHanlde = pCore->dsvHeap->GetCPUDescriptorHandleForHeapStart();
    pCore->cmdList->OMSetRenderTargets(1, &msaaRtvDescHandle, TRUE, &dsvDescHanlde);

    // Firstly draw all objects on MSAA back buffer.
    clearBackBuff(msaaRtvDescHandle, Colors::SkyBlue, dsvDescHanlde, 1.0f, 0, pCore);
    drawRitemLayerWithName(pCore, "solid");
    drawRitemLayerWithName(pCore, "billboard");
    drawRitemLayerWithName(pCore, "alpha_test");
    drawRitemLayerWithName(pCore, "alpha");

    // There is no need to draw normals of billboards.
    if (GetAsyncKeyState('1')) {
        pCore->ritems["trees"]->isVisible = false;
        drawAllRitemsFormatted(pCore, "ver_normal_visible",
            D3D_PRIMITIVE_TOPOLOGY_POINTLIST, pCore->materials["red"].get());
        pCore->ritems["trees"]->isVisible = true;
    }
    if (GetAsyncKeyState('2')) {
        pCore->ritems["trees"]->isVisible = false;
        drawAllRitemsFormatted(pCore, "tri_normal_visible",
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, pCore->materials["green"].get());
        pCore->ritems["trees"]->isVisible = true;
    }

    ID3D12Resource* processedOutput = nullptr;
    processedOutput = pCore->postprocessors["basic"]->process(pCore->msaaBackBuff.Get());
    if (GetAsyncKeyState('3')) {
        processedOutput = pCore->postprocessors["gaussian_blur"]->process(processedOutput);
    }
    if (GetAsyncKeyState('4')) {
        processedOutput = pCore->postprocessors["bilateral_blur"]->process(processedOutput);
    }

    // Write final processed output into swap chain back buffer.
    copyStatedResource(pCore,
        pCore->swapChainBuffs[pCore->currBackBuffIdx].Get(), D3D12_RESOURCE_STATE_PRESENT,
        processedOutput, D3D12_RESOURCE_STATE_COMMON);

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

void updateRenderWindowCaptionInfo(D3DCore* pCore) {
    static int framesCount = 0;
    static double elapsedSecs = 0.0;
    ++framesCount;

    HWND hWnd = pCore->hWnd;

    static double MSPF = 0.0;
    static int FPS = 0;

    Timer* pTimer = pCore->timer.get();
    // We update the window caption's timer information text per sec.
    if (pTimer->elapsedSecs - elapsedSecs >= 1.0) {
        double deltaSecs = pTimer->elapsedSecs - elapsedSecs;
        FPS = static_cast<int>((double)framesCount / deltaSecs);
        MSPF = deltaSecs / (double)framesCount * 1000.0;
        framesCount = 0;
        elapsedSecs += 1.0;
    }

    std::wstring caption = L"Render Station 渲染工坊 @ MSPF 每帧时间（毫秒）: " +
        std::to_wstring(MSPF) + L", FPS 帧率: " + std::to_wstring(FPS);

    // Vertex normal visible.
    if (GetAsyncKeyState('1')) {
        caption += L", Vertex Normal Visible 顶点法线可视化（红色）";
    }

    // Triangle normal visible.
    if (GetAsyncKeyState('2')) {
        caption += L", Triangle Normal Visible 面法线可视化（绿色）";
    }

    // Gaussian blur.
    if (GetAsyncKeyState('3')) {
        caption += L", Gaussian Blur 高斯模糊滤镜";
    }

    // Bilateral blur.
    if (GetAsyncKeyState('4')) {
        caption += L", Bilateral Blur 双边模糊滤镜";
    }

    SetWindowText(hWnd, caption.c_str());
}
