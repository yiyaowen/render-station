/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "geometry-utils.h"
#include "material.h"
#include "shader.h"

#ifndef NUM_FRAME_RESOURCES
#define NUM_FRAME_RESOURCES 3
#endif

struct FrameResource {
    // Every frame needs its own command allocator to push commands to the queue.
    ComPtr<ID3D12CommandAllocator> cmdAlloc = nullptr;
    // There are 2 types of buffers passed to shaders, so we need to maintain a
    // resource buffer of GPU and a related data block mapped of CPU for both of them.
    // Notice: each buffer here does NOT really represent only one data buffer. In other words,
    // multiple data buffers can be stored in one buffer here if they are laid continuously.
    // We actually rely on the constant buffer views to locate the data, which means that the
    // data itself can be stored at any reasonable position if the CBVs are set properly.
    BYTE* objConstBuffCPU = nullptr;
    ComPtr<ID3D12Resource> objConstBuffGPU = nullptr;
    BYTE* procConstBuffCPU = nullptr;
    ComPtr<ID3D12Resource> procConstBuffGPU = nullptr;
    BYTE* matConstBuffCPU = nullptr;
    ComPtr<ID3D12Resource> matConstBuffGPU = nullptr;
    UINT64 currFenceValue = 0;
};

struct RenderItem {
    // We use the dirty flag, i.e. count of dirty frames, to check whether the object data related to
    // this render item is changed. If changed, we must update the data of every frame as each of them
    // stores a copy of the object data, which means numDirtyFrames should be set to NUM_FRAME_RESOURCES.
    int numDirtyFrames = NUM_FRAME_RESOURCES;
    // Unique object property data maintained by this render item.
    std::vector<ObjConsts> constData = {};
    // To draw an object, we need 2 kinds of basic data: unique object property & vertex geometry data.
    // objConstBuffStartIdx helps to find the unique object property in the object constants buffer,
    // and mesh stores the vertex geometry data. We also need an extra topologyType to switch
    // between the solid mode and wireframe mode, but this is not necessary for the render process.
    // Note that one render item can hold more than one seat in the binding object constants buffer,
    // and objConstBuffSeatCount indicates how many seats this render item has in the constant buffer.
    UINT objConstBuffStartIdx = 0;
    UINT objConstBuffSeatCount = 0;
    std::unique_ptr<Vmesh> mesh = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY topologyType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    // We have material now. Cool! Every material is correspond to one object constants buffer seat in constData.
    std::vector<Material*> materials = {};
    // In general, the multi-objConstBuffSeat serves multi-boundLayer.
    // For example, layer "solid" can use seat No.0 and layer "shadow" can use seat No.1.
    // When specific render item is drawn, its constants data should decided by its bound layer.
    std::unordered_map<std::string, UINT> boundLayerSeatOffsetTable = {};
};