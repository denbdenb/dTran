#include "pch.h"
#include "QuickPopupWindow.xaml.h"
#include "SettingsWindow.xaml.h"
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

        // Swap languages
        PopupSwapLanguagesBtn().Click([this](auto&&, auto&&)
        {
            OnSwapLanguages();
        });

        // Translate button
        PopupTranslateActionBtn().Click([this](auto&&, auto&&)
        {
            OnTranslate();
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
            OnTranslate();
        }
    }

    void QuickPopupWindow::OnTranslate()
    {
        auto text = PopupSelectedTextBlock().Text();
        if (text.empty())
        {
            PopupResultTextBlock().Text(L"No text selected to translate.");
            return;
        }

        if (text == L"The quick brown fox jumps over the lazy dog.")
        {
            PopupResultTextBlock().Text(L"Быстрая коричневая лиса перепрыгивает через ленивую собаку.");
        }
        else
        {
            PopupResultTextBlock().Text(L"[" + text + L"]");
        }
    }

    void QuickPopupWindow::OnSwapLanguages()
    {
        auto srcIdx = PopupSourceLanguageCombo().SelectedIndex();
        auto tgtIdx = PopupTargetLanguageCombo().SelectedIndex();

        PopupSourceLanguageCombo().SelectedIndex(tgtIdx >= 0 ? tgtIdx : 1);
        PopupTargetLanguageCombo().SelectedIndex(srcIdx >= 0 ? srcIdx : 0);

        auto srcText = PopupSelectedTextBlock().Text();
        auto resText = PopupResultTextBlock().Text();
        if (!resText.empty())
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
