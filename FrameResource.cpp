/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT commonDataCount, UINT uniqueDataCount) {
    ThrowIfFailed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator)));

    commonConstantBuffer = std::make_unique<UploadBuffer<CommonObjectConstants>>(device, commonDataCount, true);
    uniqueConstantBuffer = std::make_unique<UploadBuffer<UniqueObjectConstants>>(device, uniqueDataCount, true);
}

FrameResource::~FrameResource() {}