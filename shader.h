/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
using namespace DirectX;
using namespace Microsoft::WRL;

struct ObjConsts {
    XMFLOAT4X4 worldTrans;
};

ComPtr<ID3DBlob> compileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines,
    const std::string& entryPoint, const std::string& target);