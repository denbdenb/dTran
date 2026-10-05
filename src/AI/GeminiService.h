#pragma once
#include "AITypes.h"

namespace dTranslate::AI
{
    class GeminiService
    {
    public:
        static GeminiService& Instance();

        AIResult Execute(AIRequest const& request);

    private:
        GeminiService() = default;
    };
}
