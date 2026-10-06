#include "pch.h"
#include "WikipediaService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include "LanguageCatalog.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Dictionary
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;
    using namespace dTranslate::Translation;

    WikipediaService& WikipediaService::Instance()
    {
        static WikipediaService instance;
        return instance;
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

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"User-Agent", L"dTranslate/1.0 (Windows 11 Desktop; contact@dtranslate.local)" }
        };

        // 1. Try direct page summary lookup
        std::wstring encodedTitle = UrlEncode(request.word);
        std::wstring directUrl = L"https://" + lang + L".wikipedia.org/api/rest_v1/page/summary/" + encodedTitle;

        auto response = HttpClient::Instance().Get(directUrl, headers, 12000);

        if (response.IsSuccess() && !response.body.empty())
        {
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

                    if (!result.content.empty())
                    {
                        result.success = true;
                        return result;
                    }
                }
            }
            catch (...) {}
        }

        // 2. Fallback: query MediaWiki search API
        std::wstring searchUrl = L"https://" + lang + L".wikipedia.org/w/api.php?action=query&list=search&srsearch=" +
            encodedTitle + L"&utf8=&format=json";

        auto searchResponse = HttpClient::Instance().Get(searchUrl, headers, 12000);
        if (searchResponse.IsSuccess() && !searchResponse.body.empty())
        {
            try
            {
                std::wstring searchJson = FromUtf8(searchResponse.body);
                JsonObject root;
                if (JsonObject::TryParse(searchJson, root) && root.HasKey(L"query"))
                {
                    auto queryObj = root.GetNamedObject(L"query");
                    if (queryObj.HasKey(L"search"))
                    {
                        auto searchArr = queryObj.GetNamedArray(L"search");
                        if (searchArr.Size() > 0)
                        {
                            auto firstMatch = searchArr.GetObjectAt(0);
                            std::wstring foundTitle = firstMatch.GetNamedString(L"title").c_str();

                            // Try getting summary for this matched title
                            std::wstring matchedSummaryUrl = L"https://" + lang + L".wikipedia.org/api/rest_v1/page/summary/" +
                                UrlEncode(foundTitle);
                            auto matchedResp = HttpClient::Instance().Get(matchedSummaryUrl, headers, 8000);
                            if (matchedResp.IsSuccess())
                            {
                                std::wstring mJson = FromUtf8(matchedResp.body);
                                JsonObject mObj;
                                if (JsonObject::TryParse(mJson, mObj) && mObj.HasKey(L"extract"))
                                {
                                    result.title = mObj.GetNamedString(L"title").c_str();
                                    result.content = mObj.GetNamedString(L"extract").c_str();
                                    if (mObj.HasKey(L"description"))
                                    {
                                        std::wstring d = mObj.GetNamedString(L"description").c_str();
                                        if (!d.empty()) result.title += L" (" + d + L")";
                                    }
                                    if (mObj.HasKey(L"content_urls"))
                                    {
                                        auto u = mObj.GetNamedObject(L"content_urls");
                                        if (u.HasKey(L"desktop"))
                                        {
                                            result.sourceUrl = u.GetNamedObject(L"desktop").GetNamedString(L"page").c_str();
                                        }
                                    }
                                    result.success = true;
                                    return result;
                                }
                            }

                            // If summary call fails, use search snippet directly
                            result.title = foundTitle;
                            std::wstring snippet = StripHtmlTags(firstMatch.GetNamedString(L"snippet").c_str());
                            result.content = snippet + L"...";
                            result.sourceUrl = L"https://" + lang + L".wikipedia.org/wiki/" + UrlEncode(foundTitle);
                            result.success = true;
                            return result;
                        }
                    }
                }
            }
            catch (...) {}
        }

        result.success = false;
        result.errorMessage = L"No Wikipedia article found for \"" + request.word +
            L"\" in [" + LanguageCatalog::GetDisplayName(lang) + L"].";
        return result;
    }
}
