/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <memory>

#include "geometry-utils.h"
#include "math-utils.h"

ObjectGeometry* copyObjectGeometry(ObjectGeometry* source) {
    std::unique_ptr<ObjectGeometry> geo = std::make_unique<ObjectGeometry>();
    (*geo) = (*source); // Flat copy.
    return geo.release();
}

void translateObjectGeometry(float dx, float dy, float dz, ObjectGeometry* geo) {
    XMMATRIX translationMat = XMMatrixTranslation(dx, dy, dz);
    applyObjectGeometryTransform(translationMat, geo);
}

void rotateObjectGeometry(float rx, float ry, float rz, ObjectGeometry* geo) {
    XMMATRIX rotationMat = XMMatrixRotationRollPitchYaw(rx, ry, rz);
    applyObjectGeometryTransform(rotationMat, geo);
}

void scaleObjectGeometry(float sx, float sy, float sz, ObjectGeometry* geo) {
    XMMATRIX scalingMat = XMMatrixScaling(sx, sy, sz);
    applyObjectGeometryTransform(scalingMat, geo);
}

void XM_CALLCONV applyObjectGeometryTransform(FXMMATRIX trans, ObjectGeometry* geo) {
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

XMVECTOR XM_CALLCONV calcTriangleClockwiseNormal(FXMVECTOR v0, FXMVECTOR v1, FXMVECTOR v2) {
    XMVECTOR x = v1 - v0;
    XMVECTOR y = v2 - v0;
    return XMVector3Normalize(XMVector3Cross(x, y));
}

void updateVertexNormals(ObjectGeometry* geo) {
    auto pVer = geo->vertices.data();
    size_t verCount = geo->vertices.size();
    auto pIdx = geo->indices.data();
    size_t idxCount = geo->indices.size();
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

/*               v0 (0)
 *                *
 *               / \
 *              /   \
 *     v01 (1) *-----* v20 (5)
 *            / \   / \
 *           /   \ /   \
 *   v1 (2) *-----*-----* v2 (4)
 *             v12 (3)
*/

void subdivide(ObjectGeometry* geo) {
    ObjectGeometry geoCopy = *geo;
    auto pVer = geoCopy.vertices.data();
    size_t verCount = geoCopy.vertices.size();
    auto pIdx = geoCopy.indices.data();
    size_t idxCount = geoCopy.indices.size();
    geo->vertices.clear();
    geo->indices.clear();
    for (size_t i = 0; i * 3 + 2 < idxCount; ++i) {
        Vertex v0 = pVer[pIdx[i * 3]];
        Vertex v1 = pVer[pIdx[i * 3 + 1]];
        Vertex v2 = pVer[pIdx[i * 3 + 2]];

        geo->vertices.push_back({ v0.pos });                   // i * 6
        geo->vertices.push_back({ midpoint(v0.pos, v1.pos) }); // i * 6 + 1
        geo->vertices.push_back({ v1.pos });                   // i * 6 + 2
        geo->vertices.push_back({ midpoint(v1.pos, v2.pos) }); // i * 6 + 3
        geo->vertices.push_back({ v2.pos });                   // i * 6 + 4
        geo->vertices.push_back({ midpoint(v2.pos, v0.pos) }); // i * 6 + 5

        UINT16 b = i * 6; // base index
        geo->indices.push_back(b);
        geo->indices.push_back(b + 1);
        geo->indices.push_back(b + 5);

        geo->indices.push_back(b + 1);
        geo->indices.push_back(b + 3);
        geo->indices.push_back(b + 5);

        geo->indices.push_back(b + 1);
        geo->indices.push_back(b + 2);
        geo->indices.push_back(b + 3);

        geo->indices.push_back(b + 3);
        geo->indices.push_back(b + 4);
        geo->indices.push_back(b + 5);
    }
}

void appendVerticesToObjectGeometry(const std::vector<Vertex>& ver, const std::vector<UINT16>& idx, ObjectGeometry* objGeo) {
    UINT count = max(ver.size(), idx.size());
    for (UINT i = 0; i < count; ++i) {
        objGeo->vertices.push_back(ver[i]);
        objGeo->indices.push_back(idx[i]);
    }
    objGeo->locationInfo.indexCount += count;
}

// @ IMPORTANT @ IMPORTANT @ IMPORTANT @ IMPORTANT @ IMPORTANT @ IMPORTANT @ IMPORTANT @
/*
 * In DX, the forward twist order is set to clockwise by default, i.e. LEFT-handed screw,
 * and it use a LEFT hand coordinate system. However, in our generate{} funcs we use a
 * RIGHT hand coordinate system out of the consideration of common modeling practice.
 * The consequence is that we have to change the twist order here to anticlockwise, i.e.
 * RIGHT-handed screw, which is a very tricky point to understand without some hints.
 * The UV coordinate system should also be adjusted. In DX, the UV original point is at
 * left-top and U-axis extends from left to right, V-axis from top to down. By our common
 * modeling practice convention, the UV original point is at right-top and U-axis extends
 * from right to left, V-axis from top to down.
*/

void generateCube(XMFLOAT3 xyz, ObjectGeometry* cube) {
    float x = xyz.x; float y = xyz.y; float z = xyz.z;
    // Note the order of vertices and indices can NOT be disorganized as we will apply
    // back-face cull to it, which means the twist number must be set correctly.
    cube->vertices =
    {
        // front face
        Vertex{ {+x, -y, +z}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} }, // 0
        Vertex{ {+x, -y, -z}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} }, // 1
        Vertex{ {-x, -y, +z}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} }, // 2
        Vertex{ {-x, -y, -z}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} }, // 3
        // back face
        Vertex{ {+x, +y, +z}, {0.0f, +1.0f, 0.0f}, {1.0f, 0.0f} }, // 4
        Vertex{ {+x, +y, -z}, {0.0f, +1.0f, 0.0f}, {1.0f, 1.0f} }, // 5
        Vertex{ {-x, +y, +z}, {0.0f, +1.0f, 0.0f}, {0.0f, 0.0f} }, // 6
        Vertex{ {-x, +y, -z}, {0.0f, +1.0f, 0.0f}, {0.0f, 1.0f} }, // 7
        // left face
        Vertex{ {-x, +y, +z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }, // 8
        Vertex{ {-x, +y, -z}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }, // 9
        Vertex{ {-x, -y, +z}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }, // 10
        Vertex{ {-x, -y, -z}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }, // 11
        // right face
        Vertex{ {+x, +y, +z}, {+1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} }, // 12
        Vertex{ {+x, +y, -z}, {+1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} }, // 13
        Vertex{ {+x, -y, +z}, {+1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} }, // 14
        Vertex{ {+x, -y, -z}, {+1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} }, // 15
        // top face
        Vertex{ {+x, +y, +z}, {0.0f, 0.0f, +1.0f}, {1.0f, 1.0f} }, // 16
        Vertex{ {+x, -y, +z}, {0.0f, 0.0f, +1.0f}, {1.0f, 0.0f} }, // 17
        Vertex{ {-x, +y, +z}, {0.0f, 0.0f, +1.0f}, {0.0f, 1.0f} }, // 18
        Vertex{ {-x, -y, +z}, {0.0f, 0.0f, +1.0f}, {0.0f, 0.0f} }, // 19
        // bottom face
        Vertex{ {+x, +y, -z}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} }, // 20
        Vertex{ {+x, -y, -z}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} }, // 21
        Vertex{ {-x, +y, -z}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} }, // 22
        Vertex{ {-x, -y, -z}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} }, // 23
    };

    cube->indices =
    {
        // front face
        2, 1, 0,
        2, 3, 1,
        // back face
        4, 5, 7,
        4, 7, 6,
        // left face
        10, 8, 9,
        10, 9, 11,
        // right face
        12, 15, 13,
        12, 14, 15,
        // top face
        16, 19, 17,
        16, 18, 19,
        // bottom face
        22, 20, 21,
        22, 21, 23
    };

    cube->locationInfo.indexCount = cube->indices.size();
    cube->locationInfo.startIndexLocation = 0;
    cube->locationInfo.baseVertexLocation = 0;
}

void generateCylinder(float topR, float bottomR, float h, UINT sliceCount, UINT stackCount, ObjectGeometry* cylinder)
{
    cylinder->vertices.clear();
    cylinder->indices.clear();

    float topH = h / 2;
    float deltaH = h / stackCount;
    float deltaR = (topR - bottomR) / stackCount;
    float deltaTheta = XM_2PI / sliceCount;
    // All vertices on the same slice line of the cylinder's side face get the same normal.
    XMFLOAT3 eachNormal;
    std::vector<XMFLOAT3> normals;
    for (UINT i = 0; i < sliceCount + 1; ++i) {
        XMVECTOR S = XMVectorSet(
            -sinf(deltaTheta * i),
            cosf(deltaTheta * i),
            0.0f,
            0.0f);
        XMVECTOR T = XMVectorSet(
            (topR - bottomR) * cosf(deltaTheta),
            (topR - bottomR) * sinf(deltaTheta),
            h / 2,
            0.0f);
        XMVECTOR N = XMVector3Normalize(XMVector3Cross(S, T));
        XMStoreFloat3(&eachNormal, N);
        normals.push_back(eachNormal);
    }
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
            cylinder->vertices.push_back({ XMFLOAT3(x, y, currH), normals[j] });
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
    generateCylinderCap(true, topR, h / 2, sliceCount, cylinder);
    generateCylinderCap(false, bottomR, -h / 2, sliceCount, cylinder);

    cylinder->locationInfo.indexCount = cylinder->indices.size();
    cylinder->locationInfo.startIndexLocation = 0;
    cylinder->locationInfo.baseVertexLocation = 0;
}

void generateCylinderCap(bool isTop, float r, float offsetH, UINT sliceCount, ObjectGeometry* cylinder) {
    float deltaTheta = XM_2PI / sliceCount;
    cylinder->vertices.push_back({ XMFLOAT3(0, 0, offsetH) });
    UINT originBaseIdx = cylinder->vertices.size();
    UINT capCenterIdx = cylinder->vertices.size() - 1;
    XMFLOAT3 normal = isTop ? XMFLOAT3{ 0.0f, 0.0f, 1.0f } : XMFLOAT3{ 0.0f, 0.0f, -1.0f };
    for (UINT i = 0; i < sliceCount + 1; ++i) {
        float x = r * cosf(i * deltaTheta);
        float y = r * sinf(i * deltaTheta);
        cylinder->vertices.push_back({ XMFLOAT3(x, y, offsetH), normal });
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

void generateSphere(float r, UINT sliceCount, UINT ringCount, ObjectGeometry* sphere) {
    sphere->vertices.clear();
    sphere->indices.clear();

    float deltaTheta = XM_2PI / sliceCount;
    float deltaPhi = XM_PI / (ringCount + 2);
    XMFLOAT3 eachNormal;
    // To make sure UV unfolds and maps properly, the first and last point on the same horizontal ring
    // must coincide, which means there are actually sliceCount + 1 points to draw.
    UINT sliceCountEx = sliceCount + 1;
    // Side vertices & indices
    for (UINT i = 1; i < ringCount + 2; ++i) {
        float currH = r * cosf(i * deltaPhi);
        float currR = r * sinf(i * deltaPhi);
        for (UINT j = 0; j < sliceCount + 1; ++j) {
            float x = currR * cosf(j * deltaTheta);
            float y = currR * sinf(j * deltaTheta);
            XMVECTOR N = sphericalToCartesianRH(1.0f, j * deltaTheta, i * deltaPhi);
            XMStoreFloat3(&eachNormal, N);
            sphere->vertices.push_back({ XMFLOAT3(x, y, currH), eachNormal });
        }
    }
    for (UINT i = 0; i < ringCount; ++i) {
        for (UINT j = 0; j < sliceCount; ++j) {
            sphere->indices.push_back((i + 1) * sliceCountEx + j);
            sphere->indices.push_back((i + 1) * sliceCountEx + j + 1);
            sphere->indices.push_back(i * sliceCountEx + j + 1);
            sphere->indices.push_back((i + 1) * sliceCountEx + j);
            sphere->indices.push_back(i * sliceCountEx + j + 1);
            sphere->indices.push_back(i * sliceCountEx + j);
        }
    }
    // Top & bottom caps vertices & indices
    generateSphereCap(true, r, sliceCount, ringCount, sphere);
    generateSphereCap(false, r, sliceCount, ringCount, sphere);

    sphere->locationInfo.indexCount = sphere->indices.size();
    sphere->locationInfo.startIndexLocation = 0;
    sphere->locationInfo.baseVertexLocation = 0;
}

void generateSphereCap(bool isTop, float r, UINT sliceCount, UINT ringCount, ObjectGeometry* sphere) {
    float deltaTheta = XM_2PI / sliceCount;
    float deltaPhi = XM_PI / (ringCount + 2);
    float h2 = r * (isTop ? cosf(deltaPhi) : -cosf(deltaPhi));
    float r2 = r * sinf(deltaPhi);
    float circlePhi = isTop ? deltaPhi : (XM_PI - deltaPhi);
    sphere->vertices.push_back({ XMFLOAT3(0, 0, isTop ? r : -r), XMFLOAT3(0, 0, isTop ? 1.0f : -1.0f) });
    UINT originBaseIdx = sphere->vertices.size();
    UINT capCenterIdx = sphere->vertices.size() - 1;
    XMFLOAT3 eachNormal;
    for (UINT i = 0; i < sliceCount + 1; ++i) {
        float x = r2 * cosf(i * deltaTheta);
        float y = r2 * sinf(i * deltaTheta);
        XMVECTOR N = sphericalToCartesianRH(1.0f, i * deltaTheta, circlePhi);
        XMStoreFloat3(&eachNormal, N);
        sphere->vertices.push_back({ XMFLOAT3(x, y, h2), eachNormal });
    }
    for (UINT i = 0; i < sliceCount; ++i) {
        sphere->indices.push_back(capCenterIdx);
        if (isTop) {
            sphere->indices.push_back(originBaseIdx + i);
            sphere->indices.push_back(originBaseIdx + i + 1);
        }
        else {
            sphere->indices.push_back(originBaseIdx + i + 1);
            sphere->indices.push_back(originBaseIdx + i);
        }
    }
}

void generateGeoSphere(float r, int subdivisionLevel, ObjectGeometry* gs) {
    // Subdivide a regular icosahedron to approximate a sphere.
    const float X = 0.525731f * r; // sqrt(50 - 10 * sqrt(5)) / 10 = 0.525731
    const float Z = 0.850651f * r; // sqrt(50 + 10 * sqrt(5)) / 10 = 0.850651

    gs->vertices = {
        Vertex{ XMFLOAT3(-X, 0.0f, Z) }, Vertex{ XMFLOAT3(X, 0.0f, Z) },
        Vertex{ XMFLOAT3(-X, 0.0f, -Z) }, Vertex{ XMFLOAT3(X, 0.0f, -Z) },
        Vertex{ XMFLOAT3(0.0f, Z, X) }, Vertex{ XMFLOAT3(0.0f, Z, -X) },
        Vertex{ XMFLOAT3(0.0f, -Z, X) }, Vertex{ XMFLOAT3(0.0f, -Z, -X) },
        Vertex{ XMFLOAT3(Z, X, 0.0f) }, Vertex{ XMFLOAT3(-Z, X, 0.0f) },
        Vertex{ XMFLOAT3(Z, -X, 0.0f) }, Vertex{ XMFLOAT3(-Z, -X, 0.0f) } };

    gs->indices = {
        1, 4, 0,    4, 9, 0,    4, 5, 9,    8, 5, 4,    1, 8, 4,
        1, 10, 8,   10, 3, 8,   8, 3, 5,    3, 2, 5,    3, 7, 2,
        3, 10, 7,   10, 6, 7,   6, 11, 7,   6, 0, 11,   6, 1, 0,
        10, 1, 6,   11, 0, 9,   2, 11, 9,   5, 2, 9,    11, 2, 7 };

    for (int i = 0; i < subdivisionLevel; ++i) subdivide(gs);

    for (size_t i = 0; i < gs->vertices.size(); ++i) {
        XMVECTOR ver_nor = XMVector3Normalize(XMLoadFloat3(&gs->vertices[i].pos));
        XMStoreFloat3(&gs->vertices[i].pos, r * ver_nor);
        XMStoreFloat3(&gs->vertices[i].normal, ver_nor);
    }

    gs->locationInfo.indexCount = gs->indices.size();
    gs->locationInfo.startIndexLocation = 0;
    gs->locationInfo.baseVertexLocation = 0;
}

/*                    /|\ y
 *     (m) vertices    |   (m) vertices     (2 * m + 1) vertices
 * @---@---@---@---@---@---@---@---@---@---@
 * |   |   |   |   |   |   |   |   |   |   | (n) vertices
 * @---@---@---@---@---@---@---@---@---@---@
 * |   |   |   |   |   |   |   |   |   |   |   \
 * @---@---@---@---@---@---@---@---@---@---@----  x
 * |   |   |   |   |   |   |   |   |   |   |   /
 * @---@---@---@---@---@---@---@---@---@---@
 * |   |   |   |   |   |   |   |   |   |   | (n) vertices
 * @---@---@---@---@---@---@---@---@---@---@
 * Total: (2*m + 1) * (2*n + 1) vertices    (2 * n + 1) vertices
*/

void generateGrid(float w, float h, UINT m, UINT n, ObjectGeometry* grid) {
    UINT M = 2 * m + 1;
    UINT N = 2 * n + 1;
    UINT verCount = M * N;
    UINT triCount = (M - 1) * (N - 1) * 2;
    float dx = w / (M - 1);
    float dy = h / (N - 1);
    float du = 1.0f / (M - 1);
    float dv = 1.0f / (N - 1);

    grid->vertices.resize(verCount);
    for (UINT i = 0; i < N; ++i) {
        float y = h / 2 - i * dy;
        for (UINT j = 0; j < M; ++j) {
            float x = -w / 2 + j * dx;
            grid->vertices[i * M + j] = {
                { x, y, 0.0f },
                { 0.0f, 0.0f, 1.0f },
                { j * du, i * dv } };
        }
    }

    grid->indices.resize(triCount * 3);
    UINT k = 0; // Index of each square.
    for (UINT i = 0; i < N - 1; ++i) {
        for (UINT j = 0; j < M - 1; ++j) {
            grid->indices[k] = i * M + j + 1;
            grid->indices[k + 1] =  i * M + j;
            grid->indices[k + 2] = (i + 1) * M + j;
            grid->indices[k + 3] = i * M + j + 1;
            grid->indices[k + 4] = (i + 1) * M + j;
            grid->indices[k + 5] = (i + 1) * M + j + 1;
            k += 6; // Next square.
        }
    }

    grid->locationInfo.indexCount = grid->indices.size();
    grid->locationInfo.startIndexLocation = 0;
    grid->locationInfo.baseVertexLocation = 0;
}

void disturbGridToHill(float hsize, float density, ObjectGeometry* hill) {
    for (auto& ver : hill->vertices) {
        ver.pos.z = calcHillVertexHeight(ver.pos.x, ver.pos.y, hsize, density);
    }
}

float calcHillVertexHeight(float x, float y, float hsize, float density) {
    return hsize * (y * sinf(density* x) + x * cosf(density * y));
}

void generateCircle2D(XMFLOAT3 center, XMFLOAT3 normal, float radius, UINT n, ObjectGeometry* circle) {
    auto nor = XMLoadFloat3(&normal);
    auto xaxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR firstCross, secondCross;
    firstCross = XMVector3Cross(nor, xaxis);
    if (XMVectorGetX(XMVector3LengthSq(firstCross)) < 0.0001f) {
        // Nearly zero vector, so take X-axis as normal.
        // Since we only draw a 2D circle the 0 degree and 180 degree are treated the same.
        firstCross = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        secondCross = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
    else {
        firstCross = XMVector3Normalize(firstCross);
        secondCross = XMVector3Normalize(XMVector3Cross(firstCross, nor));
    }

    float theta = XM_2PI / n;
    circle->vertices.resize(n + 1);
    circle->indices.resize(n + 1);
    for (UINT i = 0; i < n; ++i) {
        auto r = firstCross * cosf(theta * i) + secondCross * sinf(theta * i);
        XMStoreFloat3(&circle->vertices[i].pos, r * radius + XMLoadFloat3(&center));
        XMStoreFloat3(&circle->vertices[i].normal, r);
        circle->indices[i] = i;
    }
    // Append first vertex at last manually to make a closed loop.
    circle->vertices[n] = circle->vertices[0];
    circle->indices[n] = n;

    circle->locationInfo.indexCount = circle->indices.size();
    circle->locationInfo.startIndexLocation = 0;
    circle->locationInfo.baseVertexLocation = 0;
}
