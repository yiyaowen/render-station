/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "geometry-utils.h"

void generateCube(XMFLOAT3 xyz, XMFLOAT4 color, ObjectGeometry* cube) {
    float x = xyz.x; float y = xyz.y; float z = xyz.z;
    // Note the order of vertices and indices can NOT be disorganized as we will apply
    // back-face cull to it, which means the twist number must be set correctly.
    cube->vertices =
    {
        Vertex({ XMFLOAT3(-x, -y, -z), color }),
        Vertex({ XMFLOAT3(-x, +y, -z), color }),
        Vertex({ XMFLOAT3(+x, +y, -z), color }),
        Vertex({ XMFLOAT3(+x, -y, -z), color }),
        Vertex({ XMFLOAT3(-x, -y, +z), color }),
        Vertex({ XMFLOAT3(-x, +y, +z), color }),
        Vertex({ XMFLOAT3(+x, +y, +z), color }),
        Vertex({ XMFLOAT3(+x, -y, +z), color })
    };
    cube->indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,
        // back face
        4, 6, 5,
        4, 7, 6,
        // left face
        4, 5, 1,
        4, 1, 0,
        // right face
        3, 2, 6,
        3, 6, 7,
        // top face
        1, 5, 6,
        1, 6, 2,
        // bottom face
        4, 0, 3,
        4, 3, 7
    };
    cube->locationInfo.indexCount = cube->indices.size();
    cube->locationInfo.startIndexLocation = 0;
    cube->locationInfo.baseVertexLocation = 0;
}

void generateCylinder(float topR, float bottomR, float h,
    UINT sliceCount, UINT stackCount, XMFLOAT4 color, ObjectGeometry* cylinder)
{
    cylinder->vertices.clear();
    cylinder->indices.clear();

    float topH = h / 2;
    float deltaH = h / stackCount;
    float deltaR = (topR - bottomR) / stackCount;
    float deltaTheta = XM_2PI / sliceCount;
    // To make sure UV unfolds and maps properly, the first and last point on the same horizontal ring
    // must coincide, which means there are actually sliceCount + 1 points to draw.
    UINT sliceCountEx = sliceCount + 1;
    // Side vertices & indices
    for (UINT i = 0; i < stackCount + 1; ++i) {
        float currH = topH - i * deltaH;
        float currR = topR - i * deltaR;
        for (UINT j = 0; j < sliceCount + 1; ++j) {
            float x = currR * cosf(j * deltaTheta);
            float y = currR * sinf(j * deltaTheta);
            cylinder->vertices.push_back({ XMFLOAT3(x, y, currH), color });
        }
    }
    for (UINT i = 0; i < stackCount; ++i) {
        for (UINT j = 0; j < sliceCount; ++j) {
            cylinder->indices.push_back((i + 1) * sliceCountEx + j);
            cylinder->indices.push_back((i + 1) * sliceCountEx + j + 1);
            cylinder->indices.push_back(i * sliceCountEx + j + 1);
            cylinder->indices.push_back((i + 1) * sliceCountEx + j);
            cylinder->indices.push_back(i * sliceCountEx + j + 1);
            cylinder->indices.push_back(i * sliceCountEx + j);
        }
    }
    // Top & bottom caps vertices & indices
    generateCylinderCap(true, topR, h / 2, sliceCount, color, cylinder);
    generateCylinderCap(false, bottomR, -h / 2, sliceCount, color, cylinder);

    cylinder->locationInfo.indexCount = cylinder->indices.size();
    cylinder->locationInfo.startIndexLocation = 0;
    cylinder->locationInfo.baseVertexLocation = 0;
}

void generateCylinderCap(bool isTop, float r, float offsetH, UINT sliceCount, XMFLOAT4 color, ObjectGeometry* cylinder) {
    float deltaTheta = XM_2PI / sliceCount;
    cylinder->vertices.push_back({ XMFLOAT3(0, 0, offsetH), color });
    UINT originBaseIdx = cylinder->vertices.size();
    UINT capCenterIdx = cylinder->vertices.size() - 1;
    for (UINT i = 0; i < sliceCount + 1; ++i) {
        float x = r * cosf(i * deltaTheta);
        float y = r * sinf(i * deltaTheta);
        cylinder->vertices.push_back({ XMFLOAT3(x, y, offsetH), color });
    }
    for (UINT i = 0; i < sliceCount; ++i) {
        cylinder->indices.push_back(capCenterIdx);
        if (isTop) {
            cylinder->indices.push_back(originBaseIdx + i);
            cylinder->indices.push_back(originBaseIdx + i + 1);
        }
        else {
            cylinder->indices.push_back(originBaseIdx + i + 1);
            cylinder->indices.push_back(originBaseIdx + i);
        }
    }
}
