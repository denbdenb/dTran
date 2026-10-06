#include "pch.h"
#include "SettingsWindow.xaml.h"
#include "SettingsManager.h"
#include "CredentialStore.h"
#include "GeminiService.h"
#include "OpenAIService.h"
#include "LanguageCatalog.h"
#include "WindowsIntegration.h"

#if __has_include("SettingsWindow.g.cpp")
#include "SettingsWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::dTranslate::Storage;
using namespace ::dTranslate::AI;
using namespace ::dTranslate::Translation;
using namespace ::dTranslate::Windows;

namespace winrt::dTranslate::implementation
{
    SettingsWindow::SettingsWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(SettingsTitleBar());

        auto const& settings = SettingsManager::Instance().GetSettings();
        int w = settings.settingsWindowWidth > 400 ? settings.settingsWindowWidth : 960;
        int h = settings.settingsWindowHeight > 400 ? settings.settingsWindowHeight : 680;
        AppWindow().Resize({ w, h });

        // Apply theme to settings window
        if (auto root = Content().try_as<FrameworkElement>())
        {
            if (settings.theme == L"Light") root.RequestedTheme(ElementTheme::Light);
            else if (settings.theme == L"Dark") root.RequestedTheme(ElementTheme::Dark);
            else root.RequestedTheme(ElementTheme::Default);
        }

        PopulateLanguageDropdowns();
        LoadSettings();
        SetupEventHandlers();

        AppWindow().Changed([this](auto&& sender, Microsoft::UI::Windowing::AppWindowChangedEventArgs const& args)
        {
            if (args.DidSizeChange())
            {
                auto size = sender.Size();
                if (size.Width > 400 && size.Height > 400)
                {
                    auto s = SettingsManager::Instance().GetSettings();
                    s.settingsWindowWidth = size.Width;
                    s.settingsWindowHeight = size.Height;
                    SettingsManager::Instance().UpdateSettings(s);
                }
            }
        });
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
            ComboBoxItem item;
            item.Content(box_value(winrt::hstring(allLangs[i].DisplayName())));
            DefaultSourceLangCombo().Items().Append(item);
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
            ComboBoxItem item;
            item.Content(box_value(winrt::hstring(allLangs[i].DisplayName())));
            DefaultTargetLangCombo().Items().Append(item);
            if (_wcsicmp(allLangs[i].code.c_str(), settings.targetLanguage.c_str()) == 0)
            {
                tgtIdx = static_cast<int>(i - 1);
            }
        }
        DefaultTargetLangCombo().SelectedIndex(tgtIdx);
    }

    void SettingsWindow::LoadSettings()
    {
        auto const& settings = SettingsManager::Instance().GetSettings();
        CompareTranslationsCheck().IsChecked(settings.compareTranslations);
        AutoStartToggle().IsOn(settings.autoStart);
        MinimizeToTrayToggle().IsOn(settings.closeToTray);

        if (settings.theme == L"Light")
            ThemeCombo().SelectedIndex(1);
        else if (settings.theme == L"Dark")
            ThemeCombo().SelectedIndex(2);
        else
            ThemeCombo().SelectedIndex(0);

        // AI Model read-only info
        ModelGeminiInfoText().Text(winrt::hstring(L"Using: " + (settings.geminiModel.empty() ? L"gemini-2.5-flash" : settings.geminiModel)));
        ModelOpenAiInfoText().Text(winrt::hstring(L"Using: " + (settings.openAiModel.empty() ? L"gpt-4o-mini" : settings.openAiModel)));

        // Hotkeys
        HotkeySelectionBox().Text(winrt::hstring(settings.globalHotkey.empty() ? L"Ctrl+Alt+T" : settings.globalHotkey));
        HotkeyMainBox().Text(winrt::hstring(settings.quickHotkey.empty() ? L"Ctrl+Alt+D" : settings.quickHotkey));
        HotkeyOcrBox().Text(winrt::hstring(settings.ocrHotkey.empty() ? L"Ctrl+Alt+O" : settings.ocrHotkey));

        // Load existing API keys if configured
        auto geminiKey = CredentialStore::GetCredential(CredentialStore::KeyGemini);
        if (!geminiKey.empty()) KeyGeminiBox().Password(winrt::hstring(geminiKey));

        auto openAiKey = CredentialStore::GetCredential(CredentialStore::KeyOpenAI);
        if (!openAiKey.empty()) KeyOpenAiBox().Password(winrt::hstring(openAiKey));

        UpdateAiStatuses();
    }

    void SettingsWindow::UpdateAiStatuses()
    {
        bool hasGemini = !KeyGeminiBox().Password().empty();
        GeminiStatusText().Text(hasGemini ? L"Configured ✓" : L"Not configured");
        GeminiStatusText().Foreground(SolidColorBrush(hasGemini ?
            Windows::UI::Color{ 255, 16, 185, 129 } : Windows::UI::Color{ 255, 160, 160, 160 }));

        bool hasOpenAi = !KeyOpenAiBox().Password().empty();
        OpenAiStatusText().Text(hasOpenAi ? L"Configured ✓" : L"Not configured");
        OpenAiStatusText().Foreground(SolidColorBrush(hasOpenAi ?
            Windows::UI::Color{ 255, 16, 185, 129 } : Windows::UI::Color{ 255, 160, 160, 160 }));
    }

    void SettingsWindow::SelectSettingsTab(int index)
    {
        auto transparentBrush = SolidColorBrush(Windows::UI::Color{ 0, 0, 0, 0 });
        auto activeBrush = SolidColorBrush(Windows::UI::Color{ 25, 37, 99, 235 });

        TabGeneralBtn().Background(index == 0 ? activeBrush : transparentBrush);
        TabTranslationBtn().Background(index == 1 ? activeBrush : transparentBrush);
        TabAiBtn().Background(index == 2 ? activeBrush : transparentBrush);
        TabHotkeysBtn().Background(index == 3 ? activeBrush : transparentBrush);
        TabAboutBtn().Background(index == 4 ? activeBrush : transparentBrush);

        PanelGeneral().Visibility(index == 0 ? Visibility::Visible : Visibility::Collapsed);
        PanelTranslation().Visibility(index == 1 ? Visibility::Visible : Visibility::Collapsed);
        PanelAi().Visibility(index == 2 ? Visibility::Visible : Visibility::Collapsed);
        PanelHotkeys().Visibility(index == 3 ? Visibility::Visible : Visibility::Collapsed);
        PanelAbout().Visibility(index == 4 ? Visibility::Visible : Visibility::Collapsed);
    }

    void SettingsWindow::SetupEventHandlers()
    {
        TabGeneralBtn().Click([this](auto&&, auto&&) { SelectSettingsTab(0); });
        TabTranslationBtn().Click([this](auto&&, auto&&) { SelectSettingsTab(1); });
        TabAiBtn().Click([this](auto&&, auto&&) { SelectSettingsTab(2); });
        TabHotkeysBtn().Click([this](auto&&, auto&&) { SelectSettingsTab(3); });
        TabAboutBtn().Click([this](auto&&, auto&&) { SelectSettingsTab(4); });

        KeyGeminiBox().PasswordChanged([this](auto&&, auto&&) { UpdateAiStatuses(); });
        KeyOpenAiBox().PasswordChanged([this](auto&&, auto&&) { UpdateAiStatuses(); });

        TestGeminiBtn().Click([this](auto&&, auto&&) { TestGeminiAsync(); });
        TestOpenAiBtn().Click([this](auto&&, auto&&) { TestOpenAiAsync(); });

        // Hotkey Recorder
        auto SetupHotkeyRecorder = [this](TextBox const& box)
        {
            box.PreviewKeyDown([this, box](auto&&, KeyRoutedEventArgs const& args)
            {
                auto vk = args.Key();

                // Ignore standalone modifiers
                if (vk == Windows::System::VirtualKey::Control ||
                    vk == Windows::System::VirtualKey::LeftControl ||
                    vk == Windows::System::VirtualKey::RightControl ||
                    vk == Windows::System::VirtualKey::Menu ||
                    vk == Windows::System::VirtualKey::LeftMenu ||
                    vk == Windows::System::VirtualKey::RightMenu ||
                    vk == Windows::System::VirtualKey::Shift ||
                    vk == Windows::System::VirtualKey::LeftShift ||
                    vk == Windows::System::VirtualKey::RightShift ||
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
                HotkeyStatusText().Text(winrt::hstring(L"Captured: " + combo));
                args.Handled(true);
            });
        };

        SetupHotkeyRecorder(HotkeySelectionBox());
        SetupHotkeyRecorder(HotkeyMainBox());
        SetupHotkeyRecorder(HotkeyOcrBox());

        HotkeyResetBtn().Click([this](auto&&, auto&&)
        {
            HotkeySelectionBox().Text(L"Ctrl+Alt+T");
            HotkeyMainBox().Text(L"Ctrl+Alt+D");
            HotkeyOcrBox().Text(L"Ctrl+Alt+O");
            HotkeyStatusText().Text(L"Reset to defaults.");
        });

        SettingsCancelBtn().Click([this](auto&&, auto&&)
        {
            Close();
        });

        SettingsSaveBtn().Click([this](auto&&, auto&&)
        {
            // Validate hotkeys
            std::wstring hkSel = HotkeySelectionBox().Text().c_str();
            std::wstring hkMain = HotkeyMainBox().Text().c_str();
            std::wstring hkOcr = HotkeyOcrBox().Text().c_str();

            std::wstring err;
            if (!WindowsIntegration::ValidateHotkey(hkSel, err) ||
                !WindowsIntegration::ValidateHotkey(hkMain, err) ||
                !WindowsIntegration::ValidateHotkey(hkOcr, err))
            {
                HotkeyStatusText().Text(winrt::hstring(err));
                SelectSettingsTab(3); // Switch to hotkeys tab to show error
                return;
            }

            // Re-register hotkeys immediately
            if (!WindowsIntegration::Instance().ReRegisterHotkeys(hkSel, hkMain, hkOcr, err))
            {
                HotkeyStatusText().Text(winrt::hstring(err));
                SelectSettingsTab(3);
                return;
            }

            // 1. Save settings
            auto settings = SettingsManager::Instance().GetSettings();
            auto isChecked = CompareTranslationsCheck().IsChecked();
            if (isChecked != nullptr)
            {
                settings.compareTranslations = isChecked.Value();
            }

            settings.autoStart = AutoStartToggle().IsOn();
            settings.closeToTray = MinimizeToTrayToggle().IsOn();
            SettingsManager::SetStartWithWindows(settings.autoStart);

            int themeIdx = ThemeCombo().SelectedIndex();
            if (themeIdx == 1) settings.theme = L"Light";
            else if (themeIdx == 2) settings.theme = L"Dark";
            else settings.theme = L"Default";

            // Languages
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

            settings.globalHotkey = hkSel;
            settings.quickHotkey = hkMain;
            settings.ocrHotkey = hkOcr;

            SettingsManager::Instance().UpdateSettings(settings);

            // 2. Save or remove API credentials
            auto saveKey = [](std::wstring const& targetName, winrt::hstring const& key)
            {
                std::wstring val = key.c_str();
                if (!val.empty())
                {
                    CredentialStore::SetCredential(targetName, val);
                }
                else
                {
                    CredentialStore::DeleteCredential(targetName);
                }
            };

            saveKey(CredentialStore::KeyGemini, KeyGeminiBox().Password());
            saveKey(CredentialStore::KeyOpenAI, KeyOpenAiBox().Password());

            Close();
        });
    }

    winrt::fire_and_forget SettingsWindow::TestGeminiAsync()
    {
        auto key = KeyGeminiBox().Password();
        if (key.empty())
        {
            GeminiTestResultText().Text(L"Please enter an API key first.");
            co_return;
        }

        GeminiTestResultText().Text(L"Connecting to Gemini...");
        TestGeminiBtn().IsEnabled(false);

        CredentialStore::SetCredential(CredentialStore::KeyGemini, key.c_str());

        co_await resume_background();
        AIRequest req;
        req.operation = AIOperation::Rewrite;
        req.text = L"Hello";
        auto res = GeminiService::Instance().Execute(req);

        DispatcherQueue().TryEnqueue([this, res]()
        {
            TestGeminiBtn().IsEnabled(true);
            if (res.success)
            {
                GeminiTestResultText().Text(L"Connected successfully (gemini-2.5-flash) ✓");
            }
            else
            {
                GeminiTestResultText().Text(winrt::hstring(res.errorMessage));
            }
            UpdateAiStatuses();
        });
    }

    winrt::fire_and_forget SettingsWindow::TestOpenAiAsync()
    {
        auto key = KeyOpenAiBox().Password();
        if (key.empty())
        {
            OpenAiTestResultText().Text(L"Please enter an API key first.");
            co_return;
        }

        OpenAiTestResultText().Text(L"Connecting to OpenAI...");
        TestOpenAiBtn().IsEnabled(false);

        CredentialStore::SetCredential(CredentialStore::KeyOpenAI, key.c_str());

        co_await resume_background();
        AIRequest req;
        req.operation = AIOperation::Rewrite;
        req.text = L"Hello";
        auto res = OpenAIService::Instance().Execute(req);

        DispatcherQueue().TryEnqueue([this, res]()
        {
            TestOpenAiBtn().IsEnabled(true);
            if (res.success)
            {
                OpenAiTestResultText().Text(L"Connected successfully (gpt-4o-mini) ✓");
            }
            else
            {
                OpenAiTestResultText().Text(winrt::hstring(res.errorMessage));
            }
            UpdateAiStatuses();
        });
    }
}
