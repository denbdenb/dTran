#pragma once
#include "AITypes.h"

namespace dTranslate::AI
{
    class GeminiService
    {
    public:
        static GeminiService& Instance();

        AIResult Execute(AIRequest const& request);
        std::vector<std::wstring> ListModels(std::wstring const& apiKey);
        static std::vector<std::wstring> GetDefaultModels();

    private:
        GeminiService() = default;
    };
}
