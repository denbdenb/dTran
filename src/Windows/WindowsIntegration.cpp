#include "pch.h"
#include "WindowsIntegration.h"
#include "SelectionCapture.h"
#include "ClipboardHelper.h"
#include "../Storage/SettingsManager.h"
#include "../Storage/LocalizationManager.h"
#include <sstream>
#include <vector>
#include <filesystem>

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

    static void LogDebug(const wchar_t* fmt, ...)
    {
#if defined(_DEBUG)
        wchar_t buf[512];
        va_list args;
        va_start(args, fmt);
        _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
        va_end(args);
        OutputDebugStringW(buf);
        OutputDebugStringW(L"\n");
#else
        (void)fmt;
#endif
    }

    bool WindowsIntegration::Initialize(HINSTANCE)
    {
        LogDebug(L"WindowsIntegration::Initialize enter");
        if (m_hWnd != nullptr)
        {
            LogDebug(L"WindowsIntegration::Initialize already initialized (hWnd: 0x%p)", m_hWnd);
            return true;
        }

        if (!CreateMessageWindow())
        {
            LogDebug(L"WindowsIntegration::CreateMessageWindow failed");
            return false;
        }

        bool trayOk = SetupTrayIcon();
        LogDebug(L"SetupTrayIcon result: %d", trayOk ? 1 : 0);
        bool hotkeyOk = RegisterHotkeys();
        LogDebug(L"RegisterHotkeys result: %d", hotkeyOk ? 1 : 0);

        dTranslate::Storage::LocalizationManager::Instance().RegisterObserver(
            reinterpret_cast<uintptr_t>(this),
            [this](std::wstring const&)
            {
                UpdateTrayTooltip();
            });

        return true;
    }

    void WindowsIntegration::Shutdown()
    {
        LogDebug(L"WindowsIntegration::Shutdown enter, hWnd: 0x%p", m_hWnd);
        dTranslate::Storage::LocalizationManager::Instance().UnregisterObserver(reinterpret_cast<uintptr_t>(this));
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
        wc.lpszClassName = L"dTran_MessageWindow";

        ATOM atom = RegisterClassExW(&wc);
        DWORD regErr = GetLastError();
        LogDebug(L"RegisterClassExW atom: 0x%04X, err: %lu", atom, regErr);

        m_hWnd = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            wc.lpszClassName,
            L"dTranMsgWindow",
            WS_POPUP,
            0, 0, 0, 0,
            nullptr,
            nullptr,
            wc.hInstance,
            this);

        DWORD createErr = GetLastError();
        LogDebug(L"CreateWindowExW hWnd: 0x%p, err: %lu", m_hWnd, createErr);

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

        // Try to load custom branded icon: first from resource ID 1, then fallback to files
        HICON hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
        if (!hIcon)
        {
            wchar_t exePath[MAX_PATH] = { 0 };
            if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0)
            {
                std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
                std::vector<std::filesystem::path> candidates = {
                    dir / L"Assets" / L"app.ico",
                    dir / L"app.ico",
                    dir / L"assets" / L"app.ico",
                    dir / L".." / L".." / L"assets" / L"app.ico"
                };
                for (const auto& p : candidates)
                {
                    if (std::filesystem::exists(p))
                    {
                        hIcon = (HICON)LoadImageW(nullptr, p.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
                        if (hIcon) break;
                    }
                }
            }
        }

        if (hIcon)
        {
            m_hCustomIcon = hIcon;
            m_nid.hIcon = hIcon;
        }
        else
        {
            m_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        }

        auto const& loc = dTranslate::Storage::LocalizationManager::Instance();
        std::wstring tip = loc.Get(L"TrayTooltip");
        wcsncpy_s(m_nid.szTip, tip.c_str(), _TRUNCATE);

        m_trayAdded = Shell_NotifyIconW(NIM_ADD, &m_nid) != FALSE;
        return m_trayAdded;
    }

    void WindowsIntegration::UpdateTrayTooltip()
    {
        if (m_trayAdded && m_hWnd)
        {
            auto const& loc = dTranslate::Storage::LocalizationManager::Instance();
            std::wstring tip = loc.Get(L"TrayTooltip");
            wcsncpy_s(m_nid.szTip, tip.c_str(), _TRUNCATE);
            Shell_NotifyIconW(NIM_MODIFY, &m_nid);
        }
    }

    void WindowsIntegration::SetWindowAppIcon(HWND hwnd)
    {
        if (!hwnd) return;

        HINSTANCE hInst = GetModuleHandleW(nullptr);
        HICON hIconBig = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        HICON hIconSmall = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

        if (!hIconBig || !hIconSmall)
        {
            wchar_t exePath[MAX_PATH] = { 0 };
            if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0)
            {
                std::filesystem::path dir = std::filesystem::path(exePath).parent_path();
                std::vector<std::filesystem::path> candidates = {
                    dir / L"Assets" / L"app.ico",
                    dir / L"app.ico",
                    dir / L"assets" / L"app.ico",
                    dir / L".." / L".." / L"assets" / L"app.ico"
                };
                for (const auto& p : candidates)
                {
                    if (std::filesystem::exists(p))
                    {
                        if (!hIconBig)
                            hIconBig = (HICON)LoadImageW(nullptr, p.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
                        if (!hIconSmall)
                            hIconSmall = (HICON)LoadImageW(nullptr, p.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
                        if (hIconBig && hIconSmall) break;
                    }
                }
            }
        }

        if (hIconBig)
        {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIconBig));
            SetClassLongPtrW(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(hIconBig));
        }
        if (hIconSmall)
        {
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIconSmall));
            SetClassLongPtrW(hwnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(hIconSmall));
        }
    }

    void WindowsIntegration::RemoveTrayIcon()
    {
        if (m_trayAdded)
        {
            Shell_NotifyIconW(NIM_DELETE, &m_nid);
            m_trayAdded = false;
        }
        if (m_hCustomIcon)
        {
            DestroyIcon(m_hCustomIcon);
            m_hCustomIcon = nullptr;
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

            if (i == parts.size() - 1)
            {
                if (upper.length() == 1)
                {
                    wchar_t ch = upper[0];
                    if ((ch >= L'A' && ch <= L'Z') || (ch >= L'0' && ch <= L'9'))
                    {
                        outVk = static_cast<UINT>(ch);
                    }
                    else
                    {
                        return false;
                    }
                }
                else if (upper.rfind(L"F", 0) == 0 && upper.length() > 1)
                {
                    int fNum = _wtoi(upper.substr(1).c_str());
                    if (fNum >= 1 && fNum <= 12)
                    {
                        outVk = VK_F1 + (fNum - 1);
                    }
                    else
                    {
                        return false;
                    }
                }
                else if (upper == L"SPACE")
                {
                    outVk = VK_SPACE;
                }
                else if (upper == L"TAB")
                {
                    outVk = VK_TAB;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                if (upper == L"CTRL" || upper == L"CONTROL")
                {
                    outModifiers |= MOD_CONTROL;
                }
                else if (upper == L"ALT")
                {
                    outModifiers |= MOD_ALT;
                }
                else if (upper == L"SHIFT")
                {
                    outModifiers |= MOD_SHIFT;
                }
                else if (upper == L"WIN" || upper == L"WINDOWS")
                {
                    outModifiers |= MOD_WIN;
                }
                else
                {
                    return false;
                }
            }
        }

        return (outModifiers != 0 && outVk != 0);
    }

    bool WindowsIntegration::ValidateHotkey(std::wstring const& hotkeyStr, std::wstring& outError)
    {
        UINT mod = 0, vk = 0;
        if (!ParseHotkey(hotkeyStr, mod, vk))
        {
            outError = L"Invalid syntax. Expected format: Ctrl+Alt+Key or Ctrl+Shift+Key";
            return false;
        }
        return true;
    }

    bool WindowsIntegration::RegisterHotkeys()
    {
        std::wstring err;
        auto const& s = dTranslate::Storage::SettingsManager::Instance().GetSettings();
        return ReRegisterHotkeys(s.translateSelectedHotkey, s.ocrHotkey, err);
    }

    bool WindowsIntegration::ReRegisterHotkeys(
        std::wstring const& selectionKey,
        std::wstring const& ocrKey,
        std::wstring& outError)
    {
        if (m_hWnd == nullptr) return false;

        UnregisterHotkeys();

        UINT mod1 = 0, vk1 = 0;
        UINT mod2 = 0, vk2 = 0;

        if (!ParseHotkey(selectionKey, mod1, vk1))
        {
            mod1 = MOD_CONTROL | MOD_ALT;
            vk1 = 'T';
        }
        if (!ParseHotkey(ocrKey, mod2, vk2))
        {
            mod2 = MOD_CONTROL | MOD_ALT;
            vk2 = 'O';
        }

        bool ok1 = RegisterHotKey(m_hWnd, HOTKEY_ID_SELECTION, mod1, vk1) != FALSE;
        bool ok2 = RegisterHotKey(m_hWnd, HOTKEY_ID_OCR, mod2, vk2) != FALSE;

        if (!ok1) outError = L"Conflict: " + selectionKey + L" is in use by another application.";
        else if (!ok2) outError = L"Conflict: " + ocrKey + L" is in use by another application.";

        return ok1 && ok2;
    }

    void WindowsIntegration::UnregisterHotkeys()
    {
        if (m_hWnd != nullptr)
        {
            UnregisterHotKey(m_hWnd, HOTKEY_ID_SELECTION);
            UnregisterHotKey(m_hWnd, HOTKEY_ID_OCR);
        }
    }

    void WindowsIntegration::TriggerTranslateSelection()
    {
        // Capture text from currently focused app
        auto cap = SelectionCapture::CaptureSelectedText(true);

        if (cap.text.empty())
        {
            // Fallback: check current clipboard text
            cap.text = ClipboardHelper::GetText();
            cap.hasSelection = false;
        }

        if (m_onTranslateSelection)
        {
            m_onTranslateSelection(cap.text, cap.targetHwnd, cap.hasSelection);
        }
    }

    static HBITMAP CreateMenuGlyphBitmap(const wchar_t* glyph)
    {
        int cx = GetSystemMetrics(SM_CXSMICON);
        if (cx <= 0) cx = 16;
        int cy = GetSystemMetrics(SM_CYSMICON);
        if (cy <= 0) cy = 16;

        HDC hdcScreen = GetDC(nullptr);
        HDC memDC = CreateCompatibleDC(hdcScreen);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cx;
        bmi.bmiHeader.biHeight = cy;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!hBmp)
        {
            DeleteDC(memDC);
            ReleaseDC(nullptr, hdcScreen);
            return nullptr;
        }

        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hBmp);
        memset(bits, 0, cx * cy * 4);

        HFONT hFont = CreateFontW(
            cy - 2, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe Fluent Icons");
        if (!hFont)
        {
            hFont = CreateFontW(
                cy - 2, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                L"Segoe MDL2 Assets");
        }

        HFONT oldFont = (HFONT)SelectObject(memDC, hFont);
        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(255, 255, 255));

        RECT rc = { 0, 0, cx, cy };
        DrawTextW(memDC, glyph, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        COLORREF menuTextColor = GetSysColor(COLOR_MENUTEXT);
        BYTE tr = GetRValue(menuTextColor);
        BYTE tg = GetGValue(menuTextColor);
        BYTE tb = GetBValue(menuTextColor);

        DWORD* pPixels = (DWORD*)bits;
        for (int i = 0; i < cx * cy; ++i)
        {
            DWORD pixel = pPixels[i];
            BYTE alpha = static_cast<BYTE>(pixel & 0xFF);
            if (alpha > 0)
            {
                BYTE pr = static_cast<BYTE>((tr * alpha) / 255);
                BYTE pg = static_cast<BYTE>((tg * alpha) / 255);
                BYTE pb = static_cast<BYTE>((tb * alpha) / 255);
                pPixels[i] = (alpha << 24) | (pr << 16) | (pg << 8) | pb;
            }
            else
            {
                pPixels[i] = 0;
            }
        }

        SelectObject(memDC, oldFont);
        if (hFont) DeleteObject(hFont);
        SelectObject(memDC, oldBmp);
        DeleteDC(memDC);
        ReleaseDC(nullptr, hdcScreen);

        return hBmp;
    }

    void WindowsIntegration::ShowTrayContextMenu(HWND hWnd)
    {
        HMENU hMenu = CreatePopupMenu();
        if (hMenu == nullptr) return;

        auto const& loc = dTranslate::Storage::LocalizationManager::Instance();
        std::wstring textClipboard = loc.Get(L"TrayTranslateClipboard");
        std::wstring textOcr = loc.Get(L"TrayScreenOcr");
        std::wstring textSettings = loc.Get(L"TraySettings");
        std::wstring textExit = loc.Get(L"TrayExit");

        HBITMAP hBmpClipboard = CreateMenuGlyphBitmap(L"\uE774");
        HBITMAP hBmpOcr = CreateMenuGlyphBitmap(L"\uEE6F");
        HBITMAP hBmpSettings = CreateMenuGlyphBitmap(L"\uE713");
        HBITMAP hBmpExit = CreateMenuGlyphBitmap(L"\uE7E8");

        auto addMenuItem = [&](UINT id, const std::wstring& text, HBITMAP hBmp)
        {
            MENUITEMINFOW mii = { sizeof(mii) };
            mii.fMask = MIIM_STRING | MIIM_ID | (hBmp ? MIIM_BITMAP : 0);
            mii.wID = id;
            mii.dwTypeData = const_cast<LPWSTR>(text.c_str());
            mii.hbmpItem = hBmp;
            InsertMenuItemW(hMenu, GetMenuItemCount(hMenu), TRUE, &mii);
        };

        addMenuItem(IDM_TRAY_CLIPBOARD, textClipboard, hBmpClipboard);
        addMenuItem(IDM_TRAY_OCR, textOcr, hBmpOcr);
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        addMenuItem(IDM_TRAY_SETTINGS, textSettings, hBmpSettings);
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        addMenuItem(IDM_TRAY_EXIT, textExit, hBmpExit);

        POINT pt;
        GetCursorPos(&pt);

        SetForegroundWindow(hWnd);
        TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, hWnd, nullptr);
        DestroyMenu(hMenu);

        if (hBmpClipboard) DeleteObject(hBmpClipboard);
        if (hBmpOcr) DeleteObject(hBmpOcr);
        if (hBmpSettings) DeleteObject(hBmpSettings);
        if (hBmpExit) DeleteObject(hBmpExit);
    }

    LRESULT CALLBACK WindowsIntegration::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WindowsIntegration* pThis = &WindowsIntegration::Instance();

        switch (uMsg)
        {
        case WM_TRAYICON:
            if (LOWORD(lParam) == WM_LBUTTONUP || LOWORD(lParam) == WM_LBUTTONDBLCLK)
            {
                if (pThis->m_onOpenPopup)
                {
                    pThis->m_onOpenPopup();
                }
            }
            else if (LOWORD(lParam) == WM_RBUTTONUP)
            {
                POINT pt;
                GetCursorPos(&pt);
                if (pThis->m_onShowTrayMenu)
                {
                    pThis->m_onShowTrayMenu(pt.x, pt.y);
                }
                else
                {
                    pThis->ShowTrayContextMenu(hWnd);
                }
            }
            return 0;

        case WM_HOTKEY:
            if (wParam == HOTKEY_ID_SELECTION)
            {
                pThis->TriggerTranslateSelection();
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
            LogDebug(L"WndProc WM_COMMAND: %lu", (DWORD)LOWORD(wParam));
            switch (LOWORD(wParam))
            {
            case IDM_TRAY_OPEN:
                if (pThis->m_onOpenPopup) pThis->m_onOpenPopup();
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
