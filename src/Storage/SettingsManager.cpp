#include "pch.h"
#include "SettingsManager.h"
#include <fstream>
#include <sstream>
#include <winrt/Windows.Data.Json.h>
#include <shlobj.h>

namespace dTranslate::Storage
{
    using namespace winrt::Windows::Data::Json;

    SettingsManager& SettingsManager::Instance()
    {
        static SettingsManager instance;
        return instance;
    }

    SettingsManager::SettingsManager()
    {
        Load();
    }

    std::filesystem::path SettingsManager::GetAppDataPath()
    {
        wchar_t localAppData[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData)))
        {
            std::filesystem::path p(localAppData);
            p /= L"dTranslate";
            std::filesystem::create_directories(p);
            return p;
        }
        return std::filesystem::current_path();
    }

    void SettingsManager::UpdateSettings(AppSettings const& newSettings)
    {
        m_settings = newSettings;
        Save();
    }

    void SettingsManager::Save()
    {
        try
        {
            auto path = GetAppDataPath() / L"settings.json";
            JsonObject root;
            root.SetNamedValue(L"sourceLanguage", JsonValue::CreateStringValue(m_settings.sourceLanguage));
            root.SetNamedValue(L"targetLanguage", JsonValue::CreateStringValue(m_settings.targetLanguage));
            root.SetNamedValue(L"primaryService", JsonValue::CreateNumberValue(m_settings.primaryService));
            root.SetNamedValue(L"theme", JsonValue::CreateStringValue(m_settings.theme));
            root.SetNamedValue(L"autoStart", JsonValue::CreateBooleanValue(m_settings.autoStart));
            root.SetNamedValue(L"globalHotkey", JsonValue::CreateStringValue(m_settings.globalHotkey));
            root.SetNamedValue(L"quickHotkey", JsonValue::CreateStringValue(m_settings.quickHotkey));
            root.SetNamedValue(L"ocrHotkey", JsonValue::CreateStringValue(m_settings.ocrHotkey));
            root.SetNamedValue(L"compareTranslations", JsonValue::CreateBooleanValue(m_settings.compareTranslations));
            root.SetNamedValue(L"sidebarCollapsed", JsonValue::CreateBooleanValue(m_settings.sidebarCollapsed));
            root.SetNamedValue(L"geminiModel", JsonValue::CreateStringValue(m_settings.geminiModel));
            root.SetNamedValue(L"openAiModel", JsonValue::CreateStringValue(m_settings.openAiModel));

            std::wstring jsonStr = root.Stringify().c_str();
            std::wofstream file(path, std::ios::trunc);
            if (file.is_open())
            {
                file << jsonStr;
            }
        }
        catch (...)
        {
            // Silently handle save failure
        }
    }

    void SettingsManager::Load()
    {
        try
        {
            auto path = GetAppDataPath() / L"settings.json";
            if (!std::filesystem::exists(path))
            {
                return;
            }

            std::wifstream file(path);
            if (!file.is_open())
            {
                return;
            }

            std::wstringstream buffer;
            buffer << file.rdbuf();
            std::wstring jsonStr = buffer.str();
            if (jsonStr.empty()) return;

            JsonObject root;
            if (JsonObject::TryParse(jsonStr, root))
            {
                if (root.HasKey(L"sourceLanguage"))
                    m_settings.sourceLanguage = root.GetNamedString(L"sourceLanguage").c_str();
                if (root.HasKey(L"targetLanguage"))
                    m_settings.targetLanguage = root.GetNamedString(L"targetLanguage").c_str();
                if (root.HasKey(L"primaryService"))
                    m_settings.primaryService = static_cast<int>(root.GetNamedNumber(L"primaryService"));
                if (root.HasKey(L"theme"))
                    m_settings.theme = root.GetNamedString(L"theme").c_str();
                if (root.HasKey(L"autoStart"))
                    m_settings.autoStart = root.GetNamedBoolean(L"autoStart");
                if (root.HasKey(L"globalHotkey"))
                    m_settings.globalHotkey = root.GetNamedString(L"globalHotkey").c_str();
                if (root.HasKey(L"quickHotkey"))
                    m_settings.quickHotkey = root.GetNamedString(L"quickHotkey").c_str();
                if (root.HasKey(L"ocrHotkey"))
                    m_settings.ocrHotkey = root.GetNamedString(L"ocrHotkey").c_str();
                if (root.HasKey(L"compareTranslations"))
                    m_settings.compareTranslations = root.GetNamedBoolean(L"compareTranslations");
                if (root.HasKey(L"sidebarCollapsed"))
                    m_settings.sidebarCollapsed = root.GetNamedBoolean(L"sidebarCollapsed");
                if (root.HasKey(L"geminiModel"))
                    m_settings.geminiModel = root.GetNamedString(L"geminiModel").c_str();
                if (root.HasKey(L"openAiModel"))
                    m_settings.openAiModel = root.GetNamedString(L"openAiModel").c_str();
            }
        }
        catch (...)
        {
            // Fall back to defaults
        }
    }
}
