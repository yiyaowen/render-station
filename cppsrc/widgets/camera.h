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
    XMFLOAT3 position = { 0.0f, 0.0f, -1.0f }; // Origin

    XMFLOAT3 right = { 1.0f, 0.0f, 0.0f }; // Positive-X
    XMFLOAT3 up = { 0.0f, 1.0f, 0.0f }; // Positive-Y
    XMFLOAT3 look = { 0.0f, 0.0f, 1.0f }; // Positive-Z

    XMFLOAT4X4 viewTrans = makeIdentityFloat4x4();

    float fovAngle = 0.25f * XM_PI; // fov: field of view.

    // Note D3D12_VIEWPORT.MinDepth and D3D12_VIEWPORT.MaxDepth
    // are NOT nearZ and farZ in projection transform matrix.
    float nearZ = 1.0f, farZ = 1000.0f;

    D3D12_VIEWPORT screenViewport = {};
    D3D12_RECT scissorRect = {};

    XMFLOAT4X4 projTrans = makeIdentityFloat4x4();

    int lastMouseX = 0, lastMouseY = 0;

    // dev variables.
    bool isViewTransDirty = true;

    float moveSpeedScale = 10.0f;
    XMFLOAT3 moveSpeed = {};

    float rotateSpeedScale = 0.002f;

    float zoomSpeedScale = 0.005f;
};
void initCamera(int viewW, int viewH, Camera* pCamera);

void resizeCameraView(int w, int h, Camera* pCamera);

void rotateCamera(float pitch, float yaw, float roll, Camera* pCamera);

void zoomCamera(float dr, Camera* pCamera);

void translateCamera(float dx, float dy, float dz, Camera* pCamera);

void updateCameraViewTrans(Camera* pCamera);

void updateCameraProjTrans(Camera* pCamera);

// Tool funcs for first person perspective.

void dev_updateCameraWalk(float deltaTime, Camera* pCamera);

void dev_walkAroundCamera(float valueAD, float valueQE, float valueWS, Camera* pCamera);

void dev_lookAroundCamera(float mouseDx, float mouseDy, Camera* pCamera);