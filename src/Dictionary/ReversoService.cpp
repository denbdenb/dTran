#include "pch.h"
#include "ReversoService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include "LanguageCatalog.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Dictionary
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;
    using namespace dTranslate::Translation;

    ReversoService& ReversoService::Instance()
    {
        static ReversoService instance;
        return instance;
    }

    std::wstring ReversoService::MapLanguageCode(std::wstring const& langCode)
    {
        return LanguageCatalog::GetReversoCode(langCode);
    }

    static std::wstring StripHtmlTags(std::wstring const& text)
    {
        std::wstring result;
        result.reserve(text.size());
        bool insideTag = false;
        for (wchar_t ch : text)
        {
            if (ch == L'<')
            {
                insideTag = true;
            }
            else if (ch == L'>')
            {
                insideTag = false;
            }
            else if (!insideTag)
            {
                result += ch;
            }
        }
        return result;
    }

    DictionaryResult ReversoService::Lookup(DictionaryRequest const& request)
    {
        DictionaryResult result;
        result.serviceName = L"Reverso Context";
        result.query = request.word;

        if (request.word.empty())
        {
            result.success = true;
            return result;
        }

        std::wstring srcLang = (request.sourceLang.empty() || request.sourceLang == L"auto") ? L"en" : request.sourceLang;
        std::wstring tgtLang = request.targetLang.empty() ? L"ru" : request.targetLang;

        if (!LanguageCatalog::IsServiceSupported(4, srcLang) || !LanguageCatalog::IsServiceSupported(4, tgtLang))
        {
            result.success = false;
            result.errorMessage = L"Reverso Context does not support " +
                LanguageCatalog::GetDisplayName(srcLang) + L" ➔ " +
                LanguageCatalog::GetDisplayName(tgtLang) + L". Please try Wikipedia or standard translation.";
            return result;
        }

        std::wstring fromCode = MapLanguageCode(srcLang);
        std::wstring toCode = MapLanguageCode(tgtLang);

        std::wstring url = L"https://api.reverso.net/translate/v1/translation";

        JsonObject root;
        root.SetNamedValue(L"format", JsonValue::CreateStringValue(L"text"));
        root.SetNamedValue(L"from", JsonValue::CreateStringValue(fromCode));
        root.SetNamedValue(L"to", JsonValue::CreateStringValue(toCode));
        root.SetNamedValue(L"input", JsonValue::CreateStringValue(request.word));

        JsonObject options;
        options.SetNamedValue(L"sentenceSplitter", JsonValue::CreateBooleanValue(true));
        options.SetNamedValue(L"origin", JsonValue::CreateStringValue(L"translation.web"));
        options.SetNamedValue(L"contextResults", JsonValue::CreateBooleanValue(true));
        options.SetNamedValue(L"languageDetection", JsonValue::CreateBooleanValue(true));
        root.SetNamedValue(L"options", options);

        std::string jsonBody = ToUtf8(root.Stringify().c_str());

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"User-Agent", L"dTranslate/1.0 (Windows 11 Desktop)" }
        };

        result.sourceUrl = L"https://context.reverso.net/translation/" +
            srcLang + L"-" + tgtLang + L"/" + UrlEncode(request.word);

        auto response = HttpClient::Instance().PostJson(url, jsonBody, headers, 12000);

        if (!response.IsSuccess() || response.body.empty())
        {
            result.success = false;
            result.errorMessage = response.errorMessage.empty() ?
                (L"Reverso service unavailable (HTTP " + std::to_wstring(response.statusCode) + L").") :
                response.errorMessage;
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj))
            {
                result.title = request.word;
                std::wstring formattedContent;

                // 1. Translations / Dictionary entries
                if (resObj.HasKey(L"translation"))
                {
                    auto transArr = resObj.GetNamedArray(L"translation");
                    if (transArr.Size() > 0)
                    {
                        formattedContent += L"Translations:\n";
                        for (uint32_t i = 0; i < transArr.Size(); ++i)
                        {
                            formattedContent += L"• " + std::wstring(transArr.GetStringAt(i).c_str()) + L"\n";
                        }
                    }
                }

                // 2. Dictionary entries with parts of speech if present
                if (resObj.HasKey(L"dictionary_entry_list"))
                {
                    auto dictArr = resObj.GetNamedArray(L"dictionary_entry_list");
                    if (dictArr.Size() > 0 && !resObj.HasKey(L"translation"))
                    {
                        formattedContent += L"Dictionary Entries:\n";
                        for (uint32_t i = 0; i < dictArr.Size(); ++i)
                        {
                            auto entry = dictArr.GetObjectAt(i);
                            std::wstring term = entry.HasKey(L"term") ? entry.GetNamedString(L"term").c_str() : L"";
                            std::wstring pos = entry.HasKey(L"pos") ? (L" (" + std::wstring(entry.GetNamedString(L"pos").c_str()) + L")") : L"";
                            if (!term.empty())
                            {
                                formattedContent += L"• " + term + pos + L"\n";
                            }
                        }
                    }
                }

                // 3. Bilingual Context Examples
                if (resObj.HasKey(L"contextResults"))
                {
                    auto ctxObj = resObj.GetNamedObject(L"contextResults");
                    if (ctxObj.HasKey(L"results"))
                    {
                        auto resultsArr = ctxObj.GetNamedArray(L"results");
                        if (resultsArr.Size() > 0)
                        {
                            formattedContent += L"\nBilingual Context Examples:\n";
                            uint32_t count = (std::min)(resultsArr.Size(), 6u);
                            for (uint32_t i = 0; i < count; ++i)
                            {
                                auto item = resultsArr.GetObjectAt(i);
                                if (item.HasKey(L"source") && item.HasKey(L"target"))
                                {
                                    std::wstring cleanSource = StripHtmlTags(item.GetNamedString(L"source").c_str());
                                    std::wstring cleanTarget = StripHtmlTags(item.GetNamedString(L"target").c_str());
                                    std::wstring pos;
                                    if (item.HasKey(L"pos"))
                                    {
                                        pos = L" [" + std::wstring(item.GetNamedString(L"pos").c_str()) + L"]";
                                    }
                                    formattedContent += std::to_wstring(i + 1) + L". " + cleanSource + pos + L"\n";
                                    formattedContent += L"   ➔ " + cleanTarget + L"\n\n";
                                }
                            }
                        }
                    }
                }

                if (formattedContent.empty())
                {
                    formattedContent = L"No bilingual context entries found for \"" + request.word + L"\".";
                }

                result.content = formattedContent;
                result.success = true;
                return result;
            }
        }
        catch (...)
        {
        }

        result.success = false;
        result.errorMessage = L"Unable to parse Reverso dictionary response.";
        return result;
    }
}
