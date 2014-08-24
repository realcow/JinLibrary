#pragma once
#include <memory>
#include <windows.h>

/**
    �ڵ��Ҹ�Ǵ� HKEY
    regkey(HKEY, ::RegCloseKey) �� ���
*/
typedef std::unique_ptr<HKEY__, decltype(&::RegCloseKey)> regkey;

/**
    �ڵ��Ҹ�Ǵ� HANDLE
    winhandle(HANDLE, ::CloseHandle) �� ���
*/
typedef std::unique_ptr<void, decltype(&::CloseHandle)> winhandle;