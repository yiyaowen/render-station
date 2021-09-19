/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "d3dcore.h"

void initRitemWithGeoInfo(D3DCore* pCore, ObjectGeometry* geo, RenderItem* ritem);

// When a render item is initialized, its objConstBuffIdx is set to 0 by default. However we need the
// indices to match their actual orders in the render item collection, which is done by this func.
void updateRitemRangeObjConstBuffIdx(RenderItem** ppRitem, size_t ritemCount);