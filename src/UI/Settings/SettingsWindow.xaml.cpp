#include "pch.h"
#include "SettingsWindow.xaml.h"
#include "SettingsManager.h"
#include "CredentialStore.h"
#include "GeminiService.h"
#include "OpenAIService.h"

#if __has_include("SettingsWindow.g.cpp")
#include "SettingsWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace ::dTranslate::Storage;
using namespace ::dTranslate::AI;

namespace winrt::dTranslate::implementation
{
    SettingsWindow::SettingsWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(SettingsTitleBar());
        AppWindow().Resize({ 960, 680 });

        LoadSettings();
        SetupEventHandlers();
    }

    void SettingsWindow::LoadSettings()
    {
        auto const& settings = SettingsManager::Instance().GetSettings();
        CompareTranslationsCheck().IsChecked(settings.compareTranslations);
        AutoStartToggle().IsOn(settings.autoStart);

        if (settings.theme == L"Light")
            ThemeCombo().SelectedIndex(1);
        else if (settings.theme == L"Dark")
            ThemeCombo().SelectedIndex(2);
        else
            ThemeCombo().SelectedIndex(0);

        int engineIdx = settings.primaryService;
        if (engineIdx >= 0 && engineIdx < 4)
        {
            PrimaryEngineCombo().SelectedIndex(engineIdx);
        }

        if (!settings.geminiModel.empty())
        {
            ModelGeminiBox().Text(winrt::hstring(settings.geminiModel));
        }
        if (!settings.openAiModel.empty())
        {
            ModelOpenAiBox().Text(winrt::hstring(settings.openAiModel));
        }

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

        SettingsCancelBtn().Click([this](auto&&, auto&&)
        {
            Close();
        });

        SettingsSaveBtn().Click([this](auto&&, auto&&)
        {
            // 1. Save settings
            auto settings = SettingsManager::Instance().GetSettings();
            auto isChecked = CompareTranslationsCheck().IsChecked();
            if (isChecked != nullptr)
            {
                settings.compareTranslations = isChecked.Value();
            }

            settings.autoStart = AutoStartToggle().IsOn();

            int themeIdx = ThemeCombo().SelectedIndex();
            if (themeIdx == 1) settings.theme = L"Light";
            else if (themeIdx == 2) settings.theme = L"Dark";
            else settings.theme = L"Default";

            int primaryIdx = PrimaryEngineCombo().SelectedIndex();
            if (primaryIdx >= 0) settings.primaryService = primaryIdx;

            auto gModel = ModelGeminiBox().Text();
            if (!gModel.empty()) settings.geminiModel = gModel.c_str();

            auto oModel = ModelOpenAiBox().Text();
            if (!oModel.empty()) settings.openAiModel = oModel.c_str();

            SettingsManager::Instance().UpdateSettings(settings);

            // 2. Save or remove API credentials securely in Windows Credential Manager
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
                GeminiTestResultText().Text(L"Success! Connected to Gemini.");
            }
            else
            {
                GeminiTestResultText().Text(winrt::hstring(L"Failed: " + res.errorMessage));
            }
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
                OpenAiTestResultText().Text(L"Success! Connected to OpenAI.");
            }
            else
            {
                OpenAiTestResultText().Text(winrt::hstring(L"Failed: " + res.errorMessage));
            }
        });
    }
}
