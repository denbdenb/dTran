#pragma once
#include <string>

namespace dTranslate::Windows
{
    class SelectionCapture
    {
    public:
        static std::wstring CaptureSelectedText(bool restoreClipboard = true);
    };
}
