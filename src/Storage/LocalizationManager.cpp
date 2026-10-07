#include "pch.h"
#include "LocalizationManager.h"
#include "SettingsManager.h"

namespace dTranslate::Storage
{
    LocalizationManager& LocalizationManager::Instance()
    {
        static LocalizationManager instance;
        return instance;
    }

    LocalizationManager::LocalizationManager()
    {
        LoadBundles();
        auto const& s = SettingsManager::Instance().GetSettings();
        if (s.appLanguage == L"ru" || s.appLanguage == L"en")
        {
            m_currentLang = s.appLanguage;
        }
        else
        {
            m_currentLang = L"en";
        }
    }

    void LocalizationManager::LoadBundles()
    {
        // 1. English (Source)
        m_enStrings = {
            // General & Brands
            { L"AppTitle", L"dTran" },
            { L"GoogleTranslate", L"Google Translate" },
            { L"GoogleTts", L"Google TTS" },
            { L"YandexTranslate", L"Yandex Translate" },
            { L"Google", L"Google" },
            { L"Yandex", L"Yandex" },

            // Popup Main View
            { L"SourceTextHeader", L"Source text" },
            { L"SourcePlaceholder", L"Enter or paste text to translate..." },
            { L"ResultPlaceholder", L"Translation will appear here..." },
            { L"Translating", L"Translating..." },
            { L"BtnTranslate", L"Translate" },
            { L"TipListen", L"Listen (TTS)" },
            { L"TipClear", L"Clear" },
            { L"TipCopy", L"Copy translation" },
            { L"TipReplace", L"Replace selected text in active window" },
            { L"TipSwapLangs", L"Swap languages" },
            { L"TipRestoreDefaults", L"Restore default languages" },
            { L"TipScreenOcr", L"Screen OCR and translate" },
            { L"TipHistory", L"Translation history" },
            { L"TipBackToTranslator", L"Back to translator" },
            { L"TipSettings", L"Settings" },
            { L"TipHide", L"Hide (Esc)" },
            { L"AutoDetect", L"Auto-detect" },
            { L"ComparisonHeader", L"--- Comparison ({0}) ---" },

            // History View
            { L"HistorySearchPlaceholder", L"Search history..." },
            { L"HistoryInsert", L"Insert" },
            { L"HistoryCopy", L"Copy" },
            { L"HistoryClear", L"Clear all" },
            { L"HistoryEmpty", L"No translations in history" },
            { L"HistoryNoResults", L"No matching results" },

            // Settings Dialog
            { L"SettingsTitle", L"dTran Settings" },
            { L"SectionGeneral", L"General" },
            { L"Theme", L"Theme" },
            { L"ThemeSystem", L"System default" },
            { L"ThemeLight", L"Light" },
            { L"ThemeDark", L"Dark" },
            { L"StartWithWindows", L"Start with Windows" },
            { L"StartupTooltipEnabled", L"dTran is configured to start automatically with Windows" },
            { L"StartupTooltipDisabled", L"Start dTran in background on Windows login" },
            { L"StartupTooltipDisabledByUser", L"Startup disabled in Windows Task Manager. Enable it in Task Manager -> Startup apps." },
            { L"StartupTooltipDisabledByPolicy", L"Startup is disabled by Windows Group Policy." },
            { L"AppLanguage", L"Application language" },
            { L"SectionLanguages", L"Languages & Translation" },
            { L"DefaultSource", L"Default source" },
            { L"DefaultTarget", L"Default target" },
            { L"CompareTranslations", L"Compare Google and Yandex translations" },
            { L"SectionHotkeys", L"Hotkeys" },
            { L"HotkeyTranslateSelected", L"Translate selected" },
            { L"HotkeyOcr", L"Screen OCR and translate" },
            { L"TipHotkeyRecord", L"Click and press key combination" },
            { L"TipResetHotkey", L"Reset to default ({0})" },
            { L"HotkeyConflict", L"Conflict: {0} is in use by another application." },
            { L"HotkeyResetSuccess", L"Hotkey reset to default." },
            { L"HotkeyCaptured", L"Captured: {0}" },
            { L"BtnClose", L"Close" },
            { L"AboutVersion", L"dTran v1.0.0" },
            { L"AboutSubtitle", L"Lightweight system translator for Google & Yandex" },
            { L"AboutCreatedBy", L"Created with ❤️ by denb" },

            { L"TrayOpenWindow", L"Open dTran" },
            { L"TrayTranslateClipboard", L"Translate Clipboard" },
            { L"TrayScreenOcr", L"Screen OCR" },
            { L"TraySettings", L"Settings" },
            { L"TrayExit", L"Exit" },
            { L"TrayTooltip", L"dTran - Quick Translator" },

            // OCR & Status
            { L"OcrCapturing", L"Select screen area to recognize..." },
            { L"OcrNoTextFound", L"No text found on screen." },
            { L"OcrFailed", L"OCR failed: {0}" },
            { L"OcrBtnTranslate", L"Translate" },
            { L"OcrBtnCopy", L"Copy text" },
            { L"OcrBtnCancel", L"Cancel" },
            { L"OcrInstruction", L"Select screen area to capture" },

            // Character Counter
            { L"CharCountSingle", L"{0} char" },
            { L"CharCountPlural", L"{0} chars" },

            // Errors & Notifications
            { L"RateLimited", L"Service is temporarily rate-limited. Please wait a moment." },
            { L"ServiceUnavailable", L"Translation service is temporarily unavailable." },
            { L"ErrorYandexUnsupportedLanguage", L"Yandex Translate does not support this language. Please use Google Translate." }
        };

        // 2. Русский (Fully Localized)
        m_ruStrings = {
            // General & Brands
            { L"AppTitle", L"dTran" },
            { L"GoogleTranslate", L"Google Translate" },
            { L"GoogleTts", L"Google TTS" },
            { L"YandexTranslate", L"Яндекс Переводчик" },
            { L"Google", L"Google" },
            { L"Yandex", L"Яндекс" },

            // Popup Main View
            { L"SourceTextHeader", L"Исходный текст" },
            { L"SourcePlaceholder", L"Введите или вставьте текст для перевода..." },
            { L"ResultPlaceholder", L"Здесь появится перевод..." },
            { L"Translating", L"Перевод..." },
            { L"BtnTranslate", L"Перевести" },
            { L"TipListen", L"Прослушать (TTS)" },
            { L"TipClear", L"Очистить" },
            { L"TipCopy", L"Копировать перевод" },
            { L"TipReplace", L"Заменить выделенный текст в активном окне" },
            { L"TipSwapLangs", L"Поменять языки местами" },
            { L"TipRestoreDefaults", L"Восстановить языки по умолчанию" },
            { L"TipScreenOcr", L"Распознать с экрана (OCR)" },
            { L"TipHistory", L"История переводов" },
            { L"TipBackToTranslator", L"Назад к переводчику" },
            { L"TipSettings", L"Настройки" },
            { L"TipHide", L"Скрыть (Esc)" },
            { L"AutoDetect", L"Автоопределение" },
            { L"ComparisonHeader", L"--- Сравнение ({0}) ---" },

            // History View
            { L"HistorySearchPlaceholder", L"Поиск по истории..." },
            { L"HistoryInsert", L"Вставить" },
            { L"HistoryCopy", L"Копировать" },
            { L"HistoryClear", L"Очистить всё" },
            { L"HistoryEmpty", L"История переводов пуста" },
            { L"HistoryNoResults", L"Ничего не найдено" },

            // Settings Dialog
            { L"SettingsTitle", L"Настройки dTran" },
            { L"SectionGeneral", L"Основные" },
            { L"Theme", L"Тема оформления" },
            { L"ThemeSystem", L"Системная" },
            { L"ThemeLight", L"Светлая" },
            { L"ThemeDark", L"Тёмная" },
            { L"StartWithWindows", L"Запускать вместе с Windows" },
            { L"StartupTooltipEnabled", L"dTran запускается автоматически вместе с Windows" },
            { L"StartupTooltipDisabled", L"Запускать dTran в фоне при входе в Windows" },
            { L"StartupTooltipDisabledByUser", L"Автозапуск отключён в Диспетчере задач Windows. Включите его на вкладке «Автозагрузка приложений»." },
            { L"StartupTooltipDisabledByPolicy", L"Автозапуск заблокирован групповой политикой Windows." },
            { L"AppLanguage", L"Язык интерфейса" },
            { L"SectionLanguages", L"Языки и перевод" },
            { L"DefaultSource", L"Язык оригинала по умолчанию" },
            { L"DefaultTarget", L"Язык перевода по умолчанию" },
            { L"CompareTranslations", L"Сравнивать перевод Google и Яндекс" },
            { L"SectionHotkeys", L"Горячие клавиши" },
            { L"HotkeyTranslateSelected", L"Перевести выделенный текст" },
            { L"HotkeyOcr", L"Распознать с экрана (OCR)" },
            { L"TipHotkeyRecord", L"Нажмите и укажите сочетание клавиш" },
            { L"TipResetHotkey", L"Сбросить по умолчанию ({0})" },
            { L"HotkeyConflict", L"Конфликт: {0} уже используется другим приложением." },
            { L"HotkeyResetSuccess", L"Горячая клавиша сброшена по умолчанию." },
            { L"HotkeyCaptured", L"Установлено: {0}" },
            { L"BtnClose", L"Закрыть" },
            { L"AboutVersion", L"dTran v1.0.0" },
            { L"AboutSubtitle", L"Лёгкий системный переводчик для Google и Яндекс" },
            { L"AboutCreatedBy", L"Created with ❤️ by denb" },

            { L"TrayOpenWindow", L"Открыть dTran" },
            { L"TrayTranslateClipboard", L"Перевести из буфера" },
            { L"TrayScreenOcr", L"Распознавание с экрана" },
            { L"TraySettings", L"Настройки" },
            { L"TrayExit", L"Выход" },
            { L"TrayTooltip", L"dTran - Быстрый переводчик" },

            // OCR & Status
            { L"OcrCapturing", L"Выделите область экрана для распознавания..." },
            { L"OcrNoTextFound", L"Текст на экране не обнаружен." },
            { L"OcrFailed", L"Ошибка распознавания: {0}" },
            { L"OcrBtnTranslate", L"Перевести" },
            { L"OcrBtnCopy", L"Копировать" },
            { L"OcrBtnCancel", L"Отмена" },
            { L"OcrInstruction", L"Выделите область экрана для захвата" },

            // Character Counter
            { L"CharCountSingle", L"{0} симв." },
            { L"CharCountPlural", L"{0} симв." },

            // Errors & Notifications
            { L"RateLimited", L"Сервис временно ограничил запросы. Пожалуйста, подождите." },
            { L"ServiceUnavailable", L"Сервис перевода временно недоступен." },
            { L"ErrorYandexUnsupportedLanguage", L"Яндекс Переводчик не поддерживает этот язык. Пожалуйста, используйте Google Translate." }
        };
    }

    void LocalizationManager::SetLanguage(std::wstring const& langCode)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (langCode == L"ru" || langCode == L"en")
        {
            if (m_currentLang != langCode)
            {
                m_currentLang = langCode;
                for (auto const& [key, cb] : m_observers)
                {
                    if (cb)
                    {
                        cb(m_currentLang);
                    }
                }
            }
        }
    }

    std::wstring LocalizationManager::Get(std::wstring const& key) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto const& map = (m_currentLang == L"ru") ? m_ruStrings : m_enStrings;
        auto it = map.find(key);
        if (it != map.end())
        {
            return it->second;
        }

        // Fallback to English bundle
        auto itEn = m_enStrings.find(key);
        if (itEn != m_enStrings.end())
        {
            return itEn->second;
        }

        return key;
    }

    std::wstring LocalizationManager::Format(std::wstring const& key, std::wstring const& arg1) const
    {
        std::wstring tpl = Get(key);
        size_t pos = tpl.find(L"{0}");
        if (pos != std::wstring::npos)
        {
            tpl.replace(pos, 3, arg1);
        }
        return tpl;
    }

    void LocalizationManager::RegisterObserver(uintptr_t key, LocalizationObserver observer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_observers[key] = observer;
    }

    void LocalizationManager::UnregisterObserver(uintptr_t key)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_observers.erase(key);
    }
}
