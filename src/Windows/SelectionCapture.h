#pragma once
#include <windows.h>
#include <string>

namespace dTranslate::Windows
{
    struct SelectionCaptureResult
    {
        std::wstring text;
        HWND targetHwnd{ nullptr };
        bool hasSelection{ false };
    };

    class SelectionCapture
    {
    public:
        static SelectionCaptureResult CaptureSelectedText(bool restoreClipboard = true);
        static bool ReplaceSelection(HWND targetHwnd, std::wstring const& replacementText);
    };
}
