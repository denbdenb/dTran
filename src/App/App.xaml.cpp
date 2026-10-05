#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "QuickPopupWindow.xaml.h"
#include "SettingsWindow.xaml.h"
#include "../Windows/WindowsIntegration.h"
#include "../Windows/ClipboardHelper.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::dTranslate::implementation
{
    App::App()
    {
        InitializeComponent();

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            if (IsDebuggerPresent())
            {
                auto errorMessage = e.Message();
                __debugbreak();
            }
        });
#endif
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        m_window = make<MainWindow>();
        m_window.Activate();

        auto& win = ::dTranslate::Windows::WindowsIntegration::Instance();
        win.Initialize();

        win.SetOnShowMainWindow([this]()
        {
            if (m_window)
            {
                m_window.Activate();
            }
        });

        win.SetOnTranslateSelection([this](std::wstring const& text)
        {
            auto popup = make<QuickPopupWindow>();
            popup.SetSelectedText(winrt::hstring(text));
            popup.Activate();
            m_popupWindow = popup;
        });

        win.SetOnTranslateClipboard([this]()
        {
            auto text = ::dTranslate::Windows::ClipboardHelper::GetText();
            auto popup = make<QuickPopupWindow>();
            popup.SetSelectedText(winrt::hstring(text));
            popup.Activate();
            m_popupWindow = popup;
        });

        win.SetOnOpenSettings([this]()
        {
            auto settings = make<SettingsWindow>();
            settings.Activate();
            m_settingsWindow = settings;
        });

        win.SetOnExit([this]()
        {
            Exit();
        });
    }
}
