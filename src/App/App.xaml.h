#pragma once

#include "App.xaml.g.h"
#include <string>

namespace winrt::dTranslate::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        static App* CurrentApp();

        void ShowOrActivateMainWindow();
        void ShowOrActivateSettings();
        void ShowOrActivateQuickPopup(std::wstring const& text = L"");

    private:
        static App* s_currentApp;
        winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
        winrt::Microsoft::UI::Xaml::Window m_popupWindow{ nullptr };
        winrt::Microsoft::UI::Xaml::Window m_settingsWindow{ nullptr };
    };
}
