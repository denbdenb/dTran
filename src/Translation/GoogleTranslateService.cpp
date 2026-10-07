#include "pch.h"
#include "GoogleTranslateService.h"
#include "TextChunker.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include <winrt/Windows.Data.Json.h>
#include <algorithm>

namespace dTranslate::Translation
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;

    GoogleTranslateService& GoogleTranslateService::Instance()
    {
        static GoogleTranslateService instance;
        return instance;
    }

    bool GoogleTranslateService::IsPrimaryInCooldown()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_primaryCooldownUntil == std::chrono::steady_clock::time_point{})
        {
            return false;
        }
        return std::chrono::steady_clock::now() < m_primaryCooldownUntil;
    }

    void GoogleTranslateService::RecordPrimarySuccess()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_primaryConsecutive429 = 0;
        m_primaryCooldownUntil = std::chrono::steady_clock::time_point{};
    }

    void GoogleTranslateService::RecordPrimaryThrottled(int retryAfterSeconds)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_primaryConsecutive429++;

        int cooldownSec = 0;
        if (retryAfterSeconds > 0)
        {
            cooldownSec = (std::min)(retryAfterSeconds, 120);
        }
        else
        {
            // Exponential backoff: 5s, 10s, 20s, 40s, max 60s
            int shift = (std::min)(m_primaryConsecutive429 - 1, 4);
            cooldownSec = (std::min)(5 << shift, 60);
        }

        m_primaryCooldownUntil = std::chrono::steady_clock::now() + std::chrono::seconds(cooldownSec);
    }

    TranslationResult GoogleTranslateService::Translate(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Google Translate (Web)";
        result.originalText = request.text;

        if (request.text.empty())
        {
            result.success = true;
            result.translatedText = L"";
            return result;
        }

        // For large texts (> 2500 characters), split into natural chunks
        if (request.text.size() > 2500)
        {
            auto chunks = TextChunker::Split(request.text, 2500);
            if (chunks.size() > 1)
            {
                std::vector<std::wstring> translatedParts;
                translatedParts.reserve(chunks.size());

                for (size_t i = 0; i < chunks.size(); ++i)
                {
                    TranslationRequest subReq = request;
                    subReq.text = chunks[i].text;
                    auto subRes = TranslateSingleChunk(subReq);
                    if (!subRes.success)
                    {
                        return subRes; // Return the failure directly
                    }
                    if (result.detectedLanguage.empty() && !subRes.detectedLanguage.empty())
                    {
                        result.detectedLanguage = subRes.detectedLanguage;
                    }
                    translatedParts.push_back(subRes.translatedText);
                }

                result.translatedText = TextChunker::Combine(translatedParts, chunks);
                result.success = true;
                return result;
            }
        }

        return TranslateSingleChunk(request);
    }

    TranslationResult GoogleTranslateService::TranslateSingleChunk(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Google Translate (Web)";
        result.originalText = request.text;

        if (request.text.empty())
        {
            result.success = true;
            result.translatedText = L"";
            return result;
        }

        // 1. If primary endpoint is not in cooldown, try it first
        if (!IsPrimaryInCooldown())
        {
            result = TranslateWithInstantApi(request);
            if (result.success)
            {
                RecordPrimarySuccess();
                return result;
            }

            // Check if primary returned 429
            if (result.errorMessage.find(L"429") != std::wstring::npos)
            {
                RecordPrimaryThrottled(0);
            }
        }

        // 2. Try fallback endpoint (clients5 dict-chrome-ex)
        auto fallbackResult = TranslateWithFallbackApi(request);
        if (fallbackResult.success)
        {
            return fallbackResult;
        }

        // 3. If both failed and we had a 429, present clean rate-limit message
        if (result.errorMessage.find(L"429") != std::wstring::npos || IsPrimaryInCooldown())
        {
            result.success = false;
            result.errorMessage = L"Google Translate is temporarily rate-limited. Please wait a moment or switch to Yandex.";
            return result;
        }

        return fallbackResult.errorMessage.empty() ? result : fallbackResult;
    }

    TranslationResult GoogleTranslateService::TranslateWithInstantApi(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Google Translate (Web)";
        result.originalText = request.text;

        std::wstring sl = request.sourceLang.empty() ? L"auto" : request.sourceLang;
        std::wstring tl = request.targetLang.empty() ? L"en" : request.targetLang;

        std::string formData = "client=gtx&sl=" + ToUtf8(UrlEncode(sl)) +
            "&tl=" + ToUtf8(UrlEncode(tl)) +
            "&dt=t&q=" + ToUtf8(UrlEncode(request.text));

        std::wstring url = L"https://translate.googleapis.com/translate_a/single";

        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"Accept", L"*/*" },
            { L"User-Agent", L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/144.0.0.0 Safari/537.36" }
        };

        auto response = HttpClient::Instance().PostForm(url, formData, headers, 12000);

        if (!response.IsSuccess())
        {
            result.success = false;
            if (response.statusCode == 429)
            {
                result.errorMessage = L"HTTP Error 429";
                if (response.retryAfterSeconds > 0)
                {
                    RecordPrimaryThrottled(response.retryAfterSeconds);
                }
            }
            else
            {
                result.errorMessage = response.errorMessage.empty() ?
                    (L"HTTP Error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            }
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonArray rootArr;
            if (JsonArray::TryParse(jsonStr, rootArr) && rootArr.Size() > 0)
            {
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

    TranslationResult GoogleTranslateService::TranslateWithFallbackApi(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"Google Translate (Fallback)";
        result.originalText = request.text;

        std::wstring encodedQuery = UrlEncode(request.text);
        std::wstring sl = request.sourceLang.empty() ? L"auto" : request.sourceLang;
        std::wstring tl = request.targetLang.empty() ? L"en" : request.targetLang;

        std::wstring url = L"https://clients5.google.com/translate_a/t?client=dict-chrome-ex&sl=" +
            sl + L"&tl=" + tl + L"&q=" + encodedQuery;

        auto response = HttpClient::Instance().Get(url);

        if (!response.IsSuccess())
        {
            result.success = false;
            result.errorMessage = response.errorMessage.empty() ?
                (L"Fallback HTTP Error: " + std::to_wstring(response.statusCode)) : response.errorMessage;
            return result;
        }

        try
        {
            std::wstring jsonStr = FromUtf8(response.body);
            JsonArray rootArr;
            if (JsonArray::TryParse(jsonStr, rootArr) && rootArr.Size() > 0)
            {
                auto first = rootArr.GetAt(0);
                if (first.ValueType() == JsonValueType::String)
                {
                    result.translatedText = first.GetString().c_str();
                    if (rootArr.Size() > 1 && rootArr.GetAt(1).ValueType() == JsonValueType::String)
                    {
                        result.detectedLanguage = rootArr.GetStringAt(1).c_str();
                    }
                    result.success = true;
                    return result;
                }
                if (first.ValueType() == JsonValueType::Array)
                {
                    // Response is array of chunk pairs: [ ["translated chunk", "en"], ... ]
                    std::wstring combined;
                    for (uint32_t i = 0; i < rootArr.Size(); ++i)
                    {
                        auto item = rootArr.GetAt(i);
                        if (item.ValueType() == JsonValueType::Array)
                        {
                            auto subArr = item.GetArray();
                            if (subArr.Size() > 0 && subArr.GetAt(0).ValueType() == JsonValueType::String)
                            {
                                combined += subArr.GetStringAt(0).c_str();
                            }
                            if (subArr.Size() > 1 && subArr.GetAt(1).ValueType() == JsonValueType::String)
                            {
                                if (result.detectedLanguage.empty())
                                {
                                    result.detectedLanguage = subArr.GetStringAt(1).c_str();
                                }
                            }
                        }
                    }
                    if (!combined.empty())
                    {
                        result.translatedText = combined;
                        result.success = true;
                        return result;
                    }
                }
            }

            // In some cases response is a single string enclosed in quotes
            if (jsonStr.size() >= 2 && jsonStr.front() == L'"' && jsonStr.back() == L'"')
            {
                result.translatedText = jsonStr.substr(1, jsonStr.size() - 2);
                result.success = true;
                return result;
            }

            result.success = false;
            result.errorMessage = L"Unable to parse fallback response.";
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Error processing fallback data.";
        }

        return result;
    }
}
