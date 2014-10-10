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

    /// @brief  xml������ ���۷� �޾� WorkingDirectory Element�� �߰��� �� ���Ϸ� ���
    Status doTaskSchedulerXmlJob(char* xmlContents, const char* elementName, const char* elementText, const char* xmlOutputFilename, XmlJob job);

    /// @brief  Task ���� XML ��ȸ
    Status queryTaskToXML(const std::wstring& taskname, std::unique_ptr<char>& xmlBuff, DWORD& xmlContentSize);

    regkey getRunRegistryKey();

    std::wstring getTaskSchedulerPath();

    /// @brief  ��ο� ������ ���ԵǾ� �ִ� ��� "�� ������. �̹� ���� ������ �ƹ��͵� ����
    void nestSpacedPath(std::wstring& path);
}

namespace jin { namespace win 
{
    namespace StartupRegister
    {
        /**
            Task Scheduler(�۾� �����췯)�� ����ϴ� ���.

            ���� ���α׷��� Working Directory�� �ڽ��� �����ϴ� ��ζ�� ����ϴ� ��찡 ����.
            ������ Task Scheduler�� ����Ǵ� ���α׷��� Working Directory�� ���������� �ִ� ��ΰ� �ƴϴ�. ������ �������� ���ɼ��� �ִ�.
            �׷��� Working Directory�� ���������� �ִ� ��η� �ٲپ��ִµ�, �� ������ Ŀ�ǵ� ���� ��ɾ�� �� �� ���� XML������ ���ؾ߸� �����ϴ�.(Win8���� ����)
            �׷��� ���� ���� ������ XML�� �޾ƿ��� Working Directory �׸��� �߰��ϴ� �۾��� ���ش�.

            �����ڷ� : http://stackoverflow.com/questions/447774/specifying-the-running-directory-for-scheduled-tasks-using-schtasks-exe
        */
        Status registerToTaskScheduler(const std::wstring& taskname, const std::wstring& exePath)
        {
            wstring taskSchedulerPath = getTaskSchedulerPath();

            // Task ����
            wstring commandLine = L" /create /sc onlogon /rl highest /f /tn ";
            wstring nestedPath = exePath;
            nestSpacedPath(nestedPath);
            commandLine.append(taskname).append(L" /tr ").append(nestedPath);

            unique_ptr<WCHAR, decltype(&free)> modifiableCommandLine(_wcsdup(commandLine.c_str()), free); // CreateProcessW�� CommandLine�� ���������ϹǷ� ���������� ���� ���� ���
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

            // ascii ���ڿ��� ��ȯ & "���������� �ִ� ���" ����
#define TASK_XML_FILENAME "Task.xml"
#define TASK_XML_FILENAMEW L"Task.xml"
            ::DeleteFile(TASK_XML_FILENAMEW);
            string exePathA = string_cast<string>(exePath);
            exePathA.erase(exePathA.rfind(L'\\'), exePathA.npos);

            // WorkingDirectory �׸��� "���������� �ִ� ���"�� ����� ���� XML ���� ����.
            doTaskSchedulerXmlJob(xmlBuff.get(), "WorkingDirectory", exePathA.c_str(), TASK_XML_FILENAME, XmlJob::XML_JOB_ADD_WORKING_DIRECTORY_ELEMENT);

            // ����� XML���Ϸ� �۾� �ٽ� ����
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

            // Task ����
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

            // ���� ���μ��� ���ϸ��� run ������Ʈ���� ���
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
            Run Ű�� ���� ��ϵǾ� �ִ��� Ȯ���Ѵ�. ��ϵǾ� �ִٸ� �ùٸ� ���� �´��� Ȯ���Ѵ�.
        */
        BOOL isRegisteredToRunRegistry(const std::wstring& valueName, const std::wstring exePath)
        {
            regkey runkey = getRunRegistryKey();

            // ���� �����ϴ��� Ȯ��
            const size_t BUFFER_LENGTH = 512;
            DWORD type, bufferSize = BUFFER_LENGTH * sizeof(WCHAR);
            std::unique_ptr<WCHAR> storedValue(new WCHAR[BUFFER_LENGTH]);

            LONG ret = ::RegQueryValueEx(runkey.get(), valueName.data(), 0, &type, (LPBYTE)storedValue.get(), &bufferSize);

            if (ERROR_FILE_NOT_FOUND == ret || type != REG_SZ) return FALSE;

            // ��ϵ� ���� �´��� ��
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
            ��ȸ�� �۾��� �ִ� ��� ��� ���� (Win7 x64)

            C:\Users\JeongJinwoo>schtasks /query /tn searchexpress

            ����: \
            �۾� �̸�                                ���� ���� �ð�         ����
            ======================================== ====================== ===============
            searchexpress                            N/A                    �غ�

            ��ȸ�� �۾��� ���� ��� ��� ���� (Win7 x64)

            C:\Users\JeongJinwoo>schtasks /query /tn babo
            ����: ������ ������ ã�� �� �����ϴ�.
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

            // Command element�� ����� ��ϵǾ� �ִ��� Ȯ��
            ret = doTaskSchedulerXmlJob(xmlBuff.get(), "Command", exePathA.c_str(), nullptr, XmlJob::XML_JOB_CHECK_TEXT);
            if ((int)ret < 0)
            {
                return Status::SuccessNotRegistered;
            }

            // WorkingDirectory�� ����� ��ϵǾ� �ִ��� Ȯ��
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
        ��κ��� ���ڸ� ������ CreateProcess ȣ��.
        *TODO: Process Ŭ������ ��ü

        @param[in]      lpApplicationName   �������� ��ü��� Ȥ�� �κа��. �κа���� ��� ���� ���丮 ���(�� �� �ٸ����(search path)���� ã�� ����)
        @param[inout]   lpCommandLine       Ŀ�ǵ� ����. ������ ���μ����� ������ ���� �ִ�.
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

        // ���
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
        // ���� ��ġ�ϴ��� Ȯ��
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

        unique_ptr<WCHAR, decltype(&free)> modifiableCommandLine(_wcsdup(commandLine.c_str()), free); // CreateProcessW�� CommandLine�� ���������ϹǷ� ���������� ���� ���� ���
        modifiableCommandLine.reset(_wcsdup(commandLine.c_str()));

        IOCommProcess process;
        if (process.createProcessWithRedirection(taskSchedulerPath.c_str(), modifiableCommandLine.get()) < 0)
        {
            return Status::FailedInCreatingProcess;
        }

        // XML ���� redirection���� �޾ƿ�.
        const size_t BUFFSIZE = 4096;
        xmlBuff.reset(new char[BUFFSIZE]);
        xmlContentSize = BUFFSIZE;
        process.readUntilExit(xmlBuff.get(), xmlContentSize);
        xmlBuff.get()[xmlContentSize] = 0;
        process.close();

        // * ���� ������ ���ڶ� ��� ó��.

        // XML ������ �ʹ� ������ Task�� ��ϵǾ� ���� �ʴٰ� �Ǵ��Ѵ�.
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