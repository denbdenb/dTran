#include "pch.h"
#include "TranslationManager.h"
#include "GoogleTranslateService.h"
#include "YandexTranslateService.h"
#include "SettingsManager.h"
#include "HistoryManager.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace dTranslate::Translation
{
    using namespace winrt;
    using namespace winrt::Microsoft::UI::Dispatching;
    using namespace dTranslate::Storage;

    TranslationManager& TranslationManager::Instance()
    {
        static TranslationManager instance;
        return instance;
    }

    static std::wstring FormatCurrentTime()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        wchar_t buf[64];
        swprintf_s(buf, L"%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
        return buf;
    }

    winrt::fire_and_forget InternalTranslateWorker(
        TranslationRequest request,
        int serviceIndex,
        DispatcherQueue dispatcher,
        std::function<void(TranslationResult)> onComplete)
    {
        co_await resume_background();

        TranslationResult result;
        auto const& settings = SettingsManager::Instance().GetSettings();

        int selectedService = (serviceIndex >= 0) ? serviceIndex : settings.primaryService;

        switch (selectedService)
        {
        case 0: // Google Translate
            result = GoogleTranslateService::Instance().Translate(request);
            break;

        case 1: // Yandex Translate
            result = YandexTranslateService::Instance().Translate(request);
            break;

        default:
            result = GoogleTranslateService::Instance().Translate(request);
            break;
        }

        // Comparison mode: if user enabled Google vs Yandex comparison in settings
        if (settings.compareTranslations && (selectedService == 0 || selectedService == 1))
        {
            TranslationResult secondaryResult;
            if (selectedService == 0)
            {
                secondaryResult = YandexTranslateService::Instance().Translate(request);
            }
            else
            {
                secondaryResult = GoogleTranslateService::Instance().Translate(request);
            }

            if (secondaryResult.success && !secondaryResult.translatedText.empty())
            {
                result.translatedText += L"\n\n--- Comparison (" + secondaryResult.serviceName + L") ---\n" +
                    secondaryResult.translatedText;
            }
        }

        // Add to history if successful
        if (result.success && !result.translatedText.empty())
        {
            HistoryItem item;
            item.timestamp = FormatCurrentTime();
            item.originalText = request.text;
            item.translatedText = result.translatedText;
            item.sourceLang = request.sourceLang;
            item.targetLang = request.targetLang;
            item.service = result.serviceName;
            HistoryManager::Instance().AddItem(item);
        }

        // Safely marshal completion back to UI thread
        if (dispatcher)
        {
            dispatcher.TryEnqueue([onComplete, result]()
            {
                if (onComplete)
                {
                    onComplete(result);
                }
            });
        }
        else if (onComplete)
        {
            onComplete(result);
        }
    }

    void TranslationManager::TranslateAsync(
        TranslationRequest request,
        int serviceIndex,
        DispatcherQueue dispatcher,
        std::function<void(TranslationResult)> onComplete)
    {
        InternalTranslateWorker(request, serviceIndex, dispatcher, onComplete);
    }
}
