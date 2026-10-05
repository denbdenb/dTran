#pragma once

#include "QuickPopupWindow.g.h"
#include <string>

namespace winrt::dTranslate::implementation
{
    struct QuickPopupWindow : QuickPopupWindowT<QuickPopupWindow>
    {
        QuickPopupWindow();

        void SetSelectedText(winrt::hstring const& text);

    private:
        int m_selectedServiceIndex{ 0 };

        void SetupEventHandlers();
        void OnTranslateAsync();
        void OnSwapLanguages();
        void SelectService(int serviceId);
        void CopyTextToClipboard(winrt::hstring const& text);
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
