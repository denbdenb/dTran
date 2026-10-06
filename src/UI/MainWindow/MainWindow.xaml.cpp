#include "pch.h"
#include "MainWindow.xaml.h"
#include "App.xaml.h"
#include "SettingsWindow.xaml.h"
#include "TranslationManager.h"
#include "GoogleTtsService.h"
#include "WindowsOcrService.h"
#include "ScreenSnipper.h"
#include "WikipediaService.h"
#include "ReversoService.h"
#include "GeminiService.h"
#include "OpenAIService.h"
#include "HistoryManager.h"
#include "SettingsManager.h"
#include "CredentialStore.h"
#include "WindowsIntegration.h"
#include "ClipboardHelper.h"
#include "LanguageCatalog.h"

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

        auto const& settings = SettingsManager::Instance().GetSettings();
        int w = settings.mainWindowWidth > 500 ? settings.mainWindowWidth : 1020;
        int h = settings.mainWindowHeight > 400 ? settings.mainWindowHeight : 720;
        AppWindow().Resize({ w, h });

        ApplyTheme(settings.theme);

        // Close button -> minimize to tray handling
        AppWindow().Closing([this](auto&&, Microsoft::UI::Windowing::AppWindowClosingEventArgs const& args)
        {
            auto const& s = SettingsManager::Instance().GetSettings();
            if (s.closeToTray)
            {
                args.Cancel(true);
                AppWindow().Hide();
            }
        });

        // Window resize persistence
        AppWindow().Changed([this](auto&& sender, Microsoft::UI::Windowing::AppWindowChangedEventArgs const& args)
        {
            if (args.DidSizeChange())
            {
                auto size = sender.Size();
                if (size.Width > 500 && size.Height > 400)
                {
                    auto s = SettingsManager::Instance().GetSettings();
                    s.mainWindowWidth = size.Width;
                    s.mainWindowHeight = size.Height;
                    SettingsManager::Instance().UpdateSettings(s);
                }
            }
        });

        m_currentServiceIndex = settings.primaryService;
        PopulateLanguagesForService(m_currentServiceIndex);
        PopulateDictionaryLanguages();
        SetupEventHandlers();
        UpdateCharCount();
        UpdateOcrTooltip();

        // Restore sidebar state
        SetSidebarState(settings.sidebarCollapsed);
        SelectService(m_currentServiceIndex);

        // Default sample text
        SourceTextBox().Text(L"The quick brown fox jumps over the lazy dog.");
        ResultTextBlock().Text(L"Быстрая коричневая лиса перепрыгивает через ленивую собаку.");
        UpdateCharCount();

        // Register observer for live settings updates
        SettingsManager::Instance().RegisterObserver(reinterpret_cast<uintptr_t>(this), [this](AppSettings const& s)
        {
            DispatcherQueue().TryEnqueue([this, s]()
            {
                ApplyTheme(s.theme);
                UpdateOcrTooltip();
                PopulateLanguagesForService(m_currentServiceIndex);
            });
        });
    }

    MainWindow::~MainWindow()
    {
        SettingsManager::Instance().UnregisterObserver(reinterpret_cast<uintptr_t>(this));
    }

    void MainWindow::ApplyTheme(std::wstring const& themeName)
    {
        if (auto root = Content().try_as<FrameworkElement>())
        {
            if (themeName == L"Light")
            {
                root.RequestedTheme(ElementTheme::Light);
                m_isDarkMode = false;
            }
            else if (themeName == L"Dark")
            {
                root.RequestedTheme(ElementTheme::Dark);
                m_isDarkMode = true;
            }
            else
            {
                root.RequestedTheme(ElementTheme::Default);
                m_isDarkMode = (Application::Current().RequestedTheme() == ApplicationTheme::Dark);
            }
        }
        ThemeText().Text(m_isDarkMode ? L"Dark" : L"Light");
        ThemeIcon().Glyph(m_isDarkMode ? L"\uE708" : L"\uE706");
    }

    void MainWindow::UpdateOcrTooltip()
    {
        auto const& s = SettingsManager::Instance().GetSettings();
        std::wstring hk = s.ocrHotkey.empty() ? L"Ctrl+Alt+O" : s.ocrHotkey;
        std::wstring tip = L"Screen OCR & Translate (" + hk + L")";
        ToolTipService::SetToolTip(ScreenOcrBtn(), box_value(winrt::hstring(tip)));
    }

    void MainWindow::PopulateLanguagesForService(int serviceId)
    {
        auto const& settings = SettingsManager::Instance().GetSettings();
        auto curSrc = GetSourceLangCode();
        auto curTgt = GetTargetLangCode();
        if (curSrc.empty() || curSrc == L"auto") curSrc = settings.sourceLanguage;
        if (curTgt.empty()) curTgt = settings.targetLanguage;

        curSrc = LanguageCatalog::ValidateLanguageForService(serviceId, curSrc, true);
        curTgt = LanguageCatalog::ValidateLanguageForService(serviceId, curTgt, false);

        auto srcLangs = LanguageCatalog::GetLanguagesForService(serviceId, true);
        auto tgtLangs = LanguageCatalog::GetLanguagesForService(serviceId, false);

        m_sourceLangCodes.clear();
        SourceLanguageCombo().Items().Clear();
        int srcIdx = 0;
        for (size_t i = 0; i < srcLangs.size(); ++i)
        {
            auto const& lang = srcLangs[i];
            m_sourceLangCodes.push_back(lang.code);
            ComboBoxItem item;
            item.Content(box_value(winrt::hstring(lang.DisplayName())));
            SourceLanguageCombo().Items().Append(item);
            if (_wcsicmp(lang.code.c_str(), curSrc.c_str()) == 0)
            {
                srcIdx = static_cast<int>(i);
            }
        }
        SourceLanguageCombo().SelectedIndex(srcIdx);

        m_targetLangCodes.clear();
        TargetLanguageCombo().Items().Clear();
        int tgtIdx = 0;
        for (size_t i = 0; i < tgtLangs.size(); ++i)
        {
            auto const& lang = tgtLangs[i];
            m_targetLangCodes.push_back(lang.code);
            ComboBoxItem item;
            item.Content(box_value(winrt::hstring(lang.DisplayName())));
            TargetLanguageCombo().Items().Append(item);
            if (_wcsicmp(lang.code.c_str(), curTgt.c_str()) == 0)
            {
                tgtIdx = static_cast<int>(i);
            }
        }
        TargetLanguageCombo().SelectedIndex(tgtIdx);
    }

    void MainWindow::PopulateDictionaryLanguages()
    {
        auto const& settings = SettingsManager::Instance().GetSettings();
        auto reversoLangs = LanguageCatalog::GetLanguagesForService(4, false);

        m_dictSourceLangCodes.clear();
        m_dictTargetLangCodes.clear();
        DictSourceLangCombo().Items().Clear();
        DictTargetLangCombo().Items().Clear();

        int dSrcIdx = 0;
        int dTgtIdx = 0;

        for (size_t i = 0; i < reversoLangs.size(); ++i)
        {
            auto const& lang = reversoLangs[i];
            m_dictSourceLangCodes.push_back(lang.code);
            m_dictTargetLangCodes.push_back(lang.code);

            ComboBoxItem item1;
            item1.Content(box_value(winrt::hstring(lang.DisplayName())));
            DictSourceLangCombo().Items().Append(item1);

            ComboBoxItem item2;
            item2.Content(box_value(winrt::hstring(lang.DisplayName())));
            DictTargetLangCombo().Items().Append(item2);

            if (_wcsicmp(lang.code.c_str(), settings.dictSourceLang.c_str()) == 0)
            {
                dSrcIdx = static_cast<int>(i);
            }
            if (_wcsicmp(lang.code.c_str(), settings.dictTargetLang.c_str()) == 0)
            {
                dTgtIdx = static_cast<int>(i);
            }
        }

        DictSourceLangCombo().SelectedIndex(dSrcIdx);
        DictTargetLangCombo().SelectedIndex(dTgtIdx);
        DictionarySourceCombo().SelectedIndex(settings.dictEngine);

        auto updateDictUi = [this]()
        {
            int engine = DictionarySourceCombo().SelectedIndex();
            DictTargetLangCombo().Visibility(engine == 1 ? Visibility::Collapsed : Visibility::Visible);
        };
        updateDictUi();

        DictionarySourceCombo().SelectionChanged([updateDictUi](auto&&, auto&&)
        {
            updateDictUi();
        });
    }

    std::wstring MainWindow::GetSourceLangCode()
    {
        int idx = SourceLanguageCombo().SelectedIndex();
        if (idx >= 0 && idx < static_cast<int>(m_sourceLangCodes.size()))
        {
            return m_sourceLangCodes[idx];
        }
        return L"auto";
    }

    std::wstring MainWindow::GetTargetLangCode()
    {
        int idx = TargetLanguageCombo().SelectedIndex();
        if (idx >= 0 && idx < static_cast<int>(m_targetLangCodes.size()))
        {
            return m_targetLangCodes[idx];
        }
        return L"ru";
    }

    std::wstring MainWindow::GetDictSourceLangCode()
    {
        int idx = DictSourceLangCombo().SelectedIndex();
        if (idx >= 0 && idx < static_cast<int>(m_dictSourceLangCodes.size()))
        {
            return m_dictSourceLangCodes[idx];
        }
        return L"en";
    }

    std::wstring MainWindow::GetDictTargetLangCode()
    {
        int idx = DictTargetLangCombo().SelectedIndex();
        if (idx >= 0 && idx < static_cast<int>(m_dictTargetLangCodes.size()))
        {
            return m_dictTargetLangCodes[idx];
        }
        return L"ru";
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

        // Sidebar Collapse Toggle
        SidebarToggleBtn().Click([this](auto&&, auto&&)
        {
            ToggleSidebar();
        });

        // Translate Button
        TranslateActionBtn().Click([this](auto&&, auto&&)
        {
            OnTranslateAsync();
        });

        // Swap Languages
        SwapLanguagesBtn().Click([this](auto&&, auto&&)
        {
            OnSwapLanguages();
        });

        // Text Box change -> count
        SourceTextBox().TextChanged([this](auto&&, auto&&)
        {
            UpdateCharCount();
        });

        // Source clear, copy, speak
        ClearSourceBtn().Click([this](auto&&, auto&&)
        {
            SourceTextBox().Text(L"");
            ResultTextBlock().Text(L"");
            UpdateCharCount();
        });

        CopySourceBtn().Click([this](auto&&, auto&&)
        {
            CopyTextToClipboard(SourceTextBox().Text());
        });

        SpeakSourceBtn().Click([this](auto&&, auto&&)
        {
            OnSpeakSource();
        });

        // Result copy & speak
        CopyResultBtn().Click([this](auto&&, auto&&)
        {
            CopyTextToClipboard(ResultTextBlock().Text());
        });

        SpeakResultBtn().Click([this](auto&&, auto&&)
        {
            OnSpeakResult();
        });

        // Screen OCR Button
        ScreenOcrBtn().Click([this](auto&&, auto&&)
        {
            OnScreenSnippingOcr();
        });

        // Dictionary Search
        DictionarySearchBtn().Click([this](auto&&, auto&&)
        {
            OnDictionarySearch();
        });

        // Context Flyout Menu Items for Source
        LookupSourceDictionaryMenuItem().Click([this](auto&&, auto&&)
        {
            auto sel = SourceTextBox().SelectedText();
            std::wstring query = sel.empty() ? SourceTextBox().Text().c_str() : sel.c_str();
            LookupInDictionary(query, GetSourceLangCode(), GetTargetLangCode());
        });

        CutSourceMenuItem().Click([this](auto&&, auto&&)
        {
            auto sel = SourceTextBox().SelectedText();
            if (!sel.empty())
            {
                CopyTextToClipboard(sel);
                int start = SourceTextBox().SelectionStart();
                int len = SourceTextBox().SelectionLength();
                std::wstring txt = SourceTextBox().Text().c_str();
                if (start >= 0 && start + len <= static_cast<int>(txt.size()))
                {
                    txt.erase(start, len);
                    SourceTextBox().Text(winrt::hstring(txt));
                    SourceTextBox().SelectionStart(start);
                }
            }
        });

        CopySourceMenuItem().Click([this](auto&&, auto&&)
        {
            auto sel = SourceTextBox().SelectedText();
            CopyTextToClipboard(sel.empty() ? SourceTextBox().Text() : sel);
        });

        PasteSourceMenuItem().Click([this](auto&&, auto&&)
        {
            auto clip = ::dTranslate::Windows::ClipboardHelper::GetText();
            if (!clip.empty())
            {
                int start = SourceTextBox().SelectionStart();
                int len = SourceTextBox().SelectionLength();
                std::wstring txt = SourceTextBox().Text().c_str();
                if (start >= 0 && start + len <= static_cast<int>(txt.size()))
                {
                    txt.replace(start, len, clip);
                    SourceTextBox().Text(winrt::hstring(txt));
                    SourceTextBox().SelectionStart(static_cast<int>(start + clip.size()));
                }
                else
                {
                    SourceTextBox().Text(winrt::hstring(txt + clip));
                }
            }
        });

        SelectAllSourceMenuItem().Click([this](auto&&, auto&&)
        {
            SourceTextBox().SelectAll();
        });

        // Context Flyout Menu Items for Result
        LookupResultDictionaryMenuItem().Click([this](auto&&, auto&&)
        {
            auto sel = ResultTextBlock().SelectedText();
            std::wstring query = sel.empty() ? ResultTextBlock().Text().c_str() : sel.c_str();
            LookupInDictionary(query, GetTargetLangCode(), GetSourceLangCode());
        });

        CopyResultMenuItem().Click([this](auto&&, auto&&)
        {
            auto sel = ResultTextBlock().SelectedText();
            CopyTextToClipboard(sel.empty() ? ResultTextBlock().Text() : sel);
        });

        SelectAllResultMenuItem().Click([this](auto&&, auto&&)
        {
            ResultTextBlock().SelectAll();
        });

        // AI Run
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
            auto selected = HistoryListView().SelectedItem();
            if (selected != nullptr)
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

        // Open Settings window (Single Instance)
        SidebarSettingsBtn().Click([this](auto&&, auto&&)
        {
            if (auto app = App::CurrentApp())
            {
                app->ShowOrActivateSettings();
            }
        });

        // Global Screen OCR hotkey trigger
        ::dTranslate::Windows::WindowsIntegration::Instance().SetOnScreenOcr([this]()
        {
            OnScreenSnippingOcr();
        });
    }

    void MainWindow::LookupInDictionary(std::wstring const& query, std::wstring const& sourceLang, std::wstring const& targetLang)
    {
        if (query.empty()) return;
        SelectNavView(1);
        DictionarySearchBox().Text(winrt::hstring(query));

        for (size_t i = 0; i < m_dictSourceLangCodes.size(); ++i)
        {
            if (_wcsicmp(m_dictSourceLangCodes[i].c_str(), sourceLang.c_str()) == 0)
            {
                DictSourceLangCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        for (size_t i = 0; i < m_dictTargetLangCodes.size(); ++i)
        {
            if (_wcsicmp(m_dictTargetLangCodes[i].c_str(), targetLang.c_str()) == 0)
            {
                DictTargetLangCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        OnDictionarySearch();
    }

    void MainWindow::SetSidebarState(bool collapsed)
    {
        m_sidebarCollapsed = collapsed;
        SidebarColumn().Width(GridLength{ collapsed ? 56.0 : 180.0, GridUnitType::Pixel });

        auto visibility = collapsed ? Visibility::Collapsed : Visibility::Visible;
        SidebarBrandPanel().Visibility(visibility);
        NavTranslateText().Visibility(visibility);
        NavDictionaryText().Visibility(visibility);
        NavAiText().Visibility(visibility);
        NavHistoryText().Visibility(visibility);
        SettingsText().Visibility(visibility);
        ThemeText().Visibility(visibility);

        SidebarToggleIcon().Glyph(collapsed ? L"\uE70E" : L"\uE700");
    }

    void MainWindow::ToggleSidebar()
    {
        SetSidebarState(!m_sidebarCollapsed);

        auto s = SettingsManager::Instance().GetSettings();
        s.sidebarCollapsed = m_sidebarCollapsed;
        SettingsManager::Instance().UpdateSettings(s);
    }

    void MainWindow::SelectNavView(int index)
    {
        m_currentNavIndex = index;

        auto transparentBrush = SolidColorBrush(Windows::UI::Color{ 0, 0, 0, 0 });
        auto activeBrush = SolidColorBrush(Windows::UI::Color{ 25, 37, 99, 235 });

        NavTranslateBtn().Background(index == 0 ? activeBrush : transparentBrush);
        NavDictionaryBtn().Background(index == 1 ? activeBrush : transparentBrush);
        NavAiBtn().Background(index == 2 ? activeBrush : transparentBrush);
        NavHistoryBtn().Background(index == 3 ? activeBrush : transparentBrush);

        TranslateView().Visibility(index == 0 ? Visibility::Visible : Visibility::Collapsed);
        DictionaryView().Visibility(index == 1 ? Visibility::Visible : Visibility::Collapsed);
        AiView().Visibility(index == 2 ? Visibility::Visible : Visibility::Collapsed);
        HistoryView().Visibility(index == 3 ? Visibility::Visible : Visibility::Collapsed);
    }

    void MainWindow::UpdateCharCount()
    {
        auto text = SourceTextBox().Text();
        int count = static_cast<int>(text.size());
        CharCountText().Text(winrt::hstring(std::to_wstring(count) + L"/5000"));
    }

    void MainWindow::OnSwapLanguages()
    {
        auto srcCode = GetSourceLangCode();
        auto tgtCode = GetTargetLangCode();

        if (srcCode == L"auto")
        {
            srcCode = L"en";
        }

        for (size_t i = 0; i < m_sourceLangCodes.size(); ++i)
        {
            if (_wcsicmp(m_sourceLangCodes[i].c_str(), tgtCode.c_str()) == 0)
            {
                SourceLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        for (size_t i = 0; i < m_targetLangCodes.size(); ++i)
        {
            if (_wcsicmp(m_targetLangCodes[i].c_str(), srcCode.c_str()) == 0)
            {
                TargetLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        auto resText = ResultTextBlock().Text();
        auto srcText = SourceTextBox().Text();
        if (!resText.empty() && resText != L"Translation will appear here..." && resText != L"Translating...")
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

        TranslationRequest req;
        req.text = text.c_str();
        req.sourceLang = GetSourceLangCode();
        req.targetLang = GetTargetLangCode();

        if (m_currentServiceIndex == 2 && CredentialStore::GetCredential(CredentialStore::KeyGemini).empty())
        {
            ResultTextBlock().Text(winrt::hstring(
                L"Google Gemini requires an API key.\n\nOpen Settings (gear icon) -> AI Services tab to configure your key."));
            return;
        }
        if (m_currentServiceIndex == 3 && CredentialStore::GetCredential(CredentialStore::KeyOpenAI).empty())
        {
            ResultTextBlock().Text(winrt::hstring(
                L"OpenAI requires an API key.\n\nOpen Settings (gear icon) -> AI Services tab to configure your key."));
            return;
        }

        ResultTextBlock().Text(L"Translating...");

        TranslationManager::Instance().TranslateAsync(
            req,
            m_currentServiceIndex,
            DispatcherQueue(),
            [this, req](TranslationResult const& result)
            {
                if (result.success)
                {
                    ResultTextBlock().Text(winrt::hstring(result.translatedText));
                    HistoryItem item;
                    item.service = result.serviceName;
                    item.sourceLang = req.sourceLang;
                    item.targetLang = req.targetLang;
                    item.originalText = req.text;
                    item.translatedText = result.translatedText;
                    HistoryManager::Instance().AddItem(item);
                }
                else
                {
                    ResultTextBlock().Text(winrt::hstring(L"Error: " + result.errorMessage));
                }
            });
    }

    void MainWindow::OnSpeakSource()
    {
        auto text = SourceTextBox().Text();
        if (!text.empty())
        {
            GoogleTtsService::Instance().Speak(text.c_str(), GetSourceLangCode());
        }
    }

    void MainWindow::OnSpeakResult()
    {
        auto text = ResultTextBlock().Text();
        if (!text.empty() && text != L"Translation will appear here..." && text != L"Translating...")
        {
            GoogleTtsService::Instance().Speak(text.c_str(), GetTargetLangCode());
        }
    }

    void MainWindow::TriggerScreenOcr()
    {
        OnScreenSnippingOcr();
    }

    void MainWindow::OnScreenSnippingOcr()
    {
        ScreenSnipper::Instance().StartSnipping(
            GetSourceLangCode(),
            DispatcherQueue(),
            [this](std::wstring const& recognizedText, bool autoTranslate)
            {
                if (recognizedText.empty()) return;

                if (autoTranslate)
                {
                    SourceTextBox().Text(winrt::hstring(recognizedText));
                    UpdateCharCount();
                    SelectNavView(0);
                    AppWindow().Show();
                    OnTranslateAsync();
                }
                else
                {
                    if (recognizedText.rfind(L"Windows OCR", 0) == 0 ||
                        recognizedText.rfind(L"Selected area", 0) == 0 ||
                        recognizedText.rfind(L"No Windows OCR", 0) == 0)
                    {
                        SelectNavView(0);
                        AppWindow().Show();
                        ResultTextBlock().Text(winrt::hstring(recognizedText));
                    }
                    else
                    {
                        CopyTextToClipboard(winrt::hstring(recognizedText));
                    }
                }
            });
    }

    fire_and_forget MainWindow::OnDictionarySearch()
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
            DictionaryResultText().Text(L"Enter a word, phrase or topic to look up.");
            co_return;
        }

        int sourceIdx = DictionarySourceCombo().SelectedIndex();
        DictionaryResultText().Text(sourceIdx == 1 ? L"Searching Wikipedia..." : L"Searching Reverso Context...");

        DictionaryRequest req;
        req.word = query.c_str();
        req.sourceLang = GetDictSourceLangCode();
        req.targetLang = GetDictTargetLangCode();

        co_await resume_background();

        auto result = (sourceIdx == 1) ? WikipediaService::Instance().Lookup(req) : ReversoService::Instance().Lookup(req);
        DispatcherQueue().TryEnqueue([this, result]()
        {
            if (result.success)
            {
                std::wstring output = result.title.empty() ? result.content : (result.title + L"\n\n" + result.content);
                DictionaryResultText().Text(winrt::hstring(output));
            }
            else
            {
                DictionaryResultText().Text(winrt::hstring(result.errorMessage));
            }
        });
    }

    fire_and_forget MainWindow::OnAiRunAsync()
    {
        auto input = AiInputBox().Text();
        if (input.empty())
        {
            AiResultText().Text(L"Please enter input text.");
            co_return;
        }

        AiRunBtn().IsEnabled(false);
        AiResultText().Text(L"Processing with AI...");

        int backendIdx = AiBackendCombo().SelectedIndex();
        int opIdx = AiOperationCombo().SelectedIndex();

        AIOperation op = AIOperation::Rewrite;
        switch (opIdx)
        {
        case 0: op = AIOperation::Rewrite; break;
        case 1: op = AIOperation::Improve; break;
        case 2: op = AIOperation::Summarize; break;
        case 3: op = AIOperation::Explain; break;
        default: op = AIOperation::Rewrite; break;
        }

        AIRequest req;
        req.text = input.c_str();
        req.targetLang = GetTargetLangCode() == L"ru" ? L"Russian" : L"English";
        req.operation = op;

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

        DispatcherQueue().TryEnqueue([this, result]()
        {
            AiRunBtn().IsEnabled(true);
            if (result.success)
            {
                AiResultText().Text(winrt::hstring(result.text));
            }
            else
            {
                AiResultText().Text(winrt::hstring(L"AI Error: " + result.errorMessage));
            }
        });
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
        auto s = SettingsManager::Instance().GetSettings();
        s.theme = m_isDarkMode ? L"Dark" : L"Light";
        SettingsManager::Instance().UpdateSettings(s);
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

        PopulateLanguagesForService(serviceId);
    }

    void MainWindow::CopyTextToClipboard(winrt::hstring const& text)
    {
        if (text.empty()) return;
        DataPackage package;
        package.SetText(text);
        Clipboard::SetContent(package);
    }
}
