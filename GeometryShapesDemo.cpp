/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "Application.h"
#include "ApplicationException.h"
#include "FrameResource.h"
#include "RenderMath.h"
#include "ShaderTools.h"
#include "VertexMesh.h"

static constexpr int NUM_FRAME_RESOURCES = 3;

// Lightweight struct stores basic information to render a object
// (Notice: this struct usually varies with different applications)
struct RenderItem {

    RenderItem() = default;

    XMFLOAT4X4 worldMatrix = MakeIdentityFloat4X4();

    // Dirty flag indicating the object data has changed and we need to update the constant buffer
    // Since each FrameResource gets its own constant buffer, all of them must be updated properly
    // (Notice: when the object data is modified we should set numFrameDirty = NUM_FRAME_RESOURCES)
    int numFramesDirty = NUM_FRAME_RESOURCES;

    // Index in GPU constant buffer corresponding to the UniqueConstants for this render item
    UINT uniqueConstantBufferIndex = -1;

    // Vertex data
    VertexMesh* mesh = nullptr;

    D3D12_PRIMITIVE_TOPOLOGY primitiveTopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // DrawIndexedInstanced parameters
    UINT indexCount = 0;
    UINT startIndexLocation = 0;
    int baseVertexLocation = 0;
};

class TestApp : public Application {
public:
    TestApp(HINSTANCE hInstance) : Application(hInstance) {}
    ~TestApp() {}

protected:
    void Initialize() override;
    void Update() override;
    void Draw() override;

private:
    void UpdateCamera();
    void UpdateCommonConstantBuffers();
    void UpdateUniqueConstantBuffers();

    void InitDescriptorHeaps();
    void InitConstantBufferViews();
    void InitRootSignature();
    void InitShadersAndInputLayout();
    void InitGeometryShapes();
    void InitPSOs();
    void InitFrameResources();
    void InitRenderItems();

    // Ritem: Render Item
    void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

private:
    std::vector<std::unique_ptr<FrameResource>> frameResources;
    FrameResource* currentFrameResource = nullptr;
    int currentFrameResourceIndex = 0;

    ComPtr<ID3D12RootSignature> rootSignature = nullptr;

    // CBV: Constant Buffer View
    ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;
    // SRV: Shader Resource View
    ComPtr<ID3D12DescriptorHeap> srvHeap = nullptr;

    std::unordered_map<std::string, std::unique_ptr<VertexMesh>> meshes;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> shaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

    std::vector<RenderItem*> renderItems;

    CommonObjectConstants commonObjectConstants;
    UINT commonCbvOffset = 0;

    bool isWireFrame = false;

    XMFLOAT3 eyePosition = { 0.0f, 0.0f, 0.0f };
    XMFLOAT4X4 viewMatrix = MakeIdentityFloat4X4();
    XMFLOAT4X4 projectionMatrix = MakeIdentityFloat4X4();

    float theta = 1.5f * XM_PI;
    float phi = 0.2f * XM_PI;
    float radius = 15.0f;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    try {
        TestApp app(hInstance);
        return app.Run();
    }
    catch (ApplicationException& appException) {
        MessageBox(nullptr, appException.ToString().c_str(), L"HR Error", MB_OK | MB_ICONERROR);
    }
}

void TestApp::Initialize() {
    Application::Initialize();

    ThrowIfFailed(d3dCommandList->Reset(d3dCommandAllocator.Get(), nullptr));

    // These methods are independant of each other
    InitRootSignature();
    InitShadersAndInputLayout();
    InitGeometryShapes();

    InitRenderItems();
    // The methods below should be called only after render items initialized
    InitFrameResources();
    InitDescriptorHeaps();
    // Constant buffer views need the information from frame resources
    InitConstantBufferViews();

    // Pipeline state object relies on shader resources and input layout
    InitPSOs();

    ThrowIfFailed(d3dCommandList->Close());
    ID3D12CommandList* cmdLists[] = { d3dCommandList.Get() };
    d3dCommandQueue->ExecuteCommandLists(1, cmdLists);

    FlushCommandQueue();
}

void TestApp::Update() {
    UpdateCamera();

    currentFrameResourceIndex = (currentFrameResourceIndex + 1) % NUM_FRAME_RESOURCES;
    currentFrameResource = frameResources[currentFrameResourceIndex].get();

    if (currentFrameResource->fenceValue != 0 && d3dFence->GetCompletedValue() < currentFrameResource->fenceValue) {
        HANDLE hEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        ThrowIfFailed(d3dFence->SetEventOnCompletion(currentFrameResource->fenceValue, hEvent));
        WaitForSingleObject(hEvent, INFINITE);
        CloseHandle(hEvent);
    }

    UpdateCommonConstantBuffers();
    UpdateUniqueConstantBuffers();
}

void TestApp::Draw() {
    auto commandAllocator = currentFrameResource->commandAllocator;

    ThrowIfFailed(commandAllocator->Reset());

    if (isWireFrame) {
        ThrowIfFailed(d3dCommandList->Reset(commandAllocator.Get(), PSOs["wireframe"].Get()));
    }
    else {
        ThrowIfFailed(d3dCommandList->Reset(commandAllocator.Get(), PSOs["solid"].Get()));
    }

    d3dCommandList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            swapChainBuffer[currentBackBufferIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET));

    d3dCommandList->RSSetViewports(1, &d3dScreenViewPort);
    d3dCommandList->RSSetScissorRects(1, &d3dScissorRect);

    ClearCurrentRenderTargetView(DirectX::Colors::LightSteelBlue);
    ClearCurrentDepthStencilView(1.0f, 0);

    d3dCommandList->OMSetRenderTargets(1,
        &CD3DX12_CPU_DESCRIPTOR_HANDLE(
            rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            currentBackBufferIndex,
            rtvDescriptorSize
        ), true,
        &CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvHeap->GetCPUDescriptorHandleForHeapStart()));

    ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap.Get() };
    d3dCommandList->SetDescriptorHeaps(1, descriptorHeaps);

    d3dCommandList->SetGraphicsRootSignature(rootSignature.Get());

    int commonCbvIndex = commonCbvOffset + currentFrameResourceIndex;
    auto commonCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
    commonCbvHandle.Offset(commonCbvIndex, cbvSrvUavDescriptorSize);
    d3dCommandList->SetGraphicsRootDescriptorTable(1, commonCbvHandle);

    DrawRenderItems(d3dCommandList.Get(), renderItems);

    d3dCommandList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            swapChainBuffer[currentBackBufferIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(d3dCommandList->Close());

    ID3D12CommandList* d3dCmdLists[] = { d3dCommandList.Get() };
    d3dCommandQueue->ExecuteCommandLists(1, d3dCmdLists);

    ThrowIfFailed(dxgiSwapChain->Present(0, 0));
    currentBackBufferIndex = (currentBackBufferIndex + 1) % 2;

    currentFrameResource->fenceValue = ++currentFenceValue;
    d3dCommandQueue->Signal(d3dFence.Get(), currentFenceValue);
}

void TestApp::UpdateCamera() {
    eyePosition.x = radius * sinf(phi) * cosf(theta);
    eyePosition.z = radius * sinf(phi) * sinf(theta);
    eyePosition.y = radius * cosf(phi);

    XMVECTOR eyePos = XMVectorSet(eyePosition.x, eyePosition.y, eyePosition.z, 1.0f);
    XMVECTOR focusPos = XMVectorZero();
    XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eyePos, focusPos, upDirection);
    XMStoreFloat4x4(&viewMatrix, view);

    // TODO: This should be placed in OnResize method
    XMMATRIX projection = XMMatrixPerspectiveFovLH(0.25 * DirectX::XM_PI, 800.0f / 600.0f, 1.0f, 1000.0f);
    XMStoreFloat4x4(&projectionMatrix, projection);
}

void TestApp::UpdateCommonConstantBuffers() {
    XMMATRIX view = XMLoadFloat4x4(&viewMatrix);
    XMMATRIX projection = XMLoadFloat4x4(&projectionMatrix);

    XMMATRIX viewProjection = XMMatrixMultiply(view, projection);
    XMMATRIX inverseView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX inverseProjection = XMMatrixInverse(&XMMatrixDeterminant(projection), projection);
    XMMATRIX inverseViewProjection = XMMatrixInverse(&XMMatrixDeterminant(viewProjection), viewProjection);

    XMStoreFloat4x4(&commonObjectConstants.viewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&commonObjectConstants.projectionMatrix, XMMatrixTranspose(projection));

    XMStoreFloat4x4(&commonObjectConstants.viewProjectionMatrix, XMMatrixTranspose(viewProjection));
    XMStoreFloat4x4(&commonObjectConstants.inverseViewMatrix, XMMatrixTranspose(inverseView));
    XMStoreFloat4x4(&commonObjectConstants.inverseProjectionMatrix, XMMatrixTranspose(inverseProjection));
    XMStoreFloat4x4(&commonObjectConstants.inverseViewProjectionMatrix, XMMatrixTranspose(inverseViewProjection));

    commonObjectConstants.eyeWorldPosition = eyePosition;
    commonObjectConstants.nearZ = 1.0f;
    commonObjectConstants.farZ = 1000.0f;
    
    auto currentCommonConstantBuffers = currentFrameResource->commonConstantBuffer.get();
    currentCommonConstantBuffers->UploadData(0, commonObjectConstants);
}

void TestApp::UpdateUniqueConstantBuffers() {
    auto currentUniqueConstantBuffer = currentFrameResource->uniqueConstantBuffer.get();
    for (auto& e : renderItems) {
        if (e->numFramesDirty > 0) {
            XMMATRIX worldMatrix = XMLoadFloat4x4(&e->worldMatrix);

            UniqueObjectConstants uniqueData;
            XMStoreFloat4x4(&uniqueData.worldMatrix, XMMatrixTranspose(worldMatrix));

            currentUniqueConstantBuffer->UploadData(e->uniqueConstantBufferIndex, uniqueData);

            // After done with current frame we decrease the dirty frames count
            // Next FrameResource also needs to be updated if it is dirty
            e->numFramesDirty--;
        }
    }
}

void TestApp::InitDescriptorHeaps() {
    UINT objectCount = (UINT)renderItems.size();

    // Every frame resource gets a unique struct for each object, and one extra common struct
    UINT numDescriptors = (objectCount + 1) * NUM_FRAME_RESOURCES;
    
    // We store all common struct descriptors at the end of descriptor heap
    commonCbvOffset = objectCount * NUM_FRAME_RESOURCES;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = numDescriptors;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(
        &cbvHeapDesc,
        IID_PPV_ARGS(&cbvHeap)));
}

void TestApp::InitConstantBufferViews() {
    UINT uniqueConstantBufferByteSize = CalculateConstantBufferByteSize(sizeof(UniqueObjectConstants));
    UINT objectCount = (UINT)renderItems.size();

    for (int i = 0; i < NUM_FRAME_RESOURCES; ++i) {
        auto uniqueConstantBuffer = frameResources[i]->uniqueConstantBuffer->Resource();
        for (UINT j = 0; j < objectCount; ++j) {
            D3D12_GPU_VIRTUAL_ADDRESS uniqueDataAddressInGPU = uniqueConstantBuffer->GetGPUVirtualAddress();

            // Offset to the unique constant buffer in GPU
            uniqueDataAddressInGPU += j * uniqueConstantBufferByteSize;
            // Offset to the object descriptor in descriptor heap
            int cbvIndexInHeap = i * objectCount + j;
            auto cbvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
            cbvHandle.Offset(cbvIndexInHeap, cbvSrvUavDescriptorSize);

            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = uniqueDataAddressInGPU;
            cbvDesc.SizeInBytes = uniqueConstantBufferByteSize;

            d3dDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
        }
    }

    UINT commonConstantBufferSize = CalculateConstantBufferByteSize(sizeof(CommonObjectConstants));

    for (int i = 0; i < NUM_FRAME_RESOURCES; ++i) {
        auto commonConstantBuffer = frameResources[i]->commonConstantBuffer->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS commonDataAddressInGPU = commonConstantBuffer->GetGPUVirtualAddress();

        // Offset to the common data descriptor in descriptor heap
        int cbvIndexInHeap = commonCbvOffset + i;
        auto cbvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
        cbvHandle.Offset(cbvIndexInHeap, cbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = commonDataAddressInGPU;
        cbvDesc.SizeInBytes = commonConstantBufferSize;

        d3dDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
    }
}

void TestApp::InitRootSignature() {
    CD3DX12_DESCRIPTOR_RANGE cbvTable0, cbvTable1;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

    CD3DX12_ROOT_PARAMETER rootParameter[2];

    rootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    rootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(2, rootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> serializedRootSignature = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSignature, &errorBlob));

    ThrowIfFailed(d3dDevice->CreateRootSignature(
        0,
        serializedRootSignature->GetBufferPointer(),
        serializedRootSignature->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

void TestApp::InitShadersAndInputLayout() {
    shaders["VS"] = ShaderTools::CompileShaderSource(L"TestDemos/Shaders/GeometryShapesDemo.hlsl", nullptr, "VS", "vs_5_1");
    shaders["PS"] = ShaderTools::CompileShaderSource(L"TestDemos/Shaders/GeometryShapesDemo.hlsl", nullptr, "PS", "ps_5_1");

    inputLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void TestApp::InitGeometryShapes() {

    // TODO

    Vertex vertices[8] = {
        Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)}),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),
        Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)}),
        Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)}),
        Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)}),
        Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)}) };

    UINT16 indices[36] = {
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
        4, 3, 7 };

    auto cubeMesh = std::make_unique<VertexMesh>();
    cubeMesh->name = "cube";

    ThrowIfFailed(D3DCreateBlob(sizeof(vertices), &cubeMesh->vertexBufferInCPU));
    memcpy(cubeMesh->vertexBufferInCPU->GetBufferPointer(), vertices, sizeof(vertices));

    ThrowIfFailed(D3DCreateBlob(sizeof(indices), &cubeMesh->indexBufferInCPU));
    memcpy(cubeMesh->indexBufferInCPU->GetBufferPointer(), indices, sizeof(indices));

    cubeMesh->vertexBufferInGPU = MakeInitializedDefaultBuffer(d3dDevice.Get(), d3dCommandList.Get(),
        vertices, sizeof(vertices), cubeMesh->vertexBufferUploader);

    cubeMesh->indexBufferInGPU = MakeInitializedDefaultBuffer(d3dDevice.Get(), d3dCommandList.Get(),
        indices, sizeof(indices), cubeMesh->indexBufferUploader);

    cubeMesh->vertexBufferByteStride = sizeof(Vertex);
    cubeMesh->vertexBufferByteSize = sizeof(vertices);
    cubeMesh->indexFormat = DXGI_FORMAT_R16_UINT;
    cubeMesh->indexBufferByteSize = sizeof(indices);

    SubVertexMesh cubeSubMesh;
    cubeSubMesh.indexCount = 36;
    cubeSubMesh.startIndexLocation = 0;
    cubeSubMesh.baseVertexLocation = 0;
    cubeMesh->subMeshes["cube"] = cubeSubMesh;

    meshes[cubeMesh->name] = std::move(cubeMesh);
}

void TestApp::InitPSOs() {

    D3D12_GRAPHICS_PIPELINE_STATE_DESC solidPSODesc = {};

    solidPSODesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
    solidPSODesc.pRootSignature = rootSignature.Get();
    solidPSODesc.VS = {
        (BYTE*)(shaders["VS"]->GetBufferPointer()),
        shaders["VS"]->GetBufferSize() };
    solidPSODesc.PS = {
        (BYTE*)(shaders["PS"]->GetBufferPointer()),
        shaders["PS"]->GetBufferSize() };
    solidPSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    solidPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    solidPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    solidPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    solidPSODesc.SampleMask = UINT_MAX;
    solidPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    solidPSODesc.NumRenderTargets = 1;
    solidPSODesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    solidPSODesc.SampleDesc.Count = 1;
    solidPSODesc.SampleDesc.Quality = 0;
    solidPSODesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&solidPSODesc, IID_PPV_ARGS(&PSOs["solid"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC wireframePSODesc = solidPSODesc;
    wireframePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&wireframePSODesc, IID_PPV_ARGS(&PSOs["wireframe"])));
}

void TestApp::InitFrameResources() {
    for (int i = 0; i < NUM_FRAME_RESOURCES; ++i) {
        frameResources.push_back(std::make_unique<FrameResource>(d3dDevice.Get(), 1, (UINT)renderItems.size()));
    }
}

void TestApp::InitRenderItems() {
    auto cubeItem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&cubeItem->worldMatrix, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    cubeItem->uniqueConstantBufferIndex = 0;
    cubeItem->mesh = meshes["cube"].get();
    cubeItem->primitiveTopologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    cubeItem->indexCount = cubeItem->mesh->subMeshes["cube"].indexCount;
    cubeItem->startIndexLocation = cubeItem->mesh->subMeshes["cube"].startIndexLocation;
    cubeItem->baseVertexLocation = cubeItem->mesh->subMeshes["cube"].baseVertexLocation;
    renderItems.push_back(cubeItem.release());
}

void TestApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems) {
    UINT uniqueDataSize = CalculateConstantBufferByteSize(sizeof(UniqueObjectConstants));

    auto uniqueDataBuffer = currentFrameResource->uniqueConstantBuffer->Resource();

    for (size_t i = 0; i < renderItems.size(); ++i) {
        auto ritem = renderItems[i];

        cmdList->IASetVertexBuffers(0, 1, &ritem->mesh->VertexBufferView());
        cmdList->IASetIndexBuffer(&ritem->mesh->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ritem->primitiveTopologyType);

        UINT cbvIndex = currentFrameResourceIndex * (UINT)renderItems.size() + ritem->uniqueConstantBufferIndex;
        auto cbvHanlde = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
        cbvHanlde.Offset(cbvIndex, cbvSrvUavDescriptorSize);

        cmdList->SetGraphicsRootDescriptorTable(0, cbvHanlde);

        cmdList->DrawIndexedInstanced(ritem->indexCount, 1, ritem->startIndexLocation, ritem->baseVertexLocation, 0);
    }
}