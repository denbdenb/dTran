#include "pch.h"
#include "OpenAIService.h"
#include "HttpClient.h"
#include "CredentialStore.h"
#include "UrlEncoder.h"
#include <winrt/Windows.Data.Json.h>

namespace dTranslate::AI
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;
    using namespace dTranslate::Storage;

    OpenAIService& OpenAIService::Instance()
    {
        static OpenAIService instance;
        return instance;
    }

    AIResult OpenAIService::Execute(AIRequest const& request)
    {
        AIResult result;
        result.serviceName = L"OpenAI (GPT-4o-mini)";

        if (request.text.empty())
        {
            result.success = true;
            result.text = L"";
            return result;
        }

        std::wstring apiKey = CredentialStore::GetCredential(CredentialStore::KeyOpenAI);
        if (apiKey.empty())
        {
            result.success = false;
            result.errorMessage = L"OpenAI API key not configured. Please add your API key in Settings.";
            return result;
        }

        std::wstring url = L"https://api.openai.com/v1/chat/completions";

        std::wstring instruction = GetSystemInstruction(request.operation, request.targetLang);

        JsonObject root;
        root.SetNamedValue(L"model", JsonValue::CreateStringValue(L"gpt-4o-mini"));

        JsonArray messages;

        // System message
        JsonObject sysMsg;
        sysMsg.SetNamedValue(L"role", JsonValue::CreateStringValue(L"system"));
        sysMsg.SetNamedValue(L"content", JsonValue::CreateStringValue(instruction));
        messages.Append(sysMsg);

        // User message
        JsonObject userMsg;
        userMsg.SetNamedValue(L"role", JsonValue::CreateStringValue(L"user"));
        userMsg.SetNamedValue(L"content", JsonValue::CreateStringValue(request.text));
        messages.Append(userMsg);

        root.SetNamedValue(L"messages", messages);

        std::string jsonBody = ToUtf8(root.Stringify().c_str());

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"Authorization", L"Bearer " + apiKey }
        };

        auto response = HttpClient::Instance().PostJson(url, jsonBody, headers, 25000);

        if (!response.IsSuccess())
        {
            result.success = false;
            result.errorMessage = response.errorMessage.empty() ?
                (L"OpenAI API Error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject resObj;
            if (JsonObject::TryParse(jsonStr, resObj) && resObj.HasKey(L"choices"))
            {
                auto choices = resObj.GetNamedArray(L"choices");
                if (choices.Size() > 0)
                {
                    auto firstChoice = choices.GetObjectAt(0);
                    if (firstChoice.HasKey(L"message"))
                    {
                        auto msgObj = firstChoice.GetNamedObject(L"message");
                        if (msgObj.HasKey(L"content"))
                        {
                            result.text = msgObj.GetNamedString(L"content").c_str();
                            result.success = true;
                            return result;
                        }
                    }
                }
            }
            result.success = false;
            result.errorMessage = L"Malformed OpenAI response.";
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Error processing OpenAI response.";
        }

        return result;
    }
}
