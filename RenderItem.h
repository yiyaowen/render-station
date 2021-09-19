/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "RenderMath.h"
#include "VertexMesh.h"

// Lightweight struct stores basic information to render an object
// (Notice: this struct usually varies with different applications)
struct RenderItem {
    RenderItem() = default;

    XMFLOAT4X4 worldMatrix = MakeIdentityFloat4X4();

    int totalFrameResourcesNum;
    // Dirty flag indicating the object data has changed and we need to update the constant buffer
    // Since each FrameResource gets its own constant buffer, all of them must be updated properly
    // (Notice: when the object data is modified we should set numFrameDirty = NUM_FRAME_RESOURCES)
    int numFramesDirty;

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