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
#include <unordered_map>
#include <wrl.h>
using namespace DirectX;
using namespace Microsoft::WRL;

struct Vertex {
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

struct Vsubmesh {
    UINT indexCount = 0;
    UINT startIndexLocation = 0;
    INT baseVertexLocation = 0;
};

struct Vmesh {
    ComPtr<ID3DBlob> vertexBuffCPU = nullptr;
    ComPtr<ID3DBlob> indexBuffCPU = nullptr;

    ComPtr<ID3D12Resource> vertexBuffGPU = nullptr;
    ComPtr<ID3D12Resource> indexBuffGPU = nullptr;

    ComPtr<ID3D12Resource> vertexUploadBuff = nullptr;
    ComPtr<ID3D12Resource> indexUploadBuff = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vertexBuffView = {};
    D3D12_INDEX_BUFFER_VIEW indexBuffView = {};

    std::unordered_map<std::string, Vsubmesh> objects = {};
};