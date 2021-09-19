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

class BilateralBlur : public BasicProcess {
public:
    BilateralBlur(D3DCore* pCore, int blurRadius, float distanceGrade, float grayGrade, int blurCount);

    void init() override;

    void onResize(UINT w, UINT h) override;

    ID3D12Resource* process(ID3D12Resource* flatOrigin) override;

    inline int blurRadius() { return _blurRadius; }
    inline void setBlurRadius(int r) { _blurRadius = r; }

    inline float distanceGrade() { return _distanceGrade; }
    inline void setDistanceGrade(float g) { _distanceGrade = g; }

    inline float grayGrade() { return _grayGrade; }
    inline void setGrayGrade(float g) { _grayGrade = g; }

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
    float _distanceGrade = 0.0f;
    float _grayGrade = 0.0f;
    int _blurCount = 0;

    float _twoSigma2 = 0.0f;

    std::unique_ptr<Shader> s_bilateralBlur = nullptr;

    ComPtr<ID3D12DescriptorHeap> texDescHeap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE
        texA_SrvCPU = {}, texA_UavCPU = {},
        texB_SrvCPU = {}, texB_UavCPU = {};
    D3D12_GPU_DESCRIPTOR_HANDLE
        texA_SrvGPU = {}, texA_UavGPU = {},
        texB_SrvGPU = {}, texB_UavGPU = {};
};