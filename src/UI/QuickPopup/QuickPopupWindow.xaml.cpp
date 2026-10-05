#include "pch.h"
#include "QuickPopupWindow.xaml.h"
#include "SettingsWindow.xaml.h"
#include "TranslationManager.h"
#include "GoogleTtsService.h"

#if __has_include("QuickPopupWindow.g.cpp")
#include "QuickPopupWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Input;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::System;
using namespace ::dTranslate::Translation;
using namespace ::dTranslate::Audio;

namespace winrt::dTranslate::implementation
{
    QuickPopupWindow::QuickPopupWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(PopupTitleBar());
        AppWindow().Resize({ 370, 440 });

        SetupEventHandlers();
    }

    std::wstring QuickPopupWindow::GetSourceLangCode()
    {
        auto idx = PopupSourceLanguageCombo().SelectedIndex();
        switch (idx)
        {
        case 0: return L"auto";
        case 1: return L"en";
        case 2: return L"ru";
        case 3: return L"de";
        case 4: return L"fr";
        case 5: return L"es";
        default: return L"auto";
        }
    }

    std::wstring QuickPopupWindow::GetTargetLangCode()
    {
        auto idx = PopupTargetLanguageCombo().SelectedIndex();
        switch (idx)
        {
        case 0: return L"ru";
        case 1: return L"en";
        case 2: return L"de";
        case 3: return L"fr";
        case 4: return L"es";
        default: return L"ru";
        }
    }

    void QuickPopupWindow::SetupEventHandlers()
    {
        // Settings button
        PopupSettingsBtn().Click([this](auto&&, auto&&)
        {
            auto settingsWin = make<SettingsWindow>();
            settingsWin.Activate();
        });

        // Close with Esc or close button
        PopupCloseBtn().Click([this](auto&&, auto&&)
        {
            Close();
        });

        // Key accelerator / Esc handling on root
        PopupRootGrid().KeyDown([this](auto&&, KeyRoutedEventArgs const& e)
        {
            if (e.Key() == VirtualKey::Escape)
            {
                Close();
                e.Handled(true);
            }
        });

        // Clear text button
        PopupClearTextBtn().Click([this](auto&&, auto&&)
        {
            PopupSelectedTextBlock().Text(L"");
            PopupResultTextBlock().Text(L"");
        });

        // Copy translation button
        PopupCopyResultBtn().Click([this](auto&&, auto&&)
        {
            CopyTextToClipboard(PopupResultTextBlock().Text());
        });

        // TTS button
        PopupSpeakBtn().Click([this](auto&&, auto&&)
        {
            auto text = PopupResultTextBlock().Text();
            if (!text.empty() && text != L"Translating...")
            {
                GoogleTtsService::Instance().Speak(text.c_str(), GetTargetLangCode());
            }
        });

        // Swap languages
        PopupSwapLanguagesBtn().Click([this](auto&&, auto&&)
        {
            OnSwapLanguages();
        });

        // Translate button
        PopupTranslateActionBtn().Click([this](auto&&, auto&&)
        {
            OnTranslateAsync();
        });

        // Service buttons
        PopupServiceGoogleBtn().Click([this](auto&&, auto&&) { SelectService(0); });
        PopupServiceYandexBtn().Click([this](auto&&, auto&&) { SelectService(1); });
        PopupServiceGeminiBtn().Click([this](auto&&, auto&&) { SelectService(2); });
    }

    void QuickPopupWindow::SetSelectedText(winrt::hstring const& text)
    {
        PopupSelectedTextBlock().Text(text);
        if (!text.empty())
        {
            OnTranslateAsync();
        }
    }

    void QuickPopupWindow::OnTranslateAsync()
    {
        auto text = PopupSelectedTextBlock().Text();
        if (text.empty())
        {
            PopupResultTextBlock().Text(L"No text selected to translate.");
            return;
        }

        PopupResultTextBlock().Text(L"Translating...");
        PopupTranslateActionBtn().IsEnabled(false);

        TranslationRequest req;
        req.text = text.c_str();
        req.sourceLang = GetSourceLangCode();
        req.targetLang = GetTargetLangCode();

        TranslationManager::Instance().TranslateAsync(
            req,
            m_selectedServiceIndex,
            DispatcherQueue(),
            [this](TranslationResult const& result)
            {
                PopupTranslateActionBtn().IsEnabled(true);

                if (result.success)
                {
                    PopupResultTextBlock().Text(winrt::hstring(result.translatedText));
                }
                else
                {
                    PopupResultTextBlock().Text(winrt::hstring(L"Error: " + result.errorMessage));
                }
            });
    }

    void QuickPopupWindow::OnSwapLanguages()
    {
        auto srcIdx = PopupSourceLanguageCombo().SelectedIndex();
        auto tgtIdx = PopupTargetLanguageCombo().SelectedIndex();

        PopupSourceLanguageCombo().SelectedIndex(tgtIdx >= 0 ? tgtIdx : 1);
        PopupTargetLanguageCombo().SelectedIndex(srcIdx >= 0 ? srcIdx : 0);

        auto srcText = PopupSelectedTextBlock().Text();
        auto resText = PopupResultTextBlock().Text();
        if (!resText.empty() && resText != L"Translating...")
        {
            PopupSelectedTextBlock().Text(resText);
            PopupResultTextBlock().Text(srcText);
        }
    }

    void QuickPopupWindow::SelectService(int serviceId)
    {
        m_selectedServiceIndex = serviceId;

        auto activeBorder = SolidColorBrush(Windows::UI::Color{ 255, 37, 99, 235 });
        auto defaultBorder = SolidColorBrush(Windows::UI::Color{ 40, 128, 128, 128 });

        PopupServiceGoogleBtn().BorderBrush(serviceId == 0 ? activeBorder : defaultBorder);
        PopupServiceGoogleBtn().BorderThickness(serviceId == 0 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });

        PopupServiceYandexBtn().BorderBrush(serviceId == 1 ? activeBorder : defaultBorder);
        PopupServiceYandexBtn().BorderThickness(serviceId == 1 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });

        PopupServiceGeminiBtn().BorderBrush(serviceId == 2 ? activeBorder : defaultBorder);
        PopupServiceGeminiBtn().BorderThickness(serviceId == 2 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });
    }

    void QuickPopupWindow::CopyTextToClipboard(winrt::hstring const& text)
    {
        if (text.empty()) return;
        DataPackage package;
        package.SetText(text);
        Clipboard::SetContent(package);
    }
}
