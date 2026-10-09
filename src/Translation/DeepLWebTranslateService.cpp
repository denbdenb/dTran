#include "pch.h"
#include "DeepLWebTranslateService.h"
#include "TextChunker.h"
#include "LanguageCatalog.h"
#include "LocalizationManager.h"
#include "HttpClient.h"
#include <winrt/Windows.Data.Json.h>
#include <objbase.h>
#include <algorithm>

namespace dTranslate::Translation
{
    using namespace winrt::Windows::Data::Json;
    using namespace dTranslate::Networking;
    using namespace dTranslate::Storage;

    static std::string GenerateUUID()
    {
        GUID guid;
        if (SUCCEEDED(CoCreateGuid(&guid)))
        {
            char buf[64];
            snprintf(buf, sizeof(buf),
                "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                guid.Data1, guid.Data2, guid.Data3,
                guid.Data4[0], guid.Data4[1],
                guid.Data4[2], guid.Data4[3], guid.Data4[4],
                guid.Data4[5], guid.Data4[6], guid.Data4[7]);
            return buf;
        }
        return "c8b36873-5163-42e1-88c9-943b12345678";
    }

    DeepLWebTranslateService& DeepLWebTranslateService::Instance()
    {
        static DeepLWebTranslateService instance;
        return instance;
    }

    DeepLWebTranslateService::DeepLWebTranslateService()
        : m_instanceId(GenerateUUID())
        , m_sessionId(GenerateUUID())
    {
    }

    bool DeepLWebTranslateService::IsInCooldown()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_cooldownUntil == std::chrono::steady_clock::time_point{})
        {
            return false;
        }
        return std::chrono::steady_clock::now() < m_cooldownUntil;
    }

    void DeepLWebTranslateService::RecordSuccess()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_consecutive429 = 0;
        m_cooldownUntil = std::chrono::steady_clock::time_point{};
    }

    void DeepLWebTranslateService::RecordThrottled(int retryAfterSeconds)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_consecutive429++;

        int cooldownSec = 0;
        if (retryAfterSeconds > 0)
        {
            cooldownSec = (std::min)(retryAfterSeconds, 120);
        }
        else
        {
            // Exponential backoff: 5s, 10s, 20s, 40s, max 60s
            int shift = (std::min)(m_consecutive429 - 1, 4);
            cooldownSec = (std::min)(5 << shift, 60);
        }

        m_cooldownUntil = std::chrono::steady_clock::now() + std::chrono::seconds(cooldownSec);
    }

    std::wstring DeepLWebTranslateService::MapTargetLanguage(std::wstring const& code)
    {
        if (code == L"en") return L"en-US";
        if (code == L"pt") return L"pt-BR";
        if (code == L"zh") return L"zh-Hans";
        if (code == L"zh-TW") return L"zh-Hant";
        if (code == L"no") return L"nb";

        // Supported DeepL target language codes
        static const std::vector<std::wstring> s_validTargets = {
            L"ar", L"bg", L"cs", L"da", L"de", L"el", L"en-US", L"en-GB",
            L"es", L"et", L"fi", L"fr", L"hu", L"id", L"it", L"ja",
            L"ko", L"lt", L"lv", L"nb", L"nl", L"pl", L"pt-BR", L"pt-PT",
            L"ro", L"ru", L"sk", L"sl", L"sv", L"tr", L"uk", L"zh-Hans", L"zh-Hant"
        };

        for (auto const& valid : s_validTargets)
        {
            if (_wcsicmp(valid.c_str(), code.c_str()) == 0)
            {
                return valid;
            }
        }

        return L"";
    }

    std::wstring DeepLWebTranslateService::MapSourceLanguage(std::wstring const& code)
    {
        if (code.empty() || code == L"auto") return L"";
        if (code == L"zh-TW") return L"zh";
        if (code == L"no") return L"nb";
        return code;
    }

    TranslationResult DeepLWebTranslateService::Translate(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"DeepL (Web)";
        result.originalText = request.text;

        if (request.text.empty())
        {
            result.success = true;
            result.translatedText = L"";
            return result;
        }

        // Validate language support for DeepL
        if ((!request.sourceLang.empty() && request.sourceLang != L"auto" && !LanguageCatalog::IsServiceSupported(2, request.sourceLang)) ||
            (!request.targetLang.empty() && !LanguageCatalog::IsServiceSupported(2, request.targetLang)))
        {
            result.success = false;
            result.errorMessage = LocalizationManager::Instance().Get(L"ErrorDeepLUnsupportedLanguage");
            return result;
        }

        // Check target mapping
        auto mappedTarget = MapTargetLanguage(request.targetLang);
        if (mappedTarget.empty())
        {
            result.success = false;
            result.errorMessage = LocalizationManager::Instance().Get(L"ErrorDeepLUnsupportedLanguage");
            return result;
        }

        // For large texts (> 1200 characters), split into natural chunks (free limit is 1500)
        if (request.text.size() > 1200)
        {
            auto chunks = TextChunker::Split(request.text, 1200);
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
                        return subRes;
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

    TranslationResult DeepLWebTranslateService::TranslateSingleChunk(TranslationRequest const& request)
    {
        TranslationResult result;
        result.serviceName = L"DeepL (Web)";
        result.originalText = request.text;

        if (request.text.empty())
        {
            result.success = true;
            result.translatedText = L"";
            return result;
        }

        if (IsInCooldown())
        {
            result.success = false;
            result.errorMessage = LocalizationManager::Instance().Get(L"ErrorDeepLRateLimited");
            return result;
        }

        auto targetLang = MapTargetLanguage(request.targetLang);
        if (targetLang.empty())
        {
            result.success = false;
            result.errorMessage = LocalizationManager::Instance().Get(L"ErrorDeepLUnsupportedLanguage");
            return result;
        }

        auto sourceLang = MapSourceLanguage(request.sourceLang);

        try
        {
            JsonObject root;
            JsonArray textArr;
            textArr.Append(JsonValue::CreateStringValue(winrt::hstring(request.text)));
            root.SetNamedValue(L"text", textArr);
            root.SetNamedValue(L"target_lang", JsonValue::CreateStringValue(winrt::hstring(targetLang)));

            if (!sourceLang.empty())
            {
                root.SetNamedValue(L"source_lang", JsonValue::CreateStringValue(winrt::hstring(sourceLang)));
            }

            root.SetNamedValue(L"usage_type", JsonValue::CreateStringValue(L"translate"));

            JsonObject appInfo;
            appInfo.SetNamedValue(L"os", JsonValue::CreateStringValue(L"iOS"));
            appInfo.SetNamedValue(L"os_version", JsonValue::CreateStringValue(L"26.0"));
            appInfo.SetNamedValue(L"app_version", JsonValue::CreateStringValue(L"26.52"));
            appInfo.SetNamedValue(L"app_build", JsonValue::CreateStringValue(L"5443737"));
            appInfo.SetNamedValue(L"instance_id", JsonValue::CreateStringValue(winrt::to_hstring(m_instanceId)));
            root.SetNamedValue(L"app_information", appInfo);

            std::string payload = winrt::to_string(root.Stringify());

            std::vector<std::pair<std::wstring, std::wstring>> headers = {
                { L"Authorization", L"None" },
                { L"User-Agent", L"DeepL/26.52 CFNetwork/3826.600.41 Darwin/25.0.0" },
                { L"x-app-os-version", L"26.0" },
                { L"x-app-instance-id", winrt::to_hstring(m_instanceId).c_str() },
                { L"x-app-session-id", winrt::to_hstring(m_sessionId).c_str() }
            };

            auto response = HttpClient::Instance().PostJson(
                L"https://oneshot-free.www.deepl.com/v1/translate",
                payload,
                headers,
                15000);

            if (response.statusCode == 200)
            {
                JsonObject resJson;
                if (JsonObject::TryParse(winrt::to_hstring(response.body), resJson))
                {
                    if (resJson.HasKey(L"translations"))
                    {
                        auto translations = resJson.GetNamedArray(L"translations");
                        if (translations.Size() > 0)
                        {
                            auto item = translations.GetObjectAt(0);
                            if (item.HasKey(L"text"))
                            {
                                result.translatedText = item.GetNamedString(L"text").c_str();
                                result.success = true;
                                RecordSuccess();
                            }
                            if (item.HasKey(L"detected_source_language"))
                            {
                                result.detectedLanguage = item.GetNamedString(L"detected_source_language").c_str();
                            }
                            return result;
                        }
                    }
                }

                result.success = false;
                result.errorMessage = LocalizationManager::Instance().Get(L"ServiceUnavailable");
                return result;
            }
            else if (response.statusCode == 429)
            {
                RecordThrottled(response.retryAfterSeconds);
                result.success = false;
                result.errorMessage = LocalizationManager::Instance().Get(L"ErrorDeepLRateLimited");
                return result;
            }
            else if (response.statusCode == 403)
            {
                result.success = false;
                result.errorMessage = LocalizationManager::Instance().Get(L"ErrorDeepLUnavailable");
                return result;
            }
            else
            {
                result.success = false;
                result.errorMessage = !response.errorMessage.empty() ? response.errorMessage : LocalizationManager::Instance().Get(L"ServiceUnavailable");
                return result;
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            result.success = false;
            result.errorMessage = ex.message().c_str();
            return result;
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = LocalizationManager::Instance().Get(L"ServiceUnavailable");
            return result;
        }
    }
}
