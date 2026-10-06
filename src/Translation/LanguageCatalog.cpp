#include "pch.h"
#include "LanguageCatalog.h"
#include <algorithm>

namespace dTranslate::Translation
{
    using namespace ServiceCapability;

    static const std::vector<LanguageInfo> s_languages = {
        // Auto-detect (source only)
        { L"auto", L"Auto-detect", L"Auto-detect", L"🌐", L"", L"", Google | Yandex | Gemini | OpenAI },

        // Major European & Global Languages
        { L"en", L"English", L"English", L"🇺🇸", L"EN", L"en-US", AllCore | Reverso | WindowsOcr },
        { L"ru", L"Russian", L"Русский", L"🇷🇺", L"RU", L"ru-RU", AllCore | Reverso | WindowsOcr },
        { L"es", L"Spanish", L"Español", L"🇪🇸", L"ES", L"es-ES", AllCore | Reverso | WindowsOcr },
        { L"de", L"German", L"Deutsch", L"🇩🇪", L"DE", L"de-DE", AllCore | Reverso | WindowsOcr },
        { L"fr", L"French", L"Français", L"🇫🇷", L"FR", L"fr-FR", AllCore | Reverso | WindowsOcr },
        { L"it", L"Italian", L"Italiano", L"🇮🇹", L"IT", L"it-IT", AllCore | Reverso | WindowsOcr },
        { L"pt", L"Portuguese", L"Português", L"🇵🇹", L"PT", L"pt-PT", AllCore | Reverso | WindowsOcr },
        { L"zh", L"Chinese (Simplified)", L"简体中文", L"🇨🇳", L"ZH", L"zh-Hans-CN", AllCore | Reverso | WindowsOcr },
        { L"zh-TW", L"Chinese (Traditional)", L"繁體中文", L"🇹🇼", L"ZH-TW", L"zh-Hant-TW", Google | Gemini | OpenAI | Wikipedia | WindowsOcr },
        { L"ja", L"Japanese", L"日本語", L"🇯🇵", L"JA", L"ja-JP", AllCore | Reverso | WindowsOcr },
        { L"ko", L"Korean", L"한국어", L"🇰🇷", L"KO", L"ko-KR", AllCore | WindowsOcr },
        { L"ar", L"Arabic", L"العربية", L"🇸🇦", L"AR", L"ar-SA", AllCore | Reverso | WindowsOcr },
        { L"tr", L"Turkish", L"Türkçe", L"🇹🇷", L"TR", L"tr-TR", AllCore | Reverso | WindowsOcr },
        { L"uk", L"Ukrainian", L"Українська", L"🇺🇦", L"UK", L"uk-UA", AllCore | Reverso | WindowsOcr },
        { L"pl", L"Polish", L"Polski", L"🇵🇱", L"PL", L"pl-PL", AllCore | Reverso | WindowsOcr },
        { L"nl", L"Dutch", L"Nederlands", L"🇳🇱", L"NL", L"nl-NL", AllCore | Reverso | WindowsOcr },

        // Nordic & Central/Eastern European
        { L"sv", L"Swedish", L"Svenska", L"🇸🇪", L"SV", L"sv-SE", AllCore | Reverso | WindowsOcr },
        { L"no", L"Norwegian", L"Norsk", L"🇳🇴", L"NO", L"nb-NO", AllCore | WindowsOcr },
        { L"da", L"Danish", L"Dansk", L"🇩🇰", L"DA", L"da-DK", AllCore | WindowsOcr },
        { L"fi", L"Finnish", L"Suomi", L"🇫🇮", L"FI", L"fi-FI", AllCore | WindowsOcr },
        { L"el", L"Greek", L"Ελληνικά", L"🇬🇷", L"EL", L"el-GR", AllCore | WindowsOcr },
        { L"cs", L"Czech", L"Čeština", L"🇨🇿", L"CS", L"cs-CZ", AllCore | WindowsOcr },
        { L"sk", L"Slovak", L"Slovenčina", L"🇸🇰", L"SK", L"sk-SK", AllCore | WindowsOcr },
        { L"hu", L"Hungarian", L"Magyar", L"🇭🇺", L"HU", L"hu-HU", AllCore | WindowsOcr },
        { L"ro", L"Romanian", L"Română", L"🇷🇴", L"RO", L"ro-RO", AllCore | Reverso | WindowsOcr },
        { L"bg", L"Bulgarian", L"Български", L"🇧🇬", L"BG", L"bg-BG", AllCore | WindowsOcr },
        { L"sr", L"Serbian", L"Српски", L"🇷🇸", L"SR", L"sr-Cyrl-RS", AllCore | WindowsOcr },
        { L"hr", L"Croatian", L"Hrvatski", L"🇭🇷", L"HR", L"hr-HR", AllCore | WindowsOcr },
        { L"sl", L"Slovenian", L"Slovenščina", L"🇸🇮", L"SL", L"sl-SI", AllCore | WindowsOcr },
        { L"he", L"Hebrew", L"עברית", L"🇮🇱", L"HE", L"he-IL", AllCore | Reverso | WindowsOcr },

        // Asian & South Asian
        { L"hi", L"Hindi", L"हिन्दी", L"🇮🇳", L"HI", L"hi-IN", AllCore | WindowsOcr },
        { L"bn", L"Bengali", L"বাংলা", L"🇧🇩", L"BN", L"bn-BD", AllCore },
        { L"th", L"Thai", L"ไทย", L"🇹🇭", L"TH", L"th-TH", AllCore | WindowsOcr },
        { L"vi", L"Vietnamese", L"Tiếng Việt", L"🇻🇳", L"VI", L"vi-VN", AllCore | WindowsOcr },
        { L"id", L"Indonesian", L"Bahasa Indonesia", L"🇮🇩", L"ID", L"id-ID", AllCore | WindowsOcr },
        { L"ms", L"Malay", L"Bahasa Melayu", L"🇲🇾", L"MS", L"ms-MY", AllCore },
        { L"fil", L"Filipino", L"Tagalog", L"🇵🇭", L"FIL", L"fil-PH", AllCore },
        { L"fa", L"Persian", L"فارسی", L"🇮🇷", L"FA", L"fa-IR", AllCore | Reverso },
        { L"ur", L"Urdu", L"اردو", L"🇵🇰", L"UR", L"ur-PK", AllCore },
        { L"ta", L"Tamil", L"தமிழ்", L"🇮🇳", L"TA", L"ta-IN", AllCore },
        { L"te", L"Telugu", L"తెలుగు", L"🇮🇳", L"TE", L"te-IN", AllCore },
        { L"kn", L"Kannada", L"ಕನ್ನಡ", L"🇮🇳", L"KN", L"kn-IN", AllCore },
        { L"ml", L"Malayalam", L"മലയാളം", L"🇮🇳", L"ML", L"ml-IN", AllCore },
        { L"gu", L"Gujarati", L"ગુજરાતી", L"🇮🇳", L"GU", L"gu-IN", AllCore },
        { L"mr", L"Marathi", L"मराठी", L"🇮🇳", L"MR", L"mr-IN", AllCore },
        { L"pa", L"Punjabi", L"ਪੰਜਾਬੀ", L"🇮🇳", L"PA", L"pa-IN", AllCore },
        { L"ne", L"Nepali", L"नेपाली", L"🇳🇵", L"NE", L"ne-NP", AllCore },
        { L"si", L"Sinhala", L"සිංහල", L"🇱🇰", L"SI", L"si-LK", AllCore },
        { L"my", L"Burmese", L"မြန်မာဘာသာ", L"🇲🇲", L"MY", L"my-MM", AllCore },
        { L"km", L"Khmer", L"ភាសាខ្មែរ", L"🇰🇭", L"KM", L"km-KH", AllCore },
        { L"lo", L"Lao", L"ພາສາລາວ", L"🇱🇦", L"LO", L"lo-LA", AllCore },
        { L"mn", L"Mongolian", L"Монгол", L"🇲🇳", L"MN", L"mn-MN", AllCore },

        // Caucasus & Central Asia
        { L"ka", L"Georgian", L"ქართული", L"🇬🇪", L"KA", L"ka-GE", AllCore },
        { L"hy", L"Armenian", L"Հայերեն", L"🇦🇲", L"HY", L"hy-AM", AllCore },
        { L"az", L"Azerbaijani", L"Azərbaycan", L"🇦🇿", L"AZ", L"az-AZ", AllCore },
        { L"kk", L"Kazakh", L"Қазақша", L"🇰🇿", L"KK", L"kk-KZ", AllCore },
        { L"uz", L"Uzbek", L"Oʻzbekcha", L"🇺🇿", L"UZ", L"uz-UZ", AllCore },
        { L"tg", L"Tajik", L"Тоҷикӣ", L"🇹🇯", L"TG", L"tg-TJ", AllCore },
        { L"ky", L"Kyrgyz", L"Кыргызча", L"🇰🇬", L"KY", L"ky-KG", AllCore },
        { L"tk", L"Turkmen", L"Türkmen", L"🇹🇲", L"TK", L"tk-TM", AllCore },

        // Baltic & Eastern Europe
        { L"be", L"Belarusian", L"Беларуская", L"🇧🇾", L"BE", L"be-BY", AllCore },
        { L"lt", L"Lithuanian", L"Lietuvių", L"🇱🇹", L"LT", L"lt-LT", AllCore },
        { L"lv", L"Latvian", L"Latviešu", L"🇱🇻", L"LV", L"lv-LV", AllCore },
        { L"et", L"Estonian", L"Eesti", L"🇪🇪", L"ET", L"et-EE", AllCore },
        { L"sq", L"Albanian", L"Shqip", L"🇦🇱", L"SQ", L"sq-AL", AllCore },
        { L"mk", L"Macedonian", L"Македонски", L"🇲🇰", L"MK", L"mk-MK", AllCore },
        { L"bs", L"Bosnian", L"Bosanski", L"🇧🇦", L"BS", L"bs-BA", AllCore },
        { L"is", L"Icelandic", L"Íslenska", L"🇮🇸", L"IS", L"is-IS", AllCore },
        { L"ga", L"Irish", L"Gaeilge", L"🇮🇪", L"GA", L"ga-IE", AllCore },
        { L"cy", L"Welsh", L"Cymraeg", L"🏴", L"CY", L"cy-GB", AllCore },
        { L"eu", L"Basque", L"Euskara", L"🇪🇸", L"EU", L"eu-ES", AllCore },
        { L"ca", L"Catalan", L"Català", L"🇪🇸", L"CA", L"ca-ES", AllCore },
        { L"gl", L"Galician", L"Galego", L"🇪🇸", L"GL", L"gl-ES", AllCore },
        { L"mt", L"Maltese", L"Malti", L"🇲🇹", L"MT", L"mt-MT", AllCore },
        { L"eo", L"Esperanto", L"Esperanto", L"🟢", L"EO", L"eo", Google | Yandex | Gemini | OpenAI | Wikipedia },
        { L"la", L"Latin", L"Latina", L"🏛️", L"LA", L"la", AllCore },

        // African Languages
        { L"af", L"Afrikaans", L"Afrikaans", L"🇿🇦", L"AF", L"af-ZA", AllCore },
        { L"sw", L"Swahili", L"Kiswahili", L"🇰🇪", L"SW", L"sw-KE", AllCore },
        { L"zu", L"Zulu", L"isiZulu", L"🇿🇦", L"ZU", L"zu-ZA", AllCore },
        { L"am", L"Amharic", L"አማርኛ", L"🇪🇹", L"AM", L"am-ET", AllCore },
        { L"so", L"Somali", L"Soomaali", L"🇸🇴", L"SO", L"so-SO", AllCore },
        { L"yo", L"Yoruba", L"Èdè Yorùbá", L"🇳🇬", L"YO", L"yo-NG", AllCore },
        { L"ig", L"Igbo", L"Asụsụ Igbo", L"🇳🇬", L"IG", L"ig-NG", AllCore },
        { L"ha", L"Hausa", L"Hausa", L"🇳🇬", L"HA", L"ha-NG", AllCore },
        { L"mg", L"Malagasy", L"Malagasy", L"🇲🇬", L"MG", L"mg-MG", AllCore },

        // Additional Worldwide Languages
        { L"ku", L"Kurdish", L"Kurdî", L"☀️", L"KU", L"ku-TR", AllCore },
        { L"ps", L"Pashto", L"پښتو", L"🇦🇫", L"PS", L"ps-AF", AllCore },
        { L"yi", L"Yiddish", L"ייִדיש", L"✡️", L"YI", L"yi", AllCore },
        { L"lb", L"Luxembourgish", L"Lëtzebuergesch", L"🇱🇺", L"LB", L"lb-LU", AllCore },
        { L"mi", L"Maori", L"Māori", L"🇳🇿", L"MI", L"mi-NZ", AllCore },
        { L"sm", L"Samoan", L"Gagana Sāmoa", L"🇼🇸", L"SM", L"sm-WS", AllCore },
        { L"haw", L"Hawaiian", L"ʻŌlelo Hawaiʻi", L"🌺", L"HAW", L"haw-US", Google | Gemini | OpenAI | Wikipedia },
        { L"jv", L"Javanese", L"Basa Jawa", L"🇮🇩", L"JV", L"jv-ID", AllCore },
        { L"su", L"Sundanese", L"Basa Sunda", L"🇮🇩", L"SU", L"su-ID", AllCore }
    };

    const std::vector<LanguageInfo>& LanguageCatalog::GetAllLanguages()
    {
        return s_languages;
    }

    const LanguageInfo* LanguageCatalog::FindByCode(std::wstring const& code)
    {
        for (auto const& item : s_languages)
        {
            if (_wcsicmp(item.code.c_str(), code.c_str()) == 0)
            {
                return &item;
            }
        }
        return nullptr;
    }

    std::wstring LanguageCatalog::GetDisplayName(std::wstring const& code)
    {
        auto info = FindByCode(code);
        if (info)
        {
            return info->DisplayName();
        }
        return code;
    }

    bool LanguageCatalog::IsServiceSupported(int serviceId, std::wstring const& code)
    {
        if (code == L"auto")
        {
            // Auto is supported as source only for core translation engines (0..3)
            return serviceId >= 0 && serviceId <= 3;
        }

        auto info = FindByCode(code);
        if (!info) return true; // Assume true if unknown to avoid false blocking

        uint32_t mask = 0;
        switch (serviceId)
        {
        case 0: mask = Google; break;
        case 1: mask = Yandex; break;
        case 2: mask = Gemini; break;
        case 3: mask = OpenAI; break;
        case 4: mask = Reverso; break;
        case 5: mask = Wikipedia; break;
        case 6: mask = WindowsOcr; break;
        default: return true;
        }

        return (info->capabilities & mask) != 0;
    }

    std::vector<LanguageInfo> LanguageCatalog::GetLanguagesForService(int serviceId, bool isSource)
    {
        std::vector<LanguageInfo> result;
        for (auto const& lang : s_languages)
        {
            if (lang.code == L"auto")
            {
                if (isSource && IsServiceSupported(serviceId, lang.code))
                {
                    result.push_back(lang);
                }
                continue;
            }

            if (IsServiceSupported(serviceId, lang.code))
            {
                result.push_back(lang);
            }
        }
        return result;
    }

    std::wstring LanguageCatalog::ValidateLanguageForService(int serviceId, std::wstring const& code, bool isSource)
    {
        if (isSource && code == L"auto" && IsServiceSupported(serviceId, L"auto"))
        {
            return L"auto";
        }

        if (IsServiceSupported(serviceId, code) && (code != L"auto" || isSource))
        {
            return code;
        }

        // Fallback default
        if (isSource)
        {
            return IsServiceSupported(serviceId, L"auto") ? L"auto" : L"en";
        }
        else
        {
            return IsServiceSupported(serviceId, L"ru") ? L"ru" : L"en";
        }
    }

    std::wstring LanguageCatalog::GetBcp47Tag(std::wstring const& code)
    {
        auto info = FindByCode(code);
        if (info && !info->bcp47.empty())
        {
            return info->bcp47;
        }
        if (code == L"ru") return L"ru-RU";
        if (code == L"en") return L"en-US";
        return code;
    }

    std::wstring LanguageCatalog::GetReversoCode(std::wstring const& code)
    {
        if (code == L"en") return L"eng";
        if (code == L"ru") return L"rus";
        if (code == L"de") return L"ger";
        if (code == L"fr") return L"fra";
        if (code == L"es") return L"spa";
        if (code == L"it") return L"ita";
        if (code == L"pt") return L"por";
        if (code == L"pl") return L"pol";
        if (code == L"nl") return L"dut";
        if (code == L"he") return L"heb";
        if (code == L"ar") return L"ara";
        if (code == L"ja") return L"jpn";
        if (code == L"zh") return L"chi";
        if (code == L"uk") return L"ukr";
        if (code == L"ro") return L"rum";
        if (code == L"tr") return L"tur";
        if (code == L"sv") return L"swe";
        if (code == L"fa") return L"per";
        return code.empty() ? L"eng" : code;
    }

    int LanguageCatalog::GetLanguageIndex(std::wstring const& code, bool isSource)
    {
        int startIndex = isSource ? 0 : 1; // 0 is "auto", target starts from 1
        for (int i = startIndex; i < static_cast<int>(s_languages.size()); ++i)
        {
            if (_wcsicmp(s_languages[i].code.c_str(), code.c_str()) == 0)
            {
                return isSource ? i : (i - 1);
            }
        }
        return 0;
    }
}
