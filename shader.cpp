/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <d3dcompiler.h>

#include "debugger.h"
#include "shader.h"

ComPtr<ID3DBlob> compileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint, const std::string& target)
{
    UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    // Enable shader debug.
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr) OutputDebugStringA((char*)errors->GetBufferPointer());

    checkHR(hr);

    return byteCode;
}
