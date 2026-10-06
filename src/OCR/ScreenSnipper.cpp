#include "pch.h"
#include "ScreenSnipper.h"
#include "WindowsOcrService.h"
#include <vector>
#include <algorithm>

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

namespace dTranslate::OCR
{
    static std::vector<uint8_t> CropHBitmapToBmp(HBITMAP hSrcBmp, int x, int y, int width, int height)
    {
        if (width <= 0 || height <= 0 || hSrcBmp == nullptr) return {};

        HDC hScreenDC = GetDC(nullptr);
        HDC hSrcDC = CreateCompatibleDC(hScreenDC);
        HDC hDstDC = CreateCompatibleDC(hScreenDC);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = height; // bottom-up DIB
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;
        HBITMAP hDstBmp = CreateDIBSection(hDstDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

        HGDIOBJ hOldSrc = SelectObject(hSrcDC, hSrcBmp);
        HGDIOBJ hOldDst = SelectObject(hDstDC, hDstBmp);

        BitBlt(hDstDC, 0, 0, width, height, hSrcDC, x, y, SRCCOPY);

        SelectObject(hSrcDC, hOldSrc);
        SelectObject(hDstDC, hOldDst);
        DeleteDC(hSrcDC);
        DeleteDC(hDstDC);
        ReleaseDC(nullptr, hScreenDC);

        DWORD rowSize = width * 4;
        DWORD imageSize = rowSize * height;
        DWORD fileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + imageSize;

        std::vector<uint8_t> buffer(fileSize);

        BITMAPFILEHEADER bfh = {};
        bfh.bfType = 0x4D42; // "BM"
        bfh.bfSize = fileSize;
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

        BITMAPINFOHEADER bih = {};
        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biWidth = width;
        bih.biHeight = height;
        bih.biPlanes = 1;
        bih.biBitCount = 32;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = imageSize;

        memcpy(buffer.data(), &bfh, sizeof(bfh));
        memcpy(buffer.data() + sizeof(bfh), &bih, sizeof(bih));
        if (pBits)
        {
            memcpy(buffer.data() + bfh.bfOffBits, pBits, imageSize);
        }

        DeleteObject(hDstBmp);
        return buffer;
    }

    ScreenSnipper& ScreenSnipper::Instance()
    {
        static ScreenSnipper instance;
        return instance;
    }

    ScreenSnipper::~ScreenSnipper()
    {
        CloseOverlay();
    }

    void ScreenSnipper::StartSnipping(
        winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
        std::function<void(std::wstring const& recognizedText, bool autoTranslate)> onResult)
    {
        if (m_hWnd != nullptr)
        {
            return; // Already open
        }

        m_dispatcher = dispatcher;
        m_onResult = onResult;

        // 1. Capture virtual screen
        m_vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
        m_vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
        m_vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        m_vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        HDC hScreenDC = GetDC(nullptr);
        HDC hMemDC = CreateCompatibleDC(hScreenDC);
        m_hScreenBmp = CreateCompatibleBitmap(hScreenDC, m_vw, m_vh);
        HGDIOBJ hOld = SelectObject(hMemDC, m_hScreenBmp);
        BitBlt(hMemDC, 0, 0, m_vw, m_vh, hScreenDC, m_vx, m_vy, SRCCOPY);
        SelectObject(hMemDC, hOld);
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);

        // 2. Register overlay window class
        static bool classRegistered = false;
        if (!classRegistered)
        {
            WNDCLASSEXW wc = { sizeof(wc) };
            wc.lpfnWndProc = OverlayWndProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = L"dTranslate_ScreenSnipperOverlay";
            wc.hCursor = LoadCursorW(nullptr, IDC_CROSS);
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            RegisterClassExW(&wc);
            classRegistered = true;
        }

        // 3. Reset states
        m_isSelecting = false;
        m_selectionDone = false;
        m_selectedRect = { 0, 0, 0, 0 };
        m_hoveredBtn = 0;

        // 4. Create fullscreen overlay
        m_hWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"dTranslate_ScreenSnipperOverlay",
            L"dTranslate Screen Snipper",
            WS_POPUP | WS_VISIBLE,
            m_vx, m_vy, m_vw, m_vh,
            nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

        if (m_hWnd)
        {
            SetForegroundWindow(m_hWnd);
            SetFocus(m_hWnd);
        }
    }

    void ScreenSnipper::CloseOverlay()
    {
        if (m_hWnd != nullptr)
        {
            HWND h = m_hWnd;
            m_hWnd = nullptr;
            DestroyWindow(h);
        }
        if (m_hScreenBmp != nullptr)
        {
            DeleteObject(m_hScreenBmp);
            m_hScreenBmp = nullptr;
        }
        m_isSelecting = false;
        m_selectionDone = false;
        m_hoveredBtn = 0;
    }

    LRESULT CALLBACK ScreenSnipper::OverlayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        ScreenSnipper& snipper = ScreenSnipper::Instance();

        switch (uMsg)
        {
        case WM_PAINT:
            snipper.OnPaint(hWnd);
            return 0;

        case WM_ERASEBKGND:
            return 1; // Prevent flicker

        case WM_LBUTTONDOWN:
            snipper.OnLButtonDown(hWnd, (short)LOWORD(lParam), (short)HIWORD(lParam));
            return 0;

        case WM_MOUSEMOVE:
            snipper.OnMouseMove(hWnd, (short)LOWORD(lParam), (short)HIWORD(lParam));
            return 0;

        case WM_LBUTTONUP:
            snipper.OnLButtonUp(hWnd, (short)LOWORD(lParam), (short)HIWORD(lParam));
            return 0;

        case WM_KEYDOWN:
            snipper.OnKeyDown(hWnd, wParam);
            return 0;

        case WM_RBUTTONDOWN:
            snipper.CloseOverlay();
            return 0;

        case WM_DESTROY:
            snipper.m_hWnd = nullptr;
            return 0;
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    void ScreenSnipper::OnPaint(HWND hWnd)
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);

        HDC hMemDC = CreateCompatibleDC(hDC);
        HBITMAP hBackBmp = CreateCompatibleBitmap(hDC, m_vw, m_vh);
        HGDIOBJ hOldBack = SelectObject(hMemDC, hBackBmp);

        HDC hSrcDC = CreateCompatibleDC(hDC);
        HGDIOBJ hOldSrc = SelectObject(hSrcDC, m_hScreenBmp);

        // 1. Draw base screenshot
        BitBlt(hMemDC, 0, 0, m_vw, m_vh, hSrcDC, 0, 0, SRCCOPY);

        // 2. Dim full screen with dark tint
        HDC hBlackDC = CreateCompatibleDC(hDC);
        BITMAPINFO bmi1 = {};
        bmi1.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi1.bmiHeader.biWidth = 1;
        bmi1.bmiHeader.biHeight = 1;
        bmi1.bmiHeader.biPlanes = 1;
        bmi1.bmiHeader.biBitCount = 32;
        bmi1.bmiHeader.biCompression = BI_RGB;
        void* pBlackBits = nullptr;
        HBITMAP hBlackBmp = CreateDIBSection(hBlackDC, &bmi1, DIB_RGB_COLORS, &pBlackBits, nullptr, 0);
        HGDIOBJ hOldBlack = SelectObject(hBlackDC, hBlackBmp);
        if (pBlackBits) *static_cast<uint32_t*>(pBlackBits) = 0x00000000;

        BLENDFUNCTION bf = {};
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.SourceConstantAlpha = 140; // 55% darkness
        bf.AlphaFormat = 0;

        AlphaBlend(hMemDC, 0, 0, m_vw, m_vh, hBlackDC, 0, 0, 1, 1, bf);

        SelectObject(hBlackDC, hOldBlack);
        DeleteObject(hBlackBmp);
        DeleteDC(hBlackDC);

        // 3. If dragging or selection is done, restore clear region
        RECT sel = m_selectedRect;
        int selW = sel.right - sel.left;
        int selH = sel.bottom - sel.top;
        if (selW > 0 && selH > 0)
        {
            BitBlt(hMemDC, sel.left, sel.top, selW, selH, hSrcDC, sel.left, sel.top, SRCCOPY);

            HPEN hPen = CreatePen(PS_SOLID, 2, RGB(37, 99, 235));
            HGDIOBJ hOldPen = SelectObject(hMemDC, hPen);
            HGDIOBJ hOldBrush = SelectObject(hMemDC, GetStockObject(NULL_BRUSH));
            Rectangle(hMemDC, sel.left - 1, sel.top - 1, sel.right + 1, sel.bottom + 1);
            SelectObject(hMemDC, hOldPen);
            SelectObject(hMemDC, hOldBrush);
            DeleteObject(hPen);
        }

        // 4. If selection is done, draw floating toolbar
        if (m_selectionDone && selW > 10 && selH > 10)
        {
            int tbW = 340;
            int tbH = 46;
            int tbX = sel.left + (selW - tbW) / 2;
            if (tbX < 10) tbX = 10;
            if (tbX + tbW > m_vw - 10) tbX = m_vw - tbW - 10;

            int tbY = sel.bottom + 10;
            if (tbY + tbH > m_vh - 10) tbY = sel.top - tbH - 10;
            if (tbY < 10) tbY = 10;

            RECT tbRect = { tbX, tbY, tbX + tbW, tbY + tbH };
            HBRUSH hTbBg = CreateSolidBrush(RGB(30, 30, 30));
            HPEN hTbBorder = CreatePen(PS_SOLID, 1, RGB(70, 70, 75));
            HGDIOBJ hOldB = SelectObject(hMemDC, hTbBg);
            HGDIOBJ hOldP = SelectObject(hMemDC, hTbBorder);
            RoundRect(hMemDC, tbRect.left, tbRect.top, tbRect.right, tbRect.bottom, 10, 10);
            SelectObject(hMemDC, hOldB);
            SelectObject(hMemDC, hOldP);
            DeleteObject(hTbBg);
            DeleteObject(hTbBorder);

            m_btnTranslateRect = { tbX + 8, tbY + 7, tbX + 116, tbY + 39 };
            m_btnCopyRect = { tbX + 122, tbY + 7, tbX + 230, tbY + 39 };
            m_btnCancelRect = { tbX + 236, tbY + 7, tbX + 332, tbY + 39 };

            HFONT hFont = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HGDIOBJ hOldFont = SelectObject(hMemDC, hFont);
            SetBkMode(hMemDC, TRANSPARENT);

            auto DrawBtn = [&](RECT const& rc, wchar_t const* text, COLORREF bgCol, COLORREF textCol)
            {
                HBRUSH hBtnBg = CreateSolidBrush(bgCol);
                HPEN hBtnPen = CreatePen(PS_SOLID, 1, bgCol);
                HGDIOBJ oB = SelectObject(hMemDC, hBtnBg);
                HGDIOBJ oP = SelectObject(hMemDC, hBtnPen);
                RoundRect(hMemDC, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
                SelectObject(hMemDC, oB);
                SelectObject(hMemDC, oP);
                DeleteObject(hBtnBg);
                DeleteObject(hBtnPen);

                SetTextColor(hMemDC, textCol);
                RECT textRc = rc;
                DrawTextW(hMemDC, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            };

            COLORREF btn1Bg = (m_hoveredBtn == 1) ? RGB(29, 78, 216) : RGB(37, 99, 235);
            DrawBtn(m_btnTranslateRect, L"Translate", btn1Bg, RGB(255, 255, 255));

            COLORREF btn2Bg = (m_hoveredBtn == 2) ? RGB(63, 63, 70) : RGB(45, 45, 48);
            DrawBtn(m_btnCopyRect, L"Copy Text", btn2Bg, RGB(255, 255, 255));

            COLORREF btn3Bg = (m_hoveredBtn == 3) ? RGB(185, 28, 28) : RGB(45, 45, 48);
            DrawBtn(m_btnCancelRect, L"Cancel", btn3Bg, RGB(220, 220, 220));

            SelectObject(hMemDC, hOldFont);
            DeleteObject(hFont);
        }

        // 5. Transfer to screen
        BitBlt(hDC, 0, 0, m_vw, m_vh, hMemDC, 0, 0, SRCCOPY);

        SelectObject(hSrcDC, hOldSrc);
        DeleteDC(hSrcDC);

        SelectObject(hMemDC, hOldBack);
        DeleteObject(hBackBmp);
        DeleteDC(hMemDC);

        EndPaint(hWnd, &ps);
    }

    void ScreenSnipper::OnLButtonDown(HWND hWnd, int x, int y)
    {
        POINT pt = { x, y };

        if (m_selectionDone)
        {
            if (PtInRect(&m_btnTranslateRect, pt))
            {
                ExecuteAction(true);
                return;
            }
            if (PtInRect(&m_btnCopyRect, pt))
            {
                ExecuteAction(false);
                return;
            }
            if (PtInRect(&m_btnCancelRect, pt))
            {
                CloseOverlay();
                return;
            }

            // Clicked outside toolbar, restart selection
            m_selectionDone = false;
        }

        m_isSelecting = true;
        m_ptStart = pt;
        m_ptCurrent = pt;
        m_selectedRect = { pt.x, pt.y, pt.x, pt.y };
        SetCapture(hWnd);
        InvalidateRect(hWnd, nullptr, FALSE);
    }

    void ScreenSnipper::OnMouseMove(HWND hWnd, int x, int y)
    {
        POINT pt = { x, y };

        if (m_selectionDone)
        {
            int oldHover = m_hoveredBtn;
            if (PtInRect(&m_btnTranslateRect, pt))
            {
                m_hoveredBtn = 1;
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
            }
            else if (PtInRect(&m_btnCopyRect, pt))
            {
                m_hoveredBtn = 2;
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
            }
            else if (PtInRect(&m_btnCancelRect, pt))
            {
                m_hoveredBtn = 3;
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
            }
            else
            {
                m_hoveredBtn = 0;
                SetCursor(LoadCursorW(nullptr, IDC_CROSS));
            }

            if (m_hoveredBtn != oldHover)
            {
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return;
        }

        if (m_isSelecting)
        {
            m_ptCurrent = pt;
            int left = (std::min)(m_ptStart.x, m_ptCurrent.x);
            int top = (std::min)(m_ptStart.y, m_ptCurrent.y);
            int right = (std::max)(m_ptStart.x, m_ptCurrent.x);
            int bottom = (std::max)(m_ptStart.y, m_ptCurrent.y);
            m_selectedRect = { left, top, right, bottom };
            InvalidateRect(hWnd, nullptr, FALSE);
        }
    }

    void ScreenSnipper::OnLButtonUp(HWND hWnd, int x, int y)
    {
        if (m_isSelecting)
        {
            m_isSelecting = false;
            ReleaseCapture();

            m_ptCurrent = { x, y };
            int left = (std::min)(m_ptStart.x, m_ptCurrent.x);
            int top = (std::min)(m_ptStart.y, m_ptCurrent.y);
            int right = (std::max)(m_ptStart.x, m_ptCurrent.x);
            int bottom = (std::max)(m_ptStart.y, m_ptCurrent.y);

            if ((right - left > 10) && (bottom - top > 10))
            {
                m_selectedRect = { left, top, right, bottom };
                m_selectionDone = true;
            }
            else
            {
                m_selectedRect = { 0, 0, 0, 0 };
                m_selectionDone = false;
            }
            InvalidateRect(hWnd, nullptr, FALSE);
        }
    }

    void ScreenSnipper::OnKeyDown(HWND, WPARAM wParam)
    {
        if (wParam == VK_ESCAPE)
        {
            CloseOverlay();
        }
    }

    void ScreenSnipper::ExecuteAction(bool autoTranslate)
    {
        int cropX = m_selectedRect.left;
        int cropY = m_selectedRect.top;
        int cropW = m_selectedRect.right - m_selectedRect.left;
        int cropH = m_selectedRect.bottom - m_selectedRect.top;

        auto bmpBytes = CropHBitmapToBmp(m_hScreenBmp, cropX, cropY, cropW, cropH);

        auto dispatcher = m_dispatcher;
        auto onResult = m_onResult;

        CloseOverlay();

        if (!bmpBytes.empty())
        {
            WindowsOcrService::Instance().RecognizeBmpBufferAsync(
                bmpBytes,
                dispatcher,
                [onResult, autoTranslate](OcrResult const& res)
                {
                    if (onResult && res.success)
                    {
                        onResult(res.text, autoTranslate);
                    }
                });
        }
    }
}
