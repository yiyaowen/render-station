/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <vector>

#include "vmesh.h"

struct ObjectGeometry {
    std::vector<Vertex> vertices;
    std::vector<UINT16> indices;
    Vsubmesh locationInfo;
    UINT64 vertexDataSize() { return vertices.size() * sizeof(Vertex); }
    UINT64 indexDataSize() { return indices.size() * sizeof(UINT16); }
};

void generateCube(XMFLOAT3 xyz, XMFLOAT4 color, ObjectGeometry* cube);

void generateCylinder(float topR, float bottomR, float h,
    UINT sliceCount, UINT stackCount, XMFLOAT4 color, ObjectGeometry* cylinder);

// This is an assistant func for generateCylinder, but of course it can also be used to generate a circle.
// We do NOT recommend to use the func to generate a circle for the sake of legibility.
void generateCylinderCap(bool isTop, float r, float offsetH, UINT sliceCount, XMFLOAT4 color, ObjectGeometry* cylinder);