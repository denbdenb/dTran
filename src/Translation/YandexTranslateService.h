#pragma once
#include "TranslationTypes.h"

namespace dTranslate::Translation
{
    class YandexTranslateService
    {
    public:
        static YandexTranslateService& Instance();

        TranslationResult Translate(TranslationRequest const& request);

    private:
        YandexTranslateService() = default;

        TranslationResult TranslateSingleChunk(TranslationRequest const& request);
        TranslationResult TranslateViaBrowserApi(TranslationRequest const& request);
        TranslationResult TranslateViaMozhi(TranslationRequest const& request, std::wstring const& host);
        TranslationResult TranslateViaTrayslate(TranslationRequest const& request);
    };
}
