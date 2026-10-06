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
        void SelectSettingsTab(int index);
        void UpdateAiStatuses();
        void PopulateLanguageDropdowns();
        winrt::fire_and_forget TestGeminiAsync();
        winrt::fire_and_forget TestOpenAiAsync();
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct SettingsWindow : SettingsWindowT<SettingsWindow, implementation::SettingsWindow>
    {
    };
}
