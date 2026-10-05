#pragma once
#include "AITypes.h"

namespace dTranslate::AI
{
    class OpenAIService
    {
    public:
        static OpenAIService& Instance();

        AIResult Execute(AIRequest const& request);

    private:
        OpenAIService() = default;
    };
}
