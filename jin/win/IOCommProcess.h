#pragma once
#include "CommonDef.h"
#include <windows.h>

/**
    @brief  intput, outut 핸들이 리다이렉트된 프로세스

    콘솔 어플리케이션을 컨트롤할 때 쓸 수 있다.

    참고자료 : "Creating a Child Process with Redirected Input and Output" http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499(v=vs.85).aspx
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

    /// @brief  stdout으로 출력된 버퍼를 얻음
    BOOL read(char* buff, DWORD& readSize);

    /// @brief  프로세스가 종료될 때까지 버퍼에 모두 읽음
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