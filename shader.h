/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "light.h"
#include "math-utils.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
using namespace DirectX;
using namespace Microsoft::WRL;

struct ObjConsts {
    XMFLOAT4X4 worldTrans = makeIdentityFloat4x4();
    XMFLOAT4X4 invTrWorldTrans = makeIdentityFloat4x4(); // Tr: Transpose
    XMFLOAT4X4 texTrans = makeIdentityFloat4x4();
};

struct ProcConsts {
    XMFLOAT4X4 viewTrans = makeIdentityFloat4x4();
    XMFLOAT4X4 projTrans = makeIdentityFloat4x4();

    XMFLOAT3 eyePosW = { 0.0f, 0.0f, 0.0f };
    float _placeholder;

    XMFLOAT4 ambientLight = { 0.0f, 0.0f, 0.0f, 0.0f };
    Light lights[MAX_LIGHTS];

    XMFLOAT4 fogColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float fogFallOffStart = 0.0f;
    float fogFallOffEnd = 0.0f;
    XMFLOAT2 _placeholder2;

    XMFLOAT4X4 reflectTrans = makeIdentityFloat4x4();
    XMFLOAT4X4 invTrReflectTrans = makeIdentityFloat4x4(); // Tr: Transpose
};

ComPtr<ID3DBlob> compileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines,
    const std::string& entryPoint, const std::string& target);

void XM_CALLCONV updateProcConstsWithReflectMat(FXMMATRIX reflectMat, ProcConsts* pData);