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
#include <unordered_map>
#include <wrl.h>
using namespace Microsoft::WRL;

struct D3DCore; // Forward declaration to avoid circular reference.

class BasicProcess {
public:
    BasicProcess(D3DCore* pCore);

    // Called init func immediately after constructed to create off-screen texture resources.
    virtual void init();

    // This func has the same priority with D3DCore resizeSwapBuffs func since the texture
    // resources must be adapted to the swap chain back buffers in pixel format and resource size,
    // which means that it should be called in the main window message loop when resize triggered.
    virtual void onResize(UINT w, UINT h);

    // Return: flat processed output. Remember to copy the output to the real swap chain back buffer.
    virtual ID3D12Resource* process(ID3D12Resource* msaaOrigin);

    inline bool isPrepared() { return _isPrepared; }

protected:
    virtual void createOffscreenTextureResources();

protected:
    // Remember to set _isPrepared to true when calling init func.
    bool _isPrepared = false;

    D3DCore* pCore = nullptr;

    UINT texWidth = 0, texHeight = 0;

    std::unordered_map<std::string, ComPtr<ID3D12Resource>> textures = {};
};