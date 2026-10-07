#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace dTranslate::Audio
{
    class GoogleTtsService
    {
    public:
        static GoogleTtsService& Instance();

        bool Speak(
            std::wstring const& text,
            std::wstring const& langCode,
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher = nullptr,
            std::function<void()> onFinished = nullptr);

        void Stop();
        bool IsPlaying() const { return m_isPlaying.load(); }
        static std::vector<std::wstring> SplitIntoChunks(std::wstring const& text, size_t maxChunkSize = 160);

    private:
        GoogleTtsService();
        ~GoogleTtsService();

        void PlaybackWorker(
            std::vector<std::wstring> chunks,
            std::wstring langCode,
            uint64_t playSessionId,
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
            std::function<void()> onFinished);

        std::atomic<bool> m_cancelRequested{ false };
        std::atomic<bool> m_isPlaying{ false };
        std::atomic<uint64_t> m_sessionCounter{ 0 };
        std::thread m_workerThread;
        std::mutex m_threadMutex;
        std::wstring m_currentAlias;
        std::mutex m_aliasMutex;
    };
}
