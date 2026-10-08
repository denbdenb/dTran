#pragma once
#include <windows.h>
#include <functional>
#include <vector>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>

namespace dTranslate::UI
{
    struct TrayMenuItemVisuals
    {
        winrt::Microsoft::UI::Xaml::Controls::Button btn{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::FontIcon icon{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock tb{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock hint{ nullptr };
    };

    class TrayMenuManager
    {
    public:
        static TrayMenuManager& Instance();

        void Initialize();
        void ShowAt(int screenX, int screenY);
        void Hide();

        void SetOnTranslateClipboard(std::function<void()> cb) { m_onTranslateClipboard = cb; }
        void SetOnScreenOcr(std::function<void()> cb) { m_onScreenOcr = cb; }
        void SetOnOpenSettings(std::function<void()> cb) { m_onOpenSettings = cb; }
        void SetOnExit(std::function<void()> cb) { m_onExit = cb; }

        void UpdateLocalization();
        bool IsSystemDarkTheme() const;

    private:
        TrayMenuManager();
        void BuildUI();
        void ApplyTheme(bool isDark);

        winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
        HWND m_hwnd{ nullptr };

        std::function<void()> m_onTranslateClipboard;
        std::function<void()> m_onScreenOcr;
        std::function<void()> m_onOpenSettings;
        std::function<void()> m_onExit;

        winrt::Microsoft::UI::Xaml::Controls::Border m_rootBorder{ nullptr };
        std::vector<TrayMenuItemVisuals> m_menuItems;
        std::vector<winrt::Microsoft::UI::Xaml::Shapes::Rectangle> m_separators;

        bool m_isDarkTheme{ false };

        // Theme brushes
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_transparentBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightBgBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkBgBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightBorderBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkBorderBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightHoverBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkHoverBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightPressedBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkPressedBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightTextBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkTextBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightIconBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkIconBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightHintBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkHintBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_lightSepBrush{ nullptr };
        winrt::Microsoft::UI::Xaml::Media::SolidColorBrush m_darkSepBrush{ nullptr };

        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_tbClipboard{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_tbOcr{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_tbSettings{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_tbExit{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_hintClipboard{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_hintOcr{ nullptr };
    };
}
