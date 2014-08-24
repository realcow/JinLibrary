#pragma once
#include "WindowsHandleWrapper.h"
#include <string>

/**
    @brief  주어진 프로그램을 시작 프로그램으로 만드는 기능을 제공하는 클래스
*/
namespace jin 
{
    namespace win 
    {
        namespace StartupRegister
        {
            enum class Status : int
            {
                SuccessSame = 3,
                SuccessNotRegistered = 2,
                SuccessRegistered = 1,
                Success = 0,
                Failed = -1,
                FailedInCreatingProcess = -2,
                InvalidXMLStructure = -3,
                FailedInSavingFile = -4,
                FailedInequalValue = -5,
            };

            /// @name   Run Registry 등록 방식
            BOOL isRegisteredToRunRegistry(const std::wstring& valueName, const std::wstring exePath);

            Status registerToRunRegistry(const std::wstring& valueName, const std::wstring& exePath);

            BOOL unregisterToRunRegistry(const std::wstring& valueName);

            /// @name   Task Scheduler 등록 방식
            Status isRegisteredToTaskScheduler(const std::wstring& taskname, const std::wstring& exePath);

            Status registerToTaskScheduler(const std::wstring& taskname, const std::wstring& exePath);

            Status unregisterToTaskScheduler(const std::wstring& taskname);

            /// @name   시작 프로그램 등록 방식
            Status registerByStartProgram();
        };
    }
}