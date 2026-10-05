#pragma once

#include "MainWindow.g.h"
#include <string>

namespace winrt::dTranslate::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

    private:
        int m_currentNavIndex{ 0 };
        int m_currentServiceIndex{ 0 };
        bool m_isDarkMode{ false };

        void SetupEventHandlers();
        void SelectNavView(int index);
        void UpdateCharCount();
        void OnSwapLanguages();
        void OnTranslateAsync();
        void ToggleTheme();
        void SelectService(int serviceId);
        void CopyTextToClipboard(winrt::hstring const& text);

        // New service integrations
        void OnOcrAsync();
        winrt::fire_and_forget OnAiRunAsync();
        winrt::fire_and_forget OnWikipediaLookupAsync();
        winrt::fire_and_forget OnDictionaryLookupAsync();
        void OnSpeakSource();
        void OnSpeakResult();
        void RefreshHistory();

        std::wstring GetSourceLangCode();
        std::wstring GetTargetLangCode();
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
