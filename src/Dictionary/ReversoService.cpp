#include "pch.h"
#include "ReversoService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Dictionary
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;

    ReversoService& ReversoService::Instance()
    {
        static ReversoService instance;
        return instance;
    }

    std::wstring ReversoService::MapLanguageCode(std::wstring const& langCode)
    {
        if (langCode == L"en") return L"eng";
        if (langCode == L"ru") return L"rus";
        if (langCode == L"de") return L"ger";
        if (langCode == L"fr") return L"fra";
        if (langCode == L"es") return L"spa";
        if (langCode == L"it") return L"ita";
        if (langCode == L"zh") return L"chi";
        if (langCode == L"ja") return L"jpn";
        return langCode.empty() ? L"eng" : langCode;
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

        std::wstring fromCode = MapLanguageCode(request.sourceLang == L"auto" ? L"en" : request.sourceLang);
        std::wstring toCode = MapLanguageCode(request.targetLang.empty() ? L"ru" : request.targetLang);

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
            { L"User-Agent", L"dTranslate/1.0 (Windows 11)" }
        };

        result.sourceUrl = L"https://context.reverso.net/translation/" +
            (request.sourceLang == L"auto" ? L"english" : request.sourceLang) + L"-" +
            request.targetLang + L"/" + UrlEncode(request.word);

        auto response = HttpClient::Instance().PostJson(url, jsonBody, headers);

        if (!response.IsSuccess())
        {
            // Even if API endpoint returns status, provide context link
            result.title = request.word;
            result.content = L"View full Reverso dictionary and contextual examples on Reverso Context.";
            result.success = true;
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

                if (resObj.HasKey(L"contextResults"))
                {
                    auto ctxObj = resObj.GetNamedObject(L"contextResults");
                    if (ctxObj.HasKey(L"results"))
                    {
                        auto resultsArr = ctxObj.GetNamedArray(L"results");
                        if (resultsArr.Size() > 0)
                        {
                            formattedContent += L"\nContext Examples:\n";
                            uint32_t count = (std::min)(resultsArr.Size(), 3u);
                            for (uint32_t i = 0; i < count; ++i)
                            {
                                auto item = resultsArr.GetObjectAt(i);
                                if (item.HasKey(L"source") && item.HasKey(L"target"))
                                {
                                    formattedContent += L"— " + std::wstring(item.GetNamedString(L"source").c_str()) + L"\n";
                                    formattedContent += L"  " + std::wstring(item.GetNamedString(L"target").c_str()) + L"\n\n";
                                }
                            }
                        }
                    }
                }

                if (formattedContent.empty())
                {
                    formattedContent = L"No detailed entries found. Check Reverso Context online.";
                }

                result.content = formattedContent;
                result.success = true;
                return result;
            }
        }
        catch (...)
        {
        }

        result.title = request.word;
        result.content = L"Consult Reverso Context online for bilingual examples.";
        result.success = true;
        return result;
    }
}
