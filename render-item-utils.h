/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "d3dcore.h"

// We declare a set of generate{}Ex funcs that inherited from generate{} funcs in geometry-utils.h.
// The difference between generate{}Ex and generate{} is:
// The former will create a render item, and the latter will only create an object geometry.

void generateCubeEx(D3DCore* pCore, ObjectGeometry* geo, RenderItem* cube);

void generateCylinderEx(D3DCore* pCore, ObjectGeometry* geo, RenderItem* cylinder);

// When a render item is initialized, its objConstBuffIdx is set to 0 by default. However we need the
// indices to match their actual orders in the render item collection, which is done by this func.
void updateRitemRangeObjConstBuffIdx(RenderItem** ppRitem, size_t ritemCount);