#include "pch.h"
#include "SettingsWindow.xaml.h"
#if __has_include("SettingsWindow.g.cpp")
#include "SettingsWindow.g.cpp"
#endif

#include "../../Storage/SettingsManager.h"
#include "../../Storage/LocalizationManager.h"
#include "../../Translation/LanguageCatalog.h"
#include "../../Windows/WindowsIntegration.h"
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <microsoft.ui.xaml.window.h>
#include <filesystem>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace ::dTranslate::Storage;
using namespace ::dTranslate::Translation;
using namespace ::dTranslate::Windows;

namespace winrt::dTranslate::implementation
{
    static ComboBoxItem CreateLanguageComboItem(LanguageInfo const& lang)
    {
        ComboBoxItem item;
        StackPanel sp;
        sp.Orientation(Orientation::Horizontal);
        sp.Spacing(8);
        sp.VerticalAlignment(VerticalAlignment::Center);

        Image img;
        img.Width(20);
        img.Height(15);
        img.VerticalAlignment(VerticalAlignment::Center);

        Microsoft::UI::Xaml::Media::Imaging::SvgImageSource svg;
        svg.RasterizePixelWidth(24);
        svg.RasterizePixelHeight(18);
        svg.UriSource(Windows::Foundation::Uri(lang.FlagSvgPath()));
        img.Source(svg);
        sp.Children().Append(img);

        TextBlock tb;
        std::wstring dispName = lang.DisplayNameClean();
        if (lang.code == L"auto")
        {
            dispName = LocalizationManager::Instance().Get(L"AutoDetect");
        }
        tb.Text(winrt::hstring(dispName));
        tb.VerticalAlignment(VerticalAlignment::Center);
        sp.Children().Append(tb);

        item.Content(sp);
        return item;
    }

    SettingsWindow::SettingsWindow()
    {
        InitializeComponent();

        HWND hwnd = nullptr;
        if (auto windowNative = this->try_as<IWindowNative>())
        {
            windowNative->get_WindowHandle(&hwnd);
        }
        m_hwnd = hwnd;
        WindowsIntegration::SetWindowAppIcon(hwnd);

        wchar_t exePath[MAX_PATH] = { 0 };
        if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0)
        {
            std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
            std::vector<std::filesystem::path> candidates = {
                dir / L"Assets" / L"app.ico",
                dir / L"app.ico",
                dir / L"assets" / L"app.ico",
                dir / L".." / L".." / L"assets" / L"app.ico"
            };
            for (const auto& p : candidates)
            {
                if (std::filesystem::exists(p))
                {
                    AppWindow().SetIcon(winrt::hstring(p.wstring()));
                    break;
                }
            }
        }

        UINT dpi = hwnd ? ::GetDpiForWindow(hwnd) : 96;
        float scale = dpi / 96.0f;
        int scaledW = static_cast<int>(500 * scale);
        int scaledH = static_cast<int>(640 * scale);
        AppWindow().Resize({ scaledW, scaledH });

        if (auto presenter = AppWindow().Presenter().try_as<Microsoft::UI::Windowing::OverlappedPresenter>())
        {
            presenter.IsResizable(false);
            presenter.IsMinimizable(false);
            presenter.IsMaximizable(false);
        }

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(SettingsTitleBar());

        Closed([this](auto&&, auto&&)
        {
            SaveSettings();
            ::SetProcessWorkingSetSize(::GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        });

        // React to system theme changes in real-time when theme is System
        if (auto root = Content().try_as<FrameworkElement>())
        {
            root.ActualThemeChanged([this](auto&& sender, auto&&)
            {
                auto s = SettingsManager::Instance().GetSettings();
                if (s.theme == L"System" || s.theme.empty())
                {
                    bool isDark = (sender.ActualTheme() == ElementTheme::Dark);
                    UpdateTitleBarColors(isDark);
                }
            });
        }

        LocalizationManager::Instance().RegisterObserver(reinterpret_cast<uintptr_t>(this), [this](std::wstring const&)
        {
            DispatcherQueue().TryEnqueue([this]()
            {
                ApplyLocalization();
            });
        });

        PopulateLanguageDropdowns();
        LoadSettings();
        SetupEventHandlers();
        ApplyLocalization();
    }

    SettingsWindow::~SettingsWindow()
    {
        LocalizationManager::Instance().UnregisterObserver(reinterpret_cast<uintptr_t>(this));
    }

    void SettingsWindow::ApplyTheme(std::wstring const& themeName)
    {
        auto root = Content().try_as<FrameworkElement>();
        if (!root) return;

        if (themeName == L"Light")
            root.RequestedTheme(ElementTheme::Light);
        else if (themeName == L"Dark")
            root.RequestedTheme(ElementTheme::Dark);
        else
            root.RequestedTheme(ElementTheme::Default);

        bool isDark = false;
        if (themeName == L"Dark")
        {
            isDark = true;
        }
        else if (themeName == L"Light")
        {
            isDark = false;
        }
        else
        {
            isDark = (root.ActualTheme() == ElementTheme::Dark);
        }

        UpdateTitleBarColors(isDark);
    }

    void SettingsWindow::UpdateTitleBarColors(bool isDark)
    {
        if (m_hwnd)
        {
            BOOL dwmDark = isDark ? TRUE : FALSE;
            DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dwmDark, sizeof(dwmDark));
        }

        if (auto titleBar = AppWindow().TitleBar())
        {
            if (isDark)
            {
                // Dark Theme: Bright white close button glyph with subtle white hover/pressed states
                titleBar.ButtonBackgroundColor(Microsoft::UI::Colors::Transparent());
                titleBar.ButtonForegroundColor(Microsoft::UI::ColorHelper::FromArgb(245, 255, 255, 255));
                titleBar.ButtonHoverBackgroundColor(Microsoft::UI::ColorHelper::FromArgb(38, 255, 255, 255));
                titleBar.ButtonHoverForegroundColor(Microsoft::UI::Colors::White());
                titleBar.ButtonPressedBackgroundColor(Microsoft::UI::ColorHelper::FromArgb(65, 255, 255, 255));
                titleBar.ButtonPressedForegroundColor(Microsoft::UI::Colors::White());
                titleBar.ButtonInactiveBackgroundColor(Microsoft::UI::Colors::Transparent());
                titleBar.ButtonInactiveForegroundColor(Microsoft::UI::ColorHelper::FromArgb(135, 255, 255, 255));
            }
            else
            {
                // Light Theme: Crisp dark close button glyph with subtle dark hover/pressed states
                titleBar.ButtonBackgroundColor(Microsoft::UI::Colors::Transparent());
                titleBar.ButtonForegroundColor(Microsoft::UI::ColorHelper::FromArgb(235, 20, 20, 20));
                titleBar.ButtonHoverBackgroundColor(Microsoft::UI::ColorHelper::FromArgb(25, 0, 0, 0));
                titleBar.ButtonHoverForegroundColor(Microsoft::UI::Colors::Black());
                titleBar.ButtonPressedBackgroundColor(Microsoft::UI::ColorHelper::FromArgb(45, 0, 0, 0));
                titleBar.ButtonPressedForegroundColor(Microsoft::UI::Colors::Black());
                titleBar.ButtonInactiveBackgroundColor(Microsoft::UI::Colors::Transparent());
                titleBar.ButtonInactiveForegroundColor(Microsoft::UI::ColorHelper::FromArgb(120, 0, 0, 0));
            }
        }
    }

    void SettingsWindow::PopulateLanguageDropdowns()
    {
        auto const& allLangs = LanguageCatalog::GetAllLanguages();
        auto const& settings = SettingsManager::Instance().GetSettings();

        // Source combo
        DefaultSourceLangCombo().Items().Clear();
        int srcIdx = 0;
        for (size_t i = 0; i < allLangs.size(); ++i)
        {
            DefaultSourceLangCombo().Items().Append(CreateLanguageComboItem(allLangs[i]));
            if (_wcsicmp(allLangs[i].code.c_str(), settings.sourceLanguage.c_str()) == 0)
            {
                srcIdx = static_cast<int>(i);
            }
        }
        DefaultSourceLangCombo().SelectedIndex(srcIdx);

        // Target combo (skip auto)
        DefaultTargetLangCombo().Items().Clear();
        int tgtIdx = 0;
        for (size_t i = 1; i < allLangs.size(); ++i)
        {
            DefaultTargetLangCombo().Items().Append(CreateLanguageComboItem(allLangs[i]));
            if (_wcsicmp(allLangs[i].code.c_str(), settings.targetLanguage.c_str()) == 0)
            {
                tgtIdx = static_cast<int>(i - 1);
            }
        }
        DefaultTargetLangCombo().SelectedIndex(tgtIdx);
    }

    void SettingsWindow::UpdateAutoStartStatus(WindowsStartupState state)
    {
        auto const& loc = LocalizationManager::Instance();
        std::wstring tip;
        switch (state)
        {
        case WindowsStartupState::Enabled:
        case WindowsStartupState::EnabledByPolicy:
            tip = loc.Get(L"StartupTooltipEnabled");
            break;
        case WindowsStartupState::DisabledByUser:
            tip = loc.Get(L"StartupTooltipDisabledByUser");
            break;
        case WindowsStartupState::DisabledByPolicy:
            tip = loc.Get(L"StartupTooltipDisabledByPolicy");
            break;
        default:
            tip = loc.Get(L"StartupTooltipDisabled");
            break;
        }

        ToolTipService::SetToolTip(AutoStartToggle(), box_value(winrt::hstring(tip)));
    }

    void SettingsWindow::LoadSettings()
    {
        m_isLoadingSettings = true;
        auto const& settings = SettingsManager::Instance().GetSettings();
        CompareTranslationsToggle().IsOn(settings.compareTranslations);

        auto startupState = SettingsManager::GetStartupTaskState();
        bool isStartupEnabled = (startupState == WindowsStartupState::Enabled ||
                                 startupState == WindowsStartupState::EnabledByPolicy);
        AutoStartToggle().IsOn(isStartupEnabled);
        UpdateAutoStartStatus(startupState);

        if (settings.theme == L"Light")
            ThemeCombo().SelectedIndex(1);
        else if (settings.theme == L"Dark")
            ThemeCombo().SelectedIndex(2);
        else
            ThemeCombo().SelectedIndex(0);

        AppLanguageCombo().SelectedIndex(settings.appLanguage == L"ru" ? 1 : 0);

        ApplyTheme(settings.theme);

        // Hotkeys
        HotkeyTranslateBox().Text(winrt::hstring(settings.translateSelectedHotkey.empty() ? L"Ctrl+Alt+T" : settings.translateSelectedHotkey));
        HotkeyOcrBox().Text(winrt::hstring(settings.ocrHotkey.empty() ? L"Ctrl+Alt+O" : settings.ocrHotkey));
        m_isLoadingSettings = false;
    }

    void SettingsWindow::SetupEventHandlers()
    {
        // 0. AutoStart toggle handler with real Windows StartupTask API
        AutoStartToggle().Toggled([this](auto&& sender, auto&&) -> winrt::fire_and_forget
        {
            if (m_isLoadingSettings) co_return;

            auto toggle = sender.as<ToggleSwitch>();
            bool wantEnable = toggle.IsOn();
            toggle.IsEnabled(false);

            auto rawState = co_await SettingsManager::SetStartWithWindowsAsync(wantEnable);
            auto state = static_cast<WindowsStartupState>(rawState);

            toggle.IsEnabled(true);
            m_isLoadingSettings = true;
            if (wantEnable)
            {
                bool succeeded = (state == WindowsStartupState::Enabled || state == WindowsStartupState::EnabledByPolicy);
                toggle.IsOn(succeeded);
                auto s = SettingsManager::Instance().GetSettings();
                s.autoStart = succeeded;
                SettingsManager::Instance().UpdateSettings(s);
                UpdateAutoStartStatus(state);
            }
            else
            {
                bool isOff = (state == WindowsStartupState::Disabled);
                toggle.IsOn(!isOff);
                auto s = SettingsManager::Instance().GetSettings();
                s.autoStart = !isOff;
                SettingsManager::Instance().UpdateSettings(s);
                UpdateAutoStartStatus(state);
            }
            m_isLoadingSettings = false;
        });

        // 0b. Close on Escape key
        SettingsRootGrid().KeyDown([this](auto&&, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args)
        {
            if (args.Key() == Windows::System::VirtualKey::Escape)
            {
                SaveSettings();
                Close();
                args.Handled(true);
            }
        });

        // 1. Live Theme switching
        ThemeCombo().SelectionChanged([this](auto&&, auto&&)
        {
            int idx = ThemeCombo().SelectedIndex();
            std::wstring themeName = L"Default";
            if (idx == 1) themeName = L"Light";
            else if (idx == 2) themeName = L"Dark";

            ApplyTheme(themeName);

            auto s = SettingsManager::Instance().GetSettings();
            s.theme = themeName;
            SettingsManager::Instance().UpdateSettings(s);
        });

        // 2. Language switching
        AppLanguageCombo().SelectionChanged([this](auto&&, auto&&)
        {
            int idx = AppLanguageCombo().SelectedIndex();
            std::wstring newLang = (idx == 1) ? L"ru" : L"en";
            auto s = SettingsManager::Instance().GetSettings();
            if (s.appLanguage != newLang)
            {
                s.appLanguage = newLang;
                SettingsManager::Instance().UpdateSettings(s);
                LocalizationManager::Instance().SetLanguage(newLang);
            }
        });

        // 3. Hotkey recording helper
        auto SetupHotkeyRecorder = [this](TextBox box)
        {
            box.KeyDown([this, box](auto&&, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args)
            {
                auto vk = args.Key();
                if (vk == Windows::System::VirtualKey::Control ||
                    vk == Windows::System::VirtualKey::Menu ||
                    vk == Windows::System::VirtualKey::Shift ||
                    vk == Windows::System::VirtualKey::LeftWindows ||
                    vk == Windows::System::VirtualKey::RightWindows)
                {
                    args.Handled(true);
                    return;
                }

                bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
                bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                bool win = (GetKeyState(VK_LWIN) & 0x8000) != 0 || (GetKeyState(VK_RWIN) & 0x8000) != 0;

                std::wstring combo;
                if (ctrl) combo += L"Ctrl+";
                if (alt) combo += L"Alt+";
                if (shift) combo += L"Shift+";
                if (win) combo += L"Win+";

                int keyVal = static_cast<int>(vk);
                if (keyVal >= 'A' && keyVal <= 'Z')
                {
                    combo += static_cast<wchar_t>(keyVal);
                }
                else if (keyVal >= '0' && keyVal <= '9')
                {
                    combo += static_cast<wchar_t>(keyVal);
                }
                else if (keyVal >= static_cast<int>(Windows::System::VirtualKey::F1) &&
                         keyVal <= static_cast<int>(Windows::System::VirtualKey::F12))
                {
                    int fNum = keyVal - static_cast<int>(Windows::System::VirtualKey::F1) + 1;
                    combo += L"F" + std::to_wstring(fNum);
                }
                else if (vk == Windows::System::VirtualKey::Space)
                {
                    combo += L"Space";
                }
                else if (vk == Windows::System::VirtualKey::Tab)
                {
                    combo += L"Tab";
                }
                else
                {
                    args.Handled(true);
                    return;
                }

                box.Text(winrt::hstring(combo));
                auto const& loc = LocalizationManager::Instance();
                HotkeyStatusText().Text(winrt::hstring(loc.Format(L"HotkeyCaptured", combo)));
                args.Handled(true);
            });
        };

        SetupHotkeyRecorder(HotkeyTranslateBox());
        SetupHotkeyRecorder(HotkeyOcrBox());

        // 4. Independent reset buttons
        ResetTranslateHotkeyBtn().Click([this](auto&&, auto&&)
        {
            HotkeyTranslateBox().Text(L"Ctrl+Alt+T");
            auto const& loc = LocalizationManager::Instance();
            HotkeyStatusText().Text(winrt::hstring(loc.Get(L"HotkeyResetSuccess")));
        });

        ResetOcrHotkeyBtn().Click([this](auto&&, auto&&)
        {
            HotkeyOcrBox().Text(L"Ctrl+Alt+O");
            auto const& loc = LocalizationManager::Instance();
            HotkeyStatusText().Text(winrt::hstring(loc.Get(L"HotkeyResetSuccess")));
        });

        // 5. Close button
        SettingsCloseBtn().Click([this](auto&&, auto&&)
        {
            SaveSettings();
            Close();
        });
    }

    void SettingsWindow::SaveSettings()
    {
        std::wstring hkTrans = HotkeyTranslateBox().Text().c_str();
        std::wstring hkOcr = HotkeyOcrBox().Text().c_str();

        std::wstring err;
        if (!WindowsIntegration::ValidateHotkey(hkTrans, err))
        {
            HotkeyStatusText().Text(winrt::hstring(L"Error: " + err));
            return;
        }
        if (!WindowsIntegration::ValidateHotkey(hkOcr, err))
        {
            HotkeyStatusText().Text(winrt::hstring(L"Error: " + err));
            return;
        }

        std::wstring regErr;
        if (!WindowsIntegration::Instance().ReRegisterHotkeys(hkTrans, hkOcr, regErr))
        {
            HotkeyStatusText().Text(winrt::hstring(regErr.empty() ? L"Hotkey registration failed" : regErr));
        }

        auto settings = SettingsManager::Instance().GetSettings();
        settings.autoStart = AutoStartToggle().IsOn();

        int themeIdx = ThemeCombo().SelectedIndex();
        if (themeIdx == 1) settings.theme = L"Light";
        else if (themeIdx == 2) settings.theme = L"Dark";
        else settings.theme = L"Default";

        settings.appLanguage = (AppLanguageCombo().SelectedIndex() == 1) ? L"ru" : L"en";

        settings.compareTranslations = CompareTranslationsToggle().IsOn();

        // Default languages
        int sIdx = DefaultSourceLangCombo().SelectedIndex();
        auto const& allLangs = LanguageCatalog::GetAllLanguages();
        if (sIdx >= 0 && sIdx < static_cast<int>(allLangs.size()))
        {
            settings.sourceLanguage = allLangs[sIdx].code;
        }

        int tIdx = DefaultTargetLangCombo().SelectedIndex();
        int realTgtIdx = tIdx + 1;
        if (realTgtIdx >= 1 && realTgtIdx < static_cast<int>(allLangs.size()))
        {
            settings.targetLanguage = allLangs[realTgtIdx].code;
        }

        settings.translateSelectedHotkey = hkTrans;
        settings.ocrHotkey = hkOcr;

        SettingsManager::Instance().UpdateSettings(settings);
    }

    void SettingsWindow::ApplyLocalization()
    {
        auto const& loc = LocalizationManager::Instance();
        Title(winrt::hstring(loc.Get(L"SettingsTitle")));
        SettingsTitleTextBlock().Text(winrt::hstring(loc.Get(L"SettingsTitle")));

        SettingsSectionGeneralText().Text(winrt::hstring(loc.Get(L"SectionGeneral")));
        SettingsThemeLabel().Text(winrt::hstring(loc.Get(L"Theme")));
        ThemeItemDefault().Content(box_value(winrt::hstring(loc.Get(L"ThemeSystem"))));
        ThemeItemLight().Content(box_value(winrt::hstring(loc.Get(L"ThemeLight"))));
        ThemeItemDark().Content(box_value(winrt::hstring(loc.Get(L"ThemeDark"))));
        SettingsAutoStartLabel().Text(winrt::hstring(loc.Get(L"StartWithWindows")));
        SettingsAppLanguageLabel().Text(winrt::hstring(loc.Get(L"AppLanguage")));

        SettingsSectionLanguagesText().Text(winrt::hstring(loc.Get(L"SectionLanguages")));
        SettingsDefaultSourceLabel().Text(winrt::hstring(loc.Get(L"DefaultSource")));
        SettingsDefaultTargetLabel().Text(winrt::hstring(loc.Get(L"DefaultTarget")));
        SettingsCompareTranslationsLabel().Text(winrt::hstring(loc.Get(L"CompareTranslations")));

        SettingsSectionHotkeysText().Text(winrt::hstring(loc.Get(L"SectionHotkeys")));
        SettingsHotkeyTranslateLabel().Text(winrt::hstring(loc.Get(L"HotkeyTranslateSelected")));
        SettingsHotkeyOcrLabel().Text(winrt::hstring(loc.Get(L"HotkeyOcr")));
        ToolTipService::SetToolTip(HotkeyTranslateBox(), box_value(winrt::hstring(loc.Get(L"TipHotkeyRecord"))));
        ToolTipService::SetToolTip(HotkeyOcrBox(), box_value(winrt::hstring(loc.Get(L"TipHotkeyRecord"))));
        ToolTipService::SetToolTip(ResetTranslateHotkeyBtn(), box_value(winrt::hstring(loc.Format(L"TipResetHotkey", L"Ctrl+Alt+T"))));
        ToolTipService::SetToolTip(ResetOcrHotkeyBtn(), box_value(winrt::hstring(loc.Format(L"TipResetHotkey", L"Ctrl+Alt+O"))));

        SettingsAboutVersionText().Text(winrt::hstring(loc.Get(L"AboutVersion")));
        SettingsAboutSubtitleText().Text(winrt::hstring(loc.Get(L"AboutSubtitle")));
        SettingsAboutAuthorText().Text(winrt::hstring(loc.Get(L"AboutCreatedBy")));

        SettingsCloseBtn().Content(box_value(winrt::hstring(loc.Get(L"BtnClose"))));

        int curSrc = DefaultSourceLangCombo().SelectedIndex();
        int curTgt = DefaultTargetLangCombo().SelectedIndex();
        PopulateLanguageDropdowns();
        if (curSrc >= 0) DefaultSourceLangCombo().SelectedIndex(curSrc);
        if (curTgt >= 0) DefaultTargetLangCombo().SelectedIndex(curTgt);
    }
}
