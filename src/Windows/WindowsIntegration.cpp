#include "pch.h"
#include "WindowsIntegration.h"
#include "SelectionCapture.h"
#include "ClipboardHelper.h"
#include "SettingsManager.h"

namespace dTranslate::Windows
{
    WindowsIntegration& WindowsIntegration::Instance()
    {
        static WindowsIntegration instance;
        return instance;
    }

    WindowsIntegration::WindowsIntegration()
    {
    }

    WindowsIntegration::~WindowsIntegration()
    {
        Shutdown();
    }

    bool WindowsIntegration::Initialize(HINSTANCE hInstance)
    {
        if (m_hWnd != nullptr) return true;

        if (!CreateMessageWindow())
        {
            return false;
        }

        SetupTrayIcon();
        RegisterHotkeys();
        return true;
    }

    void WindowsIntegration::Shutdown()
    {
        UnregisterHotkeys();
        RemoveTrayIcon();

        if (m_hWnd != nullptr)
        {
            DestroyWindow(m_hWnd);
            m_hWnd = nullptr;
        }
    }

    bool WindowsIntegration::CreateMessageWindow()
    {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"dTranslate_MessageWindow";

        RegisterClassExW(&wc);

        m_hWnd = CreateWindowExW(
            0,
            wc.lpszClassName,
            L"dTranslateMsgWindow",
            0, 0, 0, 0, 0,
            HWND_MESSAGE,
            nullptr,
            wc.hInstance,
            this);

        return (m_hWnd != nullptr);
    }

    bool WindowsIntegration::SetupTrayIcon()
    {
        if (m_hWnd == nullptr) return false;

        ZeroMemory(&m_nid, sizeof(m_nid));
        m_nid.cbSize = sizeof(NOTIFYICONDATAW);
        m_nid.hWnd = m_hWnd;
        m_nid.uID = 1;
        m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_TRAYICON;

        // Use application standard icon
        m_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        wcscpy_s(m_nid.szTip, L"dTranslate - Native Windows Translator");

        m_trayAdded = Shell_NotifyIconW(NIM_ADD, &m_nid) != FALSE;
        return m_trayAdded;
    }

    void WindowsIntegration::RemoveTrayIcon()
    {
        if (m_trayAdded)
        {
            Shell_NotifyIconW(NIM_DELETE, &m_nid);
            m_trayAdded = false;
        }
    }

    static bool ParseHotkey(std::wstring const& str, UINT& outModifiers, UINT& outVk)
    {
        outModifiers = 0;
        outVk = 0;
        if (str.empty()) return false;

        std::wstringstream ss(str);
        std::wstring part;
        std::vector<std::wstring> parts;
        while (std::getline(ss, part, L'+'))
        {
            size_t first = part.find_first_not_of(L" \t");
            size_t last = part.find_last_not_of(L" \t");
            if (first != std::wstring::npos)
            {
                parts.push_back(part.substr(first, (last - first + 1)));
            }
        }

        if (parts.size() < 2) return false;

        for (size_t i = 0; i < parts.size(); ++i)
        {
            auto p = parts[i];
            std::wstring upper = p;
            for (auto& c : upper) c = towupper(c);

            if (i < parts.size() - 1)
            {
                if (upper == L"CTRL" || upper == L"CONTROL") outModifiers |= MOD_CONTROL;
                else if (upper == L"ALT") outModifiers |= MOD_ALT;
                else if (upper == L"SHIFT") outModifiers |= MOD_SHIFT;
                else if (upper == L"WIN" || upper == L"WINDOWS") outModifiers |= MOD_WIN;
                else return false;
            }
            else
            {
                if (upper.size() == 1)
                {
                    wchar_t ch = upper[0];
                    if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9'))
                    {
                        outVk = static_cast<UINT>(ch);
                    }
                    else return false;
                }
                else if (upper == L"F1") outVk = VK_F1;
                else if (upper == L"F2") outVk = VK_F2;
                else if (upper == L"F3") outVk = VK_F3;
                else if (upper == L"F4") outVk = VK_F4;
                else if (upper == L"F5") outVk = VK_F5;
                else if (upper == L"F6") outVk = VK_F6;
                else if (upper == L"F7") outVk = VK_F7;
                else if (upper == L"F8") outVk = VK_F8;
                else if (upper == L"F9") outVk = VK_F9;
                else if (upper == L"F10") outVk = VK_F10;
                else if (upper == L"F11") outVk = VK_F11;
                else if (upper == L"F12") outVk = VK_F12;
                else if (upper == L"SPACE") outVk = VK_SPACE;
                else if (upper == L"TAB") outVk = VK_TAB;
                else return false;
            }
        }

        return (outModifiers != 0 && outVk != 0);
    }

    bool WindowsIntegration::ValidateHotkey(std::wstring const& hotkeyStr, std::wstring& outError)
    {
        UINT mods = 0;
        UINT vk = 0;
        if (!ParseHotkey(hotkeyStr, mods, vk))
        {
            outError = L"Invalid combination. Must include a modifier (Ctrl, Alt, Shift, Win) and a key (e.g. Ctrl+Alt+T).";
            return false;
        }
        return true;
    }

    bool WindowsIntegration::RegisterHotkeys()
    {
        std::wstring err;
        auto const& s = dTranslate::Storage::SettingsManager::Instance().GetSettings();
        return ReRegisterHotkeys(s.globalHotkey, s.quickHotkey, s.ocrHotkey, err);
    }

    bool WindowsIntegration::ReRegisterHotkeys(
        std::wstring const& selectionKey,
        std::wstring const& mainKey,
        std::wstring const& ocrKey,
        std::wstring& outError)
    {
        if (m_hWnd == nullptr) return false;

        UnregisterHotkeys();

        UINT mod1 = 0, vk1 = 0;
        UINT mod2 = 0, vk2 = 0;
        UINT mod3 = 0, vk3 = 0;

        if (!ParseHotkey(selectionKey, mod1, vk1))
        {
            mod1 = MOD_CONTROL | MOD_ALT;
            vk1 = 'T';
        }
        if (!ParseHotkey(mainKey, mod2, vk2))
        {
            mod2 = MOD_CONTROL | MOD_ALT;
            vk2 = 'D';
        }
        if (!ParseHotkey(ocrKey, mod3, vk3))
        {
            mod3 = MOD_CONTROL | MOD_ALT;
            vk3 = 'O';
        }

        bool ok1 = RegisterHotKey(m_hWnd, HOTKEY_ID_SELECTION, mod1, vk1) != FALSE;
        bool ok2 = RegisterHotKey(m_hWnd, HOTKEY_ID_MAIN, mod2, vk2) != FALSE;
        bool ok3 = RegisterHotKey(m_hWnd, HOTKEY_ID_OCR, mod3, vk3) != FALSE;

        if (!ok1) outError = L"Conflict: " + selectionKey + L" is in use by another application.";
        else if (!ok2) outError = L"Conflict: " + mainKey + L" is in use by another application.";
        else if (!ok3) outError = L"Conflict: " + ocrKey + L" is in use by another application.";

        return ok1 && ok2 && ok3;
    }

    void WindowsIntegration::UnregisterHotkeys()
    {
        if (m_hWnd != nullptr)
        {
            UnregisterHotKey(m_hWnd, HOTKEY_ID_SELECTION);
            UnregisterHotKey(m_hWnd, HOTKEY_ID_MAIN);
            UnregisterHotKey(m_hWnd, HOTKEY_ID_OCR);
        }
    }

    void WindowsIntegration::TriggerTranslateSelection()
    {
        // Capture text from currently focused app
        std::wstring text = SelectionCapture::CaptureSelectedText(true);

        if (text.empty())
        {
            // Fallback: check current clipboard text
            text = ClipboardHelper::GetText();
        }

        if (m_onTranslateSelection)
        {
            m_onTranslateSelection(text);
        }
    }

    void WindowsIntegration::ShowTrayContextMenu(HWND hWnd)
    {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu == nullptr) return;

        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN, L"Open dTranslate");
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_CLIPBOARD, L"Translate Clipboard");
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OCR, L"Screen OCR & Translate");
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_SETTINGS, L"Settings...");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Exit");

        POINT pt;
        GetCursorPos(&pt);

        SetForegroundWindow(hWnd);
        TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, hWnd, nullptr);
        DestroyMenu(hMenu);
    }

    LRESULT CALLBACK WindowsIntegration::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WindowsIntegration* pThis = &WindowsIntegration::Instance();

        switch (uMsg)
        {
        case WM_TRAYICON:
            if (LOWORD(lParam) == WM_LBUTTONUP || LOWORD(lParam) == WM_LBUTTONDBLCLK)
            {
                if (pThis->m_onShowMainWindow)
                {
                    pThis->m_onShowMainWindow();
                }
            }
            else if (LOWORD(lParam) == WM_RBUTTONUP)
            {
                pThis->ShowTrayContextMenu(hWnd);
            }
            return 0;

        case WM_HOTKEY:
            if (wParam == HOTKEY_ID_SELECTION)
            {
                pThis->TriggerTranslateSelection();
            }
            else if (wParam == HOTKEY_ID_MAIN)
            {
                if (pThis->m_onShowMainWindow)
                {
                    pThis->m_onShowMainWindow();
                }
            }
            else if (wParam == HOTKEY_ID_OCR)
            {
                if (pThis->m_onScreenOcr)
                {
                    pThis->m_onScreenOcr();
                }
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDM_TRAY_OPEN:
                if (pThis->m_onShowMainWindow) pThis->m_onShowMainWindow();
                break;
            case IDM_TRAY_CLIPBOARD:
                if (pThis->m_onTranslateClipboard) pThis->m_onTranslateClipboard();
                break;
            case IDM_TRAY_OCR:
                if (pThis->m_onScreenOcr) pThis->m_onScreenOcr();
                break;
            case IDM_TRAY_SETTINGS:
                if (pThis->m_onOpenSettings) pThis->m_onOpenSettings();
                break;
            case IDM_TRAY_EXIT:
                if (pThis->m_onExit) pThis->m_onExit();
                break;
            }
            return 0;

        case WM_DESTROY:
            return 0;
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}
