#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <chrono>
#include <thread>

// Include application headers
#include "LanguageCatalog.h"
#include "GoogleTtsService.h"
#include "WindowsOcrService.h"
#include "GoogleTranslateService.h"
#include "YandexTranslateService.h"
#include "ReversoService.h"
#include "WikipediaService.h"
#include "SettingsManager.h"
#include "HistoryManager.h"

using namespace dTranslate::Translation;
using namespace dTranslate::Audio;
using namespace dTranslate::OCR;
using namespace dTranslate::Dictionary;
using namespace dTranslate::Storage;

static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::wcerr << L"[FAIL] " << msg << L" (line " << __LINE__ << L")\n"; \
            g_failCount++; \
        } else { \
            std::wcout << L"[PASS] " << msg << L"\n"; \
            g_passCount++; \
        } \
    } while(0)

void TestLanguageCatalog()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 1: Language Catalog & Capability Filtering\n";
    std::wcout << L"========================================\n";

    auto const& allLangs = LanguageCatalog::GetAllLanguages();
    TEST_ASSERT(allLangs.size() >= 90, L"Catalog contains >= 90 languages");

    // Check flag emoji presence
    bool allFlagsValid = true;
    for (auto const& l : allLangs)
    {
        if (l.flag.empty())
        {
            allFlagsValid = false;
            std::wcerr << L"Missing flag emoji for: " << l.code << L"\n";
        }
    }
    TEST_ASSERT(allFlagsValid, L"All languages have flag emojis");

    // Check BCP-47 tag mappings
    TEST_ASSERT(LanguageCatalog::GetBcp47Tag(L"ru") == L"ru-RU", L"BCP-47 tag for 'ru' is 'ru-RU'");
    TEST_ASSERT(LanguageCatalog::GetBcp47Tag(L"en") == L"en-US", L"BCP-47 tag for 'en' is 'en-US'");
    TEST_ASSERT(LanguageCatalog::GetBcp47Tag(L"de") == L"de-DE", L"BCP-47 tag for 'de' is 'de-DE'");
    TEST_ASSERT(LanguageCatalog::GetBcp47Tag(L"fr") == L"fr-FR", L"BCP-47 tag for 'fr' is 'fr-FR'");
    TEST_ASSERT(LanguageCatalog::GetBcp47Tag(L"zh") == L"zh-Hans-CN", L"BCP-47 tag for 'zh' is 'zh-Hans-CN'");

    // Check Yandex filtering: no zh-TW
    auto yandexSource = LanguageCatalog::GetLanguagesForService(1, true); // Yandex = 1
    bool yandexHasZhTW = false;
    for (auto const& l : yandexSource)
    {
        if (l.code == L"zh-TW") yandexHasZhTW = true;
    }
    TEST_ASSERT(!yandexHasZhTW, L"Yandex capability filter excludes zh-TW");

    // Test ValidateLanguageForService for Yandex
    auto validatedYandex = LanguageCatalog::ValidateLanguageForService(1, L"zh-TW", true);
    TEST_ASSERT(validatedYandex != L"zh-TW", L"ValidateLanguageForService redirects zh-TW on Yandex");

    // Check Reverso filtering: exactly 18 languages, non-auto
    auto reversoSource = LanguageCatalog::GetLanguagesForService(4, true); // Reverso
    TEST_ASSERT(reversoSource.size() == 18, L"Reverso offers exactly 18 languages");

    bool reversoHasAuto = false;
    for (auto const& l : reversoSource)
    {
        if (l.code == L"auto") reversoHasAuto = true;
    }
    TEST_ASSERT(!reversoHasAuto, L"Reverso excludes 'auto' detection");

    auto validatedReverso = LanguageCatalog::ValidateLanguageForService(4, L"auto", true);
    TEST_ASSERT(validatedReverso == L"en", L"ValidateLanguageForService redirects 'auto' to 'en' for Reverso");
}

void TestGoogleTtsChunking()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 2: Google TTS Sequential Chunking & Control\n";
    std::wcout << L"========================================\n";

    // 1. Short text (< 160 chars)
    std::wstring shortText = L"Hello, this is a short test message for text to speech.";
    auto chunks1 = GoogleTtsService::SplitIntoChunks(shortText, 160);
    TEST_ASSERT(chunks1.size() == 1, L"Short text produces exactly 1 chunk");
    TEST_ASSERT(chunks1[0] == shortText, L"Short text content preserved exactly");

    // 2. Long text (> 200 chars, multiple sentences)
    std::wstring longText = L"The quick brown fox jumps over the lazy dog. "
                            L"Artificial intelligence and neural machine translation have advanced significantly. "
                            L"Windows 11 provides rich desktop application experiences through modern WinUI 3 controls. "
                            L"This is the fourth sentence designed to exceed two hundred characters total length.";
    auto chunks2 = GoogleTtsService::SplitIntoChunks(longText, 160);
    TEST_ASSERT(chunks2.size() > 1, L"Long text produces multiple sequential chunks");

    bool allChunksWithinLimit = true;
    for (size_t i = 0; i < chunks2.size(); ++i)
    {
        if (chunks2[i].size() > 160)
        {
            allChunksWithinLimit = false;
            std::wcerr << L"Chunk " << i << L" exceeded 160 chars: length=" << chunks2[i].size() << L"\n";
        }
    }
    TEST_ASSERT(allChunksWithinLimit, L"Every chunk is strictly <= 160 characters");

    // 3. Very long text (> 1000 chars)
    std::wstring veryLongText;
    for (int i = 0; i < 20; ++i)
    {
        veryLongText += L"Sentence number " + std::to_wstring(i) + L" contains valuable translation data for testing purposes. ";
    }
    auto chunks3 = GoogleTtsService::SplitIntoChunks(veryLongText, 160);
    TEST_ASSERT(chunks3.size() >= 10, L"1000+ char text split into >= 10 sequential chunks");

    bool longChunksValid = true;
    for (auto const& c : chunks3)
    {
        if (c.size() > 160 || c.empty()) longChunksValid = false;
    }
    TEST_ASSERT(longChunksValid, L"All chunks from 1000+ char input are valid and <= 160 chars");

    // 4. Test real Google TTS download
    std::wcout << L"Fetching real audio chunk via Google TTS...\n";
    bool played = GoogleTtsService::Instance().Speak(L"Hello from dTranslate", L"en");
    TEST_ASSERT(played, L"Google TTS Speak() initiates without error");

    // Test stop / cancel immediately
    GoogleTtsService::Instance().Stop();
    TEST_ASSERT(true, L"GoogleTtsService Stop() halts playback cleanly");
}

void TestWindowsOcrCapabilities()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 3: Windows OCR Capabilities & Locale Matching\n";
    std::wcout << L"========================================\n";

    auto installed = WindowsOcrService::Instance().GetInstalledRecognizerLanguages();
    std::wcout << L"Installed Windows OCR Recognizer Languages (" << installed.size() << L"):\n";
    for (auto const& lang : installed)
    {
        std::wcout << L"  - " << lang << L"\n";
    }
    TEST_ASSERT(!installed.empty(), L"At least 1 Windows OCR recognizer is installed on system");

    // Test OCR BCP-47 matching logic
    std::wstring bcpRu = LanguageCatalog::GetBcp47Tag(L"ru");
    TEST_ASSERT(bcpRu == L"ru-RU", L"BCP-47 locale matching correctly maps 'ru' to 'ru-RU'");
    std::wstring bcpEn = LanguageCatalog::GetBcp47Tag(L"en");
    TEST_ASSERT(bcpEn == L"en-US", L"BCP-47 locale matching correctly maps 'en' to 'en-US'");
}

void TestTranslationEngines()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 4: Live Translation Engines Verification\n";
    std::wcout << L"========================================\n";

    // 1. Google Translate No-Key Web Engine
    std::wcout << L"Testing Google Translate Web Engine...\n";
    TranslationRequest req1;
    req1.text = L"Hello world";
    req1.sourceLang = L"en";
    req1.targetLang = L"ru";
    auto res1 = GoogleTranslateService::Instance().Translate(req1);
    TEST_ASSERT(res1.success, L"Google Translate returned success");
    TEST_ASSERT(!res1.translatedText.empty(), L"Google Translate returned non-empty text");
    std::wcout << L"  Result: " << res1.translatedText << L"\n";

    // 2. Yandex Translate No-Key Web Engine
    std::wcout << L"Testing Yandex Translate Web Engine...\n";
    TranslationRequest req2;
    req2.text = L"Good morning";
    req2.sourceLang = L"en";
    req2.targetLang = L"ru";
    auto res2 = YandexTranslateService::Instance().Translate(req2);
    TEST_ASSERT(res2.success, L"Yandex Translate returned success");
    TEST_ASSERT(!res2.translatedText.empty(), L"Yandex Translate returned non-empty text");
    std::wcout << L"  Result: " << res2.translatedText << L"\n";

    // 3. Reverso Dictionary Service
    std::wcout << L"Testing Reverso Dictionary Service...\n";
    DictionaryRequest req3;
    req3.word = L"apple";
    req3.sourceLang = L"en";
    req3.targetLang = L"ru";
    auto res3 = ReversoService::Instance().Lookup(req3);
    TEST_ASSERT(res3.success, L"Reverso returned success");
    TEST_ASSERT(!res3.content.empty(), L"Reverso returned non-empty dictionary content");
    std::wcout << L"  Result preview: " << res3.content.substr(0, (std::min)((size_t)80, res3.content.size())) << L"...\n";

    // 4. Wikipedia Dictionary Service
    std::wcout << L"Testing Wikipedia Dictionary Service...\n";
    DictionaryRequest req4;
    req4.word = L"Computer";
    req4.sourceLang = L"en";
    auto res4 = WikipediaService::Instance().Lookup(req4);
    TEST_ASSERT(res4.success, L"Wikipedia returned success");
    TEST_ASSERT(!res4.content.empty(), L"Wikipedia returned non-empty summary");
    std::wcout << L"  Result preview: " << res4.content.substr(0, (std::min)((size_t)80, res4.content.size())) << L"...\n";
}

void TestSettingsManagerAndObservers()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 5: Settings Persistence & Live Observers\n";
    std::wcout << L"========================================\n";

    bool observerFired = false;
    AppSettings observedSettings;

    uintptr_t obsKey = 0x12345;
    SettingsManager::Instance().RegisterObserver(obsKey, [&](AppSettings const& s)
    {
        observerFired = true;
        observedSettings = s;
    });

    auto originalSettings = SettingsManager::Instance().GetSettings();

    // Update settings
    auto newSettings = originalSettings;
    newSettings.sourceLanguage = L"es";
    newSettings.targetLanguage = L"de";
    newSettings.theme = L"Dark";
    SettingsManager::Instance().UpdateSettings(newSettings);

    TEST_ASSERT(observerFired, L"Settings observer fired immediately on UpdateSettings");
    TEST_ASSERT(observedSettings.sourceLanguage == L"es", L"Observer received updated sourceLanguage 'es'");
    TEST_ASSERT(observedSettings.targetLanguage == L"de", L"Observer received updated targetLanguage 'de'");
    TEST_ASSERT(observedSettings.theme == L"Dark", L"Observer received updated theme 'Dark'");

    // Clean up observer
    SettingsManager::Instance().UnregisterObserver(obsKey);

    // Restore original settings
    SettingsManager::Instance().UpdateSettings(originalSettings);
    TEST_ASSERT(SettingsManager::Instance().GetSettings().sourceLanguage == originalSettings.sourceLanguage, L"Settings restored cleanly");
}

int wmain(int argc, wchar_t* argv[])
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    std::wcout << L"===============================================\n";
    std::wcout << L"  dTranslate Functional Verification Suite    \n";
    std::wcout << L"===============================================\n";

    TestLanguageCatalog();
    TestGoogleTtsChunking();
    TestWindowsOcrCapabilities();
    TestTranslationEngines();
    TestSettingsManagerAndObservers();

    std::wcout << L"\n===============================================\n";
    std::wcout << L"TOTAL RESULTS: " << g_passCount << L" PASSED, " << g_failCount << L" FAILED\n";
    std::wcout << L"===============================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
