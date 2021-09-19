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

Shader::Shader(const std::string& name, const std::wstring& filename, FUNC_FLAG flags) {
    initWithHlslFile(name, filename, flags);
}

void Shader::initWithHlslFile(const std::string& name, const std::wstring& filename, FUNC_FLAG flags) {
    _name = name;
    _sourceFilename = filename;
    _funcFlag = flags;
    if (flags & VS) {
        funcs.vs = compileShader(filename, nullptr, "VS", "vs_5_0");
    }
    if (flags & GS) {
        funcs.gs = compileShader(filename, nullptr, "GS", "gs_5_0");
    }
    if (flags & PS) {
        funcs.ps = compileShader(filename, nullptr, "PS", "ps_5_0");
    }
}

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

void XM_CALLCONV updateProcConstsWithReflectMat(FXMMATRIX reflectMat, ProcConsts* pData) {
    // Note the matrices in shaders are the transpose of the matrices in main program.
    // For general object transform, we should pass the transpose matrix.
    // For normal vector transform, originally the inverse-transpose matrix should be passed,
    // and an extra transpose operation should be added, which means inverse is enough here.
    XMStoreFloat4x4(&pData->reflectTrans, XMMatrixTranspose(reflectMat));
    XMStoreFloat4x4(&pData->invTrReflectTrans,
        XMMatrixInverse(&XMMatrixDeterminant(reflectMat), reflectMat));
    // Update all light sources' positions and directions.
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        XMVECTOR position = XMLoadFloat3(&pData->lights[i].position);
        XMStoreFloat3(&pData->lights[i].position, XMVector3Transform(position, reflectMat));
        XMVECTOR direction =  XMLoadFloat3(&pData->lights[i].direction);
        // See Microsoft DirectX documentations:
        // https://docs.microsoft.com/en-us/windows/win32/api/directxmath/nf-directxmath-xmvector3transform
        // ---------------------------------------------------------------------------------------------------
        // XMVector3Transform ignores the w component of the input vector, and uses a value of 1 instead.
        // The w component of the returned vector may be non-homogeneous (!= 1.0).
        // ---------------------------------------------------------------------------------------------------
        XMVectorSetW(direction, 0.0f); // Make sure the direction is handled as a vector instead of a point.
        XMStoreFloat3(&pData->lights[i].direction, XMVector4Transform(direction, reflectMat));
    }
}

void bindShaderToPSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC* pso, Shader* shader) {
    if (shader->hasVS()) {
        pso->VS = { reinterpret_cast<BYTE*>(shader->funcs.vs->GetBufferPointer()), shader->funcs.vs->GetBufferSize() };
    }
    if (shader->hasGS()) {
        pso->GS = { reinterpret_cast<BYTE*>(shader->funcs.gs->GetBufferPointer()), shader->funcs.gs->GetBufferSize() };
    }
    if (shader->hasPS()) {
        pso->PS = { reinterpret_cast<BYTE*>(shader->funcs.ps->GetBufferPointer()), shader->funcs.ps->GetBufferSize() };
    }
}
