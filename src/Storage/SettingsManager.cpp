#include "pch.h"
#include "SettingsManager.h"
#include <fstream>
#include <sstream>
#include <winrt/Windows.Data.Json.h>
#include <shlobj.h>
#include <windows.h>

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

    bool SettingsManager::SetStartWithWindows(bool enable)
    {
        HKEY hKey = nullptr;
        LONG res = RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            KEY_SET_VALUE | KEY_QUERY_VALUE,
            &hKey);

        if (res != ERROR_SUCCESS) return false;

        if (enable)
        {
            wchar_t exePath[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exePath) + L"\"";
            res = RegSetValueExW(
                hKey,
                L"dTranslate",
                0,
                REG_SZ,
                reinterpret_cast<const BYTE*>(cmd.c_str()),
                static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t)));
        }
        else
        {
            res = RegDeleteValueW(hKey, L"dTranslate");
            if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
        }

        RegCloseKey(hKey);
        return res == ERROR_SUCCESS;
    }

    void SettingsManager::RegisterObserver(uintptr_t key, SettingsObserver observer)
    {
        m_observers[key] = observer;
    }

    void SettingsManager::UnregisterObserver(uintptr_t key)
    {
        m_observers.erase(key);
    }

    void SettingsManager::UpdateSettings(AppSettings const& newSettings)
    {
        m_settings = newSettings;
        Save();
        for (auto const& [key, cb] : m_observers)
        {
            if (cb)
            {
                cb(m_settings);
            }
        }
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
            root.SetNamedValue(L"closeToTray", JsonValue::CreateBooleanValue(m_settings.closeToTray));
            root.SetNamedValue(L"globalHotkey", JsonValue::CreateStringValue(m_settings.globalHotkey));
            root.SetNamedValue(L"quickHotkey", JsonValue::CreateStringValue(m_settings.quickHotkey));
            root.SetNamedValue(L"ocrHotkey", JsonValue::CreateStringValue(m_settings.ocrHotkey));
            root.SetNamedValue(L"compareTranslations", JsonValue::CreateBooleanValue(m_settings.compareTranslations));
            root.SetNamedValue(L"sidebarCollapsed", JsonValue::CreateBooleanValue(m_settings.sidebarCollapsed));
            root.SetNamedValue(L"geminiModel", JsonValue::CreateStringValue(m_settings.geminiModel));
            root.SetNamedValue(L"openAiModel", JsonValue::CreateStringValue(m_settings.openAiModel));

            root.SetNamedValue(L"dictSourceLang", JsonValue::CreateStringValue(m_settings.dictSourceLang));
            root.SetNamedValue(L"dictTargetLang", JsonValue::CreateStringValue(m_settings.dictTargetLang));
            root.SetNamedValue(L"dictEngine", JsonValue::CreateNumberValue(m_settings.dictEngine));

            root.SetNamedValue(L"mainWindowWidth", JsonValue::CreateNumberValue(m_settings.mainWindowWidth));
            root.SetNamedValue(L"mainWindowHeight", JsonValue::CreateNumberValue(m_settings.mainWindowHeight));
            root.SetNamedValue(L"quickPopupWidth", JsonValue::CreateNumberValue(m_settings.quickPopupWidth));
            root.SetNamedValue(L"quickPopupHeight", JsonValue::CreateNumberValue(m_settings.quickPopupHeight));
            root.SetNamedValue(L"settingsWindowWidth", JsonValue::CreateNumberValue(m_settings.settingsWindowWidth));
            root.SetNamedValue(L"settingsWindowHeight", JsonValue::CreateNumberValue(m_settings.settingsWindowHeight));

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
                if (root.HasKey(L"closeToTray"))
                    m_settings.closeToTray = root.GetNamedBoolean(L"closeToTray");
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

                if (root.HasKey(L"dictSourceLang"))
                    m_settings.dictSourceLang = root.GetNamedString(L"dictSourceLang").c_str();
                if (root.HasKey(L"dictTargetLang"))
                    m_settings.dictTargetLang = root.GetNamedString(L"dictTargetLang").c_str();
                if (root.HasKey(L"dictEngine"))
                    m_settings.dictEngine = static_cast<int>(root.GetNamedNumber(L"dictEngine"));

                if (root.HasKey(L"mainWindowWidth"))
                    m_settings.mainWindowWidth = static_cast<int>(root.GetNamedNumber(L"mainWindowWidth"));
                if (root.HasKey(L"mainWindowHeight"))
                    m_settings.mainWindowHeight = static_cast<int>(root.GetNamedNumber(L"mainWindowHeight"));
                if (root.HasKey(L"quickPopupWidth"))
                    m_settings.quickPopupWidth = static_cast<int>(root.GetNamedNumber(L"quickPopupWidth"));
                if (root.HasKey(L"quickPopupHeight"))
                    m_settings.quickPopupHeight = static_cast<int>(root.GetNamedNumber(L"quickPopupHeight"));
                if (root.HasKey(L"settingsWindowWidth"))
                    m_settings.settingsWindowWidth = static_cast<int>(root.GetNamedNumber(L"settingsWindowWidth"));
                if (root.HasKey(L"settingsWindowHeight"))
                    m_settings.settingsWindowHeight = static_cast<int>(root.GetNamedNumber(L"settingsWindowHeight"));
            }
        }
        catch (...)
        {
            // Fall back to defaults
        }
    }
}
