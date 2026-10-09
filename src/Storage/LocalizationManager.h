#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <mutex>
#include <map>

namespace dTranslate::Storage
{
    using LocalizationObserver = std::function<void(std::wstring const&)>;

    struct SupportedLanguageInfo
    {
        std::wstring code;
        std::wstring nativeName;
    };

    class LocalizationManager
    {
    public:
        static LocalizationManager& Instance();

        void SetLanguage(std::wstring const& langCode);
        std::wstring const& GetCurrentLanguage() const { return m_currentLang; }

        std::wstring Get(std::wstring const& key) const;
        std::wstring Format(std::wstring const& key, std::wstring const& arg1) const;

        static bool IsValidLanguage(std::wstring_view langCode);
        static const std::vector<SupportedLanguageInfo>& GetSupportedLanguages();

        void RegisterObserver(uintptr_t key, LocalizationObserver observer);
        void UnregisterObserver(uintptr_t key);

    private:
        LocalizationManager();

        std::wstring m_currentLang{ L"en" };
        std::map<uintptr_t, LocalizationObserver> m_observers;
        mutable std::mutex m_mutex;
    };
}
