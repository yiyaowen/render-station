/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include "widgets/timer.h"

void initTimer(Timer* pTimer);

void tickTimer(Timer* pTimer);

// Note this func should be called every frame/tick.
void updateRenderWindowCaptionTimerInfo(HWND hWnd, const Timer* pTimer);