#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace dTranslate::Storage
{
    struct HistoryItem
    {
        std::wstring timestamp;
        std::wstring originalText;
        std::wstring translatedText;
        std::wstring sourceLang;
        std::wstring targetLang;
        std::wstring service;
    };

    class HistoryManager
    {
    public:
        static HistoryManager& Instance();

        std::vector<HistoryItem> const& GetItems() const { return m_items; }
        void AddItem(HistoryItem const& item);
        void Clear();
        void Save();
        void Load();

    private:
        HistoryManager();
        static constexpr size_t MaxItems = 50;
        std::vector<HistoryItem> m_items;
    };
}
