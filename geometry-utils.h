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

void translateObjectGeometry(float dx, float dy, float dz, ObjectGeometry* geo);

void rotateObjectGeometry(float rx, float ry, float rz, ObjectGeometry* geo);

void scaleObjectGeometry(float sx, float sy, float sz, ObjectGeometry* geo);

void XM_CALLCONV applyObjectGeometryTransform(FXMMATRIX trans, ObjectGeometry* geo);

XMVECTOR XM_CALLCONV calcTriangleClockwiseNormal(FXMVECTOR v0, FXMVECTOR v1, FXMVECTOR v2);

// After the vertex positions and indices are decided, the vertex normals can be updated through this func.
// Note this func simply takes every triangle connected with the target vertex into account and calculates
// the mean normal vector of those triangles, which may lead to some unexpected results (incorrect normals).
// To avoid this behaviour, it is recommended to calculate the normals manually by mathematical means.
// In particular, after applied this func to the geometry without coincident vertices, it will present
// a kind of appearance of so-called "shader smooth" in many 3D modeling or rendering applications.
void updateVertexNormals(ObjectGeometry* geo);

// Note this func only changes the vertices and indices data and other data may be dirty.
void subdivide(ObjectGeometry* geo);

void appendVerticesToObjectGeometry(std::vector<Vertex> ver, std::vector<UINT16> idx, ObjectGeometry* objGeo);

void generateCube(XMFLOAT3 xyz, ObjectGeometry* cube);

void generateCylinder(float topR, float bottomR, float h, UINT sliceCount, UINT stackCount, ObjectGeometry* cylinder);

// This is an assistant func for generateCylinder, but of course it can also be used to generate a circle.
// We do NOT recommend to use the func to generate a circle for the sake of legibility.
void generateCylinderCap(bool isTop, float r, float offsetH, UINT sliceCount, ObjectGeometry* cylinder);

void generateSphere(float r, UINT sliceCount, UINT stackCount, ObjectGeometry* sphere);

void generateSphereCap(bool isTop, float r, UINT sliceCount, UINT ringCount, ObjectGeometry* sphere);

void generateGeoSphere(float r, int subdivisionLevel, ObjectGeometry* gs);

void generateGrid(float w, float h, UINT m, UINT n, ObjectGeometry* grid);

// We simply generate a grid first and then change the vertices' height to get a hill.
void disturbGridToHill(float hsize, float density, ObjectGeometry* hill);

float calcHillVertexHeight(float x, float y, float hsize, float density);

void generateCircle2D(XMFLOAT3 center, XMFLOAT3 normal, float radius, UINT n, ObjectGeometry* circle);