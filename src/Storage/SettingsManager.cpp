#include "pch.h"
#include "SettingsManager.h"
#include <fstream>
#include <sstream>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.ApplicationModel.h>
#include <appmodel.h>
#include <shlobj.h>
#include <windows.h>

namespace dTranslate::Storage
{
    using namespace winrt::Windows::Data::Json;

    static bool IsPackagedProcess()
    {
        UINT32 len = 0;
        return GetCurrentPackageFullName(&len, nullptr) != APPMODEL_ERROR_NO_PACKAGE;
    }

    SettingsManager& SettingsManager::Instance()
    {
        static SettingsManager instance;
        return instance;
    }

    SettingsManager::SettingsManager()
    {
        LANGID langId = GetUserDefaultUILanguage();
        if (PRIMARYLANGID(langId) == LANG_RUSSIAN)
        {
            m_settings.appLanguage = L"ru";
        }
        Load();
        CleanupStaleRegistryEntries();
        // Synchronize in-memory setting with real Windows startup registration
        m_settings.autoStart = IsStartWithWindowsEnabled();
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

    void SettingsManager::CleanupStaleRegistryEntries()
    {
        if (IsPackagedProcess())
        {
            HKEY hKey = nullptr;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
            {
                RegDeleteValueW(hKey, L"dTranslate");
                RegCloseKey(hKey);
            }
        }
    }

    WindowsStartupState SettingsManager::GetStartupTaskState()
    {
        if (IsPackagedProcess())
        {
            try
            {
                auto task = winrt::Windows::ApplicationModel::StartupTask::GetAsync(L"dTranStartupTask").get();
                if (!task)
                {
                    return WindowsStartupState::ErrorOrUnavailable;
                }
                return static_cast<WindowsStartupState>(task.State());
            }
            catch (...)
            {
                return WindowsStartupState::ErrorOrUnavailable;
            }
        }

        // Unpackaged fallback
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
        {
            DWORD type = 0;
            wchar_t val[MAX_PATH] = {};
            DWORD size = sizeof(val);
            LONG res = RegQueryValueExW(hKey, L"dTranslate", nullptr, &type, reinterpret_cast<LPBYTE>(val), &size);
            RegCloseKey(hKey);
            return (res == ERROR_SUCCESS) ? WindowsStartupState::Enabled : WindowsStartupState::Disabled;
        }
        return WindowsStartupState::Disabled;
    }

    bool SettingsManager::IsStartWithWindowsEnabled()
    {
        auto state = GetStartupTaskState();
        return (state == WindowsStartupState::Enabled || state == WindowsStartupState::EnabledByPolicy);
    }

    winrt::Windows::Foundation::IAsyncOperation<uint32_t> SettingsManager::SetStartWithWindowsAsync(bool enable)
    {
        CleanupStaleRegistryEntries();

        if (IsPackagedProcess())
        {
            try
            {
                auto task = co_await winrt::Windows::ApplicationModel::StartupTask::GetAsync(L"dTranStartupTask");
                if (!task)
                {
                    co_return static_cast<uint32_t>(WindowsStartupState::ErrorOrUnavailable);
                }

                if (enable)
                {
                    auto state = task.State();
                    if (state != winrt::Windows::ApplicationModel::StartupTaskState::Enabled &&
                        state != winrt::Windows::ApplicationModel::StartupTaskState::EnabledByPolicy)
                    {
                        state = co_await task.RequestEnableAsync();
                    }
                    co_return static_cast<uint32_t>(state);
                }
                else
                {
                    task.Disable();
                    auto state = task.State();
                    co_return static_cast<uint32_t>(state);
                }
            }
            catch (...)
            {
                co_return static_cast<uint32_t>(WindowsStartupState::ErrorOrUnavailable);
            }
        }

        // Unpackaged fallback
        HKEY hKey = nullptr;
        LONG res = RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0,
            KEY_SET_VALUE | KEY_QUERY_VALUE,
            &hKey);

        if (res != ERROR_SUCCESS)
        {
            co_return static_cast<uint32_t>(WindowsStartupState::ErrorOrUnavailable);
        }

        if (enable)
        {
            wchar_t exePath[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::wstring cmd = L"\"" + std::wstring(exePath) + L"\" --startup";
            res = RegSetValueExW(
                hKey,
                L"dTranslate",
                0,
                REG_SZ,
                reinterpret_cast<const BYTE*>(cmd.c_str()),
                static_cast<DWORD>((cmd.size() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
            co_return (res == ERROR_SUCCESS) ? static_cast<uint32_t>(WindowsStartupState::Enabled) : static_cast<uint32_t>(WindowsStartupState::ErrorOrUnavailable);
        }
        else
        {
            res = RegDeleteValueW(hKey, L"dTranslate");
            RegCloseKey(hKey);
            co_return static_cast<uint32_t>(WindowsStartupState::Disabled);
        }
    }

    bool SettingsManager::SetStartWithWindows(bool enable, WindowsStartupState* outActualState)
    {
        try
        {
            auto op = SetStartWithWindowsAsync(enable);
            auto state = static_cast<WindowsStartupState>(op.get());
            if (outActualState) *outActualState = state;
            bool ok = (state == WindowsStartupState::Enabled || state == WindowsStartupState::EnabledByPolicy ||
                      (!enable && state == WindowsStartupState::Disabled));
            auto& inst = Instance();
            inst.m_settings.autoStart = (state == WindowsStartupState::Enabled || state == WindowsStartupState::EnabledByPolicy);
            inst.Save();
            return ok;
        }
        catch (...)
        {
            if (outActualState) *outActualState = WindowsStartupState::ErrorOrUnavailable;
            return false;
        }
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
            root.SetNamedValue(L"appLanguage", JsonValue::CreateStringValue(m_settings.appLanguage));
            root.SetNamedValue(L"autoStart", JsonValue::CreateBooleanValue(m_settings.autoStart));
            root.SetNamedValue(L"closeToTray", JsonValue::CreateBooleanValue(m_settings.closeToTray));
            root.SetNamedValue(L"translateSelectedHotkey", JsonValue::CreateStringValue(m_settings.translateSelectedHotkey));
            root.SetNamedValue(L"ocrHotkey", JsonValue::CreateStringValue(m_settings.ocrHotkey));
            root.SetNamedValue(L"compareTranslations", JsonValue::CreateBooleanValue(m_settings.compareTranslations));
            root.SetNamedValue(L"quickPopupWidth", JsonValue::CreateNumberValue(m_settings.quickPopupWidth));
            root.SetNamedValue(L"quickPopupHeight", JsonValue::CreateNumberValue(m_settings.quickPopupHeight));

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
                if (root.HasKey(L"appLanguage"))
                    m_settings.appLanguage = root.GetNamedString(L"appLanguage").c_str();
                if (root.HasKey(L"autoStart"))
                    m_settings.autoStart = root.GetNamedBoolean(L"autoStart");
                if (root.HasKey(L"closeToTray"))
                    m_settings.closeToTray = root.GetNamedBoolean(L"closeToTray");
                if (root.HasKey(L"translateSelectedHotkey"))
                    m_settings.translateSelectedHotkey = root.GetNamedString(L"translateSelectedHotkey").c_str();
                else if (root.HasKey(L"globalHotkey"))
                    m_settings.translateSelectedHotkey = root.GetNamedString(L"globalHotkey").c_str();
                if (root.HasKey(L"ocrHotkey"))
                    m_settings.ocrHotkey = root.GetNamedString(L"ocrHotkey").c_str();
                if (root.HasKey(L"compareTranslations"))
                    m_settings.compareTranslations = root.GetNamedBoolean(L"compareTranslations");
                if (root.HasKey(L"quickPopupWidth"))
                    m_settings.quickPopupWidth = static_cast<int>(root.GetNamedNumber(L"quickPopupWidth"));
                if (root.HasKey(L"quickPopupHeight"))
                    m_settings.quickPopupHeight = static_cast<int>(root.GetNamedNumber(L"quickPopupHeight"));
            }
        }
        catch (...)
        {
            // Fall back to defaults
        }
    }
}
