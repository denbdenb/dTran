#include "pch.h"
#include "LanguageCatalog.h"
#include <algorithm>

namespace dTranslate::Translation
{
    using namespace ServiceCapability;

    static const std::vector<LanguageInfo> s_languages = {
        // Auto-detect (source only)
        { L"auto", L"Auto-detect", L"Auto-detect", L"🌐", L"", L"", AllCore },

        // Major European & Global Languages
        { L"en", L"English", L"English", L"🇺🇸", L"EN", L"en-US", AllCore | WindowsOcr },
        { L"ru", L"Russian", L"Русский", L"🇷🇺", L"RU", L"ru-RU", AllCore | WindowsOcr },
        { L"es", L"Spanish", L"Español", L"🇪🇸", L"ES", L"es-ES", AllCore | WindowsOcr },
        { L"de", L"German", L"Deutsch", L"🇩🇪", L"DE", L"de-DE", AllCore | WindowsOcr },
        { L"fr", L"French", L"Français", L"🇫🇷", L"FR", L"fr-FR", AllCore | WindowsOcr },
        { L"it", L"Italian", L"Italiano", L"🇮🇹", L"IT", L"it-IT", AllCore | WindowsOcr },
        { L"pt", L"Portuguese", L"Português", L"🇵🇹", L"PT", L"pt-PT", AllCore | WindowsOcr },
        { L"zh", L"Chinese (Simplified)", L"简体中文", L"🇨🇳", L"ZH", L"zh-Hans-CN", AllCore | WindowsOcr },
        { L"zh-TW", L"Chinese (Traditional)", L"繁體中文", L"🇹🇼", L"ZH-TW", L"zh-Hant-TW", Google | WindowsOcr },
        { L"ja", L"Japanese", L"日本語", L"🇯🇵", L"JA", L"ja-JP", AllCore | WindowsOcr },
        { L"ko", L"Korean", L"한국어", L"🇰🇷", L"KO", L"ko-KR", AllCore | WindowsOcr },
        { L"ar", L"Arabic", L"العربية", L"🇸🇦", L"AR", L"ar-SA", AllCore | WindowsOcr },
        { L"tr", L"Turkish", L"Türkçe", L"🇹🇷", L"TR", L"tr-TR", AllCore | WindowsOcr },
        { L"uk", L"Ukrainian", L"Українська", L"🇺🇦", L"UK", L"uk-UA", AllCore | WindowsOcr },
        { L"pl", L"Polish", L"Polski", L"🇵🇱", L"PL", L"pl-PL", AllCore | WindowsOcr },
        { L"nl", L"Dutch", L"Nederlands", L"🇳🇱", L"NL", L"nl-NL", AllCore | WindowsOcr },

        // Nordic & Central/Eastern European
        { L"sv", L"Swedish", L"Svenska", L"🇸🇪", L"SV", L"sv-SE", AllCore | WindowsOcr },
        { L"no", L"Norwegian", L"Norsk", L"🇳🇴", L"NO", L"nb-NO", AllCore | WindowsOcr },
        { L"da", L"Danish", L"Dansk", L"🇩🇰", L"DA", L"da-DK", AllCore | WindowsOcr },
        { L"fi", L"Finnish", L"Suomi", L"🇫🇮", L"FI", L"fi-FI", AllCore | WindowsOcr },
        { L"el", L"Greek", L"Ελληνικά", L"🇬🇷", L"EL", L"el-GR", AllCore | WindowsOcr },
        { L"cs", L"Czech", L"Čeština", L"🇨🇿", L"CS", L"cs-CZ", AllCore | WindowsOcr },
        { L"sk", L"Slovak", L"Slovenčina", L"🇸🇰", L"SK", L"sk-SK", AllCore | WindowsOcr },
        { L"hu", L"Hungarian", L"Magyar", L"🇭🇺", L"HU", L"hu-HU", AllCore | WindowsOcr },
        { L"ro", L"Romanian", L"Română", L"🇷🇴", L"RO", L"ro-RO", AllCore | WindowsOcr },
        { L"bg", L"Bulgarian", L"Български", L"🇧🇬", L"BG", L"bg-BG", AllCore | WindowsOcr },
        { L"sr", L"Serbian", L"Српски", L"🇷🇸", L"SR", L"sr-Cyrl-RS", AllCore | WindowsOcr },
        { L"hr", L"Croatian", L"Hrvatski", L"🇭🇷", L"HR", L"hr-HR", AllCore | WindowsOcr },
        { L"sl", L"Slovenian", L"Slovenščina", L"🇸🇮", L"SL", L"sl-SI", AllCore | WindowsOcr },
        { L"he", L"Hebrew", L"עברית", L"🇮🇱", L"HE", L"he-IL", AllCore | WindowsOcr },

        // Asian & South Asian
        { L"hi", L"Hindi", L"हिन्दी", L"🇮🇳", L"HI", L"hi-IN", AllCore | WindowsOcr },
        { L"bn", L"Bengali", L"বাংলা", L"🇧🇩", L"BN", L"bn-BD", AllCore },
        { L"th", L"Thai", L"ไทย", L"🇹🇭", L"TH", L"th-TH", AllCore | WindowsOcr },
        { L"vi", L"Vietnamese", L"Tiếng Việt", L"🇻🇳", L"VI", L"vi-VN", AllCore | WindowsOcr },
        { L"id", L"Indonesian", L"Bahasa Indonesia", L"🇮🇩", L"ID", L"id-ID", AllCore | WindowsOcr },
        { L"ms", L"Malay", L"Bahasa Melayu", L"🇲🇾", L"MS", L"ms-MY", AllCore },
        { L"fil", L"Filipino", L"Tagalog", L"🇵🇭", L"FIL", L"fil-PH", AllCore },
        { L"fa", L"Persian", L"فارسی", L"🇮🇷", L"FA", L"fa-IR", AllCore },
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
        { L"eo", L"Esperanto", L"Esperanto", L"🟢", L"EO", L"eo", AllCore },
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
        { L"haw", L"Hawaiian", L"ʻŌlelo Hawaiʻi", L"🌺", L"HAW", L"haw-US", Google },
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
            return serviceId == 0 || serviceId == 1;
        }

        auto info = FindByCode(code);
        if (!info) return true; // Assume true if unknown to avoid false blocking

        uint32_t mask = 0;
        switch (serviceId)
        {
        case 0: mask = Google; break;
        case 1: mask = Yandex; break;
        case 2: mask = WindowsOcr; break;
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

    std::wstring LanguageCatalog::DetectLanguage(std::wstring const& text)
    {
        if (text.empty()) return L"en";

        int cyrillicCount = 0;
        int ukrainianSpecific = 0;
        int latinCount = 0;
        int cjkCount = 0;
        int hangulCount = 0;
        int hiraganaKatakanaCount = 0;
        int arabicCount = 0;
        int hebrewCount = 0;
        int greekCount = 0;
        int germanSpecific = 0;
        int frenchSpecific = 0;
        int spanishSpecific = 0;

        for (wchar_t ch : text)
        {
            if ((ch >= 0x0400 && ch <= 0x04FF) || (ch >= 0x0500 && ch <= 0x052F))
            {
                cyrillicCount++;
                if (ch == 0x0456 || ch == 0x0406 || // і, І
                    ch == 0x0457 || ch == 0x0407 || // ї, Ї
                    ch == 0x0454 || ch == 0x0404 || // є, Є
                    ch == 0x0491 || ch == 0x0490)   // ґ, Ґ
                {
                    ukrainianSpecific++;
                }
            }
            else if ((ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || (ch >= 0x00C0 && ch <= 0x024F))
            {
                latinCount++;
                if (ch == L'ä' || ch == L'Ä' || ch == L'ö' || ch == L'Ö' || ch == L'ü' || ch == L'Ü' || ch == L'ß')
                {
                    germanSpecific += 3;
                }
                else if (ch == L'ç' || ch == L'œ' || ch == L'æ' || ch == L'è' || ch == L'ê' || ch == L'ë' || ch == L'à' || ch == L'â' || ch == L'î' || ch == L'ô' || ch == L'û' || ch == L'ù')
                {
                    frenchSpecific += 3;
                }
                else if (ch == L'ñ' || ch == L'Ñ')
                {
                    spanishSpecific += 4;
                }
            }
            else if (ch == 0x00BF || ch == 0x00A1 || ch == L'¿' || ch == L'¡') // ¿, ¡
            {
                spanishSpecific += 5;
            }
            else if (ch >= 0x4E00 && ch <= 0x9FFF)
            {
                cjkCount++;
            }
            else if ((ch >= 0x3040 && ch <= 0x309F) || (ch >= 0x30A0 && ch <= 0x30FF))
            {
                hiraganaKatakanaCount++;
            }
            else if ((ch >= 0xAC00 && ch <= 0xD7AF) || (ch >= 0x1100 && ch <= 0x11FF))
            {
                hangulCount++;
            }
            else if (ch >= 0x0600 && ch <= 0x06FF)
            {
                arabicCount++;
            }
            else if (ch >= 0x0590 && ch <= 0x05FF)
            {
                hebrewCount++;
            }
            else if (ch >= 0x0370 && ch <= 0x03FF)
            {
                greekCount++;
            }
        }

        if (cyrillicCount > latinCount && cyrillicCount > 0)
        {
            return (ukrainianSpecific > 0) ? L"uk" : L"ru";
        }
        if (hiraganaKatakanaCount > 0) return L"ja";
        if (hangulCount > 0) return L"ko";
        if (cjkCount > 0) return L"zh";
        if (arabicCount > 0) return L"ar";
        if (hebrewCount > 0) return L"he";
        if (greekCount > 0) return L"el";

        // Extract word tokens from text
        std::vector<std::wstring> tokens;
        std::wstring currentToken;
        for (wchar_t ch : text)
        {
            if (iswalpha(ch))
            {
                currentToken += towlower(ch);
            }
            else
            {
                if (!currentToken.empty())
                {
                    tokens.push_back(currentToken);
                    currentToken.clear();
                }
            }
        }
        if (!currentToken.empty())
        {
            tokens.push_back(currentToken);
        }

        int deScore = germanSpecific;
        int frScore = frenchSpecific;
        int esScore = spanishSpecific;

        for (auto const& w : tokens)
        {
            // German words
            if (w == L"der" || w == L"die" || w == L"das" || w == L"den" || w == L"dem" || w == L"des" ||
                w == L"ein" || w == L"eine" || w == L"einer" || w == L"und" || w == L"ist" || w == L"sind" ||
                w == L"nicht" || w == L"ich" || w == L"du" || w == L"er" || w == L"sie" || w == L"es" ||
                w == L"wir" || w == L"ihr" || w == L"wie" || w == L"geht" || w == L"ihnen" || w == L"guten" ||
                w == L"morgen" || w == L"tag" || w == L"abend" || w == L"danke" || w == L"bitte" ||
                w == L"auf" || w == L"mit" || w == L"haben" || w == L"sein")
            {
                deScore += 3;
            }

            // French words
            if (w == L"le" || w == L"la" || w == L"les" || w == L"un" || w == L"une" || w == L"des" ||
                w == L"et" || w == L"est" || w == L"sont" || w == L"pas" || w == L"je" || w == L"tu" ||
                w == L"il" || w == L"elle" || w == L"nous" || w == L"vous" || w == L"bonjour" || w == L"merci" ||
                w == L"comment" || w == L"allez" || w == L"avec" || w == L"pour" || w == L"dans" ||
                w == L"sur" || w == L"qui" || w == L"que")
            {
                frScore += 3;
            }

            // Spanish words
            if (w == L"el" || w == L"la" || w == L"los" || w == L"las" || w == L"un" || w == L"una" ||
                w == L"unos" || w == L"unas" || w == L"y" || w == L"es" || w == L"son" || w == L"no" ||
                w == L"yo" || w == L"tú" || w == L"él" || w == L"ella" || w == L"nosotros" || w == L"vosotros" ||
                w == L"ellos" || w == L"cómo" || w == L"como" || w == L"estás" || w == L"esta" ||
                w == L"hola" || w == L"gracias" || w == L"por" || w == L"para" || w == L"pero" ||
                w == L"qué" || w == L"que" || w == L"bien" || w == L"hoy" || w == L"buenos" || w == L"dias")
            {
                esScore += 3;
            }
        }

        if (deScore > 0 && deScore >= frScore && deScore >= esScore) return L"de";
        if (frScore > 0 && frScore >= esScore) return L"fr";
        if (esScore > 0) return L"es";

        return L"en";
    }
}
