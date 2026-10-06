#pragma once
#include <windows.h>
#include <shellapi.h>
#include <functional>
#include <string>
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

namespace dTranslate::Windows
{
    class WindowsIntegration
    {
    public:
        static WindowsIntegration& Instance();

        bool Initialize(HINSTANCE hInstance = nullptr);
        void Shutdown();

        // Callback hooks
        void SetOnShowMainWindow(std::function<void()> callback) { m_onShowMainWindow = callback; }
        void SetOnTranslateSelection(std::function<void(std::wstring const&)> callback) { m_onTranslateSelection = callback; }
        void SetOnTranslateClipboard(std::function<void()> callback) { m_onTranslateClipboard = callback; }
        void SetOnScreenOcr(std::function<void()> callback) { m_onScreenOcr = callback; }
        void SetOnOpenSettings(std::function<void()> callback) { m_onOpenSettings = callback; }
        void SetOnExit(std::function<void()> callback) { m_onExit = callback; }

        void TriggerTranslateSelection();

        bool ReRegisterHotkeys(
            std::wstring const& selectionKey,
            std::wstring const& mainKey,
            std::wstring const& ocrKey,
            std::wstring& outError);

        static bool ValidateHotkey(std::wstring const& hotkeyStr, std::wstring& outError);

    private:
        WindowsIntegration();
        ~WindowsIntegration();

        static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        bool CreateMessageWindow();
        bool SetupTrayIcon();
        void RemoveTrayIcon();
        bool RegisterHotkeys();
        void UnregisterHotkeys();
        void ShowTrayContextMenu(HWND hWnd);

        HWND m_hWnd{ nullptr };
        NOTIFYICONDATAW m_nid{};
        bool m_trayAdded{ false };
        HICON m_hCustomIcon{ nullptr };

        std::function<void()> m_onShowMainWindow;
        std::function<void(std::wstring const&)> m_onTranslateSelection;
        std::function<void()> m_onTranslateClipboard;
        std::function<void()> m_onScreenOcr;
        std::function<void()> m_onOpenSettings;
        std::function<void()> m_onExit;

        static constexpr UINT WM_TRAYICON = WM_USER + 101;
        static constexpr int HOTKEY_ID_SELECTION = 2001;
        static constexpr int HOTKEY_ID_MAIN = 2002;
        static constexpr int HOTKEY_ID_OCR = 2003;

        static constexpr UINT IDM_TRAY_OPEN = 3001;
        static constexpr UINT IDM_TRAY_CLIPBOARD = 3002;
        static constexpr UINT IDM_TRAY_OCR = 3003;
        static constexpr UINT IDM_TRAY_SETTINGS = 3004;
        static constexpr UINT IDM_TRAY_EXIT = 3005;
    };
}
