/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <Windows.h>
#include <wrl.h>
using namespace DirectX;
using namespace Microsoft::WRL;

struct SubVertexMesh {
    UINT indexCount = 0;
    UINT startIndexLocation = 0;
    int baseVertexLocation = 0;
};

struct VertexMesh {
    std::string name;

    ComPtr<ID3DBlob> vertexBufferInCPU = nullptr;
    ComPtr<ID3DBlob> indexBufferInCPU = nullptr;

    ComPtr<ID3D12Resource> vertexBufferInGPU = nullptr;
    ComPtr<ID3D12Resource> indexBufferInGPU = nullptr;

    ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;
    ComPtr<ID3D12Resource> indexBufferUploader = nullptr;

    // Information about vertex buffer and index buffer
    UINT vertexBufferByteStride = 0;
    UINT vertexBufferByteSize = 0;
    DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;
    UINT indexBufferByteSize = 0;

    // A VertexMesh can store multiple geometries in one vertex/index buffer
    // Each SubVertexMesh presents one geometry object in parent VertexMesh
    std::unordered_map<std::string, SubVertexMesh> subMeshes;

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const {
        D3D12_VERTEX_BUFFER_VIEW vbv;
        vbv.BufferLocation = vertexBufferInGPU->GetGPUVirtualAddress();
        vbv.StrideInBytes = vertexBufferByteStride;
        vbv.SizeInBytes = vertexBufferByteSize;
        return vbv;
    }

    D3D12_INDEX_BUFFER_VIEW IndexBufferView() const {
        D3D12_INDEX_BUFFER_VIEW ibv;
        ibv.BufferLocation = indexBufferInGPU->GetGPUVirtualAddress();
        ibv.Format = indexFormat;
        ibv.SizeInBytes = indexBufferByteSize;
        return ibv;
    }
};

class VertexMeshFactory {
public:
    struct Vertex {
        Vertex() : position(0,0,0), normal(0,0,0), tangent(0,0,0) {}
        Vertex(const XMFLOAT3& position, const XMFLOAT3& normal, const XMFLOAT3& tangent)
            : position(position), normal(normal), tangent(tangent) {}
        Vertex(
            float px, float py, float pz,
            float nx, float ny, float nz,
            float tx, float ty, float tz) :
            position(px, py, pz), normal(nx, ny, nz), tangent(tx, ty, tz) {}

        XMFLOAT3 position;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        std::vector<UINT32> indices32;

        std::vector<UINT16>& Indices16() {
            if (indices16.empty()) {
                indices16.resize(indices32.size());
                for (size_t i = 0; i < indices32.size(); ++i) {
                    indices16[i] = (UINT16)(indices32[i]);
                }
            }
        }
        
    private:
        std::vector<UINT16> indices16;
    };

    Mesh MakeCubeMesh(float width, float height, float depth, UINT32 subdivisionLevel);

    Mesh MakeSphereMesh(float radius, UINT32 sliceCount, UINT32 stackCount);

    Mesh MakeGeometrySphereMesh(float radius, UINT32 subdivisionLevel);

    Mesh MakeCylinderMesh(float bottomRadius, float topRadius, float height, UINT32 sliceCount, UINT stackCount);

private:
    void Subdivide(Mesh& mesh);
    Vertex MiddlePoint(const Vertex& v0, const Vertex& v1);

    void MakeCylinderTopCap(float bottomRadius, float topRadius, float height, UINT32 sliceCount, UINT32 stackCount, Mesh& mesh);
    void MakeCylinderBottomCap(float bottomRadius, float topRadius, float height, UINT32 sliceCount, UINT32 stackCount, Mesh& mesh);
};