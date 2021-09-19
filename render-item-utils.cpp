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

void generateCubeEx(D3DCore* pCore, ObjectGeometry* geo, RenderItem* cube) {
    initEmptyRenderItem(cube);
    cube->mesh = std::make_unique<Vmesh>();
    initVmesh(pCore, geo->vertices.data(), geo->vertexDataSize(),
        geo->indices.data(), geo->indexDataSize(), cube->mesh.get());
    cube->mesh->objects["main"] = geo->locationInfo;
}

void generateCylinderEx(D3DCore* pCore, ObjectGeometry* geo, RenderItem* cylinder)
{
    initEmptyRenderItem(cylinder);
    cylinder->mesh = std::make_unique<Vmesh>();
    initVmesh(pCore, geo->vertices.data(), geo->vertexDataSize(),
        geo->indices.data(), geo->indexDataSize(), cylinder->mesh.get());
    cylinder->mesh->objects["main"] = geo->locationInfo;
}

void updateRitemRangeObjConstBuffIdx(RenderItem** ppRitem, size_t ritemCount) {
    for (size_t i = 0; i < ritemCount; ++i) {
        ppRitem[i]->objConstBuffIdx = i;
    }
}
