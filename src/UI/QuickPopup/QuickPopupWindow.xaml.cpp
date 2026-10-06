#include "pch.h"
#include "QuickPopupWindow.xaml.h"
#include "TranslationManager.h"
#include "GoogleTtsService.h"
#include "LanguageCatalog.h"
#include "SettingsManager.h"

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
using namespace ::dTranslate::Storage;

namespace winrt::dTranslate::implementation
{
    QuickPopupWindow::QuickPopupWindow()
    {
        InitializeComponent();

        ExtendsContentIntoTitleBar(true);
        SetTitleBar(PopupTitleBar());

        auto const& settings = SettingsManager::Instance().GetSettings();
        int w = settings.quickPopupWidth > 250 ? settings.quickPopupWidth : 440;
        int h = settings.quickPopupHeight > 250 ? settings.quickPopupHeight : 420;
        AppWindow().Resize({ w, h });

        ApplyTheme(settings.theme);

        m_selectedServiceIndex = settings.primaryService;
        PopulateLanguagesForService(m_selectedServiceIndex);
        SetupEventHandlers();
        SelectService(m_selectedServiceIndex);

        // Window resize persistence
        AppWindow().Changed([this](auto&& sender, Microsoft::UI::Windowing::AppWindowChangedEventArgs const& args)
        {
            if (args.DidSizeChange())
            {
                auto size = sender.Size();
                if (size.Width > 200 && size.Height > 200)
                {
                    auto s = SettingsManager::Instance().GetSettings();
                    s.quickPopupWidth = size.Width;
                    s.quickPopupHeight = size.Height;
                    SettingsManager::Instance().UpdateSettings(s);
                }
            }
        });

        // Register observer for live settings updates
        SettingsManager::Instance().RegisterObserver(reinterpret_cast<uintptr_t>(this), [this](AppSettings const& s)
        {
            DispatcherQueue().TryEnqueue([this, s]()
            {
                ApplyTheme(s.theme);
                PopulateLanguagesForService(m_selectedServiceIndex);
            });
        });

        Closed([this](auto&&, auto&&)
        {
            SettingsManager::Instance().UnregisterObserver(reinterpret_cast<uintptr_t>(this));
        });
    }

    QuickPopupWindow::~QuickPopupWindow()
    {
        SettingsManager::Instance().UnregisterObserver(reinterpret_cast<uintptr_t>(this));
    }

    void QuickPopupWindow::ApplyTheme(std::wstring const& themeName)
    {
        if (auto root = Content().try_as<FrameworkElement>())
        {
            if (themeName == L"Light") root.RequestedTheme(ElementTheme::Light);
            else if (themeName == L"Dark") root.RequestedTheme(ElementTheme::Dark);
            else root.RequestedTheme(ElementTheme::Default);
        }
    }

    void QuickPopupWindow::PopulateLanguagesForService(int serviceId)
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
        PopupSourceLanguageCombo().Items().Clear();
        int srcIdx = 0;
        for (size_t i = 0; i < srcLangs.size(); ++i)
        {
            auto const& lang = srcLangs[i];
            m_sourceLangCodes.push_back(lang.code);
            ComboBoxItem item;
            item.Content(box_value(winrt::hstring(lang.DisplayName())));
            PopupSourceLanguageCombo().Items().Append(item);
            if (_wcsicmp(lang.code.c_str(), curSrc.c_str()) == 0)
            {
                srcIdx = static_cast<int>(i);
            }
        }
        PopupSourceLanguageCombo().SelectedIndex(srcIdx);

        m_targetLangCodes.clear();
        PopupTargetLanguageCombo().Items().Clear();
        int tgtIdx = 0;
        for (size_t i = 0; i < tgtLangs.size(); ++i)
        {
            auto const& lang = tgtLangs[i];
            m_targetLangCodes.push_back(lang.code);
            ComboBoxItem item;
            item.Content(box_value(winrt::hstring(lang.DisplayName())));
            PopupTargetLanguageCombo().Items().Append(item);
            if (_wcsicmp(lang.code.c_str(), curTgt.c_str()) == 0)
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
        PopupCloseBtn().Click([this](auto&&, auto&&)
        {
            Close();
        });

        PopupRootGrid().KeyDown([this](auto&&, KeyRoutedEventArgs const& e)
        {
            if (e.Key() == VirtualKey::Escape)
            {
                Close();
                e.Handled(true);
            }
        });

        PopupClearTextBtn().Click([this](auto&&, auto&&)
        {
            PopupSelectedTextBlock().Text(L"");
            PopupResultTextBlock().Text(L"");
        });

        PopupCopyResultBtn().Click([this](auto&&, auto&&)
        {
            CopyTextToClipboard(PopupResultTextBlock().Text());
        });

        PopupSpeakBtn().Click([this](auto&&, auto&&)
        {
            auto text = PopupResultTextBlock().Text();
            if (!text.empty() && text != L"Translating...")
            {
                GoogleTtsService::Instance().Speak(text.c_str(), GetTargetLangCode());
            }
        });

        PopupSwapLanguagesBtn().Click([this](auto&&, auto&&)
        {
            OnSwapLanguages();
        });

        PopupTranslateActionBtn().Click([this](auto&&, auto&&)
        {
            OnTranslateAsync();
        });

        PopupServiceGoogleBtn().Click([this](auto&&, auto&&) { SelectService(0); });
        PopupServiceYandexBtn().Click([this](auto&&, auto&&) { SelectService(1); });
        PopupServiceGeminiBtn().Click([this](auto&&, auto&&) { SelectService(2); });
        PopupServiceOpenAiBtn().Click([this](auto&&, auto&&) { SelectService(3); });
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

        PopupServiceOpenAiBtn().BorderBrush(serviceId == 3 ? activeBorder : defaultBorder);
        PopupServiceOpenAiBtn().BorderThickness(serviceId == 3 ? Thickness{ 1.5, 1.5, 1.5, 1.5 } : Thickness{ 1, 1, 1, 1 });

        PopulateLanguagesForService(serviceId);
    }

    void QuickPopupWindow::CopyTextToClipboard(winrt::hstring const& text)
    {
        if (text.empty()) return;
        DataPackage package;
        package.SetText(text);
        Clipboard::SetContent(package);
    }
}
