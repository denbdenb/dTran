#pragma once
#include <string>
#include <filesystem>
#include <functional>
#include <map>

namespace dTranslate::Storage
{
    struct AppSettings
    {
        std::wstring sourceLanguage{ L"auto" };
        std::wstring targetLanguage{ L"ru" };
        int primaryService{ 0 }; // 0 = Google, 1 = Yandex, 2 = Gemini, 3 = OpenAI
        std::wstring theme{ L"Default" }; // Default, Light, Dark
        bool autoStart{ false };
        bool closeToTray{ true };
        std::wstring globalHotkey{ L"Ctrl+Alt+T" };
        std::wstring quickHotkey{ L"Ctrl+Alt+D" };
        std::wstring ocrHotkey{ L"Ctrl+Alt+O" };
        bool compareTranslations{ false };
        bool sidebarCollapsed{ false };
        std::wstring geminiModel{ L"gemini-2.5-flash" };
        std::wstring openAiModel{ L"gpt-4o-mini" };

        // Dictionary settings
        std::wstring dictSourceLang{ L"en" };
        std::wstring dictTargetLang{ L"ru" };
        int dictEngine{ 0 }; // 0 = Reverso, 1 = Wikipedia

        // Window geometry persistence
        int mainWindowWidth{ 1020 };
        int mainWindowHeight{ 720 };
        int quickPopupWidth{ 440 };
        int quickPopupHeight{ 420 };
        int settingsWindowWidth{ 960 };
        int settingsWindowHeight{ 680 };
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
        static bool SetStartWithWindows(bool enable);

    private:
        SettingsManager();
        AppSettings m_settings;
        std::map<uintptr_t, SettingsObserver> m_observers;
    };
}
