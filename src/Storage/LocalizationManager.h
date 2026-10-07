#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <map>

namespace dTranslate::Storage
{
    using LocalizationObserver = std::function<void(std::wstring const&)>;

    class LocalizationManager
    {
    public:
        static LocalizationManager& Instance();

        void SetLanguage(std::wstring const& langCode);
        std::wstring const& GetCurrentLanguage() const { return m_currentLang; }

        std::wstring Get(std::wstring const& key) const;
        std::wstring Format(std::wstring const& key, std::wstring const& arg1) const;

        void RegisterObserver(uintptr_t key, LocalizationObserver observer);
        void UnregisterObserver(uintptr_t key);

    private:
        LocalizationManager();
        void LoadBundles();

        std::wstring m_currentLang{ L"en" };
        std::unordered_map<std::wstring, std::wstring> m_enStrings;
        std::unordered_map<std::wstring, std::wstring> m_ruStrings;
        std::map<uintptr_t, LocalizationObserver> m_observers;
        mutable std::mutex m_mutex;
    };
}
