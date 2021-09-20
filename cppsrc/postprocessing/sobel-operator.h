/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <memory>

#include "basic-process.h"
#include "graphics/shader.h"

class SobelOperator : public BasicProcess {
public:
    SobelOperator(D3DCore* pCore);

    void init() override;

    void onResize(UINT w, UINT h) override;

    ID3D12Resource* process(ID3D12Resource* flatOrigin) override;

protected:
    void createOffscreenTextureResources() override;

private:
    // Note the order in which resources, descriptors, descriptor heap are created.
    // First create the resources and prepare the descriptor heap, and then create descriptors.
    // The descriptors need to know the target resources and their stored heap. In other words,
    // they need to know who they are going to bind and where they are going to be stored.

    void createTextureDescriptorHeap();

    void createResourceDescriptors();

private:
    std::unique_ptr<Shader> s_sobelOperator = nullptr;

    ComPtr<ID3D12DescriptorHeap> texDescHeap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE
        texA_SrvCPU = {}, texB_UavCPU = {};
    D3D12_GPU_DESCRIPTOR_HANDLE
        texA_SrvGPU = {}, texB_UavGPU = {};

    constexpr static int BLACK_ON_WHITE = 0;
    constexpr static int WHITE_ON_BLACK = 1;
};