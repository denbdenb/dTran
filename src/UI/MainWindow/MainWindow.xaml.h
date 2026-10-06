#pragma once

#include "MainWindow.g.h"
#include <string>
#include <vector>

namespace winrt::dTranslate::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();
        ~MainWindow();

        void TriggerScreenOcr();
        void LookupInDictionary(std::wstring const& query, std::wstring const& sourceLang, std::wstring const& targetLang);

    private:
        int m_currentNavIndex{ 0 };
        int m_currentServiceIndex{ 0 };
        bool m_isDarkMode{ false };
        bool m_sidebarCollapsed{ false };

        std::vector<std::wstring> m_sourceLangCodes;
        std::vector<std::wstring> m_targetLangCodes;
        std::vector<std::wstring> m_dictSourceLangCodes;
        std::vector<std::wstring> m_dictTargetLangCodes;

        void SetupEventHandlers();
        void SelectNavView(int index);
        void ToggleSidebar();
        void SetSidebarState(bool collapsed);
        void UpdateCharCount();
        void OnSwapLanguages();
        void OnTranslateAsync();
        void ToggleTheme();
        void ApplyTheme(std::wstring const& themeName);
        void SelectService(int serviceId);
        void CopyTextToClipboard(winrt::hstring const& text);

        void OnScreenSnippingOcr();
        void UpdateOcrTooltip();
        winrt::fire_and_forget OnAiRunAsync();
        winrt::fire_and_forget OnDictionarySearch();
        void OnSpeakSource();
        void OnSpeakResult();
        void RefreshHistory();

        std::wstring GetSourceLangCode();
        std::wstring GetTargetLangCode();
        std::wstring GetDictSourceLangCode();
        std::wstring GetDictTargetLangCode();
        void PopulateLanguagesForService(int serviceId);
        void PopulateDictionaryLanguages();
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
