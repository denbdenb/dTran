#pragma once

#include "SettingsWindow.g.h"
#include <string>
#include "../Storage/SettingsManager.h"

namespace winrt::dTranslate::implementation
{
    struct SettingsWindow : SettingsWindowT<SettingsWindow>
    {
        SettingsWindow();
        ~SettingsWindow();

    private:
        HWND m_hwnd{ nullptr };
        bool m_isLoadingSettings{ false };
        void UpdateTitleBarColors(bool isDark);
        void UpdateAutoStartStatus(::dTranslate::Storage::WindowsStartupState state);
        void LoadSettings();
        void SetupEventHandlers();
        void PopulateLanguageDropdowns();
        void ApplyTheme(std::wstring const& themeName);
        void ApplyLocalization();
        void SaveSettings();
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct SettingsWindow : SettingsWindowT<SettingsWindow, implementation::SettingsWindow>
    {
    };
}
