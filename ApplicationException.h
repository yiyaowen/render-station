/*
 * Render Station @ https://github.com/yiyaowen/RenderStation
 *
 * Create fantastic animation and game.
 *
 * yiyaowen (c) 2021 All Rights Reserved.
*/
#pragma once

#include <comdef.h>
#include <exception>
#include <string>
#include <sstream>
#include <Windows.h>

class ApplicationException : public std::exception {
public:
    ApplicationException(const std::string& fileName, int lineNum, HRESULT hr, const std::wstring& descStr)
        : lineNum(lineNum), errorCode(hr), descStr(descStr)
    {
        WCHAR buffer[512];
        MultiByteToWideChar(CP_ACP, 0, fileName.c_str(), -1, buffer, 512);
        this->fileName = std::wstring(buffer);
    }

    std::wstring ToString() const {
        // Convert hr to hex format string
        std::wstring hrStr;
        std::wstringstream hrStrStream;
        hrStrStream << std::hex << errorCode;
        hrStrStream >> hrStr;
        // Add prefix "0x" and padding '0'
        hrStr = L"0x" + std::wstring(min(sizeof(long) * 2 - hrStr.size(), 0), '0') + hrStr;

        // Get string description of error code
        _com_error error(errorCode);
        std::wstring errorMsg = error.ErrorMessage();

        return L"Error Code: " + hrStr + L"\n" + fileName + L", Line "
            + std::to_wstring(lineNum) + L"\nDetails: " + descStr + L"\nError Message: " + errorMsg;
    }

private:
    std::wstring fileName;
    int lineNum;
    HRESULT errorCode;
    std::wstring descStr;
};

#define ThrowIfFailed(x) do { \
    HRESULT hr = (x); \
    if (FAILED(hr)) { throw ApplicationException(__FILE__, __LINE__, hr, L#x); } \
} while (0)