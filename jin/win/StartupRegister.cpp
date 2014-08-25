#include "StartupRegister.h"
#include "PathPicker.h"
#include "string_cast.h"
#include "IOCommProcess.h"
#include <third-party/tinyxml/tinyxml.h>
#include <mbstring.h>
#include <string>

using namespace std;
using namespace jin::win::StartupRegister;

namespace
{
    enum class XmlJob
    {
        XML_JOB_ADD_WORKING_DIRECTORY_ELEMENT,
        XML_JOB_CHECK_TEXT,
    };
    Status createProcess(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, bool waitForExit = false);

    /// @brief  xml내용을 버퍼로 받아 WorkingDirectory Element를 추가한 뒤 파일로 출력
    Status doTaskSchedulerXmlJob(char* xmlContents, const char* elementName, const char* elementText, const char* xmlOutputFilename, XmlJob job);

    /// @brief  Task 설정 XML 조회
    Status queryTaskToXML(const std::wstring& taskname, std::unique_ptr<char>& xmlBuff, DWORD& xmlContentSize);

    regkey getRunRegistryKey();

    std::wstring getTaskSchedulerPath();

    /// @brief  경로에 공백이 포함되어 있는 경우 "로 묶어줌. 이미 묶여 있으면 아무것도 안함
    void nestSpacedPath(std::wstring& path);
}

namespace jin { namespace win 
{
    namespace StartupRegister
    {
        /**
            Task Scheduler(작업 스케쥴러)에 등록하는 방식.

            보통 프로그램은 Working Directory가 자신이 존재하는 경로라고 기대하는 경우가 많다.
            하지만 Task Scheduler로 실행되는 프로그램은 Working Directory가 실행파일이 있는 경로가 아니다. 때문에 오동작할 가능성이 있다.
            그래서 Working Directory를 실행파일이 있는 경로로 바꾸어주는데, 이 설정은 커맨드 라인 명령어로 할 수 없고 XML파일을 통해야만 가능하다.(Win8까지 기준)
            그래서 현재 설정 내역을 XML로 받아오고 Working Directory 항목을 추가하는 작업을 해준다.

            참고자료 : http://stackoverflow.com/questions/447774/specifying-the-running-directory-for-scheduled-tasks-using-schtasks-exe
        */
        Status registerToTaskScheduler(const std::wstring& taskname, const std::wstring& exePath)
        {
            wstring taskSchedulerPath = getTaskSchedulerPath();

            // Task 생성
            wstring commandLine = L" /create /sc onlogon /rl highest /f /tn ";
            wstring nestedPath = exePath;
            nestSpacedPath(nestedPath);
            commandLine.append(taskname).append(L" /tr ").append(nestedPath);

            unique_ptr<WCHAR, decltype(&free)> modifiableCommandLine(_wcsdup(commandLine.c_str()), free); // CreateProcessW의 CommandLine은 수정가능하므로 수정가능한 버퍼 만들어서 사용
            if ((int)createProcess(taskSchedulerPath.data(), modifiableCommandLine.get(), true) < 0)
            {
                return Status::FailedInCreatingProcess;
            }

            unique_ptr<char> xmlBuff;
            DWORD dwRead;

            if ((int)queryTaskToXML(taskname, xmlBuff, dwRead) < 0)
            {
                return Status::Failed;
            }

            // ascii 문자열로 변환 & "실행파일이 있는 경로" 만듦
#define TASK_XML_FILENAME "Task.xml"
#define TASK_XML_FILENAMEW L"Task.xml"
            ::DeleteFile(TASK_XML_FILENAMEW);
            string exePathA = string_cast<string>(exePath);
            exePathA.erase(exePathA.rfind(L'\\'), exePathA.npos);

            // WorkingDirectory 항목을 "실행파일이 있는 경로"로 만들기 위해 XML 내용 수정.
            doTaskSchedulerXmlJob(xmlBuff.get(), "WorkingDirectory", exePathA.c_str(), TASK_XML_FILENAME, XmlJob::XML_JOB_ADD_WORKING_DIRECTORY_ELEMENT);

            // 변경된 XML파일로 작업 다시 생성
            (commandLine = L" /create /tn ").append(taskname).append(L" /f /xml ").append(TASK_XML_FILENAMEW);
            modifiableCommandLine.reset(_wcsdup(commandLine.c_str()));

            if ((int)createProcess(taskSchedulerPath.c_str(), modifiableCommandLine.get(), true) < 0)
            {
                ::DeleteFile(TASK_XML_FILENAMEW);
                return Status::FailedInCreatingProcess;
            }
            ::DeleteFile(TASK_XML_FILENAMEW);
            return Status::Success;
        }

        Status deregisterFromTaskScheduler(const std::wstring& taskname)
        {
            wstring taskSchedulerPath = getTaskSchedulerPath();

            // Task 생성
            wstring commandLine = L" /delete /f /tn ";
            commandLine.append(taskname);
            unique_ptr<WCHAR, decltype(&free)> modifiableCommandLine(_wcsdup(commandLine.c_str()), free);
            if ((int)createProcess(taskSchedulerPath.data(), modifiableCommandLine.get(), true) < 0)
            {
                return Status::FailedInCreatingProcess;
            }
            return Status::Success;
        }

        Status registerToRunRegistry(const std::wstring& valueName, const std::wstring& exePath)
        {
            regkey runkey = getRunRegistryKey();
            const size_t BUFFER_LENGTH = 512;

            // 현재 프로세스 파일명을 run 레지스트리에 등록
            wstring value = exePath;
            nestSpacedPath(value);
            _RPT1(_CRT_WARN, "%S", value.c_str());

            LONG ret = ::RegSetValueEx(runkey.get(), valueName.data(), 0, REG_SZ, (BYTE*)value.c_str(), (value.length() + 1) * sizeof(WCHAR));
            if (ret != ERROR_SUCCESS)
            {
                _RPT1(_CRT_WARN, "%x", ret);
                return Status::Failed;
            }
            return Status::Success;
        }

        /**
            Run 키에 값이 등록되어 있는지 확인한다. 등록되어 있다면 올바른 값이 맞는지 확인한다.
        */
        BOOL isRegisteredToRunRegistry(const std::wstring& valueName, const std::wstring exePath)
        {
            regkey runkey = getRunRegistryKey();

            // 값이 존재하는지 확인
            const size_t BUFFER_LENGTH = 512;
            DWORD type, bufferSize = BUFFER_LENGTH * sizeof(WCHAR);
            std::unique_ptr<WCHAR> storedValue(new WCHAR[BUFFER_LENGTH]);

            LONG ret = ::RegQueryValueEx(runkey.get(), valueName.data(), 0, &type, (LPBYTE)storedValue.get(), &bufferSize);

            if (ERROR_FILE_NOT_FOUND == ret || type != REG_SZ) return FALSE;

            // 등록된 값이 맞는지 비교
            wstring correctValue = exePath;
            nestSpacedPath(correctValue);

            if (_wcsicmp(storedValue.get(), correctValue.c_str()) != 0)
            {
                _RPT2(_CRT_WARN, "existing reg value : %S\ncorrect value : %S\n", storedValue.get(), correctValue.c_str());
                return FALSE;
            }
            return TRUE;
        }

        BOOL deregisterFromRunRegistry(const std::wstring& valueName)
        {
            regkey runkey = getRunRegistryKey();
            if (ERROR_SUCCESS == ::RegDeleteValue(runkey.get(), valueName.c_str()))
                return TRUE;
            return FALSE;
        }

        /**
            조회한 작업이 있는 경우 출력 예제 (Win7 x64)

            C:\Users\JeongJinwoo>schtasks /query /tn searchexpress

            폴더: \
            작업 이름                                다음 실행 시간         상태
            ======================================== ====================== ===============
            searchexpress                            N/A                    준비

            조회한 작업이 없는 경우 출력 예제 (Win7 x64)

            C:\Users\JeongJinwoo>schtasks /query /tn babo
            오류: 지정된 파일을 찾을 수 없습니다.
        */
        Status isRegisteredToTaskScheduler(const std::wstring& taskname, const std::wstring& exePath)
        {
            wstring taskSchedulerPath = getTaskSchedulerPath();
            Status ret;

            unique_ptr<char> xmlBuff;
            DWORD dwRead;

            if (((int)(ret = queryTaskToXML(taskname, xmlBuff, dwRead))) < 0)
            {
                return ret;
            }

            string exePathA = string_cast<string>(exePath);

            // Command element가 제대로 등록되어 있는지 확인
            ret = doTaskSchedulerXmlJob(xmlBuff.get(), "Command", exePathA.c_str(), nullptr, XmlJob::XML_JOB_CHECK_TEXT);
            if ((int)ret < 0)
            {
                return Status::SuccessNotRegistered;
            }

            // WorkingDirectory가 제대로 등록되어 있는지 확인
            exePathA.erase(exePathA.rfind(L'\\'), exePathA.npos);
            ret = doTaskSchedulerXmlJob(xmlBuff.get(), "WorkingDirectory", exePathA.c_str(), nullptr, XmlJob::XML_JOB_CHECK_TEXT);
            if ((int)ret < 0)
            {
                return Status::SuccessNotRegistered;
            }
            return Status::SuccessRegistered;
        }
    }
}}

namespace
{
    /**
        대부분의 인자를 생략한 CreateProcess 호출.
        *TODO: Process 클래스로 대체

        @param[in]      lpApplicationName   실행파일 전체경로 혹은 부분경로. 부분경로의 경우 현재 디렉토리 대상(그 외 다른경로(search path)에서 찾지 않음)
        @param[inout]   lpCommandLine       커맨드 라인. 생성될 프로세스가 수정할 수도 있다.
    */
    Status createProcess(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, bool waitForExit/* = false*/)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Start the child process.
        if (!::CreateProcess(lpApplicationName, // No module name (use command line)
                             lpCommandLine,     // Command line
                             nullptr,           // Process handle not inheritable
                             nullptr,           // Thread handle not inheritable
                             FALSE,             // Set handle inheritance to FALSE
                             0,                 // No creation flags
                             nullptr,           // Use parent's environment block
                             nullptr,           // Use parent's starting directory
                             &si,               // Pointer to STARTUPINFO structure
                             &pi))              // Pointer to PROCESS_INFORMATION structure
        {
            printf("CreateProcess failed (%d).\n", GetLastError());
            return Status::Failed;
        }

        if (waitForExit)
        {
            // Wait until child process exits.
            ::WaitForSingleObject(pi.hProcess, INFINITE);
        }

        // Close process and thread handles.
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);

        return Status::Success;
    }

    Status doTaskSchedulerXmlJob(char* xmlContents, const char* elementName, const char* elementText, const char* xmlOutputFilename, XmlJob job)
    {
        TiXmlDocument doc;

        doc.Parse(xmlContents);
        if (doc.Error())
        {
            _RPT3(_CRT_WARN, "%d:%d %S", doc.ErrorRow(), doc.ErrorCol(), doc.ErrorDesc());
            return Status::Failed;
        }

        TiXmlHandle hDoc(&doc);
        TiXmlElement* pElem = hDoc.FirstChildElement("Task").FirstChildElement("Actions").FirstChildElement("Exec").ToElement();
        if (!pElem)
        {
            return Status::InvalidXMLStructure;
        }

        // 등록
        if (job == XmlJob::XML_JOB_ADD_WORKING_DIRECTORY_ELEMENT)
        {
            TiXmlElement* elementToAdd = new TiXmlElement(elementName);
            elementToAdd->LinkEndChild(new TiXmlText(elementText));
            pElem->LinkEndChild(elementToAdd);

            if (!doc.SaveFile(xmlOutputFilename))
            {
                return Status::FailedInSavingFile;
            }
        }
        // 값이 일치하는지 확인
        else if (job == XmlJob::XML_JOB_CHECK_TEXT)
        {
            if (_mbsicmp((unsigned char*)pElem->FirstChildElement(elementName)->GetText(), (unsigned char*)elementText) != 0)
            {
                return Status::FailedInequalValue;
            }
            return Status::SuccessSame;
        }
        return Status::Success;
    }

    Status queryTaskToXML(const std::wstring& taskname, unique_ptr<char>& xmlBuff, DWORD& xmlContentSize)
    {
        wstring taskSchedulerPath = getTaskSchedulerPath();
        wstring commandLine = L" /query /xml /tn ";
        commandLine.append(taskname);

        unique_ptr<WCHAR, decltype(&free)> modifiableCommandLine(_wcsdup(commandLine.c_str()), free); // CreateProcessW의 CommandLine은 수정가능하므로 수정가능한 버퍼 만들어서 사용
        modifiableCommandLine.reset(_wcsdup(commandLine.c_str()));

        IOCommProcess process;
        if (process.createProcessWithRedirection(taskSchedulerPath.c_str(), modifiableCommandLine.get()) < 0)
        {
            return Status::FailedInCreatingProcess;
        }

        // XML 내용 redirection으로 받아옴.
        const size_t BUFFSIZE = 4096;
        xmlBuff.reset(new char[BUFFSIZE]);
        xmlContentSize = BUFFSIZE;
        process.readUntilExit(xmlBuff.get(), xmlContentSize);
        xmlBuff.get()[xmlContentSize] = 0;
        process.close();

        // * 버퍼 사이즈 모자랄 경우 처리.

        // XML 내용이 너무 적으면 Task가 등록되어 있지 않다고 판단한다.
        if (xmlContentSize < 100)
        {
            return Status::SuccessNotRegistered;
        }
        return Status::Success;
    }

    regkey getRunRegistryKey()
    {
        HKEY hKeyRun;

        ::RegOpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKeyRun);

        return regkey(hKeyRun, ::RegCloseKey);
    }

    std::wstring getTaskSchedulerPath()
    {
        wstring taskSchedulerPath = jin::win::PathPicker::GetSystemDirectory(true);
        taskSchedulerPath += L"schtasks.exe";
        return taskSchedulerPath;
    }

    void nestSpacedPath(std::wstring& path)
    {
        if (path.find(L' ') != path.npos &&
            path.front() != L'\"' && path.back() != L'\"')
        {
            path.insert(0, L"\"");
            path.append(L"\"");
        }
    }
}