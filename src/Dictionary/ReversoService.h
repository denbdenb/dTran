#pragma once
#include "DictionaryTypes.h"

namespace dTranslate::Dictionary
{
    class ReversoService
    {
    public:
        static ReversoService& Instance();

        DictionaryResult Lookup(DictionaryRequest const& request);

    private:
        ReversoService() = default;
        static std::wstring MapLanguageCode(std::wstring const& langCode);
    };
}
