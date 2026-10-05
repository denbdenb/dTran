#pragma once
#include <string>

namespace dTranslate::Dictionary
{
    struct DictionaryRequest
    {
        std::wstring word;
        std::wstring sourceLang{ L"en" };
        std::wstring targetLang{ L"ru" };
    };

    struct DictionaryResult
    {
        bool success{ false };
        std::wstring query;
        std::wstring title;
        std::wstring content;
        std::wstring sourceUrl;
        std::wstring serviceName;
        std::wstring errorMessage;
    };
}
