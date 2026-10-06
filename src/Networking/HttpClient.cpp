#include "pch.h"
#include "HttpClient.h"

namespace dTranslate::Networking
{
    HttpClient& HttpClient::Instance()
    {
        static HttpClient instance;
        return instance;
    }

    HttpClient::HttpClient()
    {
    }

    HttpClient::~HttpClient()
    {
        CloseSession();
    }

    bool HttpClient::EnsureSession()
    {
        if (m_hSession != nullptr)
        {
            return true;
        }

        m_hSession = WinHttpOpen(
            L"dTranslate/1.0 (Windows 11; Win64; x64)",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        return (m_hSession != nullptr);
    }

    void HttpClient::CloseSession()
    {
        if (m_hSession != nullptr)
        {
            WinHttpCloseHandle(m_hSession);
            m_hSession = nullptr;
        }
    }

    HttpResponse HttpClient::Get(
        std::wstring const& url,
        std::vector<std::pair<std::wstring, std::wstring>> const& headers,
        int timeoutMs)
    {
        return ExecuteRequest(L"GET", url, "", headers, timeoutMs);
    }

    HttpResponse HttpClient::PostJson(
        std::wstring const& url,
        std::string const& jsonBody,
        std::vector<std::pair<std::wstring, std::wstring>> const& headers,
        int timeoutMs)
    {
        auto reqHeaders = headers;
        reqHeaders.push_back({ L"Content-Type", L"application/json; charset=utf-8" });
        return ExecuteRequest(L"POST", url, jsonBody, reqHeaders, timeoutMs);
    }

    HttpResponse HttpClient::PostForm(
        std::wstring const& url,
        std::string const& formData,
        std::vector<std::pair<std::wstring, std::wstring>> const& headers,
        int timeoutMs)
    {
        auto reqHeaders = headers;
        reqHeaders.push_back({ L"Content-Type", L"application/x-www-form-urlencoded; charset=utf-8" });
        return ExecuteRequest(L"POST", url, formData, reqHeaders, timeoutMs);
    }

    HttpResponse HttpClient::ExecuteRequest(
        std::wstring const& verb,
        std::wstring const& url,
        std::string const& body,
        std::vector<std::pair<std::wstring, std::wstring>> const& headers,
        int timeoutMs)
    {
        HttpResponse response;

        if (!EnsureSession())
        {
            response.errorMessage = L"Failed to initialize network session.";
            return response;
        }

        URL_COMPONENTS urlComp = { sizeof(URL_COMPONENTS) };
        wchar_t hostName[256] = {};
        wchar_t urlPath[2048] = {};

        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = ARRAYSIZE(hostName);
        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = ARRAYSIZE(urlPath);

        if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.length()), 0, &urlComp))
        {
            response.errorMessage = L"Invalid URL format.";
            return response;
        }

        HINTERNET hConnect = WinHttpConnect(
            m_hSession,
            urlComp.lpszHostName,
            urlComp.nPort,
            0);

        if (hConnect == nullptr)
        {
            response.errorMessage = L"Unable to connect to host.";
            return response;
        }

        DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            verb.c_str(),
            urlComp.lpszUrlPath,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags);

        if (hRequest == nullptr)
        {
            WinHttpCloseHandle(hConnect);
            response.errorMessage = L"Unable to open HTTP request.";
            return response;
        }

        // Apply timeouts (resolve, connect, send, receive)
        WinHttpSetTimeouts(hRequest, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

        // Add headers
        for (auto const& [name, val] : headers)
        {
            std::wstring h = name + L": " + val + L"\r\n";
            WinHttpAddRequestHeaders(hRequest, h.c_str(), static_cast<DWORD>(h.length()), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
        }

        // Send request
        BOOL bResults = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
            static_cast<DWORD>(body.size()),
            static_cast<DWORD>(body.size()),
            0);

        if (bResults)
        {
            bResults = WinHttpReceiveResponse(hRequest, nullptr);
        }

        if (bResults)
        {
            DWORD statusCode = 0;
            DWORD size = sizeof(statusCode);
            if (WinHttpQueryHeaders(
                hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &statusCode,
                &size,
                WINHTTP_NO_HEADER_INDEX))
            {
                response.statusCode = static_cast<int>(statusCode);
            }

            // Query Retry-After header if present
            wchar_t retryHeader[64] = {};
            DWORD retrySize = sizeof(retryHeader);
            if (WinHttpQueryHeaders(
                hRequest,
                WINHTTP_QUERY_CUSTOM,
                L"Retry-After",
                retryHeader,
                &retrySize,
                WINHTTP_NO_HEADER_INDEX))
            {
                try
                {
                    response.retryAfterSeconds = std::stoi(retryHeader);
                }
                catch (...) {}
            }

            // Read response stream
            DWORD dwBytesAvailable = 0;
            while (WinHttpQueryDataAvailable(hRequest, &dwBytesAvailable) && dwBytesAvailable > 0)
            {
                std::vector<char> buffer(dwBytesAvailable);
                DWORD dwBytesRead = 0;
                if (WinHttpReadData(hRequest, buffer.data(), dwBytesAvailable, &dwBytesRead))
                {
                    response.body.append(buffer.data(), dwBytesRead);
                }
            }
        }
        else
        {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_WINHTTP_TIMEOUT)
            {
                response.errorMessage = L"Request timed out.";
            }
            else if (dwError == ERROR_WINHTTP_CANNOT_CONNECT)
            {
                response.errorMessage = L"Cannot connect to translation service.";
            }
            else
            {
                response.errorMessage = L"Network error code: " + std::to_wstring(dwError);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return response;
    }
}
