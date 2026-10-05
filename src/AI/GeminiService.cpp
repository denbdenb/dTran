#include "pch.h"
#include "GeminiService.h"
#include "HttpClient.h"
#include "CredentialStore.h"
#include "UrlEncoder.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::AI
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;
    using namespace dTranslate::Storage;

    GeminiService& GeminiService::Instance()
    {
        static GeminiService instance;
        return instance;
    }

    AIResult GeminiService::Execute(AIRequest const& request)
    {
        AIResult result;
        result.serviceName = L"Google Gemini";

        if (request.text.empty())
        {
            result.success = true;
            result.text = L"";
            return result;
        }

        std::wstring apiKey = CredentialStore::GetCredential(CredentialStore::KeyGemini);
        if (apiKey.empty())
        {
            result.success = false;
            result.errorMessage = L"Gemini API key not configured. Please add your API key in Settings.";
            return result;
        }

        std::wstring url = L"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent?key=" + apiKey;

        std::wstring instruction = GetSystemInstruction(request.operation, request.targetLang);

        JsonObject root;

        // System instruction
        JsonObject sysInst;
        JsonArray sysParts;
        JsonObject sysPartText;
        sysPartText.SetNamedValue(L"text", JsonValue::CreateStringValue(instruction));
        sysParts.Append(sysPartText);
        sysInst.SetNamedValue(L"parts", sysParts);
        root.SetNamedValue(L"system_instruction", sysInst);

        // Contents
        JsonArray contents;
        JsonObject contentObj;
        JsonArray parts;
        JsonObject partText;
        partText.SetNamedValue(L"text", JsonValue::CreateStringValue(request.text));
        parts.Append(partText);
        contentObj.SetNamedValue(L"parts", parts);
        contents.Append(contentObj);
        root.SetNamedValue(L"contents", contents);

        std::string jsonBody = ToUtf8(root.Stringify().c_str());

        auto response = HttpClient::Instance().PostJson(url, jsonBody, {}, 25000);

        if (!response.IsSuccess())
        {
            result.success = false;
            result.errorMessage = response.errorMessage.empty() ?
                (L"Gemini API Error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj) && resObj.HasKey(L"candidates"))
            {
                auto candidates = resObj.GetNamedArray(L"candidates");
                if (candidates.Size() > 0)
                {
                    auto cand = candidates.GetObjectAt(0);
                    if (cand.HasKey(L"content"))
                    {
                        auto content = cand.GetNamedObject(L"content");
                        if (content.HasKey(L"parts"))
                        {
                            auto partsArr = content.GetNamedArray(L"parts");
                            if (partsArr.Size() > 0)
                            {
                                auto p = partsArr.GetObjectAt(0);
                                result.text = p.GetNamedString(L"text").c_str();
                                result.success = true;
                                return result;
                            }
                        }
                    }
                }
            }
            result.success = false;
            result.errorMessage = L"Malformed Gemini API response.";
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Error processing Gemini response.";
        }

        return result;
    }
}
