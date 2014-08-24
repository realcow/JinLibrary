#include "PathUtils.h"
#include <cassert>

void PathUtilsTest()
{
    assert(L"\"C:\\spaced path\\executable.exe\"" == PathUtils::NestSpacedPath(L"C:\\spaced path\\executable.exe"));
    assert(L"\"C:\\spaced path\\executable.exe\"" == PathUtils::NestSpacedPath(L"\"C:\\spaced path\\executable.exe\""));
}