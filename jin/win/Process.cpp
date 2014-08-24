#include "Process.h"
#include <jin/PathUtils.h>
#include <shlwapi.h>
#pragma comment( lib, "shlwapi.lib" )

using namespace std;

namespace jin { namespace win {

Process::Process()
{
    ZeroMemory(&processInformation_, sizeof(processInformation_));
}

Process::~Process()
{
    // Close process and thread handles.   
    if (processInformation_.hThread != 0)
    {
        ::CloseHandle(processInformation_.hThread);
    }

    if (processInformation_.hProcess != 0)
    {
        ::CloseHandle(processInformation_.hProcess);
    }
}

/*
    Reference: CreateProcess function http://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx
*/
bool Process::Start()
{
    if (::PathFileExistsW(StartInfo.FileName.c_str()) == FALSE)
    {
        return false;
    }
    STARTUPINFO si;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    wstring commandLine = StartInfo.FileName;
    
    if (!StartInfo.Arguments.empty())
    {
        commandLine += L" " + StartInfo.Arguments;
    }

    // Start the child process.   
    if (!::CreateProcess(nullptr,                                    // No module name (use command line)  
                         const_cast<WCHAR*>(commandLine.data()),     // Command line  
                         nullptr,                                    // Process handle not inheritable  
                         nullptr,                                    // Thread handle not inheritable  
                         FALSE,                                      // Set handle inheritance to FALSE  
                         0,                                          // No creation flags  
                         nullptr,                                    // Use parent's environment block  
                         nullptr,                                    // Use parent's starting directory   
                         &si,                                        // Pointer to STARTUPINFO structure  
                         &processInformation_))                      // Pointer to PROCESS_INFORMATION structure  
    {  
        return false;
    }  
  
    return true;
}

void Process::WaitForExit()
{
    // Wait until child process exits.  
    ::WaitForSingleObject(processInformation_.hProcess, INFINITE);
}

bool Process::WaitForInputIdle()
{
    ::WaitForInputIdle(processInformation_.hProcess, INFINITE);
    return true;
}

}}
