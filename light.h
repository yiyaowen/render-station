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

#define MAX_LIGHTS 8

struct Light {
    XMFLOAT3 strength = { 0.5f, 0.5f, 0.5 };
    float fallOffStart = 1.0f;
    XMFLOAT3 direction = { 0.0f, -1.0f, 0.0f };
    float fallOffEnd = 10.0f;
    XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
    float spotPower = 64.0f;
};