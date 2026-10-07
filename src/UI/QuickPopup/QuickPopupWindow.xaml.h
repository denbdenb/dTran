#pragma once

#include "QuickPopupWindow.g.h"
#include <windows.h>
#include <string>
#include <vector>
#include "../../Storage/HistoryManager.h"

namespace winrt::dTranslate::implementation
{
    struct QuickPopupWindow : QuickPopupWindowT<QuickPopupWindow>
    {
        QuickPopupWindow();
        ~QuickPopupWindow();

        void SetSelectedText(winrt::hstring const& text);
        void SetSelectedTextWithContext(winrt::hstring const& text, HWND sourceHwnd, bool hasSelection);
        void OnScreenSnippingOcr();

    private:
        int m_selectedServiceIndex{ 0 };
        std::vector<std::wstring> m_sourceLangCodes;
        std::vector<std::wstring> m_targetLangCodes;
        std::wstring m_lastDetectedSourceLang;

        HWND m_sourceHwnd{ nullptr };
        bool m_hasSelectionContext{ false };
        bool m_inHistoryMode{ false };
        std::vector<::dTranslate::Storage::HistoryItem> m_currentHistoryItems;

        void SetupEventHandlers();
        void PopulateLanguagesForService(int serviceId);
        void OnTranslateAsync();
        void OnSwapLanguages();
        void SelectService(int serviceId);
        HWND m_hwnd{ nullptr };
        void UpdateTitleBarColors(bool isDark);
        void ApplyTheme(std::wstring const& themeName);
        void CopyTextToClipboard(winrt::hstring const& text);
        void OnSpeakSource();
        void OnSpeakResult();
        void OnReplaceSourceText();
        void OnRestoreDefaultLanguages();
        void ToggleHistoryView();
        void RefreshHistory();
        void ApplyLocalization();
        void UpdateCharCount();
        std::wstring GetSourceLangCode();
        std::wstring GetTargetLangCode();
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct QuickPopupWindow : QuickPopupWindowT<QuickPopupWindow, implementation::QuickPopupWindow>
    {
    };
}
