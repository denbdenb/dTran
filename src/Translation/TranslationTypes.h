#pragma once
#include <string>

namespace dTranslate::Translation
{
    struct TranslationRequest
    {
        std::wstring text;
        std::wstring sourceLang{ L"auto" };
        std::wstring targetLang{ L"en" };
    };

    struct TranslationResult
    {
        bool success{ false };
        std::wstring originalText;
        std::wstring translatedText;
        std::wstring detectedLanguage;
        std::wstring serviceName;
        std::wstring errorMessage;
    };
}
