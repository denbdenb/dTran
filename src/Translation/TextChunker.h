#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace dTranslate::Translation
{
    struct ChunkPart
    {
        std::wstring text;
        std::wstring delimiterAfter;
    };

    class TextChunker
    {
    public:
        // Splits text into chunks smaller than or equal to maxChunkSize,
        // preserving natural paragraph, newline, and sentence boundaries.
        static std::vector<ChunkPart> Split(std::wstring_view text, size_t maxChunkSize);

        // Combines translated chunks with their respective delimiters to preserve the original structure.
        static std::wstring Combine(
            std::vector<std::wstring> const& translatedParts,
            std::vector<ChunkPart> const& originalChunks);

        static std::vector<std::pair<std::wstring, std::wstring>> SplitByDelim(
            std::wstring_view text,
            std::wstring_view delim);

        static std::vector<std::pair<std::wstring, std::wstring>> SplitIntoSentences(
            std::wstring_view text);

        static std::vector<std::pair<std::wstring, std::wstring>> SplitIntoClauses(
            std::wstring_view text);

        static std::vector<std::pair<std::wstring, std::wstring>> SplitIntoWords(
            std::wstring_view text);
    };
}
