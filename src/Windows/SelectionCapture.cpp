#include "pch.h"
#include "SelectionCapture.h"
#include "ClipboardHelper.h"
#include <vector>
#include <uiautomation.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace dTranslate::Windows
{
    static std::wstring TryGetSelectedTextViaUia()
    {
        std::wstring text;
        HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bool coInit = SUCCEEDED(hrCo);

        IUIAutomation* pAutomation = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_CUIAutomation,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IUIAutomation,
            reinterpret_cast<void**>(&pAutomation));

        if (FAILED(hr) || !pAutomation)
        {
            if (coInit) CoUninitialize();
            return text;
        }

        IUIAutomationElement* pFocused = nullptr;
        hr = pAutomation->GetFocusedElement(&pFocused);
        if (SUCCEEDED(hr) && pFocused)
        {
            IUIAutomationTextPattern* pTextPattern = nullptr;
            hr = pFocused->GetCurrentPatternAs(
                UIA_TextPatternId,
                IID_IUIAutomationTextPattern,
                reinterpret_cast<void**>(&pTextPattern));

            if (SUCCEEDED(hr) && pTextPattern)
            {
                IUIAutomationTextRangeArray* pRanges = nullptr;
                hr = pTextPattern->GetSelection(&pRanges);
                if (SUCCEEDED(hr) && pRanges)
                {
                    int length = 0;
                    pRanges->get_Length(&length);
                    if (length > 0)
                    {
                        IUIAutomationTextRange* pRange = nullptr;
                        hr = pRanges->GetElement(0, &pRange);
                        if (SUCCEEDED(hr) && pRange)
                        {
                            BSTR bstr = nullptr;
                            hr = pRange->GetText(-1, &bstr);
                            if (SUCCEEDED(hr) && bstr)
                            {
                                if (SysStringLen(bstr) > 0)
                                {
                                    text = bstr;
                                }
                                SysFreeString(bstr);
                            }
                            pRange->Release();
                        }
                    }
                    pRanges->Release();
                }
                pTextPattern->Release();
            }
            pFocused->Release();
        }
        pAutomation->Release();
        if (coInit) CoUninitialize();
        return text;
    }

    static void ReleaseHeldModifiers()
    {
        std::vector<INPUT> releases;
        auto addRelease = [&releases](WORD vk)
        {
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = vk;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            releases.push_back(input);
        };

        // Unconditionally release Menu/Alt and Ctrl to avoid Alt-menu activation
        addRelease(VK_MENU);
        addRelease(VK_LMENU);
        addRelease(VK_RMENU);
        addRelease(VK_CONTROL);
        addRelease(VK_LCONTROL);
        addRelease(VK_RCONTROL);
        addRelease(VK_SHIFT);
        addRelease('T');

        if (GetAsyncKeyState(VK_LWIN) & 0x8000) addRelease(VK_LWIN);
        if (GetAsyncKeyState(VK_RWIN) & 0x8000) addRelease(VK_RWIN);

        SendInput(static_cast<UINT>(releases.size()), releases.data(), sizeof(INPUT));
        Sleep(35);
    }

    SelectionCaptureResult SelectionCapture::CaptureSelectedText(bool restoreClipboard)
    {
        SelectionCaptureResult result;

        // 1. Capture the currently active window and focused control before focus moves
        HWND fg = ::GetForegroundWindow();
        DWORD fgThread = fg ? GetWindowThreadProcessId(fg, nullptr) : 0;
        GUITHREADINFO gti = { sizeof(gti) };
        if (fgThread && GetGUIThreadInfo(fgThread, &gti) && gti.hwndFocus && ::IsWindow(gti.hwndFocus))
        {
            result.targetHwnd = gti.hwndFocus;
        }
        else
        {
            result.targetHwnd = fg;
        }

        // 2. Preferred Path: Windows UI Automation (direct selection extraction without clipboard)
        std::wstring uiaText = TryGetSelectedTextViaUia();
        if (!uiaText.empty())
        {
            result.text = uiaText;
            result.hasSelection = true;
            return result;
        }

        // 2B. Direct Win32 Edit control check (zero-flicker, non-clipboard)
        if (result.targetHwnd && ::IsWindow(result.targetHwnd))
        {
            DWORD start = 0, end = 0;
            DWORD_PTR res = 0;
            if (SendMessageTimeoutW(result.targetHwnd, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end), SMTO_ABORTIFHUNG, 50, &res) && end > start)
            {
                int len = GetWindowTextLengthW(result.targetHwnd);
                if (len > 0 && static_cast<int>(end) <= len + 1)
                {
                    std::wstring full(len + 1, L'\0');
                    GetWindowTextW(result.targetHwnd, &full[0], len + 1);
                    full.resize(len);
                    if (start < full.size() && end <= full.size())
                    {
                        result.text = full.substr(start, end - start);
                        result.hasSelection = true;
                        return result;
                    }
                }
            }
        }

        // 3. Fallback Path: Simulated Ctrl+C with physical key release
        ReleaseHeldModifiers();

        std::wstring originalText = ClipboardHelper::GetText();

        if (OpenClipboard(nullptr))
        {
            EmptyClipboard();
            CloseClipboard();
        }

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

        // Poll clipboard for copied text (up to 250ms)
        std::wstring selectedText;
        for (int i = 0; i < 20; ++i)
        {
            Sleep(12);
            selectedText = ClipboardHelper::GetText();
            if (!selectedText.empty())
            {
                break;
            }
        }

        if (!selectedText.empty())
        {
            result.text = selectedText;
            result.hasSelection = true;
        }

        // Restore clipboard if requested
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

        return result;
    }

    bool SelectionCapture::ReplaceSelection(HWND targetHwnd, std::wstring const& replacementText)
    {
        if (targetHwnd == nullptr || !::IsWindow(targetHwnd) || replacementText.empty())
        {
            return false;
        }

        // 1. Save user's current clipboard text so we don't clobber it
        std::wstring savedClipboard = ClipboardHelper::GetText();

        // 2. Put translation into clipboard (full Unicode support)
        if (!ClipboardHelper::SetText(replacementText))
        {
            return false;
        }

        // 3. Restore focus to the target application
        HWND root = ::GetAncestor(targetHwnd, GA_ROOT);
        if (root && ::IsWindow(root))
        {
            ::SetForegroundWindow(root);
        }
        else
        {
            ::SetForegroundWindow(targetHwnd);
        }
        ::SetFocus(targetHwnd);
        ::Sleep(60);

        // 4. Release any modifiers currently held
        ReleaseHeldModifiers();

        // 5. Simulate Ctrl + V to paste over the original selection
        INPUT inputs[4] = {};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;

        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = 'V';

        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = 'V';
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_CONTROL;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

        ::SendInput(4, inputs, sizeof(INPUT));

        // 6. Allow target application to process the paste message
        ::Sleep(120);

        // 7. Restore user's previous clipboard contents safely
        if (!savedClipboard.empty())
        {
            ClipboardHelper::SetText(savedClipboard);
        }
        else
        {
            if (OpenClipboard(nullptr))
            {
                EmptyClipboard();
                CloseClipboard();
            }
        }

        return true;
    }
}
