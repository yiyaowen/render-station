/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "d3dcore.h"

void initVmesh(D3DCore* pCore, const void* vertexData, UINT64 vertexDataSize,
    const void* indexData, UINT64 indexDataSize, Vmesh* pMesh);

void createDefaultBuffs(D3DCore* pCore, const void* initData, UINT64 byteSize,
    ID3DBlob** ppBuffCPU, ID3D12Resource** ppBuffGPU, ID3D12Resource** ppUploadBuff);