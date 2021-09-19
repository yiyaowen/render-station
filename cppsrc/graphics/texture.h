/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <d3d12.h>
#include <string>
#include <wrl.h>
using namespace Microsoft::WRL;

struct Texture {
    std::string name;
    size_t srvHeapIdx = 0; // The index should be set when the SRV heap is created.
    ComPtr<ID3D12Resource> resource = nullptr;
    ComPtr<ID3D12Resource> uploadHeap = nullptr;
};