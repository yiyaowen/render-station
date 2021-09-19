/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <unordered_set>

#include "d3dcore/d3dcore.h"

void initRitemWithGeoInfo(D3DCore* pCore, ObjectGeometry* geo, UINT constBuffSeatCount, RenderItem* ritem);

// When a render item is initialized, its objConstBuffStartIdx is set to 0 by default. However we need
// the indices to match their actual orders in the render item collection, which is done by this func.
void updateRitemRangeObjConstBuffIdx(RenderItem** ppRitem, size_t ritemCount);

UINT calcRitemRangeTotalObjConstBuffSeatCount(RenderItem** ppRitem, size_t ritemCount);

void moveNamedRitemToAllRitems(D3DCore* pCore, std::string name, std::unique_ptr<RenderItem>&& movedRitem);

std::vector<RenderItem*>& findRitemLayerWithName(const std::string& name,
    std::vector<std::pair<std::string, std::vector<RenderItem*>>>& layers);

void bindRitemReferenceWithLayers(D3DCore* pCore,
    std::string ritemName, std::unordered_map<std::string, UINT> layerNames);