#pragma once
#include "TranslationTypes.h"
#include <chrono>
#include <mutex>
#include <string>

namespace dTranslate::Translation
{
    class GoogleTranslateService
    {
    public:
        static GoogleTranslateService& Instance();

        TranslationResult Translate(TranslationRequest const& request);

    private:
        GoogleTranslateService() = default;
        TranslationResult TranslateSingleChunk(TranslationRequest const& request);
        TranslationResult TranslateWithInstantApi(TranslationRequest const& request);
        TranslationResult TranslateWithFallbackApi(TranslationRequest const& request);

        bool IsPrimaryInCooldown();
        void RecordPrimarySuccess();
        void RecordPrimaryThrottled(int retryAfterSeconds);

        std::mutex m_mutex;
        std::chrono::steady_clock::time_point m_primaryCooldownUntil{};
        int m_primaryConsecutive429{ 0 };
    };
}
