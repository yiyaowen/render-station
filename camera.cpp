/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "camera.h"

void initCamera(int viewW, int viewH, Camera* pCamera) {
    resizeCameraView(viewW, viewH, pCamera);
    updateCameraViewTrans(pCamera);
}

void resizeCameraView(int w, int h, Camera* pCamera) {
    pCamera->screenViewport.TopLeftX = 0;
    pCamera->screenViewport.TopLeftY = 0;
    pCamera->screenViewport.Width = w;
    pCamera->screenViewport.Height = h;
    pCamera->screenViewport.MinDepth = 0.0f;
    pCamera->screenViewport.MaxDepth = 1.0f;
    pCamera->scissorRect.left = 0;
    pCamera->scissorRect.top = 0;
    pCamera->scissorRect.right = w;
    pCamera->scissorRect.bottom = h;
    updateCameraProjTrans(pCamera);
}

void rotateCamera(float dphi, float dtheta, Camera* pCamera) {
    pCamera->phi += dphi;
    pCamera->theta += dtheta;
    updateCameraViewTrans(pCamera);
}

void zoomCamera(float dr, Camera* pCamera) {
    pCamera->radius += dr;
    pCamera->radius = clampf(pCamera->radius, 3.0f, 15.0f);
    updateCameraViewTrans(pCamera);
}

void translateCamera(float dx, float dy, float dz, Camera* pCamera) {
    //XMMATRIX viewMat = XMLoadFloat4x4(&pCamera->viewTrans);
    //XMVECTOR deltaL = XMVectorSet(dx, dy, dz, 0.0f);
    //XMMATRIX invViewMat = XMMatrixInverse(&XMMatrixDeterminant(viewMat), viewMat);
    //XMVECTOR deltaW = XMVector2Transform(deltaL, invViewMat);
    //XMFLOAT4 delta;
    //XMStoreFloat4(&delta, deltaW);
    //pCamera->x += delta.x;
    //pCamera->y += delta.y;
    //pCamera->z += delta.z;
    //updateCameraViewTrans(pCamera);
}

void updateCameraViewTrans(Camera* pCamera) {
    // Note DirectX 3D uses the left-handed coordinate system. X-axis extends
    // from left to right, Y-axis from down to top and Z-axis from outside to inside.
    // Given the spherical coordinate system, Phi is the angle between the radial and
    // Y-axis, and Theta is the angle between the radial projection and X-axis.
    float x = pCamera->radius * sinf(pCamera->phi) * cosf(pCamera->theta);
    float y = pCamera->radius * cosf(pCamera->phi);
    float z = pCamera->radius * sinf(pCamera->phi) * sinf(pCamera->theta);
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorSet(pCamera->x, pCamera->y, pCamera->z, 1.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX viewMat = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&pCamera->viewTrans, viewMat);
}

void updateCameraProjTrans(Camera* pCamera) {
    float w = pCamera->screenViewport.Width;
    float h = pCamera->screenViewport.Height;
    XMMATRIX projMat = XMMatrixPerspectiveFovLH(0.25f * XM_PI, w / h, 1.0f, 1000.0f);
    XMStoreFloat4x4(&pCamera->projTrans, projMat);
}
