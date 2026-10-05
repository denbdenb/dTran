#include "pch.h"
#include "MainWindow.xaml.h"
#include "SettingsWindow.xaml.h"
#include "TranslationManager.h"
#include "GoogleTtsService.h"
#include "WindowsOcrService.h"
#include "WikipediaService.h"
#include "ReversoService.h"
#include "GeminiService.h"
#include "OpenAIService.h"
#include "HistoryManager.h"
#include "SettingsManager.h"

#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace ::dTranslate::Translation;
using namespace ::dTranslate::Audio;
using namespace ::dTranslate::OCR;
using namespace ::dTranslate::Dictionary;
using namespace ::dTranslate::AI;
using namespace ::dTranslate::Storage;

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

        // Default sample text matching the concept design
        SourceTextBox().Text(L"The quick brown fox jumps over the lazy dog.");
        ResultTextBlock().Text(L"Быстрая коричневая лиса перепрыгивает через ленивую собаку.");
        UpdateCharCount();
    }

    std::wstring MainWindow::GetSourceLangCode()
    {
        auto idx = SourceLanguageCombo().SelectedIndex();
        switch (idx)
        {
        case 0: return L"auto";
        case 1: return L"en";
        case 2: return L"ru";
        case 3: return L"de";
        case 4: return L"fr";
        case 5: return L"es";
        case 6: return L"it";
        case 7: return L"zh";
        case 8: return L"ja";
        default: return L"auto";
        }
    }

    std::wstring MainWindow::GetTargetLangCode()
    {
        auto idx = TargetLanguageCombo().SelectedIndex();
        switch (idx)
        {
        case 0: return L"ru";
        case 1: return L"en";
        case 2: return L"de";
        case 3: return L"fr";
        case 4: return L"es";
        case 5: return L"it";
        case 6: return L"zh";
        case 7: return L"ja";
        default: return L"ru";
        }
    }

    void MainWindow::SetupEventHandlers()
    {
        // Sidebar navigation
        NavTranslateBtn().Click([this](auto&&, auto&&) { SelectNavView(0); });
        NavDictionaryBtn().Click([this](auto&&, auto&&)
        {
            SelectNavView(1);
            if (DictionarySearchBox().Text().empty())
            {
                DictionarySearchBox().Text(SourceTextBox().Text());
            }
        });
        NavAiBtn().Click([this](auto&&, auto&&)
        {
            SelectNavView(2);
            if (AiInputBox().Text().empty())
            {
                AiInputBox().Text(SourceTextBox().Text());
            }
        });
        NavHistoryBtn().Click([this](auto&&, auto&&)
        {
            SelectNavView(3);
            RefreshHistory();
        });

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
            OnTranslateAsync();
        });

        // TTS Buttons
        SpeakResultBtn().Click([this](auto&&, auto&&)
        {
            OnSpeakResult();
        });
        UtilityTtsBtn().Click([this](auto&&, auto&&)
        {
            OnSpeakSource();
        });

        // OCR Button
        UtilityOcrBtn().Click([this](auto&&, auto&&)
        {
            OnOcrAsync();
        });

        // Wikipedia Button
        UtilityWikipediaBtn().Click([this](auto&&, auto&&)
        {
            OnWikipediaLookupAsync();
        });

        // Dictionary Button
        UtilityDictionaryBtn().Click([this](auto&&, auto&&)
        {
            SelectNavView(1);
            DictionarySearchBox().Text(SourceTextBox().Text());
            OnDictionaryLookupAsync();
        });

        // Dictionary View Search
        DictionarySearchBox().KeyDown([this](auto&&, Input::KeyRoutedEventArgs const& e)
        {
            if (e.Key() == Windows::System::VirtualKey::Enter)
            {
                OnDictionaryLookupAsync();
                e.Handled(true);
            }
        });

        // AI View Execute
        AiRunBtn().Click([this](auto&&, auto&&)
        {
            OnAiRunAsync();
        });

        // History Actions
        HistoryClearBtn().Click([this](auto&&, auto&&)
        {
            HistoryManager::Instance().Clear();
            RefreshHistory();
        });
        HistoryCopyBtn().Click([this](auto&&, auto&&)
        {
            if (auto selected = HistoryListView().SelectedItem())
            {
                if (auto lvi = selected.try_as<ListViewItem>())
                {
                    auto hstr = unbox_value_or<winrt::hstring>(lvi.Content(), L"");
                    if (!hstr.empty())
                    {
                        CopyTextToClipboard(hstr);
                    }
                }
            }
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

        SourceLanguageCombo().SelectedIndex(tgtIdx >= 0 ? tgtIdx : 1);
        TargetLanguageCombo().SelectedIndex(srcIdx >= 0 ? srcIdx : 0);

        auto srcText = SourceTextBox().Text();
        auto resText = ResultTextBlock().Text();
        if (!resText.empty() && resText != L"Translation will appear here...")
        {
            SourceTextBox().Text(resText);
            ResultTextBlock().Text(srcText);
            UpdateCharCount();
        }
    }

    void MainWindow::OnTranslateAsync()
    {
        auto text = SourceTextBox().Text();
        if (text.empty())
        {
            ResultTextBlock().Text(L"Please enter text to translate.");
            return;
        }

        ResultTextBlock().Text(L"Translating...");
        TranslateActionBtn().IsEnabled(false);

        TranslationRequest req;
        req.text = text.c_str();
        req.sourceLang = GetSourceLangCode();
        req.targetLang = GetTargetLangCode();

        TranslationManager::Instance().TranslateAsync(
            req,
            m_currentServiceIndex,
            DispatcherQueue(),
            [this](TranslationResult const& result)
            {
                TranslateActionBtn().IsEnabled(true);

                if (result.success)
                {
                    ResultTextBlock().Text(winrt::hstring(result.translatedText));
                }
                else
                {
                    ResultTextBlock().Text(winrt::hstring(L"Error: " + result.errorMessage));
                }
            });
    }

    void MainWindow::OnSpeakResult()
    {
        auto text = ResultTextBlock().Text();
        if (!text.empty() && text != L"Translation will appear here..." && text != L"Translating...")
        {
            GoogleTtsService::Instance().Speak(text.c_str(), GetTargetLangCode());
        }
    }

    void MainWindow::OnSpeakSource()
    {
        auto text = SourceTextBox().Text();
        if (!text.empty())
        {
            GoogleTtsService::Instance().Speak(text.c_str(), GetSourceLangCode());
        }
    }

    void MainWindow::OnOcrAsync()
    {
        ResultTextBlock().Text(L"Scanning clipboard image with Windows OCR...");
        WindowsOcrService::Instance().RecognizeFromClipboardAsync(
            DispatcherQueue(),
            [this](OcrResult const& ocrResult)
            {
                if (ocrResult.success)
                {
                    SourceTextBox().Text(winrt::hstring(ocrResult.text));
                    UpdateCharCount();
                    OnTranslateAsync();
                }
                else
                {
                    ResultTextBlock().Text(winrt::hstring(ocrResult.errorMessage));
                }
            });
    }

    fire_and_forget MainWindow::OnWikipediaLookupAsync()
    {
        auto text = SourceTextBox().Text();
        if (text.empty())
        {
            ResultTextBlock().Text(L"Enter a search topic in the source box to look up on Wikipedia.");
            co_return;
        }

        ResultTextBlock().Text(L"Searching Wikipedia...");
        DictionaryRequest req;
        req.word = text.c_str();
        req.sourceLang = GetSourceLangCode();
        req.targetLang = GetTargetLangCode();

        co_await resume_background();
        auto result = WikipediaService::Instance().Lookup(req);

        if (result.success)
        {
            std::wstring output = result.title + L"\n\n" + result.content;
            ResultTextBlock().Text(winrt::hstring(output));
        }
        else
        {
            ResultTextBlock().Text(winrt::hstring(result.errorMessage));
        }
    }

    fire_and_forget MainWindow::OnDictionaryLookupAsync()
    {
        auto query = DictionarySearchBox().Text();
        if (query.empty())
        {
            query = SourceTextBox().Text();
            if (!query.empty())
            {
                DictionarySearchBox().Text(query);
            }
        }

        if (query.empty())
        {
            DictionaryResultText().Text(L"Enter a word or phrase to look up.");
            co_return;
        }

        DictionaryResultText().Text(L"Searching Reverso Context...");

        DictionaryRequest req;
        req.word = query.c_str();
        req.sourceLang = GetSourceLangCode();
        req.targetLang = GetTargetLangCode();

        co_await resume_background();
        auto result = ReversoService::Instance().Lookup(req);

        if (result.success)
        {
            DictionaryResultText().Text(winrt::hstring(result.content));
        }
        else
        {
            DictionaryResultText().Text(winrt::hstring(result.errorMessage));
        }
    }

    fire_and_forget MainWindow::OnAiRunAsync()
    {
        auto input = AiInputBox().Text();
        if (input.empty())
        {
            input = SourceTextBox().Text();
            if (!input.empty())
            {
                AiInputBox().Text(input);
            }
        }

        if (input.empty())
        {
            AiResultText().Text(L"Please enter text for the AI operation.");
            co_return;
        }

        AiResultText().Text(L"Processing with AI...");
        AiRunBtn().IsEnabled(false);

        int backendIdx = AiBackendCombo().SelectedIndex();
        int opIdx = AiOperationCombo().SelectedIndex();

        AIRequest req;
        req.text = input.c_str();
        req.targetLang = GetTargetLangCode() == L"ru" ? L"Russian" : L"English";
        req.operation = static_cast<AIOperation>(opIdx >= 0 ? opIdx : 0);

        co_await resume_background();

        AIResult result;
        if (backendIdx == 0)
        {
            result = GeminiService::Instance().Execute(req);
        }
        else
        {
            result = OpenAIService::Instance().Execute(req);
        }

        AiRunBtn().IsEnabled(true);

        if (result.success)
        {
            AiResultText().Text(winrt::hstring(result.text));
        }
        else
        {
            AiResultText().Text(winrt::hstring(L"AI Error: " + result.errorMessage));
        }
    }

    void MainWindow::RefreshHistory()
    {
        HistoryListView().Items().Clear();
        auto const& items = HistoryManager::Instance().GetItems();
        for (auto const& item : items)
        {
            std::wstring line = item.timestamp + L" [" + item.service + L"]: " +
                item.originalText + L" ➔ " + item.translatedText;
            ListViewItem lvi;
            lvi.Content(box_value(winrt::hstring(line)));
            HistoryListView().Items().Append(lvi);
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
