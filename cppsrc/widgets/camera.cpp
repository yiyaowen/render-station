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
    pCamera->screenViewport.Width = (FLOAT)w;
    pCamera->screenViewport.Height = (FLOAT)h;
    pCamera->screenViewport.MinDepth = 0.0f;
    pCamera->screenViewport.MaxDepth = 1.0f;

    pCamera->scissorRect.left = 0;
    pCamera->scissorRect.top = 0;
    pCamera->scissorRect.right = w;
    pCamera->scissorRect.bottom = h;

    updateCameraProjTrans(pCamera);
}

void rotateCamera(float pitch, float yaw, float roll, Camera* pCamera) {
    XMMATRIX pitchMat = XMMatrixRotationAxis(XMLoadFloat3(&pCamera->right), pitch);
    XMMATRIX yawMat = XMMatrixRotationAxis(XMLoadFloat3(&pCamera->up), yaw);
    XMMATRIX rollMat = XMMatrixRotationAxis(XMLoadFloat3(&pCamera->look), roll);

    XMMATRIX rotationMat = pitchMat * yawMat * rollMat;
    XMStoreFloat3(&pCamera->right, XMVector3Transform(XMLoadFloat3(&pCamera->right), rotationMat));
    XMStoreFloat3(&pCamera->up, XMVector3Transform(XMLoadFloat3(&pCamera->up), rotationMat));
    XMStoreFloat3(&pCamera->look, XMVector3Transform(XMLoadFloat3(&pCamera->look), rotationMat));

    updateCameraViewTrans(pCamera);
}

void zoomCamera(float dr, Camera* pCamera) {
    float zoomSpeedScale = pCamera->zoomSpeedScale;
    translateCamera(
        pCamera->look.x * dr * zoomSpeedScale,
        pCamera->look.y * dr * zoomSpeedScale,
        pCamera->look.z * dr * zoomSpeedScale,
        pCamera);
}

void translateCamera(float dx, float dy, float dz, Camera* pCamera) {
    XMVECTOR position = XMLoadFloat3(&pCamera->position);
    position += XMVectorSet(dx, dy, dz, 0.0f);
    XMStoreFloat3(&pCamera->position, position);

    updateCameraViewTrans(pCamera);
}

void updateCameraViewTrans(Camera* pCamera) {
    if (!pCamera->isViewTransDirty) return;
    pCamera->isViewTransDirty = false;

    // Note DirectX 3D uses the left-handed coordinate system. X-axis extends
    // from left to right, Y-axis from down to top and Z-axis from outside to inside.
    XMVECTOR P = XMLoadFloat3(&pCamera->position);

    XMVECTOR R = XMLoadFloat3(&pCamera->right);
    XMVECTOR U = XMLoadFloat3(&pCamera->up);
    XMVECTOR L = XMLoadFloat3(&pCamera->look);

    // Follow the left-hand spiral rule in the left-handed coordinate system.
    //L = XMVector3Normalize(L);
    //U = XMVector3Normalize(XMVector3Cross(L, R));
    //R = XMVector3Cross(U, L);
    // 
    //U = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    //L = XMVector3Normalize(XMVector3Cross(R, U));
    //R = XMVector3Cross(U, L);

    R = XMVector3Normalize(XMVectorSetY(R, 0.0f));
    L = XMVector3Normalize(XMVector3Cross(R, U));
    U = XMVector3Cross(L, R);

    // Fill the new view matrix.
    float x = -XMVectorGetX(XMVector3Dot(P, R));
    float y = -XMVectorGetX(XMVector3Dot(P, U));
    float z = -XMVectorGetX(XMVector3Dot(P, L));

    XMStoreFloat3(&pCamera->right, R);
    XMStoreFloat3(&pCamera->up, U);
    XMStoreFloat3(&pCamera->look, L);

    pCamera->viewTrans(0, 0) = pCamera->right.x;
    pCamera->viewTrans(1, 0) = pCamera->right.y;
    pCamera->viewTrans(2, 0) = pCamera->right.z;
    pCamera->viewTrans(3, 0) = x;

    pCamera->viewTrans(0, 1) = pCamera->up.x;
    pCamera->viewTrans(1, 1) = pCamera->up.y;
    pCamera->viewTrans(2, 1) = pCamera->up.z;
    pCamera->viewTrans(3, 1) = y;

    pCamera->viewTrans(0, 2) = pCamera->look.x;
    pCamera->viewTrans(1, 2) = pCamera->look.y;
    pCamera->viewTrans(2, 2) = pCamera->look.z;
    pCamera->viewTrans(3, 2) = z;

    pCamera->viewTrans(0, 3) = 0.0f;
    pCamera->viewTrans(1, 3) = 0.0f;
    pCamera->viewTrans(2, 3) = 0.0f;
    pCamera->viewTrans(3, 3) = 1.0f;
}

void updateCameraProjTrans(Camera* pCamera) {
    float fovAngle = pCamera->fovAngle;
    float w = pCamera->screenViewport.Width;
    float h = pCamera->screenViewport.Height;
    float nearZ = pCamera->nearZ;
    float farZ = pCamera->farZ;

    XMMATRIX projMat = XMMatrixPerspectiveFovLH(fovAngle, w / h, nearZ, farZ); // FOV: Field of View.
    XMStoreFloat4x4(&pCamera->projTrans, projMat);
}

void dev_updateCameraWalk(float deltaTime, Camera* pCamera) {
    float moveSpeedScale = pCamera->moveSpeedScale;

    bool A = GetAsyncKeyState('A') & 0x8000;
    bool D = GetAsyncKeyState('D') & 0x8000;
    if ((A && D) || (!A && !D)) {
        pCamera->moveSpeed.x = 0.0f;
    }
    else {
        pCamera->isViewTransDirty = true;
        if (A) pCamera->moveSpeed.x = -moveSpeedScale;
        else if (D) pCamera->moveSpeed.x = moveSpeedScale;
    }

    bool Q = GetAsyncKeyState('Q') & 0x8000;
    bool E = GetAsyncKeyState('E') & 0x8000;
    if ((Q && E) || (!Q && !E)) {
        pCamera->moveSpeed.y = 0.0f;
    }
    else {
        pCamera->isViewTransDirty = true;
        if (Q) pCamera->moveSpeed.y = -moveSpeedScale;
        else if (E) pCamera->moveSpeed.y = moveSpeedScale;
    }

    bool S = GetAsyncKeyState('S') & 0x8000;
    bool W = GetAsyncKeyState('W') & 0x8000;
    if ((S && W) || (!S && !W)) {
        pCamera->moveSpeed.z = 0.0f;
    }
    else {
        pCamera->isViewTransDirty = true;
        if (S) pCamera->moveSpeed.z = -moveSpeedScale;
        else if (W) pCamera->moveSpeed.z = moveSpeedScale;
    }

    pCamera->moveSpeed.x *= deltaTime;
    pCamera->moveSpeed.y *= deltaTime;
    pCamera->moveSpeed.z *= deltaTime;

    dev_walkAroundCamera(pCamera->moveSpeed.x, pCamera->moveSpeed.y, pCamera->moveSpeed.z, pCamera);
}

void dev_walkAroundCamera(float valueAD, float valueQE, float valueWS, Camera* pCamera) {
    XMFLOAT3 sum = {};
    XMStoreFloat3(&sum,
        XMLoadFloat3(&pCamera->right) * valueAD +
        XMLoadFloat3(&pCamera->up) * valueQE +
        XMLoadFloat3(&pCamera->look) * valueWS);

    translateCamera(sum.x, sum.y, sum.z, pCamera);
}

void dev_lookAroundCamera(float mouseDx, float mouseDy, Camera* pCamera) {
    rotateCamera(mouseDy * pCamera->rotateSpeedScale, mouseDx * pCamera->rotateSpeedScale, 0.0f, pCamera);
}
