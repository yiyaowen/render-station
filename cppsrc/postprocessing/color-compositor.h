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

class ColorCompositor : public BasicProcess {
public:
    ColorCompositor(D3DCore* pCore);

    void init() override;

    void onResize(UINT w, UINT h) override;

    ID3D12Resource* process(ID3D12Resource* flatOrigin) override;

    // Weight should range from 0.0f ~ 1.0f. Result = Bkgn(weight) MIX ORIGIN(1-weight).
    void bindBackgroundPlate(ID3D12Resource* bkgn, int mixType, float weight);

    inline float bkgnWeight() { return _weight; }
    inline float setBkgnWeight(float value) { _weight = value; }

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
    std::unique_ptr<Shader> s_mixer = nullptr;

    ComPtr<ID3D12DescriptorHeap> texDescHeap = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE
        texA_SrvCPU = {}, texB_SrvCPU = {}, texC_UavCPU = {};
    D3D12_GPU_DESCRIPTOR_HANDLE
        texA_SrvGPU = {}, texB_SrvGPU = {}, texC_UavGPU = {};

    int _mixType = ADD;
    float _weight = 0.0f;

public:
    constexpr static int ADD = 0;
    constexpr static int MULTIPLY = 1;
    constexpr static int LERP = 2;
};