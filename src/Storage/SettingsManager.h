#pragma once
#include <string>
#include <filesystem>

namespace dTranslate::Storage
{
    struct AppSettings
    {
        std::wstring sourceLanguage{ L"auto" };
        std::wstring targetLanguage{ L"ru" };
        int primaryService{ 0 }; // 0 = Google, 1 = Yandex, 2 = Gemini, 3 = OpenAI
        std::wstring theme{ L"Default" }; // Default, Light, Dark
        bool autoStart{ false };
        std::wstring globalHotkey{ L"Ctrl+Alt+T" };
        std::wstring quickHotkey{ L"Ctrl+Alt+D" };
        std::wstring ocrHotkey{ L"Ctrl+Alt+O" };
        bool compareTranslations{ false };
        bool sidebarCollapsed{ false };
        std::wstring geminiModel{ L"gemini-2.5-flash" };
        std::wstring openAiModel{ L"gpt-4o-mini" };
    };

    class SettingsManager
    {
    public:
        static SettingsManager& Instance();

        AppSettings const& GetSettings() const { return m_settings; }
        void UpdateSettings(AppSettings const& newSettings);
        void Save();
        void Load();

        static std::filesystem::path GetAppDataPath();

    private:
        SettingsManager();
        AppSettings m_settings;
    };
}
