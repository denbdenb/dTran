#pragma once
#include "TranslationTypes.h"
#include <winrt/Microsoft.UI.Dispatching.h>
#include <functional>

namespace dTranslate::Translation
{
    class TranslationManager
    {
    public:
        static TranslationManager& Instance();

        void TranslateAsync(
            TranslationRequest request,
            int serviceIndex,
            winrt::Microsoft::UI::Dispatching::DispatcherQueue dispatcher,
            std::function<void(TranslationResult)> onComplete);

    private:
        TranslationManager() = default;
    };
}
