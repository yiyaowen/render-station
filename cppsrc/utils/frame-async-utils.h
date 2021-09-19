/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "d3dcore/d3dcore.h"

void initEmptyFrameResource(D3DCore* pCore, FrameResource* pResource);

void initFResourceObjConstBuff(D3DCore* pCore, UINT objBuffCount, FrameResource* pResource);
void initFResourceProcConstBuff(D3DCore* pCore, UINT procBuffCount, FrameResource* pResource);
void initFResourceMatConstBuff(D3DCore* pCore, UINT matBuffCount, FrameResource* pResource);

void initEmptyRenderItem(RenderItem* pRitem);

void drawRenderItems(D3DCore* pCore, RenderItem** ppRitem, UINT ritemCount, std::vector<UINT> seatIdxOffsetList);

void drawRenderItemsInLayer(D3DCore* pCore, std::string name, RenderItem** ppRitem, UINT ritemCount);

void drawRitemLayerWithName(D3DCore* pCore, std::string name);

UINT calcConstBuffSize(UINT byteSize);

void createConstBuffPair(D3DCore* pCore, size_t elemSize, UINT elemCount,
    BYTE** ppBuffCPU, ID3D12Resource** ppBuffGPU);

void drawAllRitemsFormatted(D3DCore* pCore, const std::string& psoName, D3D_PRIMITIVE_TOPOLOGY primTopology, Material* mat);