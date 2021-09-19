/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
using namespace Microsoft::WRL;

class ShaderTools {
public:
    static ComPtr<ID3DBlob> CompileShaderSource(
        const std::wstring& fileName,
        const D3D_SHADER_MACRO* defines,
        const std::string& entryPoint,
        const std::string& target);
};