#include "pch.h"
#include "HistoryManager.h"
#include "SettingsManager.h"
#include <fstream>
#include <sstream>
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Storage
{
    using namespace winrt::Windows::Data::Json;

    HistoryManager& HistoryManager::Instance()
    {
        static HistoryManager instance;
        return instance;
    }

    HistoryManager::HistoryManager()
    {
        Load();
    }

    void HistoryManager::AddItem(HistoryItem const& item)
    {
        // Insert at beginning (most recent first)
        m_items.insert(m_items.begin(), item);
        if (m_items.size() > MaxItems)
        {
            m_items.resize(MaxItems);
        }
        Save();
    }

    static std::wstring ToLowerUnicode(std::wstring const& str)
    {
        if (str.empty()) return L"";
        std::wstring lower(str.length(), L'\0');
        int len = LCMapStringEx(
            LOCALE_NAME_INVARIANT,
            LCMAP_LOWERCASE,
            str.c_str(),
            static_cast<int>(str.length()),
            &lower[0],
            static_cast<int>(lower.length()),
            nullptr, nullptr, 0);
        if (len > 0)
        {
            lower.resize(len);
            return lower;
        }
        lower = str;
        for (auto& c : lower) c = towlower(c);
        return lower;
    }

    std::vector<HistoryItem> HistoryManager::Search(std::wstring const& query) const
    {
        if (query.empty())
        {
            return m_items;
        }

        std::wstring lowerQuery = ToLowerUnicode(query);

        std::vector<std::wstring> tokens;
        std::wstringstream ss(lowerQuery);
        std::wstring tok;
        while (ss >> tok)
        {
            tokens.push_back(tok);
        }

        std::vector<HistoryItem> results;
        for (auto const& item : m_items)
        {
            std::wstring origLower = ToLowerUnicode(item.originalText);
            std::wstring transLower = ToLowerUnicode(item.translatedText);

            if (origLower.find(lowerQuery) != std::wstring::npos ||
                transLower.find(lowerQuery) != std::wstring::npos)
            {
                results.push_back(item);
                continue;
            }

            if (!tokens.empty())
            {
                bool allTokensMatch = true;
                std::wstring combined = origLower + L" " + transLower;
                for (auto const& t : tokens)
                {
                    if (combined.find(t) == std::wstring::npos)
                    {
                        allTokensMatch = false;
                        break;
                    }
                }
                if (allTokensMatch)
                {
                    results.push_back(item);
                }
            }
        }

        return results;
    }

    void HistoryManager::Clear()
    {
        m_items.clear();
        Save();
    }

    void HistoryManager::Save()
    {
        try
        {
            auto path = SettingsManager::GetAppDataPath() / L"history.json";
            JsonArray arr;
            for (auto const& item : m_items)
            {
                JsonObject obj;
                obj.SetNamedValue(L"timestamp", JsonValue::CreateStringValue(item.timestamp));
                obj.SetNamedValue(L"originalText", JsonValue::CreateStringValue(item.originalText));
                obj.SetNamedValue(L"translatedText", JsonValue::CreateStringValue(item.translatedText));
                obj.SetNamedValue(L"sourceLang", JsonValue::CreateStringValue(item.sourceLang));
                obj.SetNamedValue(L"targetLang", JsonValue::CreateStringValue(item.targetLang));
                obj.SetNamedValue(L"service", JsonValue::CreateStringValue(item.service));
                arr.Append(obj);
            }

            std::wstring jsonStr = arr.Stringify().c_str();
            std::wofstream file(path, std::ios::trunc);
            if (file.is_open())
            {
                file << jsonStr;
            }
        }
        catch (...)
        {
        }
    }

    void HistoryManager::Load()
    {
        try
        {
            auto path = SettingsManager::GetAppDataPath() / L"history.json";
            if (!std::filesystem::exists(path)) return;

            std::wifstream file(path);
            if (!file.is_open()) return;

            std::wstringstream buffer;
            buffer << file.rdbuf();
            std::wstring jsonStr = buffer.str();
            if (jsonStr.empty()) return;

            JsonArray arr;
            if (JsonArray::TryParse(jsonStr, arr))
            {
                m_items.clear();
                for (uint32_t i = 0; i < arr.Size(); ++i)
                {
                    auto obj = arr.GetObjectAt(i);
                    HistoryItem item;
                    if (obj.HasKey(L"timestamp")) item.timestamp = obj.GetNamedString(L"timestamp").c_str();
                    if (obj.HasKey(L"originalText")) item.originalText = obj.GetNamedString(L"originalText").c_str();
                    if (obj.HasKey(L"translatedText")) item.translatedText = obj.GetNamedString(L"translatedText").c_str();
                    if (obj.HasKey(L"sourceLang")) item.sourceLang = obj.GetNamedString(L"sourceLang").c_str();
                    if (obj.HasKey(L"targetLang")) item.targetLang = obj.GetNamedString(L"targetLang").c_str();
                    if (obj.HasKey(L"service")) item.service = obj.GetNamedString(L"service").c_str();
                    m_items.push_back(std::move(item));
                }
            }
        }
        catch (...)
        {
        }
    }
}
