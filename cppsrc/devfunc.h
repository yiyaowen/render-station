/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "d3dcore.h"

void dev_initCoreElems(D3DCore* pCore);

void dev_updateCoreObjConsts(D3DCore* pCore);

void dev_updateCoreProcConsts(D3DCore* pCore);

void dev_updateCoreMatConsts(D3DCore* pCore);

void dev_updateCoreData(D3DCore* pCore);

void dev_drawCoreElems(D3DCore* pCore);

void dev_onMouseDown(WPARAM btnState, int x, int y, D3DCore* pCore);

void dev_onMouseUp(WPARAM btnState, int x, int y, D3DCore* pCore);

void dev_onMouseMove(WPARAM btnState, int x, int y, D3DCore* pCore);