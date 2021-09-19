/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "frame-async-utils.h"
#include "geometry-utils.h"
#include "render-item-utils.h"
#include "vmesh-utils.h"

void initRitemWithGeoInfo(D3DCore* pCore, ObjectGeometry* geo, RenderItem* ritem) {
    initEmptyRenderItem(ritem);
    ritem->mesh = std::make_unique<Vmesh>();
    initVmesh(pCore, geo->vertices.data(), geo->vertexDataSize(),
        geo->indices.data(), geo->indexDataSize(), ritem->mesh.get());
    ritem->mesh->objects["main"] = geo->locationInfo;
}

void updateRitemRangeObjConstBuffIdx(RenderItem** ppRitem, size_t ritemCount) {
    for (size_t i = 0; i < ritemCount; ++i) {
        ppRitem[i]->objConstBuffIdx = i;
    }
}

std::vector<RenderItem*>& findRitemLayerWithName(const std::string& name,
    std::vector<std::pair<std::string, std::vector<RenderItem*>>>& layers)
{
    return std::find_if(layers.begin(), layers.end(),
        [&](const std::pair<std::string, std::vector<RenderItem*>>& p) {
            return name == p.first;
        })->second;
}
