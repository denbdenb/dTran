#pragma once

#include "QuickPopupWindow.g.h"

namespace winrt::dTranslate::implementation
{
    struct QuickPopupWindow : QuickPopupWindowT<QuickPopupWindow>
    {
        QuickPopupWindow();

        void SetSelectedText(winrt::hstring const& text);

    private:
        int m_selectedServiceIndex{ 0 };

        void SetupEventHandlers();
        void OnTranslate();
        void OnSwapLanguages();
        void SelectService(int serviceId);
        void CopyTextToClipboard(winrt::hstring const& text);
    };
}

namespace winrt::dTranslate::factory_implementation
{
    struct QuickPopupWindow : QuickPopupWindowT<QuickPopupWindow, implementation::QuickPopupWindow>
    {
    };
}
