#pragma once
#include <string>
#include <functional>
#include <vector>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace dTranslate::OCR
{
    struct OcrResult
    {
        bool success{ false };
        std::wstring text;
        std::wstring errorMessage;
        bool missingLanguagePack{ false };
    };

    class WindowsOcrService
    {
    public:
        static WindowsOcrService& Instance();

        void RecognizeFromClipboardAsync(
            std::wstring const& langCode,
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
            std::function<void(OcrResult)> onComplete);

        void RecognizeBmpBufferAsync(
            std::vector<uint8_t> const& bmpBytes,
            std::wstring const& langCode,
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
            std::function<void(OcrResult)> onComplete);

        static void OpenWindowsLanguageSettings();
        static std::vector<std::wstring> GetInstalledRecognizerLanguages();

    private:
        WindowsOcrService() = default;
    };
}
