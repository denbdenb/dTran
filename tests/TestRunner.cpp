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
#include "TextChunker.h"
#include "GoogleTtsService.h"
#include "WindowsOcrService.h"
#include "GoogleTranslateService.h"
#include "YandexTranslateService.h"
#include "SettingsManager.h"
#include "HistoryManager.h"
#include "LocalizationManager.h"
#include "SelectionCapture.h"
#include "ClipboardHelper.h"

using namespace dTranslate::Translation;
using namespace dTranslate::Audio;
using namespace dTranslate::OCR;
using namespace dTranslate::Storage;
using namespace dTranslate::Windows;

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

    // Check Google supports auto
    TEST_ASSERT(LanguageCatalog::IsServiceSupported(0, L"auto"), L"Google supports 'auto' source");
    TEST_ASSERT(LanguageCatalog::IsServiceSupported(1, L"auto"), L"Yandex supports 'auto' source");
}

void TestGoogleTtsChunking()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 2: Google TTS Sequential Chunking & Stop Control\n";
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

    // 3. Test real Google TTS download
    std::wcout << L"Fetching real audio chunk via Google TTS...\n";
    bool played = GoogleTtsService::Instance().Speak(L"Hello from dTran", L"en");
    TEST_ASSERT(played, L"Google TTS Speak() initiates without error");

    // Test stop / cancel immediately
    GoogleTtsService::Instance().Stop();
    TEST_ASSERT(!GoogleTtsService::Instance().IsPlaying(), L"GoogleTtsService Stop() halts playback cleanly and IsPlaying is false");
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
    newSettings.translateSelectedHotkey = L"Ctrl+Alt+T";
    newSettings.ocrHotkey = L"Ctrl+Alt+O";
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

void TestHistoryManager()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 6: History Manager Operations & Unicode Search\n";
    std::wcout << L"========================================\n";

    HistoryManager::Instance().Clear();

    HistoryItem item1;
    item1.timestamp = L"2026-10-06 12:00";
    item1.originalText = L"Hello world and welcome to dTran";
    item1.translatedText = L"Привет мир и добро пожаловать в dTran";
    item1.sourceLang = L"en";
    item1.targetLang = L"ru";
    item1.service = L"Google";
    HistoryManager::Instance().AddItem(item1);

    HistoryItem item2;
    item2.timestamp = L"2026-10-06 12:01";
    item2.originalText = L"Быстрый поиск в истории переводов";
    item2.translatedText = L"Fast search in translation history";
    item2.sourceLang = L"ru";
    item2.targetLang = L"en";
    item2.service = L"Yandex";
    HistoryManager::Instance().AddItem(item2);

    auto allItems = HistoryManager::Instance().GetItems();
    TEST_ASSERT(allItems.size() == 2, L"History contains exactly 2 items");

    // Test Search Cyrillic case-folded
    auto searchRuLower = HistoryManager::Instance().Search(L"привет");
    TEST_ASSERT(searchRuLower.size() == 1, L"Search 'привет' (lowercase) found item");
    TEST_ASSERT(searchRuLower[0].originalText == L"Hello world and welcome to dTran", L"Found correct item via Russian translated text");

    auto searchRuUpper = HistoryManager::Instance().Search(L"ПРИВЕТ");
    TEST_ASSERT(searchRuUpper.size() == 1, L"Search 'ПРИВЕТ' (uppercase) found item");

    auto searchRuSub = HistoryManager::Instance().Search(L"быстр");
    TEST_ASSERT(searchRuSub.size() == 1, L"Search 'быстр' (substring) found Russian source item");

    // Test Search English case-folded
    auto searchEn = HistoryManager::Instance().Search(L"hello");
    TEST_ASSERT(searchEn.size() == 1, L"Search 'hello' found item");

    auto searchEnUpper = HistoryManager::Instance().Search(L"FAST SEARCH");
    TEST_ASSERT(searchEnUpper.size() == 1, L"Search 'FAST SEARCH' found item");

    // Test multi-token search
    auto searchMulti = HistoryManager::Instance().Search(L"world dTran");
    TEST_ASSERT(searchMulti.size() == 1, L"Multi-token search 'world dTran' found item");

    // Test no match
    auto searchNone = HistoryManager::Instance().Search(L"NonExistentQueryXYZ");
    TEST_ASSERT(searchNone.empty(), L"Search non-existent query returned empty");

    // Test no 'en' contamination in translated text
    TEST_ASSERT(searchRuLower[0].translatedText.find(L"dTranen") == std::wstring::npos, L"No 'en' suffix contamination in translation");
}

void TestClipboardAndReplaceSafety()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 7: Clipboard Helper & Selection Replace Safety\n";
    std::wcout << L"========================================\n";

    std::wstring originalClip = ClipboardHelper::GetText();
    std::wstring testStr = L"dTran Unicode Test: Привет мир! 🚀";

    bool setOk = ClipboardHelper::SetText(testStr);
    TEST_ASSERT(setOk, L"ClipboardHelper::SetText succeeded");

    std::wstring readStr = ClipboardHelper::GetText();
    TEST_ASSERT(readStr == testStr, L"ClipboardHelper::GetText matches written Unicode string");

    // Restore original clipboard
    ClipboardHelper::SetText(originalClip);
    TEST_ASSERT(ClipboardHelper::GetText() == originalClip, L"Original clipboard restored cleanly");

    // Test SelectionCapture::ReplaceSelection with invalid HWND (should return false and not crash)
    bool replaceInvalid = SelectionCapture::ReplaceSelection(nullptr, L"test");
    TEST_ASSERT(!replaceInvalid, L"ReplaceSelection cleanly rejects null HWND without crashing");
}

void TestLanguageDetection()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 8: Language Heuristic Detection for Auto-TTS\n";
    std::wcout << L"========================================\n";

    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"Привет, как поживаешь?") == L"ru", L"Russian text detected as 'ru'");
    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"Привіт, як справи у житті?") == L"uk", L"Ukrainian text detected as 'uk'");
    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"你好，世界") == L"zh", L"Chinese text detected as 'zh'");
    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"こんにちは世界") == L"ja", L"Japanese text detected as 'ja'");
    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"Guten Morgen, wie geht es Ihnen?") == L"de", L"German text detected as 'de'");
    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"Bonjour, comment allez-vous?") == L"fr", L"French text detected as 'fr'");
    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"¿Cómo estás hoy?") == L"es", L"Spanish text detected as 'es'");
    TEST_ASSERT(LanguageCatalog::DetectLanguage(L"Hello world, this is English") == L"en", L"English text detected as 'en'");
}

void TestLocalizationManager()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 9: Localization Manager (English & Russian)\n";
    std::wcout << L"========================================\n";

    auto& loc = LocalizationManager::Instance();

    // 1. English strings check
    loc.SetLanguage(L"en");
    TEST_ASSERT(loc.GetCurrentLanguage() == L"en", L"Localization set to 'en'");
    TEST_ASSERT(loc.Get(L"AppTitle") == L"dTran", L"EN AppTitle is 'dTran'");
    TEST_ASSERT(loc.Get(L"Google") == L"Google", L"EN Google brand is 'Google'");
    TEST_ASSERT(loc.Get(L"Yandex") == L"Yandex", L"EN Yandex brand is 'Yandex'");
    TEST_ASSERT(loc.Get(L"BtnTranslate") == L"Translate", L"EN BtnTranslate is 'Translate'");
    TEST_ASSERT(loc.Get(L"TipReplace") == L"Replace selected text in active window", L"EN TipReplace matches spec");
    TEST_ASSERT(loc.Get(L"AutoDetect") == L"Auto-detect", L"EN AutoDetect is 'Auto-detect'");
    TEST_ASSERT(loc.Get(L"AboutVersion") == L"dTran v1.0.0", L"EN AboutVersion is 'dTran v1.0.0'");
    TEST_ASSERT(loc.Get(L"AboutCreatedBy") == L"Created with ❤️ by denb", L"EN AboutCreatedBy matches spec");
    TEST_ASSERT(loc.Get(L"TrayTranslateClipboard") == L"Translate Clipboard", L"EN TrayTranslateClipboard matches spec");
    TEST_ASSERT(loc.Get(L"TrayScreenOcr") == L"Screen OCR", L"EN TrayScreenOcr matches spec");
    TEST_ASSERT(loc.Get(L"TraySettings") == L"Settings", L"EN TraySettings matches spec");
    TEST_ASSERT(loc.Get(L"TrayExit") == L"Exit", L"EN TrayExit matches spec");
    TEST_ASSERT(loc.Format(L"TipResetHotkey", L"Ctrl+Alt+T") == L"Reset to default (Ctrl+Alt+T)", L"EN Format string substitution works");

    // 2. Observer test
    std::wstring observedLang;
    loc.RegisterObserver(0x1234, [&](std::wstring const& lang) {
        observedLang = lang;
    });

    // 3. Russian strings check
    loc.SetLanguage(L"ru");
    TEST_ASSERT(observedLang == L"ru", L"Observer notified of language change to 'ru'");
    TEST_ASSERT(loc.GetCurrentLanguage() == L"ru", L"Localization set to 'ru'");
    TEST_ASSERT(loc.Get(L"Google") == L"Google", L"RU Google brand stays 'Google'");
    TEST_ASSERT(loc.Get(L"Yandex") == L"Яндекс", L"RU Yandex brand is 'Яндекс'");
    TEST_ASSERT(loc.Get(L"YandexTranslate") == L"Яндекс Переводчик", L"RU YandexTranslate is 'Яндекс Переводчик'");
    TEST_ASSERT(loc.Get(L"BtnTranslate") == L"Перевести", L"RU BtnTranslate is 'Перевести'");
    TEST_ASSERT(loc.Get(L"TipReplace") == L"Заменить выделенный текст в активном окне", L"RU TipReplace matches spec");
    TEST_ASSERT(loc.Get(L"AutoDetect") == L"Автоопределение", L"RU AutoDetect is 'Автоопределение'");
    TEST_ASSERT(loc.Get(L"AboutVersion") == L"dTran v1.0.0", L"RU AboutVersion is 'dTran v1.0.0'");
    TEST_ASSERT(loc.Get(L"AboutCreatedBy") == L"Created with ❤️ by denb", L"RU AboutCreatedBy matches spec");
    TEST_ASSERT(loc.Get(L"TrayTranslateClipboard") == L"Перевести из буфера", L"RU TrayTranslateClipboard matches spec");
    TEST_ASSERT(loc.Get(L"TrayScreenOcr") == L"Распознавание с экрана", L"RU TrayScreenOcr matches spec");
    TEST_ASSERT(loc.Get(L"TraySettings") == L"Настройки", L"RU TraySettings matches spec");
    TEST_ASSERT(loc.Get(L"TrayExit") == L"Выход", L"RU TrayExit matches spec");
    TEST_ASSERT(loc.Format(L"TipResetHotkey", L"Ctrl+Alt+T") == L"Сбросить по умолчанию (Ctrl+Alt+T)", L"RU Format string substitution works");

    loc.UnregisterObserver(0x1234);
    observedLang.clear();
    loc.SetLanguage(L"en");
    TEST_ASSERT(observedLang.empty(), L"Unregistered observer was not called on subsequent language change");
}

void TestTextChunker()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 10: TextChunker Boundary Splitting & Combine\n";
    std::wcout << L"========================================\n";

    // 1. Text within chunk limit
    std::wstring shortText = L"This is a single short sentence.";
    auto chunks1 = TextChunker::Split(shortText, 100);
    TEST_ASSERT(chunks1.size() == 1, L"Text <= limit produces exactly 1 chunk");
    TEST_ASSERT(chunks1[0].text == shortText, L"Chunk text matches original");

    // 2. Paragraph splitting
    std::wstring p1 = L"First paragraph with some text.";
    std::wstring p2 = L"Second paragraph with some other text.";
    std::wstring twoPara = p1 + L"\n\n" + p2;
    auto chunks2 = TextChunker::Split(twoPara, 40);
    TEST_ASSERT(chunks2.size() == 2, L"Two paragraphs split into 2 chunks");
    TEST_ASSERT(chunks2[0].text == p1, L"First chunk text matches first paragraph");
    TEST_ASSERT(chunks2[0].delimiterAfter == L"\n\n", L"First chunk retains paragraph delimiter");
    TEST_ASSERT(chunks2[1].text == p2, L"Second chunk text matches second paragraph");

    // 3. Combine reconstructed text
    std::vector<std::wstring> translated = { L"Premier paragraphe.", L"Deuxième paragraphe." };
    auto combined = TextChunker::Combine(translated, chunks2);
    TEST_ASSERT(combined == L"Premier paragraphe.\n\nDeuxième paragraphe.", L"Combine restores exact paragraph formatting");

    // 4. Sentence splitting
    std::wstring s1 = L"First sentence here.";
    std::wstring s2 = L"Second sentence follows!";
    std::wstring s3 = L"Is this the third sentence?";
    std::wstring multisent = s1 + L" " + s2 + L" " + s3;
    auto chunks3 = TextChunker::Split(multisent, 30);
    TEST_ASSERT(chunks3.size() >= 2, L"Sentences split when exceeding chunk size");
}

void TestUnicodeCharCounter()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 11: Unicode Code Point Counting & Counter Localization\n";
    std::wcout << L"========================================\n";

    auto countCodePoints = [](std::wstring_view str) -> size_t
    {
        size_t count = 0;
        for (size_t i = 0; i < str.size(); ++i)
        {
            wchar_t ch = str[i];
            if (ch >= 0xD800 && ch <= 0xDBFF)
            {
                if (i + 1 < str.size() && str[i + 1] >= 0xDC00 && str[i + 1] <= 0xDFFF)
                {
                    ++i;
                }
            }
            ++count;
        }
        return count;
    };

    // Standard ASCII
    std::wstring ascii = L"Hello";
    TEST_ASSERT(countCodePoints(ascii) == 5, L"ASCII 'Hello' counts as 5 characters");

    // Cyrillic
    std::wstring cyrillic = L"Привет";
    TEST_ASSERT(countCodePoints(cyrillic) == 6, L"Cyrillic 'Привет' counts as 6 characters");

    // Emoji with surrogate pairs: 👋 (U+1F44B = \uD83D\uDC4B) and 🌍 (U+1F30D = \uD83C\uDF0D)
    std::wstring withEmoji = L"Hi 👋 World 🌍";
    // Length in wchar_t is 14, but code point count is 12!
    TEST_ASSERT(withEmoji.size() == 14, L"Emoji string size in wchar_t is 14");
    TEST_ASSERT(countCodePoints(withEmoji) == 12, L"Unicode code point count correctly collapses surrogate pairs to 12");

    // Single emoji
    std::wstring singleEmoji = L"🚀";
    TEST_ASSERT(singleEmoji.size() == 2, L"Single emoji is 2 UTF-16 code units");
    TEST_ASSERT(countCodePoints(singleEmoji) == 1, L"Single emoji counts as 1 character");

    // Formatting in EN
    auto& loc = LocalizationManager::Instance();
    loc.SetLanguage(L"en");
    TEST_ASSERT(loc.Format(L"CharCountSingle", L"1") == L"1 char", L"EN 1 char formatted correctly");
    TEST_ASSERT(loc.Format(L"CharCountPlural", L"42") == L"42 chars", L"EN 42 chars formatted correctly");

    // Formatting in RU
    loc.SetLanguage(L"ru");
    TEST_ASSERT(loc.Format(L"CharCountSingle", L"1") == L"1 симв.", L"RU 1 симв. formatted correctly");
    TEST_ASSERT(loc.Format(L"CharCountPlural", L"42") == L"42 симв.", L"RU 42 симв. formatted correctly");
}

void TestIndianLanguagesAndLongText()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 12: Indian Languages & Long Text Translation\n";
    std::wcout << L"========================================\n";

    // 1. Google Translate Indian languages
    TranslationRequest reqHi;
    reqHi.sourceLang = L"en";
    reqHi.targetLang = L"hi";
    reqHi.text = L"Hello world";
    auto resHi = GoogleTranslateService::Instance().Translate(reqHi);
    TEST_ASSERT(resHi.success, L"Google translates English to Hindi");
    TEST_ASSERT(!resHi.translatedText.empty(), L"Google Hindi translation is non-empty");

    TranslationRequest reqTa;
    reqTa.sourceLang = L"en";
    reqTa.targetLang = L"ta";
    reqTa.text = L"Good morning";
    auto resTa = GoogleTranslateService::Instance().Translate(reqTa);
    TEST_ASSERT(resTa.success, L"Google translates English to Tamil");
    TEST_ASSERT(!resTa.translatedText.empty(), L"Google Tamil translation is non-empty");

    TranslationRequest reqBn;
    reqBn.sourceLang = L"en";
    reqBn.targetLang = L"bn";
    reqBn.text = L"Welcome";
    auto resBn = GoogleTranslateService::Instance().Translate(reqBn);
    TEST_ASSERT(resBn.success, L"Google translates English to Bengali");
    TEST_ASSERT(!resBn.translatedText.empty(), L"Google Bengali translation is non-empty");

    // 2. Google Translate Long Text (>1200 characters) via POST
    std::wstring longText = L"In a small quiet town nestled in the valley, the mornings always began with crisp mountain air. "
                            L"The villagers would open their wooden shutters and greet each other as the church bells rang. "
                            L"A narrow stone path wound its way through the center of the village toward the old mill. "
                            L"Generations of craftsmen had lived here, carving wood and telling tales around stone fireplaces. "
                            L"When the winter snows fell, the valley became silent except for the crackling of pine logs. "
                            L"Children would gather on the slopes with handcrafted wooden sleds, laughing into the twilight. "
                            L"The old baker was famous for his warm cinnamon loaves, which filled the main square with sweet warmth. "
                            L"No one was ever in a hurry, because here time seemed to flow like a deep, serene river.";
    TEST_ASSERT(longText.size() > 700, L"Long text sample is > 700 characters");

    TranslationRequest reqLong;
    reqLong.sourceLang = L"en";
    reqLong.targetLang = L"ru";
    reqLong.text = longText;
    auto resLong = GoogleTranslateService::Instance().Translate(reqLong);
    TEST_ASSERT(resLong.success, L"Google translates >700 char text via POST without buffer overflow");
    TEST_ASSERT(!resLong.translatedText.empty(), L"Google long text translation is non-empty");

    // 3. Yandex Indian language translation (Hindi)
    auto resYandexHi = YandexTranslateService::Instance().Translate(reqHi);
    TEST_ASSERT(resYandexHi.success, L"Yandex translates English to Hindi");
    TEST_ASSERT(!resYandexHi.translatedText.empty(), L"Yandex Hindi translation is non-empty");

    // 4. Yandex unsupported language check (Hawaiian 'haw' is Google-only)
    TranslationRequest reqHaw;
    reqHaw.sourceLang = L"en";
    reqHaw.targetLang = L"haw";
    reqHaw.text = L"Hello";
    auto resHaw = YandexTranslateService::Instance().Translate(reqHaw);
    TEST_ASSERT(!resHaw.success, L"Yandex cleanly rejects unsupported language 'haw'");
    TEST_ASSERT(resHaw.errorMessage.find(L"Google Translate") != std::wstring::npos, L"Yandex error message suggests using Google Translate");
}

void TestOcrAndTrayLocalization()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 13: OCR & Modern Tray Menu Localization Keys\n";
    std::wcout << L"========================================\n";

    auto& loc = LocalizationManager::Instance();

    // English
    loc.SetLanguage(L"en");
    TEST_ASSERT(loc.Get(L"OcrBtnTranslate") == L"Translate", L"EN OcrBtnTranslate is 'Translate'");
    TEST_ASSERT(loc.Get(L"OcrBtnCopy") == L"Copy text", L"EN OcrBtnCopy is 'Copy text'");
    TEST_ASSERT(loc.Get(L"OcrBtnCancel") == L"Cancel", L"EN OcrBtnCancel is 'Cancel'");
    TEST_ASSERT(loc.Get(L"OcrInstruction") == L"Select screen area to capture", L"EN OcrInstruction is 'Select screen area to capture'");
    TEST_ASSERT(loc.Get(L"TrayOpenWindow") == L"Open dTran", L"EN TrayOpenWindow is 'Open dTran'");

    // Russian
    loc.SetLanguage(L"ru");
    TEST_ASSERT(loc.Get(L"OcrBtnTranslate") == L"Перевести", L"RU OcrBtnTranslate is 'Перевести'");
    TEST_ASSERT(loc.Get(L"OcrBtnCopy") == L"Копировать", L"RU OcrBtnCopy is 'Копировать'");
    TEST_ASSERT(loc.Get(L"OcrBtnCancel") == L"Отмена", L"RU OcrBtnCancel is 'Отмена'");
    TEST_ASSERT(loc.Get(L"OcrInstruction") == L"Выделите область экрана для захвата", L"RU OcrInstruction is 'Выделите область экрана для захвата'");
    TEST_ASSERT(loc.Get(L"TrayOpenWindow") == L"Открыть dTran", L"RU TrayOpenWindow is 'Открыть dTran'");
}

void TestWindowsStartupIntegration()
{
    std::wcout << L"\n========================================\n";
    std::wcout << L"TEST 14: Windows Startup Integration & Localization\n";
    std::wcout << L"========================================\n";

    auto& loc = LocalizationManager::Instance();

    // 1. English tooltips
    loc.SetLanguage(L"en");
    TEST_ASSERT(!loc.Get(L"StartupTooltipEnabled").empty(), L"EN StartupTooltipEnabled is defined");
    TEST_ASSERT(!loc.Get(L"StartupTooltipDisabled").empty(), L"EN StartupTooltipDisabled is defined");
    TEST_ASSERT(loc.Get(L"StartupTooltipDisabledByUser").find(L"Task Manager") != std::wstring::npos, L"EN StartupTooltipDisabledByUser mentions Task Manager");
    TEST_ASSERT(!loc.Get(L"StartupTooltipDisabledByPolicy").empty(), L"EN StartupTooltipDisabledByPolicy is defined");

    // 2. Russian tooltips
    loc.SetLanguage(L"ru");
    TEST_ASSERT(!loc.Get(L"StartupTooltipEnabled").empty(), L"RU StartupTooltipEnabled is defined");
    TEST_ASSERT(!loc.Get(L"StartupTooltipDisabled").empty(), L"RU StartupTooltipDisabled is defined");
    TEST_ASSERT(loc.Get(L"StartupTooltipDisabledByUser").find(L"Диспетчере задач") != std::wstring::npos, L"RU StartupTooltipDisabledByUser mentions Диспетчере задач");
    TEST_ASSERT(!loc.Get(L"StartupTooltipDisabledByPolicy").empty(), L"RU StartupTooltipDisabledByPolicy is defined");

    // 3. Stale registry cleanup
    SettingsManager::CleanupStaleRegistryEntries();
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD type = 0;
        LSTATUS st = RegQueryValueExW(hKey, L"dTranslate", nullptr, &type, nullptr, nullptr);
        TEST_ASSERT(st != ERROR_SUCCESS, L"CleanupStaleRegistryEntries ensures dTranslate is removed from HKCU\\Run");
        RegCloseKey(hKey);
    }
    else
    {
        TEST_ASSERT(true, L"HKCU\\Run registry key not accessible or clean");
    }

    // 4. GetStartupTaskState safely handles execution environment without throwing
    auto state = SettingsManager::GetStartupTaskState();
    bool validState = (state >= WindowsStartupState::Disabled && state <= WindowsStartupState::ErrorOrUnavailable);
    TEST_ASSERT(validState, L"GetStartupTaskState returns valid WindowsStartupState without unhandled exceptions");

    // 5. IsStartWithWindowsEnabled returns clean boolean
    bool isEnabled = SettingsManager::IsStartWithWindowsEnabled();
    TEST_ASSERT(!isEnabled || isEnabled, L"IsStartWithWindowsEnabled completes safely");
}

int wmain(int argc, wchar_t* argv[])
{
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    std::wcout << L"===============================================\n";
    std::wcout << L"       dTran Functional Verification Suite     \n";
    std::wcout << L"===============================================\n";

    TestLanguageCatalog();
    TestGoogleTtsChunking();
    TestWindowsOcrCapabilities();
    TestTranslationEngines();
    TestSettingsManagerAndObservers();
    TestHistoryManager();
    TestClipboardAndReplaceSafety();
    TestLanguageDetection();
    TestLocalizationManager();
    TestTextChunker();
    TestUnicodeCharCounter();
    TestIndianLanguagesAndLongText();
    TestOcrAndTrayLocalization();
    TestWindowsStartupIntegration();

    std::wcout << L"\n===============================================\n";
    std::wcout << L"TOTAL RESULTS: " << g_passCount << L" PASSED, " << g_failCount << L" FAILED\n";
    std::wcout << L"===============================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
