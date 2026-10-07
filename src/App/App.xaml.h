#pragma once

#include "App.xaml.g.h"
#include <windows.h>
#include <string>

namespace winrt::dTranslate::implementation
{
    struct App : AppT<App>
    {
        App();
        ~App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        static App* CurrentApp();

        void ShowOrActivateSettings();
        void ShowOrActivateQuickPopup(std::wstring const& text = L"", HWND sourceHwnd = nullptr, bool hasSelection = false);

    private:
        static App* s_currentApp;
        HANDLE m_hSingleInstanceMutex{ nullptr };
        winrt::Microsoft::UI::Xaml::Window m_popupWindow{ nullptr };
        winrt::Microsoft::UI::Xaml::Window m_settingsWindow{ nullptr };
    };
}
