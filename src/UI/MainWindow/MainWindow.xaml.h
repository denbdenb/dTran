#pragma once

#include "MainWindow.g.h"

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
        void OnTranslate();
        void ToggleTheme();
        void SelectService(int serviceId);
        void CopyTextToClipboard(winrt::hstring const& text);
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
