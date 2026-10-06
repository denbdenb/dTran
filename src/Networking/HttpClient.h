#pragma once
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <utility>

#pragma comment(lib, "winhttp.lib")

namespace dTranslate::Networking
{
    struct HttpResponse
    {
        int statusCode{ 0 };
        std::string body;
        std::wstring errorMessage;
        int retryAfterSeconds{ 0 };

        bool IsSuccess() const
        {
            return statusCode >= 200 && statusCode < 300;
        }
    };

    class HttpClient
    {
    public:
        static HttpClient& Instance();

        HttpResponse Get(
            std::wstring const& url,
            std::vector<std::pair<std::wstring, std::wstring>> const& headers = {},
            int timeoutMs = 10000);

        HttpResponse PostJson(
            std::wstring const& url,
            std::string const& jsonBody,
            std::vector<std::pair<std::wstring, std::wstring>> const& headers = {},
            int timeoutMs = 15000);

        HttpResponse PostForm(
            std::wstring const& url,
            std::string const& formData,
            std::vector<std::pair<std::wstring, std::wstring>> const& headers = {},
            int timeoutMs = 15000);

    private:
        HttpClient();
        ~HttpClient();

        bool EnsureSession();
        void CloseSession();

        HttpResponse ExecuteRequest(
            std::wstring const& verb,
            std::wstring const& url,
            std::string const& body,
            std::vector<std::pair<std::wstring, std::wstring>> const& headers,
            int timeoutMs);

        HINTERNET m_hSession{ nullptr };
    };
}
