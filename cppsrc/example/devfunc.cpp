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

static void loadSkullModel(D3DCore* pCore);

void dev_initCoreElems(D3DCore* pCore) {
     //Note the origin render item collection has already included a set of axes (X-Y-Z).
     //However, the collection can still be cleared if the first 3 axes ritems are handled carefully.
    findRitemLayerWithName("solid", pCore->ritemLayers).clear();

    translateCamera(0.0f, 12.0f, -20.0f, pCore->camera.get());
    rotateCamera(20.0f * XM_PI / 180.0f, 0.0f, 0.0f, pCore->camera.get());
    pCore->camera->isViewTransDirty = true;

    /* Build scene object start */

    createCubeObject(pCore, "floor", XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(20.0f, 1.0f, 20.0f), "tile", { {"solid", 0}, {"wireframe", 0} });
    
    createCubeObject(pCore, "stage", XMFLOAT3(0.0f, 3.0f, 0.0f), XMFLOAT3(2.0f, 2.0f, 2.0f), "brick", { {"solid", 0}, {"wireframe", 0} });

    loadSkullModel(pCore);

    int k = 0;
    for (int i = 0; i < 3; i += 2) {
        for (int j = 0; j < 4; ++j) {
            createCylinderObject(pCore, "pillar" + std::to_string(k),
                /* pos */ XMFLOAT3(10.0f * (i - 1), 4.0f, (j / 3.0f) * -16.0f + (1.0f - j / 3.0f) * 16.0f),
                /* topR */ 0.8f, /* bottomR */ 1.2f, /* h */ 6.0f, /* sliceCount */ 30, /* stackCount */ 10,
                "brick", { {"solid", 0}, {"wireframe", 0} });
            createSphereObject(pCore, "ball" + std::to_string(k++),
                /* pos */ XMFLOAT3(10.0f * (i - 1), 8.0f, (j / 3.0f) * -16.0f + (1.0f - j / 3.0f) * 16.0f),
                /* r */ 1.0f, /* subdivisionCount */ 3, "glass", { {"alpha", 0}, {"wireframe", 0} });
        }
    }

    //createSphereObject(pCore, "test", XMFLOAT3(-0.7f, 7.0f, -0.4f), 0.1f, 2, "red", { {"solid", 0}, {"wireframe", 0} });

    /* Build scene object end */

    updateRitemRangeObjConstBuffIdx(pCore->allRitems.data(), pCore->allRitems.size());
    updateRitemRangeMaterialDataIdx(pCore->allRitems.data(), pCore->allRitems.size());

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
    // There is no need to set dirty flag for initialization purpose.
    XMStoreFloat4x4(&pCore->ritems["floor"]->constData[0].texTrans, XMMatrixScaling(5.0f, 5.0f, 1.0f));

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

    constData.eyePosW = pCore->camera->position;
    constData.elapsedSecs = (float)pCore->timer->elapsedSecs;

    constData.ambientLight = { 0.6f, 0.6f, 0.65f, 1.0f };

    /* Build scene light start */

    XMVECTOR lightDirection = -sphericalToCartesianDX(1.0f, XM_PIDIV4, XM_PIDIV4);
    XMStoreFloat3(&constData.lights[0].direction, lightDirection);
    constData.lights[0].strength = { 1.0f, 1.0f, 0.9f };

    float st = (float)abs(sin(pCore->timer->elapsedSecs));
    // Left eye of skull
    constData.lights[1].position = { 0.7f, 7.0f, -0.4f };
    constData.lights[1].strength = { 1.0f * st, 0.1f * st, 0.2f * st };
    constData.lights[1].fallOffStart = 1.0f;
    constData.lights[1].fallOffEnd = 2.0f;
    // Right eye of skull
    constData.lights[2].position = { -0.7f, 7.0f, -0.4f };
    constData.lights[2].strength = { 1.0f * st, 0.1f * st, 0.2f * st };
    constData.lights[2].fallOffStart = 1.0f;
    constData.lights[2].fallOffEnd = 2.0f;

    /* Build scene light end */

    constData.fogColor = XMFLOAT4(DirectX::Colors::Black);
    constData.fogFallOffStart = 20.0f;
    constData.fogFallOffEnd = 80.0f;

    // Apply updates.
    memcpy(pCore->currFrameResource->procConstBuffCPU, &constData, sizeof(ProcConsts));
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
}

void dev_drawCoreElems(D3DCore* pCore) {
    pCore->cmdList->RSSetViewports(1, &pCore->camera->screenViewport);
    pCore->cmdList->RSSetScissorRects(1, &pCore->camera->scissorRect);

    ID3D12DescriptorHeap* descHeaps[] = { pCore->srvUavHeap.Get() };
    pCore->cmdList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);

    pCore->cmdList->SetGraphicsRootSignature(pCore->rootSigs["main"].Get());

    // Global process data
    auto procConstBuffAddr = pCore->currFrameResource->procConstBuffGPU->GetGPUVirtualAddress();
    pCore->cmdList->SetGraphicsRootConstantBufferView(1, procConstBuffAddr);

    // Scene material infos
    auto materialStructBuffAddr = pCore->currFrameResource->matStructBuffGPU->GetGPUVirtualAddress();
    pCore->cmdList->SetGraphicsRootShaderResourceView(2, materialStructBuffAddr);

    // Actual diffuse textures
    pCore->cmdList->SetGraphicsRootDescriptorTable(3, pCore->srvUavHeap->GetGPUDescriptorHandleForHeapStart());

    auto msaaRtvDescHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pCore->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    msaaRtvDescHandle.Offset(2, pCore->rtvDescSize);
    auto dsvDescHanlde = pCore->dsvHeap->GetCPUDescriptorHandleForHeapStart();
    pCore->cmdList->OMSetRenderTargets(1, &msaaRtvDescHandle, TRUE, &dsvDescHanlde);

    // Firstly draw all objects on MSAA back buffer.
    clearBackBuff(msaaRtvDescHandle, Colors::Black, dsvDescHanlde, 1.0f, 0, pCore);
    // Switch between solid mode and wireframe mode.
    if (GetAsyncKeyState('1') & 0x8000) {
        drawRitemLayerWithName(pCore, "wireframe");
    }
    else {
        drawRitemLayerWithName(pCore, "solid");
        drawRitemLayerWithName(pCore, "alpha");
    }

    // Post Processing.
    ID3D12Resource* processedOutput = nullptr;
    processedOutput = pCore->postprocessors["basic"]->process(pCore->msaaBackBuff.Get());

    if (GetAsyncKeyState('2') & 0x8000) {
        processedOutput = pCore->postprocessors["gaussian_blur"]->process(processedOutput);
    }
    if (GetAsyncKeyState('3') & 0x8000) {
        processedOutput = pCore->postprocessors["bilateral_blur"]->process(processedOutput);
    }

    if (GetAsyncKeyState('4') & 0x8000) {
        processedOutput = pCore->postprocessors["sobel_operator"]->process(processedOutput);
    }
    else if (GetAsyncKeyState('5') & 0x8000) {
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

void dev_onKeyDown(WPARAM keyCode, D3DCore* pCore) {
    // Reserved
}

void dev_onKeyUp(WPARAM keyCode, D3DCore* pCore) {
    // Reserved
}

void dev_onMouseDown(WPARAM btnState, int x, int y, D3DCore* pCore) {
    // Reserved
}

void dev_onMouseUp(WPARAM btnState, int x, int y, D3DCore* pCore) {
    // Reserved
}

void dev_onMouseMove(WPARAM btnState, int x, int y, D3DCore* pCore) {
    Camera* camera = pCore->camera.get();
    float lastMouseX = (float)camera->lastMouseX;
    float lastMouseY = (float)camera->lastMouseY;

    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        pCore->camera->isViewTransDirty = true;
        dev_lookAroundCamera(x - lastMouseX, y - lastMouseY, camera);
    }

    camera->lastMouseX = x;
    camera->lastMouseY = y;
}

void dev_onMouseScroll(WPARAM scrollInfo, D3DCore* pCore) {
    pCore->camera->isViewTransDirty = true;
    zoomCamera(GET_WHEEL_DELTA_WPARAM(scrollInfo), pCore->camera.get());
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

    std::wstring caption = L"Render Station äÖÈ¾¹¤·» @ MSPF Ã¿Ö¡Ê±¼ä£¨ºÁÃë£©: " +
        std::to_wstring(MSPF) + L", FPS Ö¡ÂÊ: " + std::to_wstring(FPS);

    //caption += L", Camera Position Ïà»úÎ»ÖÃ£º(" +
    //    std::to_wstring(pCore->camera->position.x) + L", " +
    //    std::to_wstring(pCore->camera->position.y) + L", " +
    //    std::to_wstring(pCore->camera->position.z) + L")";

    if (GetAsyncKeyState('1') & 0x8000) {
        caption += L", Wireframe Mode Ïß¿òÄ£Ê½";
    }

    // Gaussian blur.
    if (GetAsyncKeyState('2') & 0x8000) {
        caption += L", Gaussian Blur ¸ßË¹Ä£ºýÂË¾µ";
    }

    // Bilateral blur.
    if (GetAsyncKeyState('3') & 0x8000) {
        caption += L", Bilateral Blur Ë«±ßÄ£ºýÂË¾µ";
    }

    // Sobel operator.
    if (GetAsyncKeyState('4') & 0x8000) {
        caption += L", Sobel Operator Ë÷±´¶ûÂÖÀªËã×Ó";
        if (GetAsyncKeyState(VK_SPACE)) {
            caption += L"£¨°×É«»­±Ê£©";
        }
        else {
            caption += L"£¨ºÚÉ«»­±Ê£©";
        }
    }
    else if (GetAsyncKeyState('5') & 0x8000) {
        caption += L", Outline Overlay Ãè±ß·ç¸ñÂË¾µ";
    }

    SetWindowText(hWnd, caption.c_str());
}

void createCubeObject(
    D3DCore* pCore,
    const std::string& name,
    XMFLOAT3 pos, XMFLOAT3 xyz,
    const std::string& materialName,
    const std::unordered_map<std::string, UINT>& layerInfos)
{
    auto geo = std::make_unique<ObjectGeometry>();
    generateCube(xyz, geo.get());
    translateObjectGeometry(pos.x, pos.y, pos.z, geo.get());
    auto obj = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, geo.get(), 1, obj.get());
    obj->materials = { pCore->materials[materialName].get() };
    moveNamedRitemToAllRitems(pCore, name, std::move(obj));
    bindRitemReferenceWithLayers(pCore, name, layerInfos);
}

void createCylinderObject(
    D3DCore* pCore,
    const std::string& name,
    XMFLOAT3 pos,
    float topR, float bottomR, float h,
    UINT sliceCount, UINT stackCount,
    const std::string& materialName,
    const std::unordered_map<std::string, UINT>& layerInfos)
{
    auto geo = std::make_unique<ObjectGeometry>();
    generateCylinder(topR, bottomR, h, sliceCount, stackCount, geo.get());
    rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, geo.get());
    translateObjectGeometry(pos.x, pos.y, pos.z, geo.get());
    auto obj = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, geo.get(), 1, obj.get());
    obj->materials = { pCore->materials[materialName].get() };
    moveNamedRitemToAllRitems(pCore, name, std::move(obj));
    bindRitemReferenceWithLayers(pCore, name, layerInfos);
}

void createSphereObject(
    D3DCore* pCore,
    const std::string& name,
    XMFLOAT3 pos, float r,
    UINT subdivisionCount,
    const std::string& materialName,
    const std::unordered_map<std::string, UINT>& layerInfos)
{
    auto geo = std::make_unique<ObjectGeometry>();
    generateGeoSphere(r, subdivisionCount, geo.get());
    translateObjectGeometry(pos.x, pos.y, pos.z, geo.get());
    auto obj = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, geo.get(), 1, obj.get());
    obj->materials = { pCore->materials[materialName].get() };
    moveNamedRitemToAllRitems(pCore, name, std::move(obj));
    bindRitemReferenceWithLayers(pCore, name, layerInfos);
}

void loadSkullModel(D3DCore* pCore) {
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
    skullGeo->locationInfo.indexCount = (UINT)skullGeo->indices.size();
    skullGeo->locationInfo.startIndexLocation = 0;
    skullGeo->locationInfo.baseVertexLocation = 0;
    scaleObjectGeometry(0.5f, 0.5f, 0.5f, skullGeo.get());
    translateObjectGeometry(0.0f, 5.0f, 0.0f, skullGeo.get());
    auto skull = std::make_unique<RenderItem>();
    initRitemWithGeoInfo(pCore, skullGeo.get(), 1, skull.get());
    skull->materials = { pCore->materials["skull"].get() };
    moveNamedRitemToAllRitems(pCore, "skull", std::move(skull));
    bindRitemReferenceWithLayers(pCore, "skull", { {"solid", 0}, {"wireframe", 0} });
}