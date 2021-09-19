/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "geometry-utils.h"

void translateObjectGeometry(float dx, float dy, float dz, ObjectGeometry* geo) {
    XMMATRIX translationMat = XMMatrixTranslation(dx, dy, dz);
    applyObjectGeometryTransformation(translationMat, geo);
}

void rotateObjectGeometry(float rx, float ry, float rz, ObjectGeometry* geo) {
    XMMATRIX rotationMat = XMMatrixRotationRollPitchYaw(rx, ry, rz);
    applyObjectGeometryTransformation(rotationMat, geo);
}

void scaleObjectGeometry(float sx, float sy, float sz, ObjectGeometry* geo) {
    XMMATRIX scalingMat = XMMatrixScaling(sx, sy, sz);
    applyObjectGeometryTransformation(scalingMat, geo);
}

void applyObjectGeometryTransformation(XMMATRIX trans, ObjectGeometry* geo) {
    XMMATRIX invTrTrans = XMMatrixTranspose(XMMatrixInverse(&XMMatrixDeterminant(trans), trans));
    for (auto& ver : geo->vertices) {
        XMVECTOR pos = XMLoadFloat3(&ver.pos);
        pos = XMVector3Transform(pos, trans);
        XMStoreFloat3(&ver.pos, pos);
        XMVECTOR nor = XMLoadFloat3(&ver.normal);
        nor = XMVector3Transform(nor, invTrTrans);
        XMStoreFloat3(&ver.normal, nor);
    }
}

XMVECTOR calcTriangleClockwiseNormal(FXMVECTOR v0, FXMVECTOR v1, FXMVECTOR v2) {
    XMVECTOR x = v1 - v0;
    XMVECTOR y = v2 - v0;
    return XMVector3Normalize(XMVector3Cross(x, y));
}

void updateVertexNormals(Vertex* pVer, size_t verCount, UINT16* pIdx, size_t idxCount) {
    for (size_t i = 0; i * 3 + 2 < idxCount; ++i) {
        UINT16 i0 = pIdx[i * 3];
        UINT16 i1 = pIdx[i * 3 + 1];
        UINT16 i2 = pIdx[i * 3 + 2];

        Vertex v0 = pVer[i0];
        Vertex v1 = pVer[i1];
        Vertex v2 = pVer[i2];

        XMVECTOR normal = calcTriangleClockwiseNormal(
            XMLoadFloat3(&v0.pos),
            XMLoadFloat3(&v1.pos),
            XMLoadFloat3(&v2.pos));

        XMStoreFloat3(&pVer[i0].normal, XMLoadFloat3(&v0.normal) += normal);
        XMStoreFloat3(&pVer[i1].normal, XMLoadFloat3(&v1.normal) += normal);
        XMStoreFloat3(&pVer[i2].normal, XMLoadFloat3(&v2.normal) += normal);
    }
    for (size_t i = 0; i < verCount; ++i) {
        XMFLOAT3 nor = pVer[i].normal;
        XMStoreFloat3(&pVer[i].normal, XMVector3Normalize(XMLoadFloat3(&pVer[i].normal)));
    }
}

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
        1, 3, 0,
        1, 2, 3,
        // back face
        4, 6, 5,
        4, 7, 6,
        // left face
        1, 0, 4,
        1, 4, 5,
        // right face
        3, 2, 6,
        3, 6, 7,
        // top face
        6, 1, 5,
        6, 2, 1,
        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    cube->locationInfo.indexCount = cube->indices.size();
    cube->locationInfo.startIndexLocation = 0;
    cube->locationInfo.baseVertexLocation = 0;

    updateVertexNormals(cube->vertices.data(), cube->vertices.size(), cube->indices.data(), cube->indices.size());
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

    updateVertexNormals(cylinder->vertices.data(), cylinder->vertices.size(), cylinder->indices.data(), cylinder->indices.size());
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
