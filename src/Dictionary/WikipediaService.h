#pragma once
#include "DictionaryTypes.h"

namespace dTranslate::Dictionary
{
    class WikipediaService
    {
    public:
        static WikipediaService& Instance();

        DictionaryResult Lookup(DictionaryRequest const& request);

    private:
        WikipediaService() = default;
    };
}
