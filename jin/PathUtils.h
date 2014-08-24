#pragma once

#include <string>

namespace PathUtils
{
    inline void NestSpacedPath(std::wstring& path)
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
        NestSpacedPath(ret);
        return ret;
    }
}