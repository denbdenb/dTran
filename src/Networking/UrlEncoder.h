#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <windows.h>

namespace dTranslate::Networking
{
    inline std::string ToUtf8(std::wstring const& wstr)
    {
        if (wstr.empty()) return {};
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string result(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), sizeNeeded, nullptr, nullptr);
        return result;
    }

    inline std::wstring FromUtf8(std::string const& str)
    {
        if (str.empty()) return {};
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
        std::wstring result(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded);
        return result;
    }

    inline std::wstring UrlEncode(std::wstring const& input)
    {
        std::string utf8 = ToUtf8(input);
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : utf8)
        {
            unsigned char uc = static_cast<unsigned char>(c);
            if ((uc >= 'a' && uc <= 'z') ||
                (uc >= 'A' && uc <= 'Z') ||
                (uc >= '0' && uc <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~')
            {
                escaped << c;
            }
            else
            {
                escaped << '%' << std::uppercase << std::setw(2) << static_cast<int>(uc);
            }
        }
        return FromUtf8(escaped.str());
    }
}
