#include "pch.h"
#include "GeminiService.h"
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

        std::wstring model = SettingsManager::Instance().GetSettings().geminiModel;
        if (model.empty()) model = L"gemini-2.5-flash";

        // Normalize model name: strip "models/" prefix if present
        if (model.rfind(L"models/", 0) == 0)
        {
            model = model.substr(7);
        }

        std::wstring url = L"https://generativelanguage.googleapis.com/v1beta/models/" + model + L":generateContent?key=" + apiKey;

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
            // Parse Google JSON error payload for clear diagnosis
            std::wstring detailedError;
            try
            {
                if (!response.body.empty())
                {
                    std::wstring errJson = FromUtf8(response.body);
                    JsonObject errObj;
                    if (JsonObject::TryParse(errJson, errObj) && errObj.HasKey(L"error"))
                    {
                        auto errNode = errObj.GetNamedObject(L"error");
                        if (errNode.HasKey(L"message"))
                        {
                            detailedError = errNode.GetNamedString(L"message").c_str();
                        }
                    }
                }
            }
            catch (...) {}

            if (!detailedError.empty())
            {
                result.errorMessage = detailedError;
            }
            else if (response.statusCode == 404)
            {
                result.errorMessage = L"Gemini model \"" + model + L"\" not found (HTTP 404). Please refresh models in Settings.";
            }
            else if (response.statusCode == 400)
            {
                result.errorMessage = L"Invalid Gemini request or API key (HTTP 400). Please check your key in Settings.";
            }
            else if (response.statusCode == 429)
            {
                result.errorMessage = L"Gemini rate limit / quota exceeded (HTTP 429). Please wait before retrying.";
            }
            else
            {
                result.errorMessage = response.errorMessage.empty() ?
                    (L"Gemini API Error: HTTP " + std::to_wstring(response.statusCode)) : response.errorMessage;
            }
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

    std::vector<std::wstring> GeminiService::GetDefaultModels()
    {
        return {
            L"gemini-2.5-flash",
            L"gemini-2.5-pro",
            L"gemini-2.0-flash",
            L"gemini-2.0-flash-lite"
        };
    }

    std::vector<std::wstring> GeminiService::ListModels(std::wstring const& apiKey)
    {
        if (apiKey.empty())
        {
            return GetDefaultModels();
        }

        std::wstring url = L"https://generativelanguage.googleapis.com/v1beta/models?key=" + apiKey;
        auto response = HttpClient::Instance().Get(url, {}, 15000);

        if (!response.IsSuccess() || response.body.empty())
        {
            return GetDefaultModels();
        }

        std::vector<std::wstring> models;
        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonObject root;
            if (JsonObject::TryParse(jsonStr, root) && root.HasKey(L"models"))
            {
                auto modelsArr = root.GetNamedArray(L"models");
                for (uint32_t i = 0; i < modelsArr.Size(); ++i)
                {
                    auto mObj = modelsArr.GetObjectAt(i);
                    // Filter: must support generateContent
                    bool supportsGen = false;
                    if (mObj.HasKey(L"supportedGenerationMethods"))
                    {
                        auto methods = mObj.GetNamedArray(L"supportedGenerationMethods");
                        for (uint32_t j = 0; j < methods.Size(); ++j)
                        {
                            if (methods.GetStringAt(j) == L"generateContent")
                            {
                                supportsGen = true;
                                break;
                            }
                        }
                    }

                    if (supportsGen && mObj.HasKey(L"name"))
                    {
                        std::wstring name = mObj.GetNamedString(L"name").c_str();
                        if (name.rfind(L"models/", 0) == 0)
                        {
                            name = name.substr(7);
                        }
                        // Only add modern text/chat models
                        if (name.rfind(L"gemini-", 0) == 0)
                        {
                            models.push_back(name);
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

        // Sort models
        std::sort(models.begin(), models.end());
        return models;
    }
}
