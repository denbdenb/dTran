#include "pch.h"
#include "GoogleTranslateService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include "CredentialStore.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::Translation
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;
    using namespace dTranslate::Storage;

    GoogleTranslateService& GoogleTranslateService::Instance()
    {
        static GoogleTranslateService instance;
        return instance;
    }

    TranslationResult GoogleTranslateService::Translate(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Google Translate";
        result.originalText = request.text;

        if (request.text.empty())
        {
            result.success = true;
            result.translatedText = L"";
            return result;
        }

        std::wstring apiKey = CredentialStore::GetCredential(CredentialStore::KeyGoogle);
        if (!apiKey.empty())
        {
            return TranslateWithCloudApi(request, apiKey);
        }
        else
        {
            return TranslateWithInstantApi(request);
        }
    }

    TranslationResult GoogleTranslateService::TranslateWithInstantApi(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Google Translate";
        result.originalText = request.text;

        std::wstring encodedQuery = UrlEncode(request.text);
        std::wstring sl = request.sourceLang.empty() ? L"auto" : request.sourceLang;
        std::wstring tl = request.targetLang.empty() ? L"en" : request.targetLang;

        std::wstring url = L"https://translate.googleapis.com/translate_a/single?client=gtx&sl=" +
            sl + L"&tl=" + tl + L"&dt=t&q=" + encodedQuery;

        auto response = HttpClient::Instance().Get(url);

        if (!response.IsSuccess())
        {
            result.success = false;
            result.errorMessage = response.errorMessage.empty() ?
                (L"HTTP Error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonArray rootArr;
            if (JsonArray::TryParse(jsonStr, rootArr) && rootArr.Size() > 0)
            {
                // rootArr[0] contains array of sentence pairs [[trans, orig], ...]
                auto sentencesVal = rootArr.GetAt(0);
                if (sentencesVal.ValueType() == JsonValueType::Array)
                {
                    auto sentencesArr = sentencesVal.GetArray();
                    std::wstring fullTranslation;
                    for (uint32_t i = 0; i < sentencesArr.Size(); ++i)
                    {
                        auto pairVal = sentencesArr.GetAt(i);
                        if (pairVal.ValueType() == JsonValueType::Array)
                        {
                            auto pairArr = pairVal.GetArray();
                            if (pairArr.Size() > 0 && pairArr.GetAt(0).ValueType() == JsonValueType::String)
                            {
                                fullTranslation += pairArr.GetStringAt(0).c_str();
                            }
                        }
                    }
                    result.translatedText = fullTranslation;
                    result.success = true;
                }

                // Detected language is at index 2 if present
                if (rootArr.Size() > 2 && rootArr.GetAt(2).ValueType() == JsonValueType::String)
                {
                    result.detectedLanguage = rootArr.GetStringAt(2).c_str();
                }
            }
            else
            {
                result.success = false;
                result.errorMessage = L"Unable to parse translation response.";
            }
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Error processing translation data.";
        }

        return result;
    }

    TranslationResult GoogleTranslateService::TranslateWithCloudApi(TranslationRequest const& request, std::wstring const& apiKey)
    {
        TranslationResult result;
        result.serviceName = L"Google Translate (Cloud)";
        result.originalText = request.text;

        std::wstring url = L"https://translation.googleapis.com/language/translate/v2?key=" + apiKey;

        JsonObject root;
        root.SetNamedValue(L"q", JsonValue::CreateStringValue(request.text));
        if (!request.sourceLang.empty() && request.sourceLang != L"auto")
        {
            root.SetNamedValue(L"source", JsonValue::CreateStringValue(request.sourceLang));
        }
        root.SetNamedValue(L"target", JsonValue::CreateStringValue(request.targetLang));
        root.SetNamedValue(L"format", JsonValue::CreateStringValue(L"text"));

        std::string jsonPayload = ToUtf8(root.Stringify().c_str());
        auto response = HttpClient::Instance().PostJson(url, jsonPayload);

        if (!response.IsSuccess())
        {
            result.success = false;
            result.errorMessage = response.errorMessage.empty() ?
                (L"Google Cloud API Error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj) && resObj.HasKey(L"data"))
            {
                auto dataObj = resObj.GetNamedObject(L"data");
                if (dataObj.HasKey(L"translations"))
                {
                    auto transArr = dataObj.GetNamedArray(L"translations");
                    if (transArr.Size() > 0)
                    {
                        auto first = transArr.GetObjectAt(0);
                        result.translatedText = first.GetNamedString(L"translatedText").c_str();
                        if (first.HasKey(L"detectedSourceLanguage"))
                        {
                            result.detectedLanguage = first.GetNamedString(L"detectedSourceLanguage").c_str();
                        }
                        result.success = true;
                        return result;
                    }
                }
            }
            result.success = false;
            result.errorMessage = L"Malformed Cloud Translate response.";
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Error reading Cloud Translate response.";
        }

        return result;
    }
}
