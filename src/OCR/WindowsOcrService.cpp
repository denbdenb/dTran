#include "pch.h"
#include "WindowsOcrService.h"
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>

namespace dTranslate::OCR
{
    using namespace winrt;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Media::Ocr;
    using namespace winrt::Windows::Graphics::Imaging;
    using namespace winrt::Windows::ApplicationModel::DataTransfer;
    using namespace winrt::Microsoft::UI::Dispatching;

    WindowsOcrService& WindowsOcrService::Instance()
    {
        static WindowsOcrService instance;
        return instance;
    }

    winrt::fire_and_forget InternalOcrWorker(
        DispatcherQueue dispatcher,
        std::function<void(OcrResult)> onComplete)
    {
        OcrResult result;

        try
        {
            auto dataPackage = Clipboard::GetContent();
            if (!dataPackage.Contains(StandardDataFormats::Bitmap()))
            {
                result.success = false;
                result.errorMessage = L"No image found in clipboard. Take a screenshot (Win+Shift+S) first.";
            }
            else
            {
                auto streamRef = co_await dataPackage.GetBitmapAsync();
                auto stream = co_await streamRef.OpenReadAsync();

                auto decoder = co_await BitmapDecoder::CreateAsync(stream);
                auto softwareBitmap = co_await decoder.GetSoftwareBitmapAsync();

                auto ocrEngine = OcrEngine::TryCreateFromUserProfileLanguages();
                if (!ocrEngine)
                {
                    result.success = false;
                    result.errorMessage = L"Windows OCR engine is not available for current system languages.";
                }
                else
                {
                    auto ocrResult = co_await ocrEngine.RecognizeAsync(softwareBitmap);
                    result.text = ocrResult.Text().c_str();
                    result.success = true;
                }
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            result.success = false;
            result.errorMessage = L"OCR Error: " + std::wstring(ex.message().c_str());
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Unexpected error during OCR processing.";
        }

        if (dispatcher)
        {
            dispatcher.TryEnqueue([onComplete, result]()
            {
                if (onComplete)
                {
                    onComplete(result);
                }
            });
        }
        else if (onComplete)
        {
            onComplete(result);
        }
    }

    winrt::fire_and_forget InternalOcrFromBufferWorker(
        std::vector<uint8_t> bmpBytes,
        DispatcherQueue dispatcher,
        std::function<void(OcrResult)> onComplete)
    {
        OcrResult result;
        try
        {
            using namespace winrt::Windows::Storage::Streams;
            InMemoryRandomAccessStream stream;
            DataWriter writer(stream);
            writer.WriteBytes(winrt::array_view<uint8_t const>(bmpBytes.data(), bmpBytes.data() + bmpBytes.size()));
            co_await writer.StoreAsync();
            writer.DetachStream();
            stream.Seek(0);

            auto decoder = co_await BitmapDecoder::CreateAsync(stream);
            auto softwareBitmap = co_await decoder.GetSoftwareBitmapAsync();

            auto ocrEngine = OcrEngine::TryCreateFromUserProfileLanguages();
            if (!ocrEngine)
            {
                auto availableLangs = OcrEngine::AvailableRecognizerLanguages();
                if (availableLangs.Size() > 0)
                {
                    ocrEngine = OcrEngine::TryCreateFromLanguage(availableLangs.GetAt(0));
                }
            }

            if (!ocrEngine)
            {
                result.success = false;
                result.errorMessage = L"Windows OCR engine is not available for current system languages.";
            }
            else
            {
                auto ocrResult = co_await ocrEngine.RecognizeAsync(softwareBitmap);
                result.text = ocrResult.Text().c_str();
                result.success = true;
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            result.success = false;
            result.errorMessage = L"OCR Error: " + std::wstring(ex.message().c_str());
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Unexpected error during OCR processing.";
        }

        if (dispatcher)
        {
            dispatcher.TryEnqueue([onComplete, result]()
            {
                if (onComplete)
                {
                    onComplete(result);
                }
            });
        }
        else if (onComplete)
        {
            onComplete(result);
        }
    }

    void WindowsOcrService::RecognizeFromClipboardAsync(
        DispatcherQueue dispatcher,
        std::function<void(OcrResult)> onComplete)
    {
        InternalOcrWorker(dispatcher, onComplete);
    }

    void WindowsOcrService::RecognizeBmpBufferAsync(
        std::vector<uint8_t> const& bmpBytes,
        DispatcherQueue dispatcher,
        std::function<void(OcrResult)> onComplete)
    {
        InternalOcrFromBufferWorker(bmpBytes, dispatcher, onComplete);
    }
}

