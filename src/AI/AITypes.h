#pragma once
#include <string>

namespace dTranslate::AI
{
    enum class AIOperation
    {
        Translate,
        Rewrite,
        Improve,
        Summarize,
        Explain
    };

    struct AIRequest
    {
        AIOperation operation{ AIOperation::Translate };
        std::wstring text;
        std::wstring targetLang{ L"English" };
    };

    struct AIResult
    {
        bool success{ false };
        std::wstring text;
        std::wstring serviceName;
        std::wstring errorMessage;
    };

    inline std::wstring GetSystemInstruction(AIOperation op, std::wstring const& targetLang)
    {
        switch (op)
        {
        case AIOperation::Translate:
            return L"Translate the following text into " + targetLang + L". Return only the translated text without preamble.";
        case AIOperation::Rewrite:
            return L"Rewrite the following text clearly, naturally, and concisely in the same language. Preserve the original meaning.";
        case AIOperation::Improve:
            return L"Improve the grammar, flow, tone, and clarity of the following text. Preserve the original language and meaning.";
        case AIOperation::Summarize:
            return L"Summarize the key ideas and points of the following text concisely.";
        case AIOperation::Explain:
            return L"Explain the meaning, idioms, nuances, and cultural context of the following text in depth.";
        default:
            return L"Process the following text:";
        }
    }
}
