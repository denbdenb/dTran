#pragma once
#include <string>

namespace dTranslate::Audio
{
    class GoogleTtsService
    {
    public:
        static GoogleTtsService& Instance();

        bool Speak(std::wstring const& text, std::wstring const& langCode);
        void Stop();

    private:
        GoogleTtsService() = default;
    };
}
