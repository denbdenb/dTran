#include "pch.h"
#include "SelectionCapture.h"
#include "ClipboardHelper.h"

namespace dTranslate::Windows
{
    std::wstring SelectionCapture::CaptureSelectedText(bool restoreClipboard)
    {
        // 1. Save original clipboard text
        std::wstring originalText = ClipboardHelper::GetText();

        // 2. Clear clipboard temporarily so we can detect new copied content
        if (OpenClipboard(nullptr))
        {
            EmptyClipboard();
            CloseClipboard();
        }

        // 3. Simulate Ctrl + C to copy from active application
        INPUT inputs[4] = {};

        // Ctrl Down
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;

        // C Down
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 'C';

        // C Up
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = 'C';
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

        // Ctrl Up
        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_CONTROL;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

        SendInput(4, inputs, sizeof(INPUT));

        // 4. Wait for the active app to update the clipboard (up to 150 ms)
        std::wstring selectedText;
        for (int i = 0; i < 15; ++i)
        {
            Sleep(10);
            selectedText = ClipboardHelper::GetText();
            if (!selectedText.empty())
            {
                break;
            }
        }

        // 5. Restore clipboard if requested
        if (restoreClipboard)
        {
            if (!originalText.empty())
            {
                ClipboardHelper::SetText(originalText);
            }
            else
            {
                if (OpenClipboard(nullptr))
                {
                    EmptyClipboard();
                    CloseClipboard();
                }
            }
        }

        return selectedText;
    }
}
