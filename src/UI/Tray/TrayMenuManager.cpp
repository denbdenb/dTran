#include "pch.h"
#include "TrayMenuManager.h"
#include "LocalizationManager.h"
#include "SettingsManager.h"
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.h>
#include <microsoft.ui.xaml.window.h>
#include <dwmapi.h>
#include <commctrl.h>
#include <cmath>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI::Windowing;
using namespace dTranslate::Storage;

namespace dTranslate::UI
{
    static LRESULT CALLBACK TrayMenuSubclassProc(
        HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
        UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
    {
        if (uMsg == WM_ACTIVATE)
        {
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                ShowWindow(hWnd, SW_HIDE);
            }
        }
        else if (uMsg == WM_KILLFOCUS)
        {
            ShowWindow(hWnd, SW_HIDE);
        }
        else if (uMsg == WM_NCPAINT)
        {
            return 0; // Suppress any non-client frame/border drawing
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    TrayMenuManager& TrayMenuManager::Instance()
    {
        static TrayMenuManager instance;
        return instance;
    }

    TrayMenuManager::TrayMenuManager()
    {
        // Pre-create reusable brushes for Light & Dark appearance
        m_transparentBrush   = SolidColorBrush(Microsoft::UI::Colors::Transparent());

        // Light Theme Brushes
        m_lightBgBrush       = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(246, 250, 250, 250));
        m_lightBorderBrush   = m_transparentBrush;
        m_lightHoverBrush    = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(24, 0, 0, 0));    // ~10% black, distinct & clean
        m_lightPressedBrush  = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(42, 0, 0, 0));    // ~16% black
        m_lightTextBrush     = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(240, 20, 20, 20));
        m_lightIconBrush     = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(230, 25, 25, 25));
        m_lightHintBrush     = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(145, 0, 0, 0));
        m_lightSepBrush      = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(28, 0, 0, 0));

        // Dark Theme Brushes
        m_darkBgBrush        = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(242, 36, 36, 36));
        m_darkBorderBrush    = m_transparentBrush;
        m_darkHoverBrush     = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(38, 255, 255, 255)); // ~15% white, distinct & clean
        m_darkPressedBrush   = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(62, 255, 255, 255)); // ~24% white
        m_darkTextBrush      = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(245, 255, 255, 255));
        m_darkIconBrush      = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(235, 255, 255, 255));
        m_darkHintBrush      = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(155, 255, 255, 255));
        m_darkSepBrush       = SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(35, 255, 255, 255));
    }

    bool TrayMenuManager::IsSystemDarkTheme() const
    {
        DWORD value = 0;
        DWORD size = sizeof(value);

        // 1. Check SystemUsesLightTheme (controls Taskbar / System Tray appearance)
        if (RegGetValueW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"SystemUsesLightTheme",
            RRF_RT_REG_DWORD,
            nullptr,
            &value,
            &size) == ERROR_SUCCESS)
        {
            return (value == 0);
        }

        // 2. Fallback to AppsUseLightTheme
        if (RegGetValueW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"AppsUseLightTheme",
            RRF_RT_REG_DWORD,
            nullptr,
            &value,
            &size) == ERROR_SUCCESS)
        {
            return (value == 0);
        }

        return false;
    }

    void TrayMenuManager::Initialize()
    {
        if (m_window) return;

        BuildUI();

        LocalizationManager::Instance().RegisterObserver(
            reinterpret_cast<uintptr_t>(this),
            [this](std::wstring const&)
            {
                if (m_window && m_window.DispatcherQueue())
                {
                    m_window.DispatcherQueue().TryEnqueue([this]()
                    {
                        UpdateLocalization();
                    });
                }
            });
    }

    void TrayMenuManager::BuildUI()
    {
        m_window = Window();

        try
        {
            m_window.SystemBackdrop(DesktopAcrylicBackdrop());
        }
        catch (...) {}

        if (auto windowNative = m_window.try_as<IWindowNative>())
        {
            windowNative->get_WindowHandle(&m_hwnd);
        }

        auto const& loc = LocalizationManager::Instance();
        auto const& settings = SettingsManager::Instance().GetSettings();

        m_menuItems.clear();
        m_separators.clear();

        auto createMenuItemHelper = [this](
            wchar_t const* glyph,
            TextBlock& outTextBlock,
            std::wstring const& initialText,
            TextBlock* outHintBlock,
            std::wstring const& shortcutHint,
            std::function<void()> onClick) -> Button
        {
            Button btn;
            btn.HorizontalAlignment(HorizontalAlignment::Stretch);
            btn.HorizontalContentAlignment(HorizontalAlignment::Left);
            btn.Background(m_transparentBrush);
            btn.BorderThickness({ 0, 0, 0, 0 });
            btn.Padding({ 10, 6, 10, 6 });
            btn.CornerRadius({ 5, 5, 5, 5 });
            btn.Height(34);

            Grid grid;
            ColumnDefinition colIcon;
            colIcon.Width({ 0, GridUnitType::Auto });
            ColumnDefinition colText;
            colText.Width({ 1, GridUnitType::Star });
            ColumnDefinition colHint;
            colHint.Width({ 0, GridUnitType::Auto });

            grid.ColumnDefinitions().Append(colIcon);
            grid.ColumnDefinitions().Append(colText);
            grid.ColumnDefinitions().Append(colHint);

            FontIcon icon;
            icon.Glyph(winrt::hstring(glyph));
            icon.FontSize(15);
            icon.Margin({ 0, 0, 10, 0 });
            Grid::SetColumn(icon, 0);
            grid.Children().Append(icon);

            TextBlock tb;
            tb.Text(winrt::hstring(initialText));
            tb.FontSize(13);
            tb.VerticalAlignment(VerticalAlignment::Center);
            Grid::SetColumn(tb, 1);
            grid.Children().Append(tb);
            outTextBlock = tb;

            TextBlock hint{ nullptr };
            if (!shortcutHint.empty())
            {
                hint = TextBlock();
                hint.Text(winrt::hstring(shortcutHint));
                hint.FontSize(11);
                hint.VerticalAlignment(VerticalAlignment::Center);
                hint.Margin({ 14, 0, 4, 0 });
                Grid::SetColumn(hint, 2);
                grid.Children().Append(hint);
                if (outHintBlock) *outHintBlock = hint;
            }

            btn.Content(grid);

            // Setup pointer hover/pressed reactions
            btn.PointerEntered([this, btn](auto&&, auto&&)
            {
                btn.Background(m_isDarkTheme ? m_darkHoverBrush : m_lightHoverBrush);
            });
            btn.PointerExited([this, btn](auto&&, auto&&)
            {
                btn.Background(m_transparentBrush);
            });
            btn.PointerPressed([this, btn](auto&&, auto&&)
            {
                btn.Background(m_isDarkTheme ? m_darkPressedBrush : m_lightPressedBrush);
            });
            btn.PointerReleased([this, btn](auto&&, auto&&)
            {
                btn.Background(m_isDarkTheme ? m_darkHoverBrush : m_lightHoverBrush);
            });

            btn.Click([this, onClick](auto&&, auto&&)
            {
                Hide();
                if (onClick) onClick();
            });

            TrayMenuItemVisuals visuals;
            visuals.btn = btn;
            visuals.icon = icon;
            visuals.tb = tb;
            visuals.hint = hint;
            m_menuItems.push_back(visuals);

            return btn;
        };

        auto createSeparatorHelper = [this]() -> Shapes::Rectangle
        {
            Shapes::Rectangle rect;
            rect.Height(1);
            rect.Margin({ 6, 3, 6, 3 });
            rect.HorizontalAlignment(HorizontalAlignment::Stretch);
            rect.Fill(m_lightSepBrush);
            m_separators.push_back(rect);
            return rect;
        };

        // Build items: (Open dTran completely omitted)
        auto btnClipboard = createMenuItemHelper(L"\uE774", m_tbClipboard, loc.Get(L"TrayTranslateClipboard"), &m_hintClipboard, settings.translateSelectedHotkey, [this]()
        {
            if (m_onTranslateClipboard) m_onTranslateClipboard();
        });

        auto btnOcr = createMenuItemHelper(L"\uEE6F", m_tbOcr, loc.Get(L"TrayScreenOcr"), &m_hintOcr, settings.ocrHotkey, [this]()
        {
            if (m_onScreenOcr) m_onScreenOcr();
        });

        auto sep1 = createSeparatorHelper();

        auto btnSettings = createMenuItemHelper(L"\uE713", m_tbSettings, loc.Get(L"TraySettings"), nullptr, L"", [this]()
        {
            if (m_onOpenSettings) m_onOpenSettings();
        });

        auto sep2 = createSeparatorHelper();

        auto btnExit = createMenuItemHelper(L"\uE7E8", m_tbExit, loc.Get(L"TrayExit"), nullptr, L"", [this]()
        {
            if (m_onExit) m_onExit();
        });

        StackPanel sp;
        sp.Spacing(1);
        sp.Children().Append(btnClipboard);
        sp.Children().Append(btnOcr);
        sp.Children().Append(sep1);
        sp.Children().Append(btnSettings);
        sp.Children().Append(sep2);
        sp.Children().Append(btnExit);

        Border rootBorder;
        rootBorder.CornerRadius({ 8, 8, 8, 8 });
        rootBorder.BorderThickness({ 0, 0, 0, 0 });
        rootBorder.BorderBrush(m_transparentBrush);
        rootBorder.Padding({ 4, 4, 4, 4 });
        rootBorder.Child(sp);

        m_rootBorder = rootBorder;
        m_window.Content(m_rootBorder);

        if (m_hwnd)
        {
            // Strip any legacy window borders, caption and sizing frame
            LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_BORDER | WS_DLGFRAME);
            style |= WS_POPUP;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);

            LONG_PTR exStyle = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);
            exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
            exStyle |= (WS_EX_TOPMOST | WS_EX_TOOLWINDOW);
            SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, exStyle);

            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, 
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

            DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;
            DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
            SetWindowSubclass(m_hwnd, TrayMenuSubclassProc, 1, 0);
        }

        try
        {
            auto appWin = m_window.AppWindow();
            if (appWin)
            {
                appWin.IsShownInSwitchers(false);
                if (auto presenter = appWin.Presenter().try_as<OverlappedPresenter>())
                {
                    presenter.IsResizable(false);
                    presenter.IsMinimizable(false);
                    presenter.IsMaximizable(false);
                    presenter.SetBorderAndTitleBar(false, false);
                }
            }
        }
        catch (...) {}

        // Apply initial system theme
        ApplyTheme(IsSystemDarkTheme());
    }

    void TrayMenuManager::ApplyTheme(bool isDark)
    {
        m_isDarkTheme = isDark;

        if (m_hwnd)
        {
            BOOL dwmDark = isDark ? TRUE : FALSE;
            DwmSetWindowAttribute(m_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dwmDark, sizeof(dwmDark));

            DWM_WINDOW_CORNER_PREFERENCE pref = DWMWCP_ROUND;
            DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

            COLORREF borderColor = isDark ? RGB(36, 36, 36) : RGB(250, 250, 250);
            DwmSetWindowAttribute(m_hwnd, 34 /* DWMWA_BORDER_COLOR */, &borderColor, sizeof(borderColor));
        }

        if (m_rootBorder)
        {
            m_rootBorder.RequestedTheme(isDark ? ElementTheme::Dark : ElementTheme::Light);
            m_rootBorder.Background(isDark ? m_darkBgBrush : m_lightBgBrush);
            m_rootBorder.BorderThickness({ 0, 0, 0, 0 });
            m_rootBorder.BorderBrush(m_transparentBrush);
        }

        auto textBrush = isDark ? m_darkTextBrush : m_lightTextBrush;
        auto iconBrush = isDark ? m_darkIconBrush : m_lightIconBrush;
        auto hintBrush = isDark ? m_darkHintBrush : m_lightHintBrush;
        auto sepBrush  = isDark ? m_darkSepBrush  : m_lightSepBrush;

        for (auto& item : m_menuItems)
        {
            if (item.btn) item.btn.Background(m_transparentBrush);
            if (item.tb) item.tb.Foreground(textBrush);
            if (item.icon) item.icon.Foreground(iconBrush);
            if (item.hint) item.hint.Foreground(hintBrush);
        }

        for (auto& sep : m_separators)
        {
            if (sep) sep.Fill(sepBrush);
        }
    }

    void TrayMenuManager::ShowAt(int screenX, int screenY)
    {
        if (!m_window)
        {
            BuildUI();
        }
        if (!m_hwnd) return;

        // 1. Detect dynamic Windows system theme & apply immediately
        bool isDark = IsSystemDarkTheme();
        ApplyTheme(isDark);

        // 2. Ensure localized strings and hotkey hints are up to date
        UpdateLocalization();

        UINT dpi = GetDpiForWindow(m_hwnd);
        if (dpi == 0) dpi = GetDpiForSystem();
        float scale = (dpi > 0) ? (static_cast<float>(dpi) / 96.0f) : 1.0f;

        int w = static_cast<int>(270 * scale);

        // Measure content dynamically to eliminate extra vertical whitespace
        int h = static_cast<int>(162 * scale);
        if (m_rootBorder)
        {
            m_rootBorder.Measure({ static_cast<float>(270), 10000.0f });
            float desiredH = m_rootBorder.DesiredSize().Height;
            if (desiredH > 50.0f && desiredH < 350.0f)
            {
                h = static_cast<int>(std::ceil(desiredH * scale));
            }
        }

        POINT pt = { screenX, screenY };
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfoW(hMon, &mi);

        int x = pt.x - w / 2;
        int y = pt.y - h - static_cast<int>(8 * scale);

        // Clamping to work area near system tray
        if (x + w > mi.rcWork.right) x = mi.rcWork.right - w - static_cast<int>(8 * scale);
        if (x < mi.rcWork.left) x = mi.rcWork.left + static_cast<int>(8 * scale);
        if (y < mi.rcWork.top) y = pt.y + static_cast<int>(8 * scale);
        if (y + h > mi.rcWork.bottom) y = mi.rcWork.bottom - h - static_cast<int>(8 * scale);

        auto appWin = m_window.AppWindow();
        appWin.MoveAndResize({ x, y, w, h });
        appWin.Show();
        SetForegroundWindow(m_hwnd);
        SetActiveWindow(m_hwnd);

        // Ensure border color matches background after activation
        COLORREF borderColor = isDark ? RGB(36, 36, 36) : RGB(250, 250, 250);
        DwmSetWindowAttribute(m_hwnd, 34 /* DWMWA_BORDER_COLOR */, &borderColor, sizeof(borderColor));
    }

    void TrayMenuManager::Hide()
    {
        if (m_hwnd)
        {
            ShowWindow(m_hwnd, SW_HIDE);
        }
    }

    void TrayMenuManager::UpdateLocalization()
    {
        auto const& loc = LocalizationManager::Instance();
        auto const& settings = SettingsManager::Instance().GetSettings();
        if (m_tbClipboard) m_tbClipboard.Text(winrt::hstring(loc.Get(L"TrayTranslateClipboard")));
        if (m_tbOcr) m_tbOcr.Text(winrt::hstring(loc.Get(L"TrayScreenOcr")));
        if (m_tbSettings) m_tbSettings.Text(winrt::hstring(loc.Get(L"TraySettings")));
        if (m_tbExit) m_tbExit.Text(winrt::hstring(loc.Get(L"TrayExit")));
        if (m_hintClipboard) m_hintClipboard.Text(winrt::hstring(settings.translateSelectedHotkey));
        if (m_hintOcr) m_hintOcr.Text(winrt::hstring(settings.ocrHotkey));
    }
}
