#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace dTranslate::Translation
{
    namespace ServiceCapability
    {
        constexpr uint32_t None        = 0;
        constexpr uint32_t Google      = 1 << 0;
        constexpr uint32_t Yandex      = 1 << 1;
        constexpr uint32_t Gemini      = 1 << 2;
        constexpr uint32_t OpenAI      = 1 << 3;
        constexpr uint32_t Reverso     = 1 << 4;
        constexpr uint32_t Wikipedia   = 1 << 5;
        constexpr uint32_t WindowsOcr  = 1 << 6;
        constexpr uint32_t AllCore     = Google | Yandex | Gemini | OpenAI | Wikipedia;
    }

    struct LanguageInfo
    {
        std::wstring code;        // ISO 639-1 code (e.g. "en", "ru", "auto")
        std::wstring name;        // English name (e.g. "English", "Russian")
        std::wstring nativeName;  // Native name (e.g. "English", "Русский")
        std::wstring flag;        // Country / Regional flag emoji (e.g. "🇺🇸", "🇷🇺")
        std::wstring badge;       // Clean text badge: "EN", "RU"
        std::wstring bcp47;       // Full BCP-47 tag for OCR/Windows API (e.g. "en-US", "ru-RU")
        uint32_t capabilities;    // Capability bitmask

        std::wstring DisplayName() const
        {
            if (code == L"auto")
            {
                return L"🌐 Auto-detect";
            }
            if (!flag.empty())
            {
                if (name != nativeName && !nativeName.empty())
                {
                    return flag + L" " + name + L" (" + nativeName + L")";
                }
                return flag + L" " + name;
            }
            return L"[" + badge + L"] " + name;
        }
    };

    class LanguageCatalog
    {
    public:
        static const std::vector<LanguageInfo>& GetAllLanguages();
        static const LanguageInfo* FindByCode(std::wstring const& code);
        static std::wstring GetDisplayName(std::wstring const& code);
        static bool IsServiceSupported(int serviceId, std::wstring const& code);
        static std::vector<LanguageInfo> GetLanguagesForService(int serviceId, bool isSource);
        static std::wstring ValidateLanguageForService(int serviceId, std::wstring const& code, bool isSource);
        static std::wstring GetBcp47Tag(std::wstring const& code);
        static std::wstring GetReversoCode(std::wstring const& code);
        static int GetLanguageIndex(std::wstring const& code, bool isSource = false);
    };
}
