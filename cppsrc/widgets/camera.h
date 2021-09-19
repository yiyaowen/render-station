/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <d3d12.h>

#include "utils/math-utils.h"

struct Camera {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float radius = 10.0f;
    float theta = XM_PIDIV4;
    float phi = XM_PIDIV4;
    float upY = 1.0f;
    XMFLOAT4X4 viewTrans = makeIdentityFloat4x4();
    XMFLOAT4X4 projTrans = makeIdentityFloat4x4();

    D3D12_VIEWPORT screenViewport = {};
    D3D12_RECT scissorRect = {};

    int lastMouseX = 0, lastMouseY = 0;
};
void initCamera(int viewW, int viewH, Camera* pCamera);

void resizeCameraView(int w, int h, Camera* pCamera);

void rotateCamera(float dphi, float dtheta, Camera* pCamera);

void zoomCamera(float dr, Camera* pCamera);

void translateCamera(float dx, float dy, float dz, Camera* pCamera);

void updateCameraViewTrans(Camera* pCamera);

void updateCameraProjTrans(Camera* pCamera);