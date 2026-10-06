#pragma once

#include "QuickPopupWindow.g.h"
#include <string>
#include <vector>

namespace winrt::dTranslate::implementation
{
    struct QuickPopupWindow : QuickPopupWindowT<QuickPopupWindow>
    {
        QuickPopupWindow();
        ~QuickPopupWindow();

        void SetSelectedText(winrt::hstring const& text);

    private:
        int m_selectedServiceIndex{ 0 };
        std::vector<std::wstring> m_sourceLangCodes;
        std::vector<std::wstring> m_targetLangCodes;

        void SetupEventHandlers();
        void PopulateLanguagesForService(int serviceId);
        void OnTranslateAsync();
        void OnSwapLanguages();
        void SelectService(int serviceId);
        void ApplyTheme(std::wstring const& themeName);
        std::wstring m_lastDetectedSourceLang;
        void CopyTextToClipboard(winrt::hstring const& text);
        void OnSpeakSource();
        void OnSpeakResult();
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
