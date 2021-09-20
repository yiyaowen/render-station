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
#include <vector>

XMFLOAT4X4 makeIdentityFloat4x4();

float clampf(float value, float min, float max);

XMFLOAT3 midpoint(XMFLOAT3 a, XMFLOAT3 b);

XMVECTOR sphericalToCartesianDX(float radius, float theta, float phi);

XMVECTOR sphericalToCartesianRH(float radius, float theta, float phi);

std::vector<float> calcGaussianBlurWeight(uint8_t blurRadius, float blurGrade);

int randint(int min, int max);

float rand01();

float randfloat(float min, float max);

float randfloatEx(const std::vector<std::pair<float, float>>& ranges);
