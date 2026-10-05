#include "pch.h"
#include "ClipboardHelper.h"

namespace dTranslate::Windows
{
    std::wstring ClipboardHelper::GetText()
    {
        std::wstring result;
        if (!OpenClipboard(nullptr))
        {
            return result;
        }

        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData != nullptr)
        {
            wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
            if (pszText != nullptr)
            {
                result = pszText;
                GlobalUnlock(hData);
            }
        }

        CloseClipboard();
        return result;
    }

    bool ClipboardHelper::SetText(std::wstring const& text)
    {
        if (!OpenClipboard(nullptr))
        {
            return false;
        }

        EmptyClipboard();

        size_t bytes = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hGlobal == nullptr)
        {
            CloseClipboard();
            return false;
        }

        wchar_t* pDest = static_cast<wchar_t*>(GlobalLock(hGlobal));
        if (pDest != nullptr)
        {
            memcpy(pDest, text.c_str(), bytes);
            GlobalUnlock(hGlobal);
            SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        else
        {
            GlobalFree(hGlobal);
        }

        CloseClipboard();
        return true;
    }
}
