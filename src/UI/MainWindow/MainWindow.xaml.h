#pragma once

#include "MainWindow.g.h"
#include <string>

namespace winrt::dTranslate::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void TriggerScreenOcr();

    private:
        int m_currentNavIndex{ 0 };
        int m_currentServiceIndex{ 0 };
        bool m_isDarkMode{ false };
        bool m_sidebarCollapsed{ false };

        void SetupEventHandlers();
        void SelectNavView(int index);
        void ToggleSidebar();
        void SetSidebarState(bool collapsed);
        void UpdateCharCount();
        void OnSwapLanguages();
        void OnTranslateAsync();
        void ToggleTheme();
        void SelectService(int serviceId);
        void CopyTextToClipboard(winrt::hstring const& text);

        void OnScreenSnippingOcr();
        winrt::fire_and_forget OnAiRunAsync();
        winrt::fire_and_forget OnDictionarySearch();
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
