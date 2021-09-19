/*
** Render Station @ https://gitee.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

struct Timer {
    double secsPerCount = 0.0;

    __int64 baseTickCount = 0;
    __int64 prevTickCount = 0;
    __int64 currTickCount = 0;

    double elapsedSecs = 0.0; // Total elapsed time from render start to now.
    double deltaSecs = 0.0; // Elapsed time of last frame.
};