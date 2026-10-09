#pragma once
#include "TranslationTypes.h"
#include <chrono>
#include <mutex>
#include <string>

namespace dTranslate::Translation
{
    class DeepLWebTranslateService
    {
    public:
        static DeepLWebTranslateService& Instance();

        TranslationResult Translate(TranslationRequest const& request);

        static std::wstring MapTargetLanguage(std::wstring const& code);
        static std::wstring MapSourceLanguage(std::wstring const& code);
        bool IsInCooldown();

    private:
        DeepLWebTranslateService();

        TranslationResult TranslateSingleChunk(TranslationRequest const& request);

        void RecordSuccess();
        void RecordThrottled(int retryAfterSeconds = 0);

        std::mutex m_mutex;
        std::chrono::steady_clock::time_point m_cooldownUntil{};
        int m_consecutive429{ 0 };
        std::string m_instanceId;
        std::string m_sessionId;
    };
}
