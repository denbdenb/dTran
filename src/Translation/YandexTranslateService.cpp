#include "pch.h"
#include "YandexTranslateService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Translation
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;

    YandexTranslateService& YandexTranslateService::Instance()
    {
        static YandexTranslateService instance;
        return instance;
    }

    TranslationResult YandexTranslateService::Translate(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Yandex Translate (Web)";
        result.originalText = request.text;

        if (request.text.empty())
        {
            result.success = true;
            result.translatedText = L"";
            return result;
        }

        // 1. Primary: Yandex Browser Web endpoint (QTranslate reference)
        result = TranslateViaBrowserApi(request);
        if (result.success && !result.translatedText.empty())
        {
            return result;
        }

        // 2. Fallback: Mozhi instance 1
        result = TranslateViaMozhi(request, L"https://mozhi.adminforge.de");
        if (result.success && !result.translatedText.empty())
        {
            return result;
        }

        // 3. Fallback: Mozhi instance 2
        result = TranslateViaMozhi(request, L"https://mozhi.pussthecat.org");
        if (result.success && !result.translatedText.empty())
        {
            return result;
        }

        // 4. Fallback: Trayslate legacy endpoint
        result = TranslateViaTrayslate(request);
        if (result.success && !result.translatedText.empty())
        {
            return result;
        }

        // If all web endpoints failed
        if (result.errorMessage.empty())
        {
            result.errorMessage = L"Yandex Web service is temporarily unavailable. Please try again later.";
        }
        return result;
    }

    TranslationResult YandexTranslateService::TranslateViaBrowserApi(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Yandex Translate (Web)";
        result.originalText = request.text;

        std::wstring targetLang = request.targetLang.empty() ? L"ru" : request.targetLang;
        std::wstring encodedText = UrlEncode(request.text);

        std::string formData = "text=" + ToUtf8(encodedText) +
            "&brandID=int" +
            "&statLang=" + ToUtf8(targetLang) +
            "&targetLang=auto" +
            "&locale=" + ToUtf8(targetLang) +
            "&clid=2270494&disable=serp&use_llm_srv=0";

        std::wstring url = L"https://api.browser.yandex.com/instaserp/translate";

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"Accept", L"application/json" },
            { L"User-Agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/144.0.0.0 YaBrowser/26.3.0.0 Safari/537.36" }
        };

        auto response = HttpClient::Instance().PostForm(url, formData, headers, 8000);

        if (!response.IsSuccess())
        {
            if (response.statusCode == 429)
            {
                result.errorMessage = L"Yandex Web is temporarily rate-limited. Please wait a moment.";
            }
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonArray rootArr;
            if (JsonArray::TryParse(jsonStr, rootArr) && rootArr.Size() > 0)
            {
                auto first = rootArr.GetObjectAt(0);
                if (first.HasKey(L"text"))
                {
                    result.translatedText = first.GetNamedString(L"text").c_str();
                    if (first.HasKey(L"from"))
                    {
                        result.detectedLanguage = first.GetNamedString(L"from").c_str();
                    }
                    result.success = true;
                    return result;
                }
            }
        }
        catch (...)
        {
        }

        return result;
    }

    TranslationResult YandexTranslateService::TranslateViaMozhi(TranslationRequest const& request, std::wstring const& host)
    {
        TranslationResult result;
        result.serviceName = L"Yandex Translate (Web)";
        result.originalText = request.text;

        std::wstring targetLang = request.targetLang.empty() ? L"ru" : request.targetLang;
        std::wstring encodedText = UrlEncode(request.text);

        std::wstring url = host + L"/api/translate?engine=yandex&from=auto&to=" + targetLang + L"&text=" + encodedText;

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"User-Agent", L"dTranslate/1.0 (Windows 11)" }
        };

        auto response = HttpClient::Instance().Get(url, headers, 8000);

        if (!response.IsSuccess())
        {
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj))
            {
                if (resObj.HasKey(L"translated-text"))
                {
                    result.translatedText = resObj.GetNamedString(L"translated-text").c_str();
                    if (resObj.HasKey(L"source_language"))
                    {
                        result.detectedLanguage = resObj.GetNamedString(L"source_language").c_str();
                    }
                    result.success = true;
                    return result;
                }
            }
        }
        catch (...)
        {
        }

        return result;
    }

    TranslationResult YandexTranslateService::TranslateViaTrayslate(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Yandex Translate (Web)";
        result.originalText = request.text;

        std::wstring sl = (request.sourceLang.empty() || request.sourceLang == L"auto") ? L"" : (request.sourceLang + L"-");
        std::wstring tl = request.targetLang.empty() ? L"ru" : request.targetLang;
        std::wstring langParam = sl.empty() ? tl : (sl + tl);
        std::wstring encodedText = UrlEncode(request.text);

        std::wstring url = L"https://translate.yandex.net/api/v1/tr.json/translate?srv=tr-text&lang=" +
            langParam + L"&text=" + encodedText + L"&id=dtranslate-0-0";

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"User-Agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64)" }
        };

        auto response = HttpClient::Instance().Get(url, headers, 8000);

        if (!response.IsSuccess())
        {
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj))
            {
                if (resObj.HasKey(L"text"))
                {
                    auto textArr = resObj.GetNamedArray(L"text");
                    if (textArr.Size() > 0)
                    {
                        result.translatedText = textArr.GetStringAt(0).c_str();
                        result.success = true;
                        return result;
                    }
                }
            }
        }
        catch (...)
        {
        }

        return result;
    }
}
