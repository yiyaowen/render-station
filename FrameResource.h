/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <d3d12.h>
#include <DirectXPackedVector.h>
#include <memory>
#include <wrl.h>
using namespace DirectX;
using namespace Microsoft::WRL;

#include "RenderMath.h"
#include "UploadBuffer.h"

// Data that is different between objects
struct UniqueObjectConstants {
    XMFLOAT4X4 worldMatrix = MakeIdentityFloat4X4();
};

// Data that is used commonly by all objects
struct CommonObjectConstants {

    XMFLOAT4X4 viewMatrix = MakeIdentityFloat4X4();
    XMFLOAT4X4 inverseViewMatrix = MakeIdentityFloat4X4();

    XMFLOAT4X4 projectionMatrix = MakeIdentityFloat4X4();
    XMFLOAT4X4 inverseProjectionMatrix = MakeIdentityFloat4X4();

    XMFLOAT4X4 viewProjectionMatrix = MakeIdentityFloat4X4();
    XMFLOAT4X4 inverseViewProjectionMatrix = MakeIdentityFloat4X4();

    XMFLOAT3 eyeWorldPosition = { 0.0f, 0.0f, 0.0f };

    float nearZ = 0.0f;
    float farZ = 0.0f;
};

// Data that will be stored in vertex buffer
// Each vertex gets its own Vertex struct
struct Vertex {
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct FrameResource {
public:
    // commonDataCount: number of CommonObjectConstants structs in the frame resource
    // uniqueDataCount: number of UniqueObjectConstants structs in the frame resource
    FrameResource(ID3D12Device* device, UINT commonDataCount, UINT uniqueDataCount);
    FrameResource(const FrameResource& fr) = delete;
    FrameResource& operator=(const FrameResource& fr) = delete;
    ~FrameResource();

    // Every frame needs its own command allocator to push commands independently
    ComPtr<ID3D12CommandAllocator> commandAllocator;

    std::unique_ptr<UploadBuffer<CommonObjectConstants>> commonConstantBuffer = nullptr;
    std::unique_ptr<UploadBuffer<UniqueObjectConstants>> uniqueConstantBuffer = nullptr;

    // Store a fence value to judge whether the GPU has executed
    // all the commands pushed during this frame resource processed
    UINT64 fenceValue = 0;
};