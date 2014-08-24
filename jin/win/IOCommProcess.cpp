#include "IOCommProcess.h"
#include <memory>

using namespace std;

IOCommProcess::IOCommProcess()
    : hChildStdoutRead_(0), hChildStdoutWrite_(0), hChildStdinRead_(0), hChildStdinWrite_(0), hChildProcess_(0)
{}

IOCommProcess::~IOCommProcess()
{
    close();
}

IOCommProcess::Status IOCommProcess::createProcessWithRedirection(LPCWSTR lpApplicationName, LPWSTR lpCommandLine)
{
    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    // Create a pipe for the child process's STDOUT.
    if (!::CreatePipe(&hChildStdoutRead_, &hChildStdoutWrite_, &saAttr, 0))
    {
        return Status::Failed;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!::SetHandleInformation(hChildStdoutRead_, HANDLE_FLAG_INHERIT, 0))
    {
        close();
        return Status::Failed;
    }

    // Create a pipe for the child process's STDIN.
    if (!::CreatePipe(&hChildStdinRead_, &hChildStdinWrite_, &saAttr, 0))
    {
        close();
        return Status::Failed;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if (!::SetHandleInformation(hChildStdinWrite_, HANDLE_FLAG_INHERIT, 0))
    {
        close();
        return Status::Failed;
    }

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;

    ZeroMemory(&piProcInfo, sizeof(piProcInfo));
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));

    siStartInfo.cb = sizeof(siStartInfo);
    siStartInfo.hStdError = siStartInfo.hStdOutput = hChildStdoutWrite_;
    siStartInfo.hStdInput = hChildStdinRead_;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    BOOL ret = ::CreateProcessW(lpApplicationName,
                                lpCommandLine,
                                NULL,          // process security attributes
                                NULL,          // primary thread security attributes
                                TRUE,          // handles are inherited
                                0,             // creation flags
                                NULL,          // use parent's environment
                                NULL,          // use parent's current directory
                                &siStartInfo,  // STARTUPINFO pointer
                                &piProcInfo);  // receives PROCESS_INFORMATION
    if (!ret)
    {
        close();
        return Status::FailedWithCreateProcessAPI;
    }
    hChildProcess_= piProcInfo.hProcess;
    ::CloseHandle(piProcInfo.hThread);
    return Status::Success;
}

BOOL IOCommProcess::read(char* buff, DWORD& readSize)
{
    DWORD ret = ::ReadFile(hChildStdoutRead_, buff, readSize, &readSize, nullptr);
    return ret;
}

/**
    * WaitForSingleObject() ���� read()���̿� stdout�� �ƹ��͵� ������� �ʰ� ����Ǹ�
      read�� ���� �����͸� ������ ����� ���ɼ��� �ִ�. WAIT_INTERVAL�� �ø��� ���ɼ��� ���� �� �ִ�.
*/
BOOL IOCommProcess::readUntilExit(char* buff, DWORD& readSize)
{
    DWORD readSum = 0;
    bool exited = false;
    do
    {
        // ���μ��� ���Ῡ�� üũ
        const DWORD WAIT_INTERVAL = 200;
        DWORD ret = ::WaitForSingleObject(hChildProcess_, WAIT_INTERVAL);
        if (ret == WAIT_OBJECT_0)
        {
            exited = true;
        }
        else if (ret == WAIT_FAILED)
        {
            return FALSE;
        }

        // ������ �о� ���ۿ� ������Ŵ
        DWORD nNumberOfBytesToRead = readSize - readSum;
        if (read(buff + readSum, nNumberOfBytesToRead))
        {
            readSum += nNumberOfBytesToRead;
        }
    }
    while (!exited);
    readSize = readSum;
    return TRUE;
}

BOOL IOCommProcess::waitUntilExit()
{
    DWORD ret = ::WaitForSingleObject(hChildProcess_, INFINITE);
    return ret;
}

BOOL IOCommProcess::close()
{
    if (hChildStdoutRead_)
    {
        ::CloseHandle(hChildStdoutRead_);
        hChildStdoutRead_ = 0;
    }
    if (hChildStdoutWrite_)
    {
        ::CloseHandle(hChildStdoutWrite_);
        hChildStdoutWrite_ = 0;
    }
    if (hChildStdinRead_)
    {
        ::CloseHandle(hChildStdinRead_);
        hChildStdinRead_ = 0;
    }
    if (hChildStdinWrite_)
    {
        ::CloseHandle(hChildStdinWrite_);
        hChildStdinWrite_ = 0;
    }
    if (hChildProcess_)
    {
        ::CloseHandle(hChildProcess_);
        hChildProcess_ = 0;
    }
    return TRUE;
}

BOOL IOCommProcess::write(char* buff, DWORD& writeSize)
{
    DWORD ret = ::WriteFile(hChildStdinWrite_, buff, writeSize, &writeSize, nullptr);
    return ret;
}

