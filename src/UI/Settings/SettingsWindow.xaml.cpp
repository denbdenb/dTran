#include "pch.h"
#include "SettingsWindow.xaml.h"
#include "SettingsManager.h"
#include "CredentialStore.h"

#if __has_include("SettingsWindow.g.cpp")
#include "SettingsWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace ::dTranslate::Storage;

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

        // Load existing API keys if configured
        auto geminiKey = CredentialStore::GetCredential(CredentialStore::KeyGemini);
        if (!geminiKey.empty()) KeyGeminiBox().Password(winrt::hstring(geminiKey));

        auto openAiKey = CredentialStore::GetCredential(CredentialStore::KeyOpenAI);
        if (!openAiKey.empty()) KeyOpenAiBox().Password(winrt::hstring(openAiKey));

        auto yandexKey = CredentialStore::GetCredential(CredentialStore::KeyYandex);
        if (!yandexKey.empty()) KeyYandexBox().Password(winrt::hstring(yandexKey));

        auto googleKey = CredentialStore::GetCredential(CredentialStore::KeyGoogle);
        if (!googleKey.empty()) KeyGoogleBox().Password(winrt::hstring(googleKey));
    }

    void SettingsWindow::SetupEventHandlers()
    {
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
            SettingsManager::Instance().UpdateSettings(settings);

            // 2. Save or remove API credentials securely
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
            saveKey(CredentialStore::KeyYandex, KeyYandexBox().Password());
            saveKey(CredentialStore::KeyGoogle, KeyGoogleBox().Password());

            Close();
        });
    }
}
