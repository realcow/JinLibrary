#pragma once

#include <string>
#include <windows.h>
#include <boost/utility.hpp>

namespace jin {
    namespace win {

        /*
            References
                C# Process Class: http://msdn.microsoft.com/library/system.diagnostics.process(v=vs.110).aspx
        */
        class Process : boost::noncopyable
        {
        public:
            struct ProcessStartInfo
            {
                std::wstring FileName;
                std::wstring Arguments;
            };
            Process();
            ~Process();

            bool Start();

            void WaitForExit();

            bool WaitForInputIdle();

            ProcessStartInfo StartInfo;

        private:
            PROCESS_INFORMATION processInformation_;

        };
    }
}