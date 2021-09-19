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

XMFLOAT4X4 makeIdentityFloat4x4();

float clampf(float value, float min, float max);

XMFLOAT3 midpoint(XMFLOAT3 a, XMFLOAT3 b);

XMVECTOR sphericalToCartesianDX(float radius, float theta, float phi);

XMVECTOR sphericalToCartesianRH(float radius, float theta, float phi);