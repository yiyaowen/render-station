/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include <windows.h>
#include <string>

#include "timer-utils.h"

void initTimer(Timer* pTimer) {
    __int64 countsPerSec;
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
    pTimer->secsPerCount = 1.0 / (double)countsPerSec;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&pTimer->baseTickCount));
    pTimer->prevTickCount = pTimer->baseTickCount;
}

void tickTimer(Timer* pTimer) {
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&pTimer->currTickCount));
    // On a multiprocessor computer, it should not matter which processor is called. However, you can get
    // different results on different processors dur to bugs in the basic input/output system (BIOS) or
    // the hardware abstraction layer (HAL). Besides, the results may also fluctuate when the computer is
    // in energy-saving mode. To sum up, currTickCount might be less than prevTickCount.
    double currElapsedSecs = (pTimer->currTickCount - pTimer->baseTickCount) * pTimer->secsPerCount;
    pTimer->deltaSecs = currElapsedSecs - pTimer->elapsedSecs;
    pTimer->elapsedSecs = currElapsedSecs;
    pTimer->prevTickCount = pTimer->currTickCount;
}
