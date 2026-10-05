#pragma once
#include "TranslationTypes.h"

namespace dTranslate::Translation
{
    class GoogleTranslateService
    {
    public:
        static GoogleTranslateService& Instance();

        TranslationResult Translate(TranslationRequest const& request);

    private:
        GoogleTranslateService() = default;
        TranslationResult TranslateWithInstantApi(TranslationRequest const& request);
        TranslationResult TranslateWithCloudApi(TranslationRequest const& request, std::wstring const& apiKey);
    };
}
