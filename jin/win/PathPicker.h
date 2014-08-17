#pragma once

#include <string>
#include <memory>
#include <windows.h>

namespace jin {
    namespace win {

        /**
            @brief  각종 경로를 std::wstring 형태로 얻는다

            Win API함수의 문자열 연산 스트레스 제거!
            공통사항 - wstring 리턴, 실패시 빈 wstring 리턴.

            - Windows API를 wstring타입을 리턴하게 wrappng한 것.
            - 성능을 염두해두지 않았기 때문에 다소 비효율적으로 짜여진 코드가 있을 수 있음.
        */
        class PathPickerW
        {
        public:
            /**
                @param[in]  trailingDirSign     끝에 \가 필요한지 여부. GetSystemDirectoryW는 끝에 \를 붙이지 않기 때문에 직접 붙여줌.
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
                먼저 길이를 얻고 길이만큼 버퍼를 할당함. ex) "C:\MyDir"
                @param[in]  trailingDirSign     끝에 \가 필요한지 여부. GetCurrentDirectory는 끝에 \를 붙이지 않기 때문에 직접 붙여줌.
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