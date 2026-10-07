#include "pch.h"
#include "QuickPopupWindow.xaml.h"
#if __has_include("QuickPopupWindow.g.cpp")
#include "QuickPopupWindow.g.cpp"
#endif

#include "../../Translation/TranslationManager.h"
#include "../../Translation/LanguageCatalog.h"
#include "../../Audio/GoogleTtsService.h"
#include "../../Storage/SettingsManager.h"
#include "../../Storage/HistoryManager.h"
#include "../../Storage/LocalizationManager.h"
#include "../../OCR/ScreenSnipper.h"
#include "../../OCR/WindowsOcrService.h"
#include "../../Windows/SelectionCapture.h"
#include "../../Windows/WindowsIntegration.h"
#include "../App/App.xaml.h"

#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.System.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.Foundation.h>
#include <microsoft.ui.xaml.window.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::System;
using namespace ::dTranslate::Translation;
using namespace ::dTranslate::Audio;
using namespace ::dTranslate::Storage;
using namespace ::dTranslate::OCR;
using namespace ::dTranslate::Windows;

namespace winrt::dTranslate::implementation
{
    QuickPopupWindow::QuickPopupWindow()
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

        auto const& settings = SettingsManager::Instance().GetSettings();
        int w = settings.quickPopupWidth > 350 ? settings.quickPopupWidth : 440;
        int h = settings.quickPopupHeight > 350 ? settings.quickPopupHeight : 460;
        AppWindow().Resize({ w, h });

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(PopupTitleBar());

        // Intercept close button: Hide to tray instead of destroying
        AppWindow().Closing([this](auto&&, Microsoft::UI::Windowing::AppWindowClosingEventArgs const& args)
        {
            args.Cancel(true);
            AppWindow().Hide();
            ::SetProcessWorkingSetSize(::GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        });

        // Persist window size
        AppWindow().Changed([this](auto&& sender, Microsoft::UI::Windowing::AppWindowChangedEventArgs const& args)
        {
            if (args.DidSizeChange())
            {
                auto size = sender.Size();
                if (size.Width > 350 && size.Height > 350)
                {
                    auto s = SettingsManager::Instance().GetSettings();
                    s.quickPopupWidth = size.Width;
                    s.quickPopupHeight = size.Height;
                    SettingsManager::Instance().UpdateSettings(s);
                }
            }
        });

        // React to system theme changes in real-time when theme is System
        if (auto fe = Content().try_as<FrameworkElement>())
        {
            fe.ActualThemeChanged([this](auto&& sender, auto&&)
            {
                auto s = SettingsManager::Instance().GetSettings();
                if (s.theme == L"System" || s.theme.empty())
                {
                    bool isDark = (sender.ActualTheme() == ElementTheme::Dark);
                    UpdateTitleBarColors(isDark);
                }
            });
        }

        ApplyTheme(settings.theme);

        SettingsManager::Instance().RegisterObserver(reinterpret_cast<uintptr_t>(this), [this](AppSettings const& s)
        {
            DispatcherQueue().TryEnqueue([this, theme = s.theme]()
            {
                ApplyTheme(theme);
            });
        });

        LocalizationManager::Instance().RegisterObserver(reinterpret_cast<uintptr_t>(this), [this](std::wstring const&)
        {
            DispatcherQueue().TryEnqueue([this]()
            {
                ApplyLocalization();
            });
        });

        SelectService(0);
        SetupEventHandlers();
        ApplyLocalization();
    }

    QuickPopupWindow::~QuickPopupWindow()
    {
        SettingsManager::Instance().UnregisterObserver(reinterpret_cast<uintptr_t>(this));
        LocalizationManager::Instance().UnregisterObserver(reinterpret_cast<uintptr_t>(this));
    }

    void QuickPopupWindow::ApplyTheme(std::wstring const& themeName)
    {
        auto fe = Content().try_as<FrameworkElement>();
        if (!fe) return;

        if (themeName == L"Light")
            fe.RequestedTheme(ElementTheme::Light);
        else if (themeName == L"Dark")
            fe.RequestedTheme(ElementTheme::Dark);
        else
            fe.RequestedTheme(ElementTheme::Default);

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
            isDark = (fe.ActualTheme() == ElementTheme::Dark);
        }

        UpdateTitleBarColors(isDark);
    }

    void QuickPopupWindow::UpdateTitleBarColors(bool isDark)
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
                // Dark Theme: Bright white glyphs with subtle white hover/pressed states
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
                // Light Theme: Crisp dark glyphs with subtle dark hover/pressed states
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

    void QuickPopupWindow::PopulateLanguagesForService(int serviceId)
    {
        auto srcLangs = LanguageCatalog::GetLanguagesForService(serviceId, true);
        auto tgtLangs = LanguageCatalog::GetLanguagesForService(serviceId, false);

        m_sourceLangCodes.clear();
        m_targetLangCodes.clear();

        PopupSourceLanguageCombo().Items().Clear();
        PopupTargetLanguageCombo().Items().Clear();

        auto const& settings = SettingsManager::Instance().GetSettings();
        std::wstring prefSrc = LanguageCatalog::ValidateLanguageForService(serviceId, settings.sourceLanguage, true);
        std::wstring prefTgt = LanguageCatalog::ValidateLanguageForService(serviceId, settings.targetLanguage, false);

        int srcIdx = 0;
        for (size_t i = 0; i < srcLangs.size(); ++i)
        {
            m_sourceLangCodes.push_back(srcLangs[i].code);
            PopupSourceLanguageCombo().Items().Append(CreateLanguageComboItem(srcLangs[i]));
            if (_wcsicmp(srcLangs[i].code.c_str(), prefSrc.c_str()) == 0)
            {
                srcIdx = static_cast<int>(i);
            }
        }
        PopupSourceLanguageCombo().SelectedIndex(srcIdx);

        int tgtIdx = 0;
        for (size_t i = 0; i < tgtLangs.size(); ++i)
        {
            m_targetLangCodes.push_back(tgtLangs[i].code);
            PopupTargetLanguageCombo().Items().Append(CreateLanguageComboItem(tgtLangs[i]));
            if (_wcsicmp(tgtLangs[i].code.c_str(), prefTgt.c_str()) == 0)
            {
                tgtIdx = static_cast<int>(i);
            }
        }
        PopupTargetLanguageCombo().SelectedIndex(tgtIdx);
    }

    std::wstring QuickPopupWindow::GetSourceLangCode()
    {
        auto idx = PopupSourceLanguageCombo().SelectedIndex();
        if (idx >= 0 && idx < static_cast<int>(m_sourceLangCodes.size()))
        {
            return m_sourceLangCodes[idx];
        }
        return L"auto";
    }

    std::wstring QuickPopupWindow::GetTargetLangCode()
    {
        auto idx = PopupTargetLanguageCombo().SelectedIndex();
        if (idx >= 0 && idx < static_cast<int>(m_targetLangCodes.size()))
        {
            return m_targetLangCodes[idx];
        }
        return L"ru";
    }

    void QuickPopupWindow::SetupEventHandlers()
    {
        // 1. Close button hides window
        PopupCloseBtn().Click([this](auto&&, auto&&)
        {
            AppWindow().Hide();
            ::SetProcessWorkingSetSize(::GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        });

        // 2. Global key down
        PopupRootGrid().KeyDown([this](auto&&, KeyRoutedEventArgs const& e)
        {
            if (e.Key() == VirtualKey::Escape)
            {
                if (m_inHistoryMode)
                {
                    ToggleHistoryView();
                }
                else
                {
                    AppWindow().Hide();
                    ::SetProcessWorkingSetSize(::GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
                }
                e.Handled(true);
            }
        });

        // 2B. Live character counter on text change
        PopupSourceTextBox().TextChanged([this](auto&&, auto&&)
        {
            UpdateCharCount();
        });

        // 3. Clear button
        PopupClearTextBtn().Click([this](auto&&, auto&&)
        {
            PopupSourceTextBox().Text(L"");
            PopupResultTextBlock().Text(L"");
            m_hasSelectionContext = false;
            m_sourceHwnd = nullptr;
            PopupReplaceBtn().IsEnabled(false);
            UpdateCharCount();
        });

        // 4. Copy button
        PopupCopyResultBtn().Click([this](auto&&, auto&&)
        {
            CopyTextToClipboard(PopupResultTextBlock().Text());
        });

        // 5. Replace source text button
        PopupReplaceBtn().Click([this](auto&&, auto&&)
        {
            OnReplaceSourceText();
        });

        // 6. Audio speak buttons
        PopupSourceSpeakBtn().Click([this](auto&&, auto&&) { OnSpeakSource(); });
        PopupSpeakBtn().Click([this](auto&&, auto&&) { OnSpeakResult(); });

        // 7. Swap languages
        PopupSwapLanguagesBtn().Click([this](auto&&, auto&&) { OnSwapLanguages(); });

        // 8. Restore defaults
        PopupRestoreDefaultsBtn().Click([this](auto&&, auto&&) { OnRestoreDefaultLanguages(); });

        // 9. OCR button
        PopupOcrBtn().Click([this](auto&&, auto&&) { OnScreenSnippingOcr(); });

        // 10. Translate action button
        PopupTranslateActionBtn().Click([this](auto&&, auto&&) { OnTranslateAsync(); });

        // 11. Service buttons
        PopupServiceGoogleBtn().Click([this](auto&&, auto&&) { SelectService(0); });
        PopupServiceYandexBtn().Click([this](auto&&, auto&&) { SelectService(1); });

        // 12. Bottom utility buttons: History & Settings
        PopupHistoryBtn().Click([this](auto&&, auto&&) { ToggleHistoryView(); });
        PopupSettingsBtn().Click([this](auto&&, auto&&)
        {
            App::CurrentApp()->ShowOrActivateSettings();
        });

        // 13. History search & action buttons
        PopupHistorySearchBox().TextChanged([this](auto&&, auto&&) { RefreshHistory(); });

        PopupHistoryInsertBtn().Click([this](auto&&, auto&&)
        {
            int idx = PopupHistoryListView().SelectedIndex();
            if (idx >= 0 && idx < static_cast<int>(m_currentHistoryItems.size()))
            {
                PopupSourceTextBox().Text(winrt::hstring(m_currentHistoryItems[idx].originalText));
                PopupResultTextBlock().Text(winrt::hstring(m_currentHistoryItems[idx].translatedText));
                ToggleHistoryView();
            }
        });

        PopupHistoryCopyBtn().Click([this](auto&&, auto&&)
        {
            int idx = PopupHistoryListView().SelectedIndex();
            if (idx >= 0 && idx < static_cast<int>(m_currentHistoryItems.size()))
            {
                CopyTextToClipboard(winrt::hstring(m_currentHistoryItems[idx].translatedText));
            }
        });

        PopupHistoryClearBtn().Click([this](auto&&, auto&&)
        {
            HistoryManager::Instance().Clear();
            RefreshHistory();
        });
    }

    void QuickPopupWindow::SetSelectedTextWithContext(winrt::hstring const& text, HWND sourceHwnd, bool hasSelection)
    {
        m_sourceHwnd = sourceHwnd;
        m_hasSelectionContext = hasSelection;
        PopupReplaceBtn().IsEnabled(hasSelection);

        PopupSourceTextBox().Text(text);
        UpdateCharCount();

        if (m_inHistoryMode)
        {
            ToggleHistoryView();
        }

        if (!text.empty())
        {
            OnTranslateAsync();
        }
    }

    void QuickPopupWindow::SetSelectedText(winrt::hstring const& text)
    {
        SetSelectedTextWithContext(text, nullptr, false);
    }

    void QuickPopupWindow::OnTranslateAsync()
    {
        auto text = PopupSourceTextBox().Text();
        if (text.empty())
        {
            PopupResultTextBlock().Text(L"");
            return;
        }

        PopupResultTextBlock().Text(winrt::hstring(LocalizationManager::Instance().Get(L"Translating")));

        TranslationRequest req;
        req.text = text.c_str();
        req.sourceLang = GetSourceLangCode();
        req.targetLang = GetTargetLangCode();

        TranslationManager::Instance().TranslateAsync(
            req,
            m_selectedServiceIndex,
            DispatcherQueue(),
            [this](TranslationResult res)
            {
                if (res.success)
                {
                    PopupResultTextBlock().Text(winrt::hstring(res.translatedText));
                    if (!res.detectedLanguage.empty())
                    {
                        m_lastDetectedSourceLang = res.detectedLanguage;
                    }
                }
                else
                {
                    PopupResultTextBlock().Text(winrt::hstring(L"Error: " + res.errorMessage));
                }
            });
    }

    void QuickPopupWindow::OnReplaceSourceText()
    {
        if (!m_hasSelectionContext || m_sourceHwnd == nullptr || !::IsWindow(m_sourceHwnd))
        {
            m_hasSelectionContext = false;
            m_sourceHwnd = nullptr;
            PopupReplaceBtn().IsEnabled(false);
            return;
        }

        auto trans = PopupResultTextBlock().Text();
        auto translatingStr = LocalizationManager::Instance().Get(L"Translating");
        if (trans.empty() || trans == L"Translating..." || trans == translatingStr) return;

        bool ok = SelectionCapture::ReplaceSelection(m_sourceHwnd, trans.c_str());
        (void)ok;

        // Reset state after replacement
        m_hasSelectionContext = false;
        m_sourceHwnd = nullptr;
        PopupReplaceBtn().IsEnabled(false);
    }

    void QuickPopupWindow::OnScreenSnippingOcr()
    {
        ScreenSnipper::Instance().StartSnipping(
            GetSourceLangCode(),
            DispatcherQueue(),
            [this](std::wstring const& recognizedText, bool /*autoTranslate*/)
            {
                if (!recognizedText.empty())
                {
                    PopupSourceTextBox().Text(winrt::hstring(recognizedText));
                    m_hasSelectionContext = false;
                    m_sourceHwnd = nullptr;
                    PopupReplaceBtn().IsEnabled(false);
                    OnTranslateAsync();
                }
            });
    }

    void QuickPopupWindow::OnRestoreDefaultLanguages()
    {
        auto const& settings = SettingsManager::Instance().GetSettings();
        auto const& sLangs = m_sourceLangCodes;
        auto const& tLangs = m_targetLangCodes;

        for (size_t i = 0; i < sLangs.size(); ++i)
        {
            if (_wcsicmp(sLangs[i].c_str(), settings.sourceLanguage.c_str()) == 0)
            {
                PopupSourceLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        for (size_t i = 0; i < tLangs.size(); ++i)
        {
            if (_wcsicmp(tLangs[i].c_str(), settings.targetLanguage.c_str()) == 0)
            {
                PopupTargetLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        if (!PopupSourceTextBox().Text().empty())
        {
            OnTranslateAsync();
        }
    }

    void QuickPopupWindow::OnSpeakSource()
    {
        if (GoogleTtsService::Instance().IsPlaying())
        {
            GoogleTtsService::Instance().Stop();
            PopupSourceSpeakIcon().Glyph(L"\uE767");
            PopupSpeakResultIcon().Glyph(L"\uE767");
            return;
        }

        auto text = PopupSourceTextBox().Text();
        if (text.empty()) return;

        std::wstring lang = GetSourceLangCode();
        if (lang == L"auto" && !m_lastDetectedSourceLang.empty())
        {
            lang = m_lastDetectedSourceLang;
        }
        if (lang == L"auto")
        {
            lang = LanguageCatalog::DetectLanguage(text.c_str());
        }
        if (lang == L"auto" || lang.empty())
        {
            lang = L"en";
        }

        PopupSourceSpeakIcon().Glyph(L"\uE74F");
        GoogleTtsService::Instance().Speak(
            text.c_str(),
            lang,
            DispatcherQueue(),
            [this]()
            {
                PopupSourceSpeakIcon().Glyph(L"\uE767");
            });
    }

    void QuickPopupWindow::OnSpeakResult()
    {
        if (GoogleTtsService::Instance().IsPlaying())
        {
            GoogleTtsService::Instance().Stop();
            PopupSourceSpeakIcon().Glyph(L"\uE767");
            PopupSpeakResultIcon().Glyph(L"\uE767");
            return;
        }

        auto text = PopupResultTextBlock().Text();
        if (text.empty() || text == L"Translating...") return;

        PopupSpeakResultIcon().Glyph(L"\uE74F");
        GoogleTtsService::Instance().Speak(
            text.c_str(),
            GetTargetLangCode(),
            DispatcherQueue(),
            [this]()
            {
                PopupSpeakResultIcon().Glyph(L"\uE767");
            });
    }

    void QuickPopupWindow::OnSwapLanguages()
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
                PopupSourceLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        for (size_t i = 0; i < m_targetLangCodes.size(); ++i)
        {
            if (_wcsicmp(m_targetLangCodes[i].c_str(), srcCode.c_str()) == 0)
            {
                PopupTargetLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        auto srcText = PopupSourceTextBox().Text();
        auto resText = PopupResultTextBlock().Text();
        if (!resText.empty() && resText != L"Translating...")
        {
            PopupSourceTextBox().Text(resText);
            PopupResultTextBlock().Text(srcText);
            m_hasSelectionContext = false;
            m_sourceHwnd = nullptr;
            PopupReplaceBtn().IsEnabled(false);
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

        PopulateLanguagesForService(serviceId);
    }

    void QuickPopupWindow::ToggleHistoryView()
    {
        m_inHistoryMode = !m_inHistoryMode;

        auto const& loc = LocalizationManager::Instance();
        if (m_inHistoryMode)
        {
            PopupTranslateGrid().Visibility(Visibility::Collapsed);
            PopupHistoryGrid().Visibility(Visibility::Visible);
            PopupHistoryBtnIcon().Glyph(L"\uE774"); // Globe / translate icon to return
            ToolTipService::SetToolTip(PopupHistoryBtn(), box_value(winrt::hstring(loc.Get(L"TipBackToTranslator"))));
            RefreshHistory();
        }
        else
        {
            PopupHistoryGrid().Visibility(Visibility::Collapsed);
            PopupTranslateGrid().Visibility(Visibility::Visible);
            PopupHistoryBtnIcon().Glyph(L"\uE81C"); // History icon
            ToolTipService::SetToolTip(PopupHistoryBtn(), box_value(winrt::hstring(loc.Get(L"TipHistory"))));
        }
    }

    void QuickPopupWindow::RefreshHistory()
    {
        std::wstring query = PopupHistorySearchBox().Text().c_str();
        auto items = HistoryManager::Instance().Search(query);

        m_currentHistoryItems = items;
        PopupHistoryListView().Items().Clear();

        for (auto const& item : items)
        {
            std::wstring display = item.originalText;
            if (display.length() > 30) display = display.substr(0, 30) + L"...";
            display += L" ➔ ";
            std::wstring trans = item.translatedText;
            if (trans.length() > 35) trans = trans.substr(0, 35) + L"...";
            display += trans;

            ListViewItem lvi;
            lvi.Content(box_value(winrt::hstring(display)));
            PopupHistoryListView().Items().Append(lvi);
        }
    }

    void QuickPopupWindow::CopyTextToClipboard(winrt::hstring const& text)
    {
        if (text.empty()) return;
        DataPackage package;
        package.SetText(text);
        Clipboard::SetContent(package);
    }

    void QuickPopupWindow::ApplyLocalization()
    {
        auto const& loc = LocalizationManager::Instance();
        Title(winrt::hstring(loc.Get(L"AppTitle")));

        ToolTipService::SetToolTip(PopupCloseBtn(), box_value(winrt::hstring(loc.Get(L"TipHide"))));
        ToolTipService::SetToolTip(PopupSwapLanguagesBtn(), box_value(winrt::hstring(loc.Get(L"TipSwapLangs"))));
        ToolTipService::SetToolTip(PopupRestoreDefaultsBtn(), box_value(winrt::hstring(loc.Get(L"TipRestoreDefaults"))));
        ToolTipService::SetToolTip(PopupOcrBtn(), box_value(winrt::hstring(loc.Get(L"TipScreenOcr"))));

        PopupSourceHeaderTextBlock().Text(winrt::hstring(loc.Get(L"SourceTextHeader")));
        ToolTipService::SetToolTip(PopupSourceSpeakBtn(), box_value(winrt::hstring(loc.Get(L"TipListen"))));
        ToolTipService::SetToolTip(PopupClearTextBtn(), box_value(winrt::hstring(loc.Get(L"TipClear"))));
        PopupSourceTextBox().PlaceholderText(winrt::hstring(loc.Get(L"SourcePlaceholder")));

        PopupTranslateActionText().Text(winrt::hstring(loc.Get(L"BtnTranslate")));
        PopupResultTextBlock().PlaceholderText(winrt::hstring(loc.Get(L"ResultPlaceholder")));

        ToolTipService::SetToolTip(PopupSpeakBtn(), box_value(winrt::hstring(loc.Get(L"TipListen"))));
        ToolTipService::SetToolTip(PopupReplaceBtn(), box_value(winrt::hstring(loc.Get(L"TipReplace"))));
        ToolTipService::SetToolTip(PopupCopyResultBtn(), box_value(winrt::hstring(loc.Get(L"TipCopy"))));

        PopupHistorySearchBox().PlaceholderText(winrt::hstring(loc.Get(L"HistorySearchPlaceholder")));
        PopupHistoryInsertBtn().Content(box_value(winrt::hstring(loc.Get(L"HistoryInsert"))));
        PopupHistoryCopyBtn().Content(box_value(winrt::hstring(loc.Get(L"HistoryCopy"))));
        PopupHistoryClearBtn().Content(box_value(winrt::hstring(loc.Get(L"HistoryClear"))));

        ToolTipService::SetToolTip(PopupServiceGoogleBtn(), box_value(winrt::hstring(loc.Get(L"GoogleTranslate"))));
        PopupServiceGoogleText().Text(winrt::hstring(loc.Get(L"Google")));

        ToolTipService::SetToolTip(PopupServiceYandexBtn(), box_value(winrt::hstring(loc.Get(L"YandexTranslate"))));
        PopupServiceYandexText().Text(winrt::hstring(loc.Get(L"Yandex")));

        ToolTipService::SetToolTip(PopupHistoryBtn(), box_value(winrt::hstring(m_inHistoryMode ? loc.Get(L"TipBackToTranslator") : loc.Get(L"TipHistory"))));
        ToolTipService::SetToolTip(PopupSettingsBtn(), box_value(winrt::hstring(loc.Get(L"TipSettings"))));

        // Re-populate combos to update "Auto-detect" item text while preserving current selections
        auto curSrc = GetSourceLangCode();
        auto curTgt = GetTargetLangCode();
        PopulateLanguagesForService(m_selectedServiceIndex);
        for (size_t i = 0; i < m_sourceLangCodes.size(); ++i)
        {
            if (_wcsicmp(m_sourceLangCodes[i].c_str(), curSrc.c_str()) == 0)
            {
                PopupSourceLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }
        for (size_t i = 0; i < m_targetLangCodes.size(); ++i)
        {
            if (_wcsicmp(m_targetLangCodes[i].c_str(), curTgt.c_str()) == 0)
            {
                PopupTargetLanguageCombo().SelectedIndex(static_cast<int>(i));
                break;
            }
        }

        UpdateCharCount();
    }

    void QuickPopupWindow::UpdateCharCount()
    {
        auto text = PopupSourceTextBox().Text();
        std::wstring str = text.c_str();

        // Unicode-aware character count: combine surrogate pairs (0xD800-0xDBFF + 0xDC00-0xDFFF) as 1 character
        size_t count = 0;
        for (size_t i = 0; i < str.size(); ++i)
        {
            wchar_t ch = str[i];
            if (ch >= 0xD800 && ch <= 0xDBFF)
            {
                if (i + 1 < str.size() && str[i + 1] >= 0xDC00 && str[i + 1] <= 0xDFFF)
                {
                    ++i;
                }
            }
            ++count;
        }

        auto const& loc = LocalizationManager::Instance();
        std::wstring countStr = std::to_wstring(count);
        std::wstring formatted = (count == 1) ?
            loc.Format(L"CharCountSingle", countStr) :
            loc.Format(L"CharCountPlural", countStr);

        PopupCharCountTextBlock().Text(winrt::hstring(formatted));
    }
}
