#pragma once

#include "SettingsWindow.g.h"

namespace winrt::dTranslate::implementation
{
    struct SettingsWindow : SettingsWindowT<SettingsWindow>
    {
        SettingsWindow();

    private:
        void LoadSettings();
        void SetupEventHandlers();
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct SettingsWindow : SettingsWindowT<SettingsWindow, implementation::SettingsWindow>
    {
    };
}
