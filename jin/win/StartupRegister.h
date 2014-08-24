#pragma once
#include "WindowsHandleWrapper.h"
#include <string>

/**
    @brief  �־��� ���α׷��� ���� ���α׷����� ����� ����� �����ϴ� Ŭ����
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

            /// @name   Run Registry ��� ���
            BOOL isRegisteredToRunRegistry(const std::wstring& valueName, const std::wstring exePath);

            Status registerToRunRegistry(const std::wstring& valueName, const std::wstring& exePath);

            BOOL unregisterToRunRegistry(const std::wstring& valueName);

            /// @name   Task Scheduler ��� ���
            Status isRegisteredToTaskScheduler(const std::wstring& taskname, const std::wstring& exePath);

            Status registerToTaskScheduler(const std::wstring& taskname, const std::wstring& exePath);

            Status unregisterToTaskScheduler(const std::wstring& taskname);

            /// @name   ���� ���α׷� ��� ���
            Status registerByStartProgram();
        };
    }
}