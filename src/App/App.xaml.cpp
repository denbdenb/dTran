#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "QuickPopupWindow.xaml.h"
#include "SettingsWindow.xaml.h"
#include "../Windows/WindowsIntegration.h"
#include "../Windows/ClipboardHelper.h"
#include <microsoft.ui.xaml.window.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::dTranslate::implementation
{
    App* App::s_currentApp = nullptr;

    App::App()
    {
        s_currentApp = this;
        InitializeComponent();

        UnhandledException([](IInspectable const&, UnhandledExceptionEventArgs const& e)
        {
            auto errorMessage = e.Message();
            FILE* f = nullptr;
            _wfopen_s(&f, L"crash_log.txt", L"a");
            if (f)
            {
                fwprintf(f, L"UnhandledException: %s\n", errorMessage.c_str());
                fclose(f);
            }
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
            if (IsDebuggerPresent())
            {
                __debugbreak();
            }
#endif
        });
    }

    App* App::CurrentApp()
    {
        return s_currentApp;
    }

    static void BringWindowToFront(winrt::Microsoft::UI::Xaml::Window const& window)
    {
        if (!window) return;
        HWND hwnd = nullptr;
        if (auto windowNative = window.try_as<IWindowNative>())
        {
            windowNative->get_WindowHandle(&hwnd);
        }
        window.AppWindow().Show();
        if (hwnd)
        {
            if (::IsIconic(hwnd))
            {
                ::ShowWindow(hwnd, SW_RESTORE);
            }
            else
            {
                ::ShowWindow(hwnd, SW_SHOW);
            }
            ::SetForegroundWindow(hwnd);
        }
        window.Activate();
    }

    void App::ShowOrActivateMainWindow()
    {
        if (m_window)
        {
            BringWindowToFront(m_window);
        }
    }

    void App::ShowOrActivateSettings()
    {
        if (m_settingsWindow)
        {
            BringWindowToFront(m_settingsWindow);
            return;
        }

        auto settings = make<SettingsWindow>();
        settings.Closed([this](auto&&, auto&&)
        {
            m_settingsWindow = nullptr;
        });
        m_settingsWindow = settings;
        m_settingsWindow.Activate();
    }

    void App::ShowOrActivateQuickPopup(std::wstring const& text)
    {
        if (m_popupWindow)
        {
            if (auto popupImpl = m_popupWindow.try_as<winrt::dTranslate::QuickPopupWindow>())
            {
                if (!text.empty())
                {
                    popupImpl.SetSelectedText(winrt::hstring(text));
                }
            }
            BringWindowToFront(m_popupWindow);
            return;
        }

        auto popup = make<QuickPopupWindow>();
        popup.Closed([this](auto&&, auto&&)
        {
            m_popupWindow = nullptr;
        });
        m_popupWindow = popup;
        m_popupWindow.Activate();
        if (!text.empty())
        {
            popup.SetSelectedText(winrt::hstring(text));
        }
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        try
        {
            m_window = make<MainWindow>();
            m_window.Activate();

            auto& win = ::dTranslate::Windows::WindowsIntegration::Instance();
            win.Initialize();

            win.SetOnShowMainWindow([this]()
            {
                ShowOrActivateMainWindow();
            });

            win.SetOnTranslateSelection([this](std::wstring const& text)
            {
                ShowOrActivateQuickPopup(text);
            });

            win.SetOnTranslateClipboard([this]()
            {
                auto text = ::dTranslate::Windows::ClipboardHelper::GetText();
                ShowOrActivateQuickPopup(text);
            });

            win.SetOnOpenSettings([this]()
            {
                ShowOrActivateSettings();
            });

            win.SetOnExit([this]()
            {
                Exit();
            });
        }
        catch (winrt::hresult_error const& ex)
        {
            FILE* f = nullptr;
            _wfopen_s(&f, L"crash_log.txt", L"a");
            if (f)
            {
                fwprintf(f, L"hresult_error in OnLaunched: 0x%08X: %s\n", ex.code().value, ex.message().c_str());
                fclose(f);
            }
        }
        catch (std::exception const& ex)
        {
            FILE* f = nullptr;
            _wfopen_s(&f, L"crash_log.txt", L"a");
            if (f)
            {
                fprintf(f, "std::exception in OnLaunched: %s\n", ex.what());
                fclose(f);
            }
        }
        catch (...)
        {
            FILE* f = nullptr;
            _wfopen_s(&f, L"crash_log.txt", L"a");
            if (f)
            {
                fprintf(f, "unknown exception in OnLaunched\n");
                fclose(f);
            }
        }
    }
}
