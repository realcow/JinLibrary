#pragma once
#include <memory>
#include <windows.h>

/**
    자동소멸되는 HKEY
    regkey(HKEY, ::RegCloseKey) 로 사용
*/
typedef std::unique_ptr<HKEY__, decltype(&::RegCloseKey)> regkey;

/**
    자동소멸되는 HANDLE
    winhandle(HANDLE, ::CloseHandle) 로 사용
*/
typedef std::unique_ptr<void, decltype(&::CloseHandle)> winhandle;