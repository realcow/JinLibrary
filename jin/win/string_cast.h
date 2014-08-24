#pragma once
#include <string>
#include <memory>
#include <mbstring.h>

/**
    참고 : http://codereview.stackexchange.com/questions/1205/c-string-cast-template-function/1466#1466
*/
template <typename To, typename From>
struct string_cast_imp;

/// target type is the same as the source type
template <typename T>
struct string_cast_imp<T, T>
{
    T convert(const T& source)
    {
        return source;
    }
};

/// string -> wstring
template <>
struct string_cast_imp<std::wstring, std::string>
{
    std::wstring convert(const std::string& source)
    {
        size_t charCnt = _mbsnccnt((const unsigned char*)source.c_str(), source.size());
        std::unique_ptr<wchar_t> s(new wchar_t[charCnt + 1]);
        mbstowcs(s.get(), source.c_str(), charCnt + 1);
        return std::wstring(s.get());
    }
};

/// char* -> wstring
// 이 클래스를 꼭 따로둬야 하는가? string 임시객체로 대신할 수 있지 않나
template <>
struct string_cast_imp<std::wstring, const char*>
{
    std::wstring convert(const char* source)
    {
        size_t charCnt = _mbsnccnt((const unsigned char*)source, _mbslen((const unsigned char*)source));
        std::unique_ptr<wchar_t> s(new wchar_t[charCnt + 1]);
        mbstowcs(s.get(), source, charCnt + 1);
        return std::wstring(s.get());
    }
};

// * 이게 왜 굳이 필요한지 모르겠다.
template <>
struct string_cast_imp<std::wstring, char*>
{
    std::wstring convert(char* source)
    {
        return string_cast_imp<std::wstring, const char*>().convert(source);
    }
};

/// wstring -> string
template <>
struct string_cast_imp<std::string, std::wstring>
{
    std::string convert(const std::wstring& source)
    {
        size_t maxByteLength = source.length() * 2;
        std::unique_ptr<char> s(new char[maxByteLength + 1]);
        wcstombs(s.get(), source.c_str(), maxByteLength + 1);
        return std::string(s.get());
    }
};

/// wchar_t* -> string
// 이 클래스를 꼭 따로둬야 하는가? wstring 임시객체로 대신할 수 있지 않나
template <>
struct string_cast_imp<std::string, const wchar_t*>
{
    std::string convert(const wchar_t* source)
    {
        size_t maxByteLength = wcslen(source) * 2;
        std::unique_ptr<char> s(new char[maxByteLength + 1]);
        wcstombs(s.get(), source, maxByteLength + 1);
        return std::string(s.get());
    }
};

// * 마찬가지로 이게 왜 굳이 필요한지 모르겠다.
template <>
struct string_cast_imp<std::string, wchar_t*>
{
    std::string convert(wchar_t* source)
    {
        return string_cast_imp<std::string, const wchar_t*>().convert(source);
    }
};


template <typename To, typename From>
To string_cast(From source)
{
    return string_cast_imp<To, From>().convert(source);
}

//template <typename To, typename From>
//To string_cast(const From& source)
//{
//    return string_cast_imp<To, From>().convert(source);
//}

//template <wstring, const char*>
//To string_cast(const char* source)
//{
//    return string_cast_imp<To, const char*>().convert(source);
//}