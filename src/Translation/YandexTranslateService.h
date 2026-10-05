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
    };
}
