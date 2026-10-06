#include "pch.h"
#include "WindowsIntegration.h"
#include "SelectionCapture.h"
#include "ClipboardHelper.h"

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

    bool WindowsIntegration::RegisterHotkeys()
    {
        if (m_hWnd == nullptr) return false;

        // Hotkey 1: Ctrl + Alt + T for Translate Selected Text
        RegisterHotKey(m_hWnd, HOTKEY_ID_SELECTION, MOD_CONTROL | MOD_ALT, 'T');

        // Hotkey 2: Ctrl + Alt + D for Show Main Window
        RegisterHotKey(m_hWnd, HOTKEY_ID_MAIN, MOD_CONTROL | MOD_ALT, 'D');

        // Hotkey 3: Ctrl + Alt + O for Screen OCR & Translate
        RegisterHotKey(m_hWnd, HOTKEY_ID_OCR, MOD_CONTROL | MOD_ALT, 'O');

        return true;
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
