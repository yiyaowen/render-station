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

#include "light.h"
#include "utils/math-utils.h"

struct ShaderFuncs {
    ComPtr<ID3DBlob> vs = nullptr;
    ComPtr<ID3DBlob> hs = nullptr;
    ComPtr<ID3DBlob> ds = nullptr;
    ComPtr<ID3DBlob> gs = nullptr;
    ComPtr<ID3DBlob> ps = nullptr;
    ComPtr<ID3DBlob> cs = nullptr;
};

struct ShaderFuncEntryPoints {
    std::string vs = "VS";
    std::string hs = "HS";
    std::string ds = "DS";
    std::string gs = "GS";
    std::string ps = "PS";
    std::string cs = "CS";
};

class Shader {
    using FUNC_FLAG = unsigned char;
public:
    static constexpr FUNC_FLAG VS = 1;
    static constexpr FUNC_FLAG HS = VS << 1;
    static constexpr FUNC_FLAG DS = HS << 1;
    static constexpr FUNC_FLAG GS = DS << 1;
    static constexpr FUNC_FLAG PS = GS << 1;
    static constexpr FUNC_FLAG CS = PS << 1;

public:
    inline std::string name() { return _name; }
    inline std::wstring sourceFilename() { return _sourceFilename; }
    inline std::string vsEntryPoint() { return _entryPoints.vs; }
    inline std::string hsEntryPoint() { return _entryPoints.hs; }
    inline std::string dsEntryPoint() { return _entryPoints.ds; }
    inline std::string gsEntryPoint() { return _entryPoints.gs; }
    inline std::string psEntryPoint() { return _entryPoints.ps; }
    inline std::string csEntryPoint() { return _entryPoints.cs; }

    ShaderFuncs funcs = {};
    inline bool hasVS() { return _funcFlag & VS; }
    inline bool hasHS() { return _funcFlag & HS; }
    inline bool hasDS() { return _funcFlag & DS; }
    inline bool hasGS() { return _funcFlag & GS; }
    inline bool hasPS() { return _funcFlag & PS; }
    inline bool hasCS() { return _funcFlag & CS; }

    Shader(const std::string& name,
        const std::wstring& filename,
        FUNC_FLAG flags,
        const ShaderFuncEntryPoints& entryPoints);

    void initWithHlslFile(const std::string& name,
        const std::wstring& filename,
        FUNC_FLAG flags,
        const ShaderFuncEntryPoints& entryPoints);

private:
    std::string _name = {};
    std::wstring _sourceFilename = {};
    FUNC_FLAG _funcFlag = 0;
    ShaderFuncEntryPoints _entryPoints = {};
};

struct ObjConsts {
    XMFLOAT4X4 worldTrans = makeIdentityFloat4x4();
    XMFLOAT4X4 invTrWorldTrans = makeIdentityFloat4x4(); // Tr: Transpose
    XMFLOAT4X4 texTrans = makeIdentityFloat4x4();

    int hasDisplacementMap = 0;
    int hasNormalMap = 0;
};

struct ProcConsts {
    XMFLOAT4X4 viewTrans = makeIdentityFloat4x4();
    XMFLOAT4X4 projTrans = makeIdentityFloat4x4();

    XMFLOAT3 eyePosW = { 0.0f, 0.0f, 0.0f };
    float elapsedSecs = 0.0f;

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

void bindShaderToPSO(D3D12_GRAPHICS_PIPELINE_STATE_DESC* pso, Shader* shader);

void bindShaderToCPSO(D3D12_COMPUTE_PIPELINE_STATE_DESC* cpso, Shader* shader);