#include "pch.h"
#include "App.xaml.h"
#include "QuickPopupWindow.xaml.h"
#include "SettingsWindow.xaml.h"
#include "../Windows/WindowsIntegration.h"
#include "../Windows/ClipboardHelper.h"
#include "../UI/Tray/TrayMenuManager.h"
#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.Windows.AppLifecycle.h>

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
            _wfopen_s(&f, L"C:\\Users\\denb\\.gemini\\antigravity\\scratch\\dTranslate\\crash_log.txt", L"a");
            if (f)
            {
                fwprintf(f, L"UnhandledException: %s\n", errorMessage.c_str());
                fflush(f);
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

    App::~App()
    {
        if (m_hSingleInstanceMutex)
        {
            ReleaseMutex(m_hSingleInstanceMutex);
            CloseHandle(m_hSingleInstanceMutex);
            m_hSingleInstanceMutex = nullptr;
        }
    }

    App* App::CurrentApp()
    {
        return s_currentApp;
    }

    static void LogAppDebug(const wchar_t* fmt, ...)
    {
#if defined(_DEBUG)
        wchar_t buf[512];
        va_list args;
        va_start(args, fmt);
        _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
        va_end(args);
        OutputDebugStringW(buf);
        OutputDebugStringW(L"\n");
#else
        (void)fmt;
#endif
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

    void App::ShowOrActivateQuickPopup(std::wstring const& text, HWND sourceHwnd, bool hasSelection)
    {
        LogAppDebug(L"App::ShowOrActivateQuickPopup enter (text len: %zu)", text.length());
        if (!m_popupWindow)
        {
            LogAppDebug(L"App::ShowOrActivateQuickPopup creating new QuickPopupWindow");
            auto popup = make<QuickPopupWindow>();
            popup.Closed([this](auto&&, auto&&)
            {
                LogAppDebug(L"QuickPopupWindow Closed event fired");
                m_popupWindow = nullptr;
            });
            m_popupWindow = popup;
        }

        if (auto popupImpl = winrt::get_self<QuickPopupWindow>(m_popupWindow.as<winrt::dTranslate::QuickPopupWindow>()))
        {
            if (!text.empty() || hasSelection)
            {
                popupImpl->SetSelectedTextWithContext(winrt::hstring(text), sourceHwnd, hasSelection);
            }
        }

        BringWindowToFront(m_popupWindow);
        LogAppDebug(L"App::ShowOrActivateQuickPopup finished");
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        LogAppDebug(L"App::OnLaunched enter");
        try
        {
            // 1. Single Instance check via named mutex
            HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"dTran_SingleInstance_Mutex");
            DWORD mutErr = GetLastError();
            LogAppDebug(L"CreateMutexW handle: 0x%p, err: %lu", hMutex, mutErr);

            if (hMutex != nullptr && mutErr == ERROR_ALREADY_EXISTS)
            {
                LogAppDebug(L"Second instance detected, searching for message window...");
                // Second instance launched: notify existing instance to open popup
                HWND hMsg = FindWindowW(L"dTran_MessageWindow", nullptr);
                LogAppDebug(L"FindWindowW message window: 0x%p", hMsg);
                if (hMsg != nullptr)
                {
                    PostMessageW(hMsg, WM_COMMAND, 3001 /* IDM_TRAY_OPEN */, 0);
                }
                CloseHandle(hMutex);
                Exit();
                return;
            }
            m_hSingleInstanceMutex = hMutex;

            // 2. Initialize Windows Integration (Tray + Hotkeys)
            auto& win = ::dTranslate::Windows::WindowsIntegration::Instance();
            win.Initialize();

            // 3. Setup Modern WinUI 3 Tray Menu callbacks (initialized on demand)
            auto& trayMenu = ::dTranslate::UI::TrayMenuManager::Instance();


            trayMenu.SetOnTranslateClipboard([this]()
            {
                auto text = ::dTranslate::Windows::ClipboardHelper::GetText();
                ShowOrActivateQuickPopup(text, nullptr, false);
            });

            trayMenu.SetOnScreenOcr([this]()
            {
                ShowOrActivateQuickPopup();
                if (m_popupWindow)
                {
                    if (auto popupImpl = winrt::get_self<QuickPopupWindow>(m_popupWindow.as<winrt::dTranslate::QuickPopupWindow>()))
                    {
                        popupImpl->OnScreenSnippingOcr();
                    }
                }
            });

            trayMenu.SetOnOpenSettings([this]()
            {
                ShowOrActivateSettings();
            });

            trayMenu.SetOnExit([this]()
            {
                Exit();
            });

            win.SetOnShowTrayMenu([&trayMenu](int x, int y)
            {
                trayMenu.ShowAt(x, y);
            });

            win.SetOnOpenPopup([this]()
            {
                ShowOrActivateQuickPopup();
            });

            win.SetOnTranslateSelection([this](std::wstring const& text, HWND sourceHwnd, bool hasSelection)
            {
                ShowOrActivateQuickPopup(text, sourceHwnd, hasSelection);
            });

            win.SetOnTranslateClipboard([this]()
            {
                auto text = ::dTranslate::Windows::ClipboardHelper::GetText();
                ShowOrActivateQuickPopup(text, nullptr, false);
            });

            win.SetOnScreenOcr([this]()
            {
                ShowOrActivateQuickPopup();
                if (m_popupWindow)
                {
                    if (auto popupImpl = winrt::get_self<QuickPopupWindow>(m_popupWindow.as<winrt::dTranslate::QuickPopupWindow>()))
                    {
                        popupImpl->OnScreenSnippingOcr();
                    }
                }
            });

            win.SetOnOpenSettings([this]()
            {
                ShowOrActivateSettings();
            });

            win.SetOnExit([this]()
            {
                Exit();
            });

            // 4. Check whether launched silently on Windows startup
            bool isStartupLaunch = false;
            try
            {
                auto actArgs = winrt::Microsoft::Windows::AppLifecycle::AppInstance::GetCurrent().GetActivatedEventArgs();
                if (actArgs && actArgs.Kind() == winrt::Microsoft::Windows::AppLifecycle::ExtendedActivationKind::StartupTask)
                {
                    isStartupLaunch = true;
                }
            }
            catch (...) {}

            if (!isStartupLaunch)
            {
                LPCWSTR cmdLine = GetCommandLineW();
                if (cmdLine && (wcsstr(cmdLine, L"--startup") != nullptr || wcsstr(cmdLine, L"/startup") != nullptr))
                {
                    isStartupLaunch = true;
                }
            }

            if (!isStartupLaunch)
            {
                ShowOrActivateQuickPopup();
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            FILE* f = nullptr;
            _wfopen_s(&f, L"C:\\Users\\denb\\.gemini\\antigravity\\scratch\\dTranslate\\crash_log.txt", L"a");
            if (f)
            {
                fwprintf(f, L"hresult_error in OnLaunched: 0x%08X: %s\n", ex.code().value, ex.message().c_str());
                fflush(f);
                fclose(f);
            }
        }
        catch (std::exception const& ex)
        {
            FILE* f = nullptr;
            _wfopen_s(&f, L"C:\\Users\\denb\\.gemini\\antigravity\\scratch\\dTranslate\\crash_log.txt", L"a");
            if (f)
            {
                fprintf(f, "std::exception in OnLaunched: %s\n", ex.what());
                fflush(f);
                fclose(f);
            }
        }
        catch (...)
        {
            FILE* f = nullptr;
            _wfopen_s(&f, L"C:\\Users\\denb\\.gemini\\antigravity\\scratch\\dTranslate\\crash_log.txt", L"a");
            if (f)
            {
                fprintf(f, "unknown exception in OnLaunched\n");
                fflush(f);
                fclose(f);
            }
        }
    }
}
