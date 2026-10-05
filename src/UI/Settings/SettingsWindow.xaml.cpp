#include "pch.h"
#include "SettingsWindow.xaml.h"
#if __has_include("SettingsWindow.g.cpp")
#include "SettingsWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::dTranslate::implementation
{
    SettingsWindow::SettingsWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(SettingsTitleBar());
        AppWindow().Resize({ 960, 620 });

        SetupEventHandlers();
    }

    void SettingsWindow::SetupEventHandlers()
    {
        SettingsCancelBtn().Click([this](auto&&, auto&&)
        {
            Close();
        });

        SettingsSaveBtn().Click([this](auto&&, auto&&)
        {
            // Will integrate with Settings storage in Phase 11
            Close();
        });
    }
}
