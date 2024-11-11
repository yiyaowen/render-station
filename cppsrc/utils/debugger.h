/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <comdef.h>
#include <initguid.h> // MUST include before dxgidebug.h
#include <dxgidebug.h>
#include <string>
#include <windows.h>
#pragma warning(disable:4996) // Disable insecure func warning.

#define popupDebugWnd(ERR_STR) do { \
    wchar_t fileStr[512]; \
    MultiByteToWideChar(CP_ACP, 0, __FILE__, -1, fileStr, 512); \
    std::wstring debugStr = std::wstring{ERR_STR} + L", in file: " + std::wstring{fileStr} + L", line " + std::to_wstring(__LINE__); \
    MessageBox(NULL, debugStr.c_str(), L"Error", MB_OK | MB_ICONERROR); \
} while (0)

#define checkNull(VALUE) do { \
    if ((VALUE) == NULL) { \
        std::string errStrA = "Unexpected NULL value: " #VALUE; \
        wchar_t errStr[512]; \
        MultiByteToWideChar(CP_ACP, 0, errStrA.c_str(), -1, errStr, 512); \
        popupDebugWnd(errStr); \
        exit(1); \
    } \
} while (0)

#define checkHR(HR) do { \
    if (FAILED(HR)) { \
        _com_error err(HR); \
        std::wstring hrDesc = err.ErrorMessage(); \
        wchar_t errStr[512]; \
        wsprintf(errStr, L"HR Code: 0x%8x, details: %ws", HR, hrDesc.c_str()); \
        popupDebugWnd(errStr); \
        exit(1); \
    } \
} while (0)