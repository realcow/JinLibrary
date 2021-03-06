#pragma once

#include <string>

namespace jin 
{
    namespace PathUtils
    {
        inline void NestSpacedPathInplace(std::wstring& path)
        {
            if (path.empty()) { return; }
            if (path[0] == L'\"') { return; }

            if (path.find(L' ') != path.npos &&
                path.front() != L'\"' && path.back() != L'\"')
            {
                path.insert(0, L"\"");
                path.append(L"\"");
            }
        }

        inline std::wstring NestSpacedPath(const std::wstring& path)
        {
            std::wstring ret = path;
            NestSpacedPathInplace(ret);
            return ret;
        }
    }
}