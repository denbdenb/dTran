#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <vector>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace dTranslate::OCR
{
    class ScreenSnipper
    {
    public:
        static ScreenSnipper& Instance();

        void StartSnipping(
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
            std::function<void(std::wstring const& recognizedText, bool autoTranslate)> onResult);

        void CloseOverlay();

    private:
        ScreenSnipper() = default;
        ~ScreenSnipper();

        static LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void OnPaint(HWND hWnd);
        void OnLButtonDown(HWND hWnd, int x, int y);
        void OnMouseMove(HWND hWnd, int x, int y);
        void OnLButtonUp(HWND hWnd, int x, int y);
        void OnKeyDown(HWND hWnd, WPARAM wParam);
        void ExecuteAction(bool autoTranslate);

        HWND m_hWnd{ nullptr };
        HBITMAP m_hScreenBmp{ nullptr };
        int m_vx{ 0 };
        int m_vy{ 0 };
        int m_vw{ 0 };
        int m_vh{ 0 };

        bool m_isSelecting{ false };
        bool m_selectionDone{ false };
        POINT m_ptStart{ 0, 0 };
        POINT m_ptCurrent{ 0, 0 };
        RECT m_selectedRect{ 0, 0, 0, 0 };

        RECT m_btnTranslateRect{ 0, 0, 0, 0 };
        RECT m_btnCopyRect{ 0, 0, 0, 0 };
        RECT m_btnCancelRect{ 0, 0, 0, 0 };
        int m_hoveredBtn{ 0 }; // 0 = none, 1 = Translate, 2 = Copy, 3 = Cancel

        winrt::Microsoft::UI::Dispatching::DispatcherQueue m_dispatcher{ nullptr };
        std::function<void(std::wstring const& recognizedText, bool autoTranslate)> m_onResult;
    };
}
