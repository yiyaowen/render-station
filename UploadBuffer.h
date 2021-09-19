/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "ApplicationException.h"
#include "d3dx12.h"

inline UINT CalculateConstantBufferByteSize(UINT byteSize) {
    // Constant buffer must be a multiple of the minimum hardware allocate size (usually 256 bytes)
    // The formula below rounds up the byte size to the nearest multiple of 256
    return (byteSize + 255) & ~255;
}

// Create a default buffer filled with initData.  In order to copy CPU memory 
// into the new default buffer, we need an extra intermediate upload heap
ComPtr<ID3D12Resource> MakeInitializedDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
    const void* initData, UINT64 initDataByteSize, ComPtr<ID3D12Resource>& intermediateHeap);

template<typename T>
class UploadBuffer {
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) {

        elementByteSize = sizeof(T);
        if (isConstantBuffer) elementByteSize = CalculateConstantBufferByteSize(sizeof(T));

        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer)));
        
        ThrowIfFailed(uploadBuffer->Map(0, nullptr, (void**)(&mappedDataInGPU)));
    }

    UploadBuffer(const UploadBuffer& buffer) = delete;
    UploadBuffer& operator=(const UploadBuffer& buffer) = delete;
    ~UploadBuffer() {
        if (uploadBuffer != nullptr) uploadBuffer->Unmap(0, nullptr);
        mappedDataInGPU = nullptr;
    }

    ID3D12Resource* Resource() const {
        return uploadBuffer.Get();
    }

    void UploadData(int offsetOfMappedDataInGPU, const T& dataFromCPU) {
        memcpy(&mappedDataInGPU[offsetOfMappedDataInGPU * elementByteSize], &dataFromCPU, sizeof(T));
    }

private:
    UINT elementByteSize = 0;
    ComPtr<ID3D12Resource> uploadBuffer;
    BYTE* mappedDataInGPU = nullptr;
};