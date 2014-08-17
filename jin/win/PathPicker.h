#pragma once

#include <string>
#include <memory>
#include <windows.h>

namespace jin {
    namespace win {

        /**
            @brief  ���� ��θ� std::wstring ���·� ��´�

            Win API�Լ��� ���ڿ� ���� ��Ʈ���� ����!
            ������� - wstring ����, ���н� �� wstring ����.

            - Windows API�� wstringŸ���� �����ϰ� wrappng�� ��.
            - ������ �����ص��� �ʾұ� ������ �ټ� ��ȿ�������� ¥���� �ڵ尡 ���� �� ����.
        */
        class PathPickerW
        {
        public:
            /**
                @param[in]  trailingDirSign     ���� \�� �ʿ����� ����. GetSystemDirectoryW�� ���� \�� ������ �ʱ� ������ ���� �ٿ���.
            */
            static std::wstring GetSystemDirectory(bool trailingDirSign = false)
            {
                const size_t BUFFER_LENGTH = 256;
                std::unique_ptr<wchar_t> path(new wchar_t[BUFFER_LENGTH]);

                if (0 == ::GetSystemDirectoryW(path.get(), BUFFER_LENGTH))
                {
                    return std::wstring();
                }

                std::wstring ret = path.get();
                if (trailingDirSign)
                {
                    ret += L"\\";
                }
                return ret;
            }

            /**
                ���� ���̸� ��� ���̸�ŭ ���۸� �Ҵ���. ex) "C:\MyDir"
                @param[in]  trailingDirSign     ���� \�� �ʿ����� ����. GetCurrentDirectory�� ���� \�� ������ �ʱ� ������ ���� �ٿ���.
            */
            static std::wstring GetCurrentDirectory(bool trailingDirSign = false)
            {
                DWORD ret = ::GetCurrentDirectoryW(0, nullptr);

                if (ret != 0)
                {
                    std::unique_ptr<wchar_t> path(new wchar_t[ret + (trailingDirSign ? 1 : 0)]);
                    ret = ::GetCurrentDirectoryW(ret, path.get());
                    if (ret != 0)
                    {
                        if (trailingDirSign)
                        {
                            path.get()[ret] = L'\\';
                            path.get()[ret + 1] = L'\0';
                        }
                        return std::wstring(path.get());
                    }
                }
                return std::wstring();
            }

            static std::wstring GetModuleFileName(HMODULE hModule)
            {
                const size_t BUFFER_LENGTH = 256;
                std::unique_ptr<wchar_t> path(new wchar_t[BUFFER_LENGTH]);

                DWORD ret = ::GetModuleFileNameW(hModule, path.get(), BUFFER_LENGTH);
                if (ret == BUFFER_LENGTH)
                {
                    path.reset(new wchar_t[BUFFER_LENGTH * 10]);

                    ret = ::GetModuleFileNameW(hModule, path.get(), BUFFER_LENGTH * 10);

                    if (ret == BUFFER_LENGTH * 10 || ret == 0) return std::wstring();
                }
                // If the function fails, the return value is 0 (zero). To get extended error information, call GetLastError.
                else if (ret == 0)
                {
                    return std::wstring();
                }
                return std::wstring(path.get());
            }

            std::wstring GetFullPathName(std::wstring lpFileName);
        };
    }
}
#ifdef _UNICODE
#define PathPicker PathPickerW
#else
#define PathPicker PathPickerA
#endif