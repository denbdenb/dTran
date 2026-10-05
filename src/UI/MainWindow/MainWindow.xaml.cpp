#include "pch.h"
#include "MainWindow.xaml.h"
#include "SettingsWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::ApplicationModel::DataTransfer;

namespace winrt::dTranslate::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
        AppWindow().Resize({ 1020, 720 });

        SetupEventHandlers();
        UpdateCharCount();

        // Default sample text matching the concept design for immediate visual validation
        SourceTextBox().Text(L"The quick brown fox jumps over the lazy dog.");
        ResultTextBlock().Text(L"Быстрая коричневая лиса перепрыгивает через ленивую собаку.");
        UpdateCharCount();
    }

    void MainWindow::SetupEventHandlers()
    {
        // Sidebar navigation
        NavTranslateBtn().Click([this](auto&&, auto&&) { SelectNavView(0); });
        NavDictionaryBtn().Click([this](auto&&, auto&&) { SelectNavView(1); });
        NavAiBtn().Click([this](auto&&, auto&&) { SelectNavView(2); });
        NavHistoryBtn().Click([this](auto&&, auto&&) { SelectNavView(3); });

        // Source text changes
        SourceTextBox().TextChanged([this](auto&&, auto&&) { UpdateCharCount(); });
        ClearSourceBtn().Click([this](auto&&, auto&&)
        {
            SourceTextBox().Text(L"");
            ResultTextBlock().Text(L"");
            UpdateCharCount();
        });

        // Clipboard copy
        CopySourceBtn().Click([this](auto&&, auto&&)
        {
            CopyTextToClipboard(SourceTextBox().Text());
        });
        CopyResultBtn().Click([this](auto&&, auto&&)
        {
            CopyTextToClipboard(ResultTextBlock().Text());
        });

        // Language swap
        SwapLanguagesBtn().Click([this](auto&&, auto&&)
        {
            OnSwapLanguages();
        });

        // Translate button
        TranslateActionBtn().Click([this](auto&&, auto&&)
        {
            OnTranslate();
        });

        // Theme toggle
        ThemeToggleBtn().Click([this](auto&&, auto&&)
        {
            ToggleTheme();
        });

        // Service selection buttons
        ServiceGoogleBtn().Click([this](auto&&, auto&&) { SelectService(0); });
        ServiceYandexBtn().Click([this](auto&&, auto&&) { SelectService(1); });
        ServiceGeminiBtn().Click([this](auto&&, auto&&) { SelectService(2); });
        ServiceOpenAiBtn().Click([this](auto&&, auto&&) { SelectService(3); });

        // Open Settings window
        auto openSettings = [this](auto&&, auto&&)
        {
            auto settingsWin = make<SettingsWindow>();
            settingsWin.Activate();
        };
        TopSettingsBtn().Click(openSettings);
        SidebarSettingsBtn().Click(openSettings);
    }

    void MainWindow::SelectNavView(int index)
    {
        m_currentNavIndex = index;

        // Reset sidebar button appearances
        auto transparentBrush = SolidColorBrush(Windows::UI::Color{ 0, 0, 0, 0 });
        NavTranslateBtn().Background(transparentBrush);
        NavDictionaryBtn().Background(transparentBrush);
        NavAiBtn().Background(transparentBrush);
        NavHistoryBtn().Background(transparentBrush);

        // Highlight active sidebar item
        auto subtleBrush = SolidColorBrush(Windows::UI::Color{ 25, 37, 99, 235 });
        if (index == 0) NavTranslateBtn().Background(subtleBrush);
        else if (index == 1) NavDictionaryBtn().Background(subtleBrush);
        else if (index == 2) NavAiBtn().Background(subtleBrush);
        else if (index == 3) NavHistoryBtn().Background(subtleBrush);

        // Switch views
        TranslateView().Visibility(index == 0 ? Visibility::Visible : Visibility::Collapsed);
        DictionaryView().Visibility(index == 1 ? Visibility::Visible : Visibility::Collapsed);
        AiView().Visibility(index == 2 ? Visibility::Visible : Visibility::Collapsed);
        HistoryView().Visibility(index == 3 ? Visibility::Visible : Visibility::Collapsed);
    }

    void MainWindow::UpdateCharCount()
    {
        auto len = SourceTextBox().Text().size();
        CharCountText().Text(winrt::to_hstring(len) + L"/5000");
    }

    void MainWindow::OnSwapLanguages()
    {
        auto srcIdx = SourceLanguageCombo().SelectedIndex();
        auto tgtIdx = TargetLanguageCombo().SelectedIndex();

        // Swap combo selections
        SourceLanguageCombo().SelectedIndex(tgtIdx >= 0 ? tgtIdx : 1);
        TargetLanguageCombo().SelectedIndex(srcIdx >= 0 ? srcIdx : 0);

        // Swap text contents if available
        auto srcText = SourceTextBox().Text();
        auto resText = ResultTextBlock().Text();
        if (!resText.empty() && resText != L"Translation will appear here...")
        {
            SourceTextBox().Text(resText);
            ResultTextBlock().Text(srcText);
            UpdateCharCount();
        }
    }

    void MainWindow::OnTranslate()
    {
        auto text = SourceTextBox().Text();
        if (text.empty())
        {
            ResultTextBlock().Text(L"Please enter text to translate.");
            return;
        }

        // Phase 2 visual preview / response:
        if (text == L"The quick brown fox jumps over the lazy dog.")
        {
            ResultTextBlock().Text(L"Быстрая коричневая лиса перепрыгивает через ленивую собаку.");
        }
        else
        {
            ResultTextBlock().Text(L"[" + text + L"]");
        }
    }

    void MainWindow::ToggleTheme()
    {
        m_isDarkMode = !m_isDarkMode;
        if (auto content = Content().try_as<FrameworkElement>())
        {
            content.RequestedTheme(m_isDarkMode ? ElementTheme::Dark : ElementTheme::Light);
        }

        ThemeText().Text(m_isDarkMode ? L"Dark" : L"Light");
        ThemeIcon().Glyph(m_isDarkMode ? L"\uE708" : L"\uE706");
    }

    void MainWindow::SelectService(int serviceId)
    {
        m_currentServiceIndex = serviceId;

        auto activeBorder = SolidColorBrush(Windows::UI::Color{ 255, 37, 99, 235 });
        auto defaultBorder = SolidColorBrush(Windows::UI::Color{ 40, 128, 128, 128 });

        ServiceGoogleBtn().BorderBrush(serviceId == 0 ? activeBorder : defaultBorder);
        ServiceGoogleBtn().BorderThickness(serviceId == 0 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });

        ServiceYandexBtn().BorderBrush(serviceId == 1 ? activeBorder : defaultBorder);
        ServiceYandexBtn().BorderThickness(serviceId == 1 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });

        ServiceGeminiBtn().BorderBrush(serviceId == 2 ? activeBorder : defaultBorder);
        ServiceGeminiBtn().BorderThickness(serviceId == 2 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });

        ServiceOpenAiBtn().BorderBrush(serviceId == 3 ? activeBorder : defaultBorder);
        ServiceOpenAiBtn().BorderThickness(serviceId == 3 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });
    }

    void MainWindow::CopyTextToClipboard(winrt::hstring const& text)
    {
        if (text.empty()) return;
        DataPackage package;
        package.SetText(text);
        Clipboard::SetContent(package);
    }
}
