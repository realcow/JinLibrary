#pragma once
#include "CommonDef.h"
#include <windows.h>

/**
    @brief  intput, outut �ڵ��� �����̷�Ʈ�� ���μ���

    �ܼ� ���ø����̼��� ��Ʈ���� �� �� �� �ִ�.

    �����ڷ� : "Creating a Child Process with Redirected Input and Output" http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx
*/
class IOCommProcess
{
public:
    enum Status
    {
    	Success                         =  0,
    	Failed                          = -1,
        FailedWithCreateProcessAPI      = -2,
    };

    IOCommProcess();
    ~IOCommProcess();

    Status createProcessWithRedirection(LPCWSTR lpApplicationName, LPWSTR lpCommandLine);

    /// @brief  stdout���� ��µ� ���۸� ����
    BOOL read(char* buff, DWORD& readSize);

    /// @brief  ���μ����� ����� ������ ���ۿ� ��� ����
    BOOL readUntilExit(char* buff, DWORD& readSize);

    BOOL write(char* buff, DWORD& writeSize);

    BOOL waitUntilExit();

    BOOL close();

private:
    HANDLE hChildProcess_;
    HANDLE hChildStdoutRead_, hChildStdoutWrite_;
    HANDLE hChildStdinRead_, hChildStdinWrite_;

    DISALLOW_COPY_AND_ASSIGN(IOCommProcess);
};