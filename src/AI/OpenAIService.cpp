#include "pch.h"
#include "OpenAIService.h"
#include "HttpClient.h"
#include "CredentialStore.h"
#include "SettingsManager.h"
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
        result.serviceName = L"OpenAI";

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

        std::wstring model = SettingsManager::Instance().GetSettings().openAiModel;
        if (model.empty()) model = L"gpt-4o-mini";

        JsonObject root;
        root.SetNamedValue(L"model", JsonValue::CreateStringValue(model));

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
            std::wstring errMessage, errType, errCode;
            try
            {
                if (!response.body.empty())
                {
                    std::wstring errJson = FromUtf8(response.body);
                    JsonObject errObj;
                    if (JsonObject::TryParse(errJson, errObj) && errObj.HasKey(L"error"))
                    {
                        auto errVal = errObj.GetNamedValue(L"error");
                        if (errVal.ValueType() == JsonValueType::Object)
                        {
                            auto errNode = errVal.GetObject();
                            if (errNode.HasKey(L"message")) errMessage = errNode.GetNamedString(L"message").c_str();
                            if (errNode.HasKey(L"type")) errType = errNode.GetNamedString(L"type").c_str();
                            if (errNode.HasKey(L"code") && errNode.GetNamedValue(L"code").ValueType() == JsonValueType::String)
                            {
                                errCode = errNode.GetNamedString(L"code").c_str();
                            }
                        }
                    }
                }
            }
            catch (...) {}

            // Distinct error states per AGENTS.md:
            // 1. No API credits (insufficient_quota)
            if (errCode == L"insufficient_quota" || errType == L"insufficient_quota" ||
                errMessage.find(L"quota") != std::wstring::npos ||
                errMessage.find(L"billing") != std::wstring::npos)
            {
                result.errorMessage = L"No API credits. Your OpenAI account has exhausted its quota or has no billing credits.";
            }
            // 2. Invalid API key (HTTP 401)
            else if (response.statusCode == 401 || errCode == L"invalid_api_key")
            {
                result.errorMessage = L"Invalid API key. Please check your OpenAI API key in Settings.";
            }
            // 3. Rate limited (HTTP 429)
            else if (response.statusCode == 429 || errCode == L"rate_limit_exceeded")
            {
                result.errorMessage = L"OpenAI rate limit reached. Please wait before retrying.";
            }
            // 4. Model not found (HTTP 404)
            else if (response.statusCode == 404)
            {
                result.errorMessage = L"OpenAI model \"" + model + L"\" not found (HTTP 404).";
            }
            // 5. Detailed message or status code
            else if (!errMessage.empty())
            {
                result.errorMessage = errMessage;
            }
            else
            {
                result.errorMessage = response.errorMessage.empty() ?
                    (L"OpenAI API Error: HTTP " + std::to_wstring(response.statusCode)) : response.errorMessage;
            }
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

    std::vector<std::wstring> OpenAIService::GetDefaultModels()
    {
        return {
            L"gpt-4o-mini",
            L"gpt-4o",
            L"gpt-4.1-turbo",
            L"gpt-4-turbo",
            L"o3-mini"
        };
    }

    std::vector<std::wstring> OpenAIService::ListModels(std::wstring const& apiKey)
    {
        if (apiKey.empty())
        {
            return GetDefaultModels();
        }

        std::wstring url = L"https://api.openai.com/v1/models";
        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"Authorization", L"Bearer " + apiKey }
        };

        auto response = HttpClient::Instance().Get(url, headers, 15000);

        if (!response.IsSuccess() || response.body.empty())
        {
            return GetDefaultModels();
        }

        std::vector<std::wstring> models;
        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject root;
            if (JsonObject::TryParse(jsonStr, root) && root.HasKey(L"data"))
            {
                auto dataArr = root.GetNamedArray(L"data");
                for (uint32_t i = 0; i < dataArr.Size(); ++i)
                {
                    auto mObj = dataArr.GetObjectAt(i);
                    if (mObj.HasKey(L"id"))
                    {
                        std::wstring id = mObj.GetNamedString(L"id").c_str();
                        // Filter for chat completion models
                        if (id.rfind(L"gpt-", 0) == 0 ||
                            id.rfind(L"o1", 0) == 0 ||
                            id.rfind(L"o3", 0) == 0 ||
                            id.rfind(L"chatgpt-", 0) == 0)
                        {
                            // Filter out fine-tuned, audio, realtime, or preview embedding variants
                            if (id.find(L"realtime") == std::wstring::npos &&
                                id.find(L"audio") == std::wstring::npos &&
                                id.find(L"instruct") == std::wstring::npos)
                            {
                                models.push_back(id);
                            }
                        }
                    }
                }
            }
        }
        catch (...)
        {
            return GetDefaultModels();
        }

        if (models.empty())
        {
            return GetDefaultModels();
        }

        std::sort(models.begin(), models.end());
        return models;
    }
}
