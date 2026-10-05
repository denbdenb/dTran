#include "pch.h"
#include "WikipediaService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Dictionary
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;

    WikipediaService& WikipediaService::Instance()
    {
        static WikipediaService instance;
        return instance;
    }

    DictionaryResult WikipediaService::Lookup(DictionaryRequest const& request)
    {
        DictionaryResult result;
        result.serviceName = L"Wikipedia Reference";
        result.query = request.word;

        if (request.word.empty())
        {
            result.success = true;
            return result;
        }

        std::wstring lang = request.sourceLang;
        if (lang.empty() || lang == L"auto")
        {
            lang = L"en";
        }

        std::wstring encodedTitle = UrlEncode(request.word);
        std::wstring url = L"https://" + lang + L".wikipedia.org/api/rest_v1/page/summary/" + encodedTitle;

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"User-Agent", L"dTranslate/1.0 (Windows 11 Desktop; contact@dtranslate.local)" }
        };

        auto response = HttpClient::Instance().Get(url, headers);

        if (!response.IsSuccess())
        {
            if (response.statusCode == 404)
            {
                result.success = false;
                result.errorMessage = L"No Wikipedia article found for \"" + request.word + L"\".";
            }
            else
            {
                result.success = false;
                result.errorMessage = response.errorMessage.empty() ?
                    (L"Wikipedia API error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            }
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj))
            {
                if (resObj.HasKey(L"title"))
                {
                    result.title = resObj.GetNamedString(L"title").c_str();
                }

                if (resObj.HasKey(L"extract"))
                {
                    result.content = resObj.GetNamedString(L"extract").c_str();
                }

                if (resObj.HasKey(L"description"))
                {
                    std::wstring desc = resObj.GetNamedString(L"description").c_str();
                    if (!desc.empty())
                    {
                        result.title += L" (" + desc + L")";
                    }
                }

                if (resObj.HasKey(L"content_urls"))
                {
                    auto urls = resObj.GetNamedObject(L"content_urls");
                    if (urls.HasKey(L"desktop"))
                    {
                        auto desktop = urls.GetNamedObject(L"desktop");
                        if (desktop.HasKey(L"page"))
                        {
                            result.sourceUrl = desktop.GetNamedString(L"page").c_str();
                        }
                    }
                }

                result.success = true;
                return result;
            }
            result.success = false;
            result.errorMessage = L"Unable to parse Wikipedia article response.";
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Error reading Wikipedia response.";
        }

        return result;
    }
}
