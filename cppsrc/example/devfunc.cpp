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
#include "postprocessing/blur-filter.h"
#include "utils/debugger.h"
#include "utils/frame-async-utils.h"
#include "utils/render-item-utils.h"
#include "utils/vmesh-utils.h"

struct VecLengthDemoCSData {
    XMFLOAT3 v;
};

struct VecLengthDemoCSResult {
    float l;
};

void dev_initCoreElems(D3DCore* pCore) {
     //Note the origin render item collection has already included a set of axes (X-Y-Z).
     //However, the collection can still be cleared if the first 3 axes ritems are handled carefully.
    findRitemLayerWithName("solid", pCore->ritemLayers).clear();

    //auto boxGeo = std::make_unique<ObjectGeometry>();
    //generateCube(XMFLOAT3(1.0f, 1.0f, 1.0f), boxGeo.get());
    //translateObjectGeometry(-4.0f, 0.0f, 0.0f, boxGeo.get());
    //auto box = std::make_unique<RenderItem>();
    //initRitemWithGeoInfo(pCore, boxGeo.get(), 1, box.get());
    //box->materials = { pCore->materials["fence"].get() };
    //moveNamedRitemToAllRitems(pCore, "box", std::move(box));
    //bindRitemReferenceWithLayers(pCore, "box", { {"alpha_test", 0} });

    //auto hillGeo = std::make_unique<ObjectGeometry>();
    //generateGrid(200.0f, 200.0f, 100, 100, hillGeo.get());
    //disturbGridToHill(0.2f, 0.2f, hillGeo.get());
    //rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, hillGeo.get());
    //auto hill = std::make_unique<RenderItem>();
    //initRitemWithGeoInfo(pCore, hillGeo.get(), 1, hill.get());
    //hill->materials = { pCore->materials["grass"].get() };
    //moveNamedRitemToAllRitems(pCore, "hill", std::move(hill));
    //bindRitemReferenceWithLayers(pCore, "hill", { {"solid", 0} });

    //auto lakeGeo = std::make_unique<ObjectGeometry>();
    //generateGrid(200.0f, 200.0f, 100, 100, lakeGeo.get());
    //rotateObjectGeometry(-XM_PIDIV2, 0.0f, 0.0f, lakeGeo.get());
    //translateObjectGeometry(0.0f, -0.2f, 0.0f, lakeGeo.get());
    //auto lake = std::make_unique<RenderItem>();
    //initRitemWithGeoInfo(pCore, lakeGeo.get(), 1, lake.get());
    //lake->materials = { pCore->materials["water"].get() };
    //moveNamedRitemToAllRitems(pCore, "lake", std::move(lake));
    //bindRitemReferenceWithLayers(pCore, "lake", { {"alpha", 0} });

    //auto treesGeo = std::make_unique<ObjectGeometry>();
    //appendVerticesToObjectGeometry(
    //    {
    //        { { 3.0f, 7.0f, -6.0f }, {}, {}, { 14.0f, 14.0f } },
    //        { { 8.0f, 9.5f, -18.0f }, {}, {}, { 19.0f, 19.0f } },
    //        { { -6.0f, 8.8f, 7.0f }, {}, {}, { 17.6f, 17.6f } },
    //        { { -17.0f, 10.0f, -12.0f }, {}, {}, { 20.0f, 20.0f } }
    //    },
    //    { 0, 1, 2, 3 }, treesGeo.get());
    //auto trees = std::make_unique<RenderItem>();
    //initRitemWithGeoInfo(pCore, treesGeo.get(), 1, trees.get());
    //// Pass the point to geometry shader and then the shader will generate 4 new points from the point passed.
    //trees->topologyType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    //trees->materials = { pCore->materials["trees"].get() };
    //moveNamedRitemToAllRitems(pCore, "trees", std::move(trees));
    //bindRitemReferenceWithLayers(pCore, "trees", { {"billboard", 0} });

    updateRitemRangeObjConstBuffIdx(pCore->allRitems.data(), pCore->allRitems.size());

    pCore->frameResources.clear();
    createFrameResources(pCore);

    // Init all created postprocessors.
    for (auto p = pCore->postprocessors.begin(); p != pCore->postprocessors.end(); ++p)
        if (!p->second->isPrepared()) p->second->init();

    /* GPGPU Programming: Simple Computer Shader Demo */

    // Prepare data and resource.
    std::vector<VecLengthDemoCSData> data(64);
    std::vector<VecLengthDemoCSResult> result(64);
    //std::vector<XMFLOAT3> data(64);
    //std::vector<float> result(64);

    for (int i = 0; i < 64; ++i) {
        data[i].v = { rand01(), rand01(), rand01() };
        //data[i] = { rand01(), rand01(), rand01() };
    }

    UINT64 dataByteSize = data.size() * sizeof(VecLengthDemoCSData);
    UINT64 resultByteSize = result.size() * sizeof(VecLengthDemoCSResult);
    //UINT64 dataByteSize = data.size() * sizeof(XMFLOAT3);
    //UINT64 resultByteSize = result.size() * sizeof(float);

    ComPtr<ID3DBlob> vecLengthDemoCsInputCPU = nullptr;
    ComPtr<ID3D12Resource> vecLengthDemoCsInputGPU = nullptr, vecLengthDemoCsInputUploader = nullptr;
    VecLengthDemoCSResult* downloadResult = nullptr;
    //float* downloadResult = nullptr;
    ComPtr<ID3D12Resource> vecLengthDemoCsOutputGPU, vecLengthDemoCsOutputDownloader = nullptr;

    createDefaultBuffs(pCore, data.data(), dataByteSize,
        &vecLengthDemoCsInputCPU,
        &vecLengthDemoCsInputGPU,
        &vecLengthDemoCsInputUploader);
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(resultByteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&vecLengthDemoCsOutputGPU)));
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(resultByteSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&vecLengthDemoCsOutputDownloader)));
    checkHR(vecLengthDemoCsOutputDownloader->Map(0, nullptr, reinterpret_cast<void**>(&downloadResult)));

    // Create root signature and PSO etc.
    CD3DX12_ROOT_PARAMETER vecLengthDemoCsRootParameter[2];
    vecLengthDemoCsRootParameter[0].InitAsShaderResourceView(0);
    vecLengthDemoCsRootParameter[1].InitAsUnorderedAccessView(0);
    CD3DX12_ROOT_SIGNATURE_DESC vecLengthDemoCsRootSigDesc(2, vecLengthDemoCsRootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_NONE);
    createRootSig(pCore, "vec_length_demo", &vecLengthDemoCsRootSigDesc);

    ShaderFuncEntryPoints entries = {};
    pCore->shaders["vec_length_demo"] = std::make_unique<Shader>(
        "vec_length_demo", L"shaders/test-demos/vec-length-demo-cs.hlsl", Shader::CS, entries);

    D3D12_COMPUTE_PIPELINE_STATE_DESC vecLengthDemoCsCPsoDesc = {};
    vecLengthDemoCsCPsoDesc.pRootSignature = pCore->rootSigs["vec_length_demo"].Get();
    bindShaderToCPSO(&vecLengthDemoCsCPsoDesc, pCore->shaders["vec_length_demo"].get());
    vecLengthDemoCsCPsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    pCore->device->CreateComputePipelineState(&vecLengthDemoCsCPsoDesc, IID_PPV_ARGS(&pCore->PSOs["vec_length_demo"]));

    // Execute dispatchs.
    checkHR(pCore->cmdAlloc->Reset());
    checkHR(pCore->cmdList->Reset(pCore->cmdAlloc.Get(), pCore->PSOs["vec_length_demo"].Get()));

    pCore->cmdList->SetComputeRootSignature(pCore->rootSigs["vec_length_demo"].Get());

    pCore->cmdList->SetComputeRootShaderResourceView(0, vecLengthDemoCsInputGPU->GetGPUVirtualAddress());
    pCore->cmdList->SetComputeRootUnorderedAccessView(1, vecLengthDemoCsOutputGPU->GetGPUVirtualAddress());

    pCore->cmdList->Dispatch(1, 1, 1);

    // Read back computation result.
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            vecLengthDemoCsOutputGPU.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COPY_SOURCE));
    pCore->cmdList->CopyResource(vecLengthDemoCsOutputDownloader.Get(), vecLengthDemoCsOutputGPU.Get());
    pCore->cmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            vecLengthDemoCsOutputGPU.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    checkHR(pCore->cmdList->Close());
    ID3D12CommandList* cmdLists[] = { pCore->cmdList.Get() };
    pCore->cmdQueue->ExecuteCommandLists(1, cmdLists);

    flushCmdQueue(pCore);

    // Write to local file to check result.
    std::ofstream fout("result.txt");
    for (int i = 0; i < 64; ++i) {
        fout << "length(" << data[i].v.x << ", " << data[i].v.y << ", " << data[i].v.z << ") = "
            << downloadResult[i].l << std::endl;
        //fout << "length(" << data[i].x << ", " << data[i].y << ", " << data[i].z << ") = "
        //    << downloadResult[i] << std::endl;
    }
    fout.close();

    vecLengthDemoCsOutputDownloader->Unmap(0, nullptr);
}

void dev_updateCoreObjConsts(D3DCore* pCore) {
    //XMStoreFloat4x4(&pCore->ritems["hill"]->constData[0].texTrans,
    //    XMMatrixTranspose(XMMatrixScaling(40.0f, 40.0f, 1.0f)));

    //float timeArg = pCore->timer->elapsedSecs;
    //auto lakeTexScaleMat = XMMatrixScaling(6.0f, 6.0f, 1.0f);
    //auto lakeTexTransMat = XMMatrixTranslation(timeArg * 0.01f, timeArg * 0.002f, 0.0f);
    //auto lakeTexMat = lakeTexTransMat * lakeTexScaleMat;
    //XMStoreFloat4x4(&pCore->ritems["lake"]->constData[0].texTrans,
    //    XMMatrixTranspose(lakeTexMat));
    //pCore->ritems["lake"]->numDirtyFrames = NUM_FRAME_RESOURCES;

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
    //drawRitemLayerWithName(pCore, "solid");
    //drawRitemLayerWithName(pCore, "billboard");
    //drawRitemLayerWithName(pCore, "alpha_test");
    //drawRitemLayerWithName(pCore, "alpha");

    ID3D12Resource* processedOutput = nullptr;
    processedOutput = pCore->postprocessors["basic"]->process(pCore->msaaBackBuff.Get());

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
