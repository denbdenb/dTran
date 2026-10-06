#pragma once
#include "AITypes.h"

namespace dTranslate::AI
{
    class OpenAIService
    {
    public:
        static OpenAIService& Instance();

        AIResult Execute(AIRequest const& request);
        std::vector<std::wstring> ListModels(std::wstring const& apiKey);
        static std::vector<std::wstring> GetDefaultModels();

    private:
        OpenAIService() = default;
    };
}
