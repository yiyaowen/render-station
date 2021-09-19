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

class BlurFilter : public BasicProcess {
public:
    BlurFilter(D3DCore* pCore, int blurRadius, float blurGrade, int blurCount);

    void init() override;

    void onResize(UINT w, UINT h) override;

    ID3D12Resource* process(ID3D12Resource* flatOrigin) override;

    inline int blurRadius() { return _blurRadius; }
    inline void setBlurRadius(int r) { _blurRadius = r; }

    inline float blurGrade() { return _blurGrade; }
    inline void setBlurGrade(float g) { _blurGrade = g; }

    inline int blurCount() { return _blurCount; }
    inline void setBlurCount(int c) { _blurCount = c; }

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
    int _blurRadius = 0;
    float _blurGrade = 0.0f;
    int _blurCount = 0;

    std::unique_ptr<Shader> s_gaussianBlurHorz = nullptr;
    std::unique_ptr<Shader> s_gaussianBlurVert = nullptr;

    ComPtr<ID3D12DescriptorHeap> texDescHeap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE
        texA_SrvCPU = {}, texA_UavCPU = {},
        texB_SrvCPU = {}, texB_UavCPU = {};
    D3D12_GPU_DESCRIPTOR_HANDLE
        texA_SrvGPU = {}, texA_UavGPU = {},
        texB_SrvGPU = {}, texB_UavGPU = {};
};