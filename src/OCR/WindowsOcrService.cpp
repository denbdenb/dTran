#include "pch.h"
#include "WindowsOcrService.h"
#include "LanguageCatalog.h"
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.System.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace dTranslate::OCR
{
    using namespace winrt;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Media::Ocr;
    using namespace winrt::Windows::Graphics::Imaging;
    using namespace winrt::Windows::ApplicationModel::DataTransfer;
    using namespace winrt::Windows::Globalization;
    using namespace winrt::Microsoft::UI::Dispatching;
    using namespace dTranslate::Translation;

    WindowsOcrService& WindowsOcrService::Instance()
    {
        static WindowsOcrService instance;
        return instance;
    }

    static void LogOcrDiagnostic(std::wstring const& message)
    {
        std::wstring line = L"[dTranslate OCR] " + message + L"\n";
        OutputDebugStringW(line.c_str());

        wchar_t tempPath[MAX_PATH];
        if (GetTempPathW(MAX_PATH, tempPath) > 0)
        {
            std::wstring logPath = std::wstring(tempPath) + L"ocr_diagnostic.log";
            FILE* f = nullptr;
            _wfopen_s(&f, logPath.c_str(), L"a");
            if (f)
            {
                fwprintf(f, L"%s", line.c_str());
                fclose(f);
            }
        }
    }

    void WindowsOcrService::OpenWindowsLanguageSettings()
    {
        try
        {
            Windows::System::Launcher::LaunchUriAsync(Uri(L"ms-settings:regionlanguage"));
        }
        catch (...) {}
    }

    std::vector<std::wstring> WindowsOcrService::GetInstalledRecognizerLanguages()
    {
        std::vector<std::wstring> list;
        try
        {
            auto available = OcrEngine::AvailableRecognizerLanguages();
            for (uint32_t i = 0; i < available.Size(); ++i)
            {
                auto lang = available.GetAt(i);
                list.push_back(lang.LanguageTag().c_str() + std::wstring(L" (") + lang.DisplayName().c_str() + L")");
            }
        }
        catch (...) {}
        return list;
    }

    static OcrEngine GetOcrEngineForLanguage(std::wstring const& langCode, std::wstring& outError, bool& outMissingPack)
    {
        outError.clear();
        outMissingPack = false;

        auto available = OcrEngine::AvailableRecognizerLanguages();
        uint32_t totalAvailable = available.Size();

        LogOcrDiagnostic(L"Resolving OCR engine for code: '" + langCode +
            L"', total recognizers available: " + std::to_wstring(totalAvailable));

        if (totalAvailable == 0)
        {
            outMissingPack = true;
            outError = L"No Windows OCR language packs are installed on this PC. "
                       L"Please install an OCR pack in Windows Settings -> Time & Language -> Language & Region (ms-settings:regionlanguage).";
            LogOcrDiagnostic(L"Error: zero available recognizers on system.");
            return nullptr;
        }

        if (langCode.empty() || langCode == L"auto")
        {
            auto engine = OcrEngine::TryCreateFromUserProfileLanguages();
            if (engine)
            {
                LogOcrDiagnostic(L"Created OCR engine from UserProfileLanguages: " +
                    std::wstring(engine.RecognizerLanguage().LanguageTag().c_str()));
                return engine;
            }

            auto firstLang = available.GetAt(0);
            LogOcrDiagnostic(L"Fallback: using first available recognizer: " +
                std::wstring(firstLang.LanguageTag().c_str()));
            return OcrEngine::TryCreateFromLanguage(firstLang);
        }

        std::wstring bcp47 = LanguageCatalog::GetBcp47Tag(langCode);

        // 1. Direct match in AvailableRecognizerLanguages
        for (uint32_t i = 0; i < totalAvailable; ++i)
        {
            auto l = available.GetAt(i);
            std::wstring tag = l.LanguageTag().c_str();

            if (_wcsicmp(tag.c_str(), bcp47.c_str()) == 0 ||
                _wcsicmp(tag.c_str(), langCode.c_str()) == 0)
            {
                auto engine = OcrEngine::TryCreateFromLanguage(l);
                if (engine)
                {
                    LogOcrDiagnostic(L"Found exact match: " + tag);
                    return engine;
                }
            }
        }

        // 2. Prefix match (e.g. "ru" matching "ru-RU", "zh" matching "zh-Hans-CN")
        for (uint32_t i = 0; i < totalAvailable; ++i)
        {
            auto l = available.GetAt(i);
            std::wstring tag = l.LanguageTag().c_str();

            if (_wcsnicmp(tag.c_str(), (langCode + L"-").c_str(), langCode.length() + 1) == 0 ||
                _wcsnicmp(tag.c_str(), (bcp47 + L"-").c_str(), bcp47.length() + 1) == 0)
            {
                auto engine = OcrEngine::TryCreateFromLanguage(l);
                if (engine)
                {
                    LogOcrDiagnostic(L"Found prefix match: " + tag + L" for request: " + langCode);
                    return engine;
                }
            }
        }

        // 3. Try WinRT Language creation
        try
        {
            Language lang(bcp47);
            if (OcrEngine::IsLanguageSupported(lang))
            {
                auto engine = OcrEngine::TryCreateFromLanguage(lang);
                if (engine)
                {
                    LogOcrDiagnostic(L"Created engine via WinRT Language tag: " + bcp47);
                    return engine;
                }
            }
        }
        catch (...) {}

        outMissingPack = true;
        outError = L"Windows OCR language pack for " + LanguageCatalog::GetDisplayName(langCode) +
            L" (" + bcp47 + L") is not installed on this PC. You can install it in Windows Settings -> Time & Language -> Language & Region (ms-settings:regionlanguage).";
        LogOcrDiagnostic(L"Missing language pack: " + outError);
        return nullptr;
    }

    winrt::fire_and_forget InternalOcrWorker(
        std::wstring langCode,
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
                uint32_t origW = decoder.PixelWidth();
                uint32_t origH = decoder.PixelHeight();

                LogOcrDiagnostic(L"Clipboard bitmap decoded: " + std::to_wstring(origW) + L"x" + std::to_wstring(origH));

                if (origW < 40 || origH < 40)
                {
                    result.success = false;
                    result.errorMessage = L"Image area is too small for OCR (" +
                        std::to_wstring(origW) + L"x" + std::to_wstring(origH) + L" px). Minimum size is 40x40 pixels.";
                }
                else
                {
                    SoftwareBitmap softwareBitmap{ nullptr };
                    if (origW > 2600 || origH > 2600)
                    {
                        BitmapTransform transform;
                        float scale = 2600.0f / static_cast<float>((std::max)(origW, origH));
                        transform.ScaledWidth(static_cast<uint32_t>(origW * scale));
                        transform.ScaledHeight(static_cast<uint32_t>(origH * scale));
                        transform.InterpolationMode(BitmapInterpolationMode::Fant);
                        softwareBitmap = co_await decoder.GetSoftwareBitmapAsync(
                            BitmapPixelFormat::Bgra8,
                            BitmapAlphaMode::Premultiplied,
                            transform,
                            ExifOrientationMode::IgnoreExifOrientation,
                            ColorManagementMode::ColorManageToSRgb);
                    }
                    else
                    {
                        softwareBitmap = co_await decoder.GetSoftwareBitmapAsync(
                            BitmapPixelFormat::Bgra8,
                            BitmapAlphaMode::Premultiplied);
                    }

                    std::wstring engineError;
                    bool missingPack = false;
                    auto ocrEngine = GetOcrEngineForLanguage(langCode, engineError, missingPack);
                    if (!ocrEngine)
                    {
                        result.success = false;
                        result.missingLanguagePack = missingPack;
                        result.errorMessage = engineError.empty() ? L"Windows OCR engine is not available." : engineError;
                    }
                    else
                    {
                        auto ocrResult = co_await ocrEngine.RecognizeAsync(softwareBitmap);
                        result.text = ocrResult.Text().c_str();
                        result.success = true;
                        LogOcrDiagnostic(L"Clipboard OCR success, recognized " +
                            std::to_wstring(result.text.length()) + L" characters.");
                    }
                }
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            result.success = false;
            result.errorMessage = L"OCR Error: " + std::wstring(ex.message().c_str());
            LogOcrDiagnostic(L"OCR HRESULT Exception: " + result.errorMessage);
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Unexpected error during OCR processing.";
            LogOcrDiagnostic(L"OCR Unknown Exception");
        }

        if (dispatcher)
        {
            dispatcher.TryEnqueue([onComplete, result]()
            {
                if (onComplete) onComplete(result);
            });
        }
        else if (onComplete)
        {
            onComplete(result);
        }
    }

    winrt::fire_and_forget InternalOcrFromBufferWorker(
        std::vector<uint8_t> bmpBytes,
        std::wstring langCode,
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
            uint32_t origW = decoder.PixelWidth();
            uint32_t origH = decoder.PixelHeight();

            LogOcrDiagnostic(L"Buffer bitmap decoded: " + std::to_wstring(origW) + L"x" + std::to_wstring(origH));

            if (origW < 40 || origH < 40)
            {
                result.success = false;
                result.errorMessage = L"Selected area is too small for OCR (" +
                    std::to_wstring(origW) + L"x" + std::to_wstring(origH) + L" px). Minimum size is 40x40 pixels.";
            }
            else
            {
                SoftwareBitmap softwareBitmap{ nullptr };
                if (origW > 2600 || origH > 2600)
                {
                    BitmapTransform transform;
                    float scale = 2600.0f / static_cast<float>((std::max)(origW, origH));
                    transform.ScaledWidth(static_cast<uint32_t>(origW * scale));
                    transform.ScaledHeight(static_cast<uint32_t>(origH * scale));
                    transform.InterpolationMode(BitmapInterpolationMode::Fant);
                    softwareBitmap = co_await decoder.GetSoftwareBitmapAsync(
                        BitmapPixelFormat::Bgra8,
                        BitmapAlphaMode::Premultiplied,
                        transform,
                        ExifOrientationMode::IgnoreExifOrientation,
                        ColorManagementMode::ColorManageToSRgb);
                }
                else
                {
                    softwareBitmap = co_await decoder.GetSoftwareBitmapAsync(
                        BitmapPixelFormat::Bgra8,
                        BitmapAlphaMode::Premultiplied);
                }

                std::wstring engineError;
                bool missingPack = false;
                auto ocrEngine = GetOcrEngineForLanguage(langCode, engineError, missingPack);
                if (!ocrEngine)
                {
                    result.success = false;
                    result.missingLanguagePack = missingPack;
                    result.errorMessage = engineError.empty() ? L"Windows OCR engine is not available." : engineError;
                }
                else
                {
                    auto ocrResult = co_await ocrEngine.RecognizeAsync(softwareBitmap);
                    result.text = ocrResult.Text().c_str();
                    result.success = true;
                    LogOcrDiagnostic(L"Buffer OCR success, recognized " +
                        std::to_wstring(result.text.length()) + L" characters.");
                }
            }
        }
        catch (winrt::hresult_error const& ex)
        {
            result.success = false;
            result.errorMessage = L"OCR Error: " + std::wstring(ex.message().c_str());
            LogOcrDiagnostic(L"Buffer OCR HRESULT Exception: " + result.errorMessage);
        }
        catch (...)
        {
            result.success = false;
            result.errorMessage = L"Unexpected error during buffer OCR processing.";
            LogOcrDiagnostic(L"Buffer OCR Unknown Exception");
        }

        if (dispatcher)
        {
            dispatcher.TryEnqueue([onComplete, result]()
            {
                if (onComplete) onComplete(result);
            });
        }
        else if (onComplete)
        {
            onComplete(result);
        }
    }

    void WindowsOcrService::RecognizeFromClipboardAsync(
        std::wstring const& langCode,
        DispatcherQueue dispatcher,
        std::function<void(OcrResult)> onComplete)
    {
        InternalOcrWorker(langCode, dispatcher, onComplete);
    }

    void WindowsOcrService::RecognizeBmpBufferAsync(
        std::vector<uint8_t> const& bmpBytes,
        std::wstring const& langCode,
        DispatcherQueue dispatcher,
        std::function<void(OcrResult)> onComplete)
    {
        InternalOcrFromBufferWorker(bmpBytes, langCode, dispatcher, onComplete);
    }
}
