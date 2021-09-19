/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#include <d3dcompiler.h>

#include "ApplicationException.h"
#include "ShaderTools.h"

ComPtr<ID3DBlob> ShaderTools::CompileShaderSource(
    const std::wstring& fileName,
    const D3D_SHADER_MACRO* defines,
    const std::string& entryPoint,
    const std::string& target)
{
    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    
    ThrowIfFailed(D3DCompileFromFile(fileName.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint.c_str(), target.c_str(), 0, 0, &byteCode, &errorBlob));

    return byteCode;
}
