#pragma once
#include <string>
#include <functional>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace dTranslate::OCR
{
    struct OcrResult
    {
        bool success{ false };
        std::wstring text;
        std::wstring errorMessage;
    };

    class WindowsOcrService
    {
    public:
        static WindowsOcrService& Instance();

        void RecognizeFromClipboardAsync(
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
            std::function<void(OcrResult)> onComplete);

        void RecognizeBmpBufferAsync(
            std::vector<uint8_t> const& bmpBytes,
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
            std::function<void(OcrResult)> onComplete);

    private:
        WindowsOcrService() = default;
    };
}
