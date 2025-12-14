#pragma once
#pragma once
#include "pch.h"

#define STRINGFY(s)  #s
#define TO_STRING(x) STRINGFY(x)


namespace Utility
{
    inline std::wstring StringToWString(const std::string& input)
    {
        size_t i;
        wchar_t* buffer = new wchar_t[input.size() + 1];
        mbstowcs_s(&i, buffer, input.size() + 1, input.c_str(), _TRUNCATE);
        std::wstring result = buffer;
        delete[] buffer;
        return result;
    }

    inline std::string WStringToString(const std::wstring& input)
    {
        size_t i;
        char* buffer = new char[input.size() * MB_CUR_MAX + 1];
        wcstombs_s(&i, buffer, input.size() * MB_CUR_MAX + 1, input.c_str(), _TRUNCATE);
        std::string result = buffer;
        delete[] buffer;
        return result;
    }

    inline std::wstring GetCurrentDir()
    {
        WCHAR GetTextureFilePath[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, GetTextureFilePath);
        std::wstring erasePath = GetTextureFilePath;
        erasePath = erasePath.erase(erasePath.length());

        return erasePath;
    }

    //! @brief テクスチャファイル名のみ取り出す
    inline std::wstring FileOnlyName(const std::wstring& path)
    {
        auto idx = path.rfind(L'/');
        if (idx == std::wstring::npos)
        {
            idx = path.rfind(L'\\');
        }
        return path.substr(idx + 1, path.length() - idx - 1);
    }

    inline std::wstring FileExtension(const std::wstring& filePath)
    {
        auto idx = filePath.rfind(L'.');
        return filePath.substr(idx + 1, filePath.length() - idx - 1);
    }

    inline std::wstring ExChangeFileExtension(const std::wstring& filePath)
    {
        auto idx = filePath.rfind(L'.');
        if (filePath.substr(idx + 1, filePath.length() - idx - 1) == L"psd")
        {
            return filePath.substr(0, idx) + L".tga";
        }
        else
            return filePath;
    }
    const static std::wstring windowClassName = L"ModelViewerWindow";
}