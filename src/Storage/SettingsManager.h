#pragma once
#include <string>
#include <filesystem>
#include <functional>
#include <map>

#include <winrt/Windows.Foundation.h>

namespace dTranslate::Storage
{
    enum class WindowsStartupState : uint32_t
    {
        Disabled = 0,
        DisabledByUser = 1,
        Enabled = 2,
        DisabledByPolicy = 3,
        EnabledByPolicy = 4,
        ErrorOrUnavailable = 5
    };

    struct AppSettings
    {
        std::wstring sourceLanguage{ L"auto" };
        std::wstring targetLanguage{ L"ru" };
        int primaryService{ 0 }; // 0 = Google, 1 = Yandex
        std::wstring theme{ L"Default" }; // Default, Light, Dark
        std::wstring appLanguage{ L"en" }; // "en" or "ru"
        bool autoStart{ false };
        bool closeToTray{ true };
        std::wstring translateSelectedHotkey{ L"Ctrl+Alt+T" };
        std::wstring ocrHotkey{ L"Ctrl+Alt+O" };
        bool compareTranslations{ false };

        // Window geometry persistence for QuickPopup
        int quickPopupWidth{ 460 };
        int quickPopupHeight{ 460 };
    };

    using SettingsObserver = std::function<void(AppSettings const&)>;

    class SettingsManager
    {
    public:
        static SettingsManager& Instance();

        AppSettings const& GetSettings() const { return m_settings; }
        void UpdateSettings(AppSettings const& newSettings);
        void Save();
        void Load();

        void RegisterObserver(uintptr_t key, SettingsObserver observer);
        void UnregisterObserver(uintptr_t key);

        static std::filesystem::path GetAppDataPath();
        static WindowsStartupState GetStartupTaskState();
        static bool IsStartWithWindowsEnabled();
        static winrt::Windows::Foundation::IAsyncOperation<uint32_t> SetStartWithWindowsAsync(bool enable);
        static bool SetStartWithWindows(bool enable, WindowsStartupState* outActualState = nullptr);
        static void CleanupStaleRegistryEntries();

    private:
        SettingsManager();
        AppSettings m_settings;
        std::map<uintptr_t, SettingsObserver> m_observers;
    };
}
