/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "math-utils.h"

XMFLOAT4X4 makeIdentityFloat4x4() {
    XMFLOAT4X4 I = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };
    return I;
}

float clampf(float value, float min, float max) {
    return value > min ? (value < max ? value : max) : min;
}

XMFLOAT3 midpoint(XMFLOAT3 a, XMFLOAT3 b) {
    return XMFLOAT3(
        (a.x + b.x) / 2,
        (a.y + b.y) / 2,
        (a.z + b.z) / 2);
}

// Left hand coordinate system in DX (Y-axis is upward).
// Phi is the angle between the radial and Y-axis,
// and Theta is the angle between the radial PROJECTION and X-axis.
XMVECTOR sphericalToCartesianDX(float radius, float theta, float phi) {
    return XMVectorSet(
        radius * sinf(phi) * cosf(theta),
        radius * cosf(phi),
        radius * sinf(phi) * sinf(theta),
        1.0f);
}

// Right hand coordinate system in common modeling practice (Z-axis is upward).
// Phi is the angle between the radial and Z-axis,
// and Theta is the angle between the radial PROJECTION and X-axis.
XMVECTOR sphericalToCartesianRH(float radius, float theta, float phi) {
    return XMVectorSet(
        radius * sinf(phi) * cosf(theta),
        radius * sinf(phi) * sinf(theta),
        radius * cosf(phi),
        1.0f);
}

std::vector<float> calcGaussianBlurWeight(uint8_t blurRadius, float blurGrade) {
    // The bigger blur grade is, the more blur the texture will be.
    float sigma = blurGrade;
    float twoSigma2 = 2.0f * sigma * sigma;
    int radius = blurRadius;

    std::vector<float> weights(2 * radius + 1);

    float weightSum = 0.0f;

    for (int i = -radius; i <= radius; ++i) {
        float x = (float)i;

        weights[i + radius] = expf(-x * x / twoSigma2);

        weightSum += weights[i + radius];
    }

    // Divide by the sum so all the weights add up to 1.0.
    for (int i = 0; i < weights.size(); ++i)
    {
        weights[i] /= weightSum;
    }

    return weights;
}
