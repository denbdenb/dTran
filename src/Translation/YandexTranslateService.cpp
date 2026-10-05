#include "pch.h"
#include "YandexTranslateService.h"
#include "HttpClient.h"
#include "CredentialStore.h"
#include "UrlEncoder.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Translation
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;
    using namespace dTranslate::Storage;

    YandexTranslateService& YandexTranslateService::Instance()
    {
        static YandexTranslateService instance;
        return instance;
    }

    TranslationResult YandexTranslateService::Translate(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Yandex Translate";
        result.originalText = request.text;

        if (request.text.empty())
        {
            result.success = true;
            result.translatedText = L"";
            return result;
        }

        std::wstring apiKey = CredentialStore::GetCredential(CredentialStore::KeyYandex);
        if (apiKey.empty())
        {
            result.success = false;
            result.errorMessage = L"Yandex API key not configured. Please add your API key in Settings.";
            return result;
        }

        std::wstring url = L"https://translate.api.cloud.yandex.net/translate/v2/translate";

        JsonObject root;
        root.SetNamedValue(L"targetLanguageCode", JsonValue::CreateStringValue(request.targetLang));
        if (!request.sourceLang.empty() && request.sourceLang != L"auto")
        {
            root.SetNamedValue(L"sourceLanguageCode", JsonValue::CreateStringValue(request.sourceLang));
        }

        JsonArray texts;
        texts.Append(JsonValue::CreateStringValue(request.text));
        root.SetNamedValue(L"texts", texts);

        std::string jsonBody = ToUtf8(root.Stringify().c_str());

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"Authorization", L"Api-Key " + apiKey }
        };

        auto response = HttpClient::Instance().PostJson(url, jsonBody, headers);

        if (!response.IsSuccess())
        {
            result.success = false;
            result.errorMessage = response.errorMessage.empty() ?
                (L"Yandex API Error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj) && resObj.HasKey(L"translations"))
            {
                auto transArr = resObj.GetNamedArray(L"translations");
                if (transArr.Size() > 0)
                {
                    auto first = transArr.GetObjectAt(0);
                    result.translatedText = first.GetNamedString(L"text").c_str();
                    if (first.HasKey(L"detectedLanguageCode"))
                    {
                        result.detectedLanguage = first.GetNamedString(L"detectedLanguageCode").c_str();
                    }
                    result.success = true;
                    return result;
                }
            }
            result.success = false;
            result.errorMessage = L"Malformed Yandex Translate response.";
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Error reading Yandex response.";
        }

        return result;
    }
}
