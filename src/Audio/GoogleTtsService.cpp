#include "pch.h"
#include "GoogleTtsService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include <fstream>
#include <filesystem>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

namespace dTranslate::Audio
{
    using namespace dTranslate::Networking;

    GoogleTtsService& GoogleTtsService::Instance()
    {
        static GoogleTtsService instance;
        return instance;
    }

    void GoogleTtsService::Stop()
    {
        mciSendStringW(L"stop dtrans_tts", nullptr, 0, nullptr);
        mciSendStringW(L"close dtrans_tts", nullptr, 0, nullptr);
    }

    bool GoogleTtsService::Speak(std::wstring const& text, std::wstring const& langCode)
    {
        if (text.empty()) return false;

        // Stop any currently playing audio
        Stop();

        std::wstring tl = langCode.empty() || langCode == L"auto" ? L"en" : langCode;
        std::wstring encodedText = UrlEncode(text);

        std::wstring url = L"https://translate.google.com/translate_tts?ie=UTF-8&tl=" +
            tl + L"&client=tw-ob&q=" + encodedText;

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"User-Agent", L"dTranslate/1.0 (Windows 11)" },
            { L"Referer", L"https://translate.google.com/" }
        };

        auto response = HttpClient::Instance().Get(url, headers);

        if (!response.IsSuccess() || response.body.empty())
        {
            return false;
        }

        try
        {
            auto tempFile = std::filesystem::temp_directory_path() / L"dtranslate_tts.mp3";
            std::ofstream out(tempFile, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) return false;

            out.write(response.body.data(), response.body.size());
            out.close();

            std::wstring openCmd = L"open \"" + tempFile.wstring() + L"\" type mpegvideo alias dtrans_tts";
            mciSendStringW(openCmd.c_str(), nullptr, 0, nullptr);
            mciSendStringW(L"play dtrans_tts", nullptr, 0, nullptr);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
}
