/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "d3dcore.h"

void initFrameResource(D3DCore* pCore, UINT objBuffCount, UINT procBuffCount, FrameResource* pResource);

void drawRenderItems(D3DCore* pCore, RenderItem** ppRitem, UINT ritemCount);

UINT calcConstBuffSize(UINT byteSize);

void createConstBuffPair(D3DCore* pCore, size_t elemSize, UINT elemCount,
    BYTE** ppBuffCPU, ID3D12Resource** ppBuffGPU);