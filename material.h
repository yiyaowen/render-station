/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <DirectXMath.h>
using namespace DirectX;
#include <string>

#ifndef NUM_FRAME_RESOURCES
#define NUM_FRAME_RESOURCES 3
#endif

struct MatConsts {
    XMFLOAT4 diffuseAlbedo = { 0.0f, 0.0f, 0.0f, 0.0f };
    // R0 is used to simulate the Fresnel effect in real world. Note however the
    // exact Fresnel equation is expensive for real-time rendering, so we use the
    // Schlick approximation method, which also needs R0 as a calculation property.
    XMFLOAT3 fresnelR0 = { 0.0f, 0.0f, 0.0f };
    // shininess = clampf(1 - roughness, 0, 1).
    float roughness = 0.0f;
    XMFLOAT4X4 matTrans = makeIdentityFloat4x4();
};

struct Material {
    std::string name;
    // We decide the material should be a frame-asynchronized data type.
    // For more information about numDirtyFrames please turn to frame-async.h.
    int numDirtyFrames = NUM_FRAME_RESOURCES;
    // Specific material properties.
    MatConsts constData;
    size_t matConstBuffIdx = -1;
    size_t texSrvHeapIdx = -1;
};