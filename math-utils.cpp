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
