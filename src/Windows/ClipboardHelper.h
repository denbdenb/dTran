#pragma once
#include <string>

namespace dTranslate::Windows
{
    class ClipboardHelper
    {
    public:
        static std::wstring GetText();
        static bool SetText(std::wstring const& text);
    };
}
