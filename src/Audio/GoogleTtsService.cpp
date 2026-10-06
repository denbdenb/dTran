#include "pch.h"
#include "GoogleTtsService.h"
#include "HttpClient.h"
#include "UrlEncoder.h"
#include <fstream>
#include <filesystem>
#include <mmsystem.h>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

namespace dTranslate::Audio
{
    using namespace dTranslate::Networking;

    GoogleTtsService& GoogleTtsService::Instance()
    {
        static GoogleTtsService instance;
        return instance;
    }

    GoogleTtsService::GoogleTtsService()
    {
    }

    GoogleTtsService::~GoogleTtsService()
    {
        Stop();
    }

    void GoogleTtsService::Stop()
    {
        m_cancelRequested.store(true);
        m_sessionCounter.fetch_add(1);

        mciSendStringW(L"stop all", nullptr, 0, nullptr);
        mciSendStringW(L"close all", nullptr, 0, nullptr);

        std::unique_lock<std::mutex> lock(m_threadMutex);
        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
        m_isPlaying.store(false);
        m_cancelRequested.store(false);
    }

    std::vector<std::wstring> GoogleTtsService::SplitIntoChunks(std::wstring const& text, size_t maxChunkSize)
    {
        std::vector<std::wstring> chunks;
        if (text.empty()) return chunks;

        size_t start = 0;
        size_t len = text.length();

        while (start < len)
        {
            // Skip leading whitespace
            while (start < len && iswspace(text[start]))
            {
                ++start;
            }
            if (start >= len) break;

            size_t remaining = len - start;
            if (remaining <= maxChunkSize)
            {
                std::wstring chunk = text.substr(start);
                if (!chunk.empty()) chunks.push_back(chunk);
                break;
            }

            // Look for best delimiter within maxChunkSize
            size_t searchEnd = start + maxChunkSize;
            size_t bestSplit = std::wstring::npos;

            // 1. Try sentence delimiters: . ! ? ; \n
            for (size_t i = searchEnd; i > start; --i)
            {
                wchar_t ch = text[i - 1];
                if (ch == L'.' || ch == L'!' || ch == L'?' || ch == L';' || ch == L'\n' ||
                    ch == 0x3002 || ch == 0xFF01 || ch == 0xFF1F) // Japanese/Chinese full stops
                {
                    bestSplit = i;
                    break;
                }
            }

            // 2. Try clause delimiters: , : - —
            if (bestSplit == std::wstring::npos)
            {
                for (size_t i = searchEnd; i > start; --i)
                {
                    wchar_t ch = text[i - 1];
                    if (ch == L',' || ch == L':' || ch == L'-' || ch == 0x2014 || ch == 0xFF0C)
                    {
                        bestSplit = i;
                        break;
                    }
                }
            }

            // 3. Try word boundary: space
            if (bestSplit == std::wstring::npos)
            {
                for (size_t i = searchEnd; i > start; --i)
                {
                    if (iswspace(text[i - 1]))
                    {
                        bestSplit = i;
                        break;
                    }
                }
            }

            // 4. Hard cutoff if no delimiter found
            if (bestSplit == std::wstring::npos || bestSplit <= start)
            {
                bestSplit = searchEnd;
            }

            std::wstring chunk = text.substr(start, bestSplit - start);
            if (!chunk.empty())
            {
                chunks.push_back(chunk);
            }
            start = bestSplit;
        }

        return chunks;
    }

    void GoogleTtsService::PlaybackWorker(std::vector<std::wstring> chunks, std::wstring langCode, uint64_t playSessionId)
    {
        std::wstring tl = langCode.empty() || langCode == L"auto" ? L"en" : langCode;
        std::vector<std::pair<std::wstring, std::wstring>> headers = {
            { L"User-Agent", L"dTranslate/1.0 (Windows 11)" },
            { L"Referer", L"https://translate.google.com/" }
        };

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            if (m_cancelRequested.load() || playSessionId != m_sessionCounter.load())
            {
                break;
            }

            auto const& chunk = chunks[i];
            if (chunk.empty()) continue;

            std::wstring encodedText = UrlEncode(chunk);
            std::wstring url = L"https://translate.google.com/translate_tts?ie=UTF-8&tl=" +
                tl + L"&client=tw-ob&q=" + encodedText;

            auto response = HttpClient::Instance().Get(url, headers);

            if (m_cancelRequested.load() || playSessionId != m_sessionCounter.load())
            {
                break;
            }

            if (!response.IsSuccess() || response.body.empty())
            {
                continue;
            }

            std::wstring aliasName = L"dtrans_tts_" + std::to_wstring(playSessionId) + L"_" + std::to_wstring(i);
            auto tempFile = std::filesystem::temp_directory_path() / (aliasName + L".mp3");

            try
            {
                std::ofstream out(tempFile, std::ios::binary | std::ios::trunc);
                if (out.is_open())
                {
                    out.write(response.body.data(), response.body.size());
                    out.close();

                    std::wstring openCmd = L"open \"" + tempFile.wstring() + L"\" type mpegvideo alias " + aliasName;
                    mciSendStringW(openCmd.c_str(), nullptr, 0, nullptr);

                    if (!m_cancelRequested.load() && playSessionId == m_sessionCounter.load())
                    {
                        std::wstring playCmd = L"play " + aliasName + L" wait";
                        mciSendStringW(playCmd.c_str(), nullptr, 0, nullptr);
                    }

                    std::wstring closeCmd = L"close " + aliasName;
                    mciSendStringW(closeCmd.c_str(), nullptr, 0, nullptr);
                }

                std::error_code ec;
                std::filesystem::remove(tempFile, ec);
            }
            catch (...)
            {
                // Silently handle chunk playback failure
            }
        }

        if (playSessionId == m_sessionCounter.load())
        {
            m_isPlaying.store(false);
        }
    }

    bool GoogleTtsService::Speak(std::wstring const& text, std::wstring const& langCode)
    {
        if (text.empty()) return false;

        // Stop prior audio and terminate prior worker
        Stop();

        auto chunks = SplitIntoChunks(text, 160);
        if (chunks.empty()) return false;

        uint64_t currentSession = m_sessionCounter.load();
        m_isPlaying.store(true);
        m_cancelRequested.store(false);

        std::unique_lock<std::mutex> lock(m_threadMutex);
        m_workerThread = std::thread([this, chunks = std::move(chunks), langCode, currentSession]()
        {
            PlaybackWorker(chunks, langCode, currentSession);
        });

        return true;
    }
}
