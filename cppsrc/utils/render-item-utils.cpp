/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "frame-async-utils.h"
#include "geometry-utils.h"
#include "render-item-utils.h"
#include "vmesh-utils.h"

void initRitemWithGeoInfo(D3DCore* pCore, ObjectGeometry* geo, UINT constBuffSeatCount, RenderItem* ritem) {
    initEmptyRenderItem(ritem);
    ritem->objConstBuffSeatCount = constBuffSeatCount;
    ritem->constData.resize(constBuffSeatCount);
    ritem->materials.resize(constBuffSeatCount);
    ritem->mesh = std::make_unique<Vmesh>();
    initVmesh(pCore, geo->vertices.data(), geo->vertexDataSize(),
        geo->indices.data(), geo->indexDataSize(), ritem->mesh.get());
    ritem->mesh->objects["main"] = geo->locationInfo;
}

void updateRitemRangeObjConstBuffIdx(RenderItem** ppRitem, size_t ritemCount) {
    size_t tmpStartIdx = 0;
    for (size_t i = 0; i < ritemCount; ++i) {
        ppRitem[i]->objConstBuffStartIdx = tmpStartIdx;
        tmpStartIdx += ppRitem[i]->objConstBuffSeatCount;
    }
}

UINT calcRitemRangeTotalObjConstBuffSeatCount(RenderItem** ppRitem, size_t ritemCount) {
    UINT seatCount = 0;
    for (size_t i = 0; i < ritemCount; ++i) {
        seatCount += ppRitem[i]->objConstBuffSeatCount;
    }
    return seatCount;
}

void moveNamedRitemToAllRitems(D3DCore* pCore, std::string name, std::unique_ptr<RenderItem>&& movedRitem) {
    pCore->ritems.insert({ name, std::move(movedRitem) });
    pCore->allRitems.push_back(pCore->ritems[name].get());
}

std::vector<RenderItem*>& findRitemLayerWithName(const std::string& name,
    std::vector<std::pair<std::string, std::vector<RenderItem*>>>& layers)
{
    return std::find_if(layers.begin(), layers.end(),
        [&](const std::pair<std::string, std::vector<RenderItem*>>& p) {
            return name == p.first;
        })->second;
}

// Layer Info: [layer name], [bound layer seat offset]
// About the details of bound layer seat offset, see the description in RenderItem struct declaration.
void bindRitemReferenceWithLayers(D3DCore* pCore,
    std::string ritemName, std::unordered_map<std::string, UINT> layerInfos)
{
    for (auto& info : layerInfos) {
        pCore->ritems[ritemName]->boundLayerSeatOffsetTable[info.first] = info.second;
        findRitemLayerWithName(info.first, pCore->ritemLayers).push_back(pCore->ritems[ritemName].get());
    }
}
