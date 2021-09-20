/*
** Render Station @ https://github.com/yiyaowen/render-station
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
#include "postprocessing/color-compositor.h"
#include "postprocessing/gaussian-blur.h"
#include "postprocessing/sobel-operator.h"
#include "utils/debugger.h"
#include "utils/frame-async-utils.h"
#include "utils/render-item-utils.h"
#include "utils/vmesh-utils.h"

void dev_initCoreElems(D3DCore* pCore) {
     //Note the origin render item collection has already included a set of axes (X-Y-Z).
     //However, the collection can still be cleared if the first 3 axes ritems are handled carefully.
    findRitemLayerWithName("solid", pCore->ritemLayers).clear();

    auto hillGeo = std::make_unique<ObjectGeometry>();
    appendVerticesToObjectGeometry({
        { {-10.0f, 0.0f, +10.0f } },
        { {+10.0f, 0.0f, +10.0f } },
        { {-10.0f, 0.0f, -10.0f } },
        { {+10.0f, 0.0f, -10.0f } } },
        { 0, 1, 2, 3 },
        hillGeo.get());
    auto hill = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, hillGeo.get(), 1, hill.get());
    hill->topologyType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
    hill->materials = { pCore->materials["skull"].get() };
    moveNamedRitemToAllRitems(pCore, "hill", std::move(hill));
    bindRitemReferenceWithLayers(pCore, "hill", { {"hill_tessellation", 0}, {"hill_tessellation_wireframe", 0} });

    updateRitemRangeObjConstBuffIdx(pCore->allRitems.data(), pCore->allRitems.size());

    pCore->frameResources.clear();
    createFrameResources(pCore);

    // Initialiez all postprocess effects.
    pCore->postprocessors["gaussian_blur"] = std::make_unique<GaussianBlur>(pCore, 5, 256.0f, 1);
    pCore->postprocessors["bilateral_blur"] = std::make_unique<BilateralBlur>(pCore, 5, 256.0f, 0.1f, 1);
    pCore->postprocessors["sobel_operator"] = std::make_unique<SobelOperator>(pCore);
    pCore->postprocessors["color_compositor"] = std::make_unique<ColorCompositor>(pCore);

    // Init all created postprocessors.
    for (auto p = pCore->postprocessors.begin(); p != pCore->postprocessors.end(); ++p)
        if (!p->second->isPrepared()) p->second->init();
}

void dev_updateCoreObjConsts(D3DCore* pCore) {
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
    constData.fogFallOffStart = 200.0f;
    constData.fogFallOffEnd = 800.0f;

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

    dev_updateCoreDynamicMesh(pCore); // The update will be executed with command queue.

    dev_updateCoreObjConsts(pCore);
    dev_updateCoreProcConsts(pCore);
    dev_updateCoreMatConsts(pCore);
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
    // Switch between solid mode and wireframe mode.
    if (GetAsyncKeyState('1')) {
        drawRitemLayerWithName(pCore, "wireframe");
        drawRitemLayerWithName(pCore, "hill_tessellation_wireframe");
    }
    else {
        drawRitemLayerWithName(pCore, "solid");
        drawRitemLayerWithName(pCore, "hill_tessellation");
        drawRitemLayerWithName(pCore, "billboard");
        drawRitemLayerWithName(pCore, "alpha_test");
        if (GetAsyncKeyState('0')) {
            drawRitemLayerWithName(pCore, "alpha_cartoon");
        }
        else {
            drawRitemLayerWithName(pCore, "alpha");
        }
    }

    // There is no need to draw normals of billboards.
    if (GetAsyncKeyState('2')) {
        drawAllRitemsFormatted(pCore, "ver_normal_visible",
            D3D_PRIMITIVE_TOPOLOGY_POINTLIST, pCore->materials["red"].get());
    }
    if (GetAsyncKeyState('3')) {
        drawAllRitemsFormatted(pCore, "tri_normal_visible",
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST, pCore->materials["green"].get());
    }

    // Post Processing.
    ID3D12Resource* processedOutput = nullptr;
    processedOutput = pCore->postprocessors["basic"]->process(pCore->msaaBackBuff.Get());

    if (GetAsyncKeyState('4')) {
        processedOutput = pCore->postprocessors["gaussian_blur"]->process(processedOutput);
    }
    if (GetAsyncKeyState('5')) {
        processedOutput = pCore->postprocessors["bilateral_blur"]->process(processedOutput);
    }

    if (GetAsyncKeyState('6')) {
        processedOutput = pCore->postprocessors["sobel_operator"]->process(processedOutput);
    }
    else if (GetAsyncKeyState('7')) {
        auto sobelOutput = pCore->postprocessors["sobel_operator"]->process(processedOutput);
        ((ColorCompositor*)pCore->postprocessors["color_compositor"].get())
            ->bindBackgroundPlate(sobelOutput, ColorCompositor::MULTIPLY, 1.0f);
        processedOutput = pCore->postprocessors["color_compositor"]->process(processedOutput);
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

    if (GetAsyncKeyState('1')) {
        caption += L", Wireframe Mode 线框模式";
    }

    // Vertex normal visible.
    if (GetAsyncKeyState('2')) {
        caption += L", Vertex Normal Visible 顶点法线可视化（红色）";
    }

    // Triangle normal visible.
    if (GetAsyncKeyState('3')) {
        caption += L", Triangle Normal Visible 面法线可视化（绿色）";
    }

    // Gaussian blur.
    if (GetAsyncKeyState('4')) {
        caption += L", Gaussian Blur 高斯模糊滤镜";
    }

    // Bilateral blur.
    if (GetAsyncKeyState('5')) {
        caption += L", Bilateral Blur 双边模糊滤镜";
    }

    // Sobel operator.
    if (GetAsyncKeyState('6')) {
        caption += L", Sobel Operator 索贝尔轮廓算子";
        if (GetAsyncKeyState(VK_SPACE)) {
            caption += L"（白色画笔）";
        }
        else {
            caption += L"（黑色画笔）";
        }
    }
    else if (GetAsyncKeyState('7')) {
        caption += L", Outline Overlay 描边风格滤镜";
    }

    if (GetAsyncKeyState('0')) {
        caption += L", Cartoon Water 卡通水面";
    }

    SetWindowText(hWnd, caption.c_str());
}
