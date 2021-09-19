/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#include "VertexMesh.h"

VertexMeshFactory::Mesh VertexMeshFactory::MakeCubeMesh(
    float width, float height, float depth, UINT32 subdivisionLevel)
{
    return Mesh();
}

VertexMeshFactory::Mesh VertexMeshFactory::MakeSphereMesh(
    float radius, UINT32 sliceCount, UINT32 stackCount)
{
    return Mesh();
}

VertexMeshFactory::Mesh VertexMeshFactory::MakeGeometrySphereMesh(
    float radius, UINT32 subdivisionLevel)
{
    return Mesh();
}

VertexMeshFactory::Mesh VertexMeshFactory::MakeCylinderMesh(
    float bottomRadius, float topRadius, float height, UINT32 sliceCount, UINT stackCount)
{
    return Mesh();
}

void VertexMeshFactory::Subdivide(Mesh& mesh)
{
}

VertexMeshFactory::Vertex VertexMeshFactory::MiddlePoint(const Vertex& v0, const Vertex& v1)
{
    return Vertex();
}

void VertexMeshFactory::MakeCylinderTopCap(
    float bottomRadius, float topRadius, float height, UINT32 sliceCount, UINT32 stackCount, Mesh& mesh)
{
}

void VertexMeshFactory::MakeCylinderBottomCap(
    float bottomRadius, float topRadius, float height, UINT32 sliceCount, UINT32 stackCount, Mesh& mesh)
{
}
