#include "Process.h"
#include <jin/win/PathPicker.h>

void ProcessTest()
{
    jin::win::Process notepadProcess;

    notepadProcess.StartInfo.FileName = L"C:\\Windows\\System32\\notepad.exe";
    notepadProcess.StartInfo.Arguments = L" " + jin::win::PathPicker::GetCurrentDirectory() + L"\\..\\test\\data\\test-text.txt";
    notepadProcess.Start();
}