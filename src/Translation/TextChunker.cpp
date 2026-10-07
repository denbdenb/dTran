#include "pch.h"
#include "TextChunker.h"
#include <algorithm>

namespace dTranslate::Translation
{
    std::vector<std::pair<std::wstring, std::wstring>> TextChunker::SplitByDelim(
        std::wstring_view text,
        std::wstring_view delim)
    {
        std::vector<std::pair<std::wstring, std::wstring>> parts;
        size_t start = 0;
        while (start < text.size())
        {
            size_t pos = text.find(delim, start);
            if (pos == std::wstring_view::npos)
            {
                parts.push_back({ std::wstring(text.substr(start)), L"" });
                break;
            }
            parts.push_back({ std::wstring(text.substr(start, pos - start)), std::wstring(delim) });
            start = pos + delim.size();
        }
        return parts;
    }

    std::vector<std::pair<std::wstring, std::wstring>> TextChunker::SplitIntoSentences(
        std::wstring_view text)
    {
        std::vector<std::pair<std::wstring, std::wstring>> sentences;
        size_t start = 0;
        const wchar_t* punc = L".!?\u2026"; // '.', '!', '?', '…'

        for (size_t i = 0; i < text.size(); ++i)
        {
            if (wcschr(punc, text[i]) != nullptr)
            {
                // Include consecutive punctuation marks (e.g. "?!", "...")
                size_t pEnd = i;
                while (pEnd + 1 < text.size() && wcschr(punc, text[pEnd + 1]) != nullptr)
                {
                    ++pEnd;
                }

                // Check if followed by whitespace or end of text
                if (pEnd + 1 == text.size() || iswspace(text[pEnd + 1]))
                {
                    size_t wsStart = pEnd + 1;
                    size_t wsEnd = wsStart;
                    while (wsEnd < text.size() && iswspace(text[wsEnd]))
                    {
                        ++wsEnd;
                    }

                    std::wstring sentence(text.substr(start, (pEnd + 1) - start));
                    std::wstring delim(text.substr(wsStart, wsEnd - wsStart));
                    sentences.push_back({ sentence, delim });
                    start = wsEnd;
                    i = wsEnd - 1;
                }
            }
        }

        if (start < text.size())
        {
            sentences.push_back({ std::wstring(text.substr(start)), L"" });
        }

        return sentences;
    }

    std::vector<std::pair<std::wstring, std::wstring>> TextChunker::SplitIntoClauses(
        std::wstring_view text)
    {
        std::vector<std::pair<std::wstring, std::wstring>> clauses;
        size_t start = 0;

        for (size_t i = 0; i < text.size(); ++i)
        {
            wchar_t ch = text[i];
            if (ch == L',' || ch == L';' || ch == L':' || ch == L'\u2014' || ch == L'-')
            {
                if (i + 1 < text.size() && iswspace(text[i + 1]))
                {
                    size_t wsStart = i + 1;
                    size_t wsEnd = wsStart;
                    while (wsEnd < text.size() && iswspace(text[wsEnd]))
                    {
                        ++wsEnd;
                    }

                    std::wstring clause(text.substr(start, (i + 1) - start));
                    std::wstring delim(text.substr(wsStart, wsEnd - wsStart));
                    clauses.push_back({ clause, delim });
                    start = wsEnd;
                    i = wsEnd - 1;
                }
            }
        }

        if (start < text.size())
        {
            clauses.push_back({ std::wstring(text.substr(start)), L"" });
        }

        return clauses;
    }

    std::vector<std::pair<std::wstring, std::wstring>> TextChunker::SplitIntoWords(
        std::wstring_view text)
    {
        std::vector<std::pair<std::wstring, std::wstring>> words;
        size_t start = 0;

        for (size_t i = 0; i < text.size(); ++i)
        {
            if (iswspace(text[i]))
            {
                size_t wsStart = i;
                size_t wsEnd = wsStart;
                while (wsEnd < text.size() && iswspace(text[wsEnd]))
                {
                    ++wsEnd;
                }

                std::wstring word(text.substr(start, wsStart - start));
                std::wstring delim(text.substr(wsStart, wsEnd - wsStart));
                words.push_back({ word, delim });
                start = wsEnd;
                i = wsEnd - 1;
            }
        }

        if (start < text.size())
        {
            words.push_back({ std::wstring(text.substr(start)), L"" });
        }

        return words;
    }

    // Helper to recursively break down a unit into safe subchunks
    static void RecursivelySubdivide(
        std::wstring const& unitText,
        std::wstring const& unitDelim,
        size_t maxChunkSize,
        std::vector<ChunkPart>& outChunks)
    {
        if (unitText.empty())
        {
            if (!unitDelim.empty() && !outChunks.empty())
            {
                outChunks.back().delimiterAfter += unitDelim;
            }
            return;
        }

        if (unitText.size() <= maxChunkSize)
        {
            outChunks.push_back({ unitText, unitDelim });
            return;
        }

        // 1. Try sentence splitting
        auto sentences = TextChunker::SplitIntoSentences(unitText);
        if (sentences.size() > 1)
        {
            std::wstring cur;
            std::wstring curDelim;
            for (size_t i = 0; i < sentences.size(); ++i)
            {
                auto const& [sText, sDelim] = sentences[i];
                if (sText.size() > maxChunkSize)
                {
                    if (!cur.empty())
                    {
                        outChunks.push_back({ cur, curDelim });
                        cur.clear();
                        curDelim.clear();
                    }
                    RecursivelySubdivide(sText, sDelim, maxChunkSize, outChunks);
                }
                else if (cur.size() + curDelim.size() + sText.size() <= maxChunkSize)
                {
                    cur += curDelim + sText;
                    curDelim = sDelim;
                }
                else
                {
                    if (!cur.empty())
                    {
                        outChunks.push_back({ cur, curDelim });
                    }
                    cur = sText;
                    curDelim = sDelim;
                }
            }
            if (!cur.empty())
            {
                outChunks.push_back({ cur, curDelim + unitDelim });
            }
            return;
        }

        // 2. Try clause splitting
        auto clauses = TextChunker::SplitIntoClauses(unitText);
        if (clauses.size() > 1)
        {
            std::wstring cur;
            std::wstring curDelim;
            for (size_t i = 0; i < clauses.size(); ++i)
            {
                auto const& [cText, cDelim] = clauses[i];
                if (cText.size() > maxChunkSize)
                {
                    if (!cur.empty())
                    {
                        outChunks.push_back({ cur, curDelim });
                        cur.clear();
                        curDelim.clear();
                    }
                    RecursivelySubdivide(cText, cDelim, maxChunkSize, outChunks);
                }
                else if (cur.size() + curDelim.size() + cText.size() <= maxChunkSize)
                {
                    cur += curDelim + cText;
                    curDelim = cDelim;
                }
                else
                {
                    if (!cur.empty())
                    {
                        outChunks.push_back({ cur, curDelim });
                    }
                    cur = cText;
                    curDelim = cDelim;
                }
            }
            if (!cur.empty())
            {
                outChunks.push_back({ cur, curDelim + unitDelim });
            }
            return;
        }

        // 3. Try word splitting
        auto words = TextChunker::SplitIntoWords(unitText);
        if (words.size() > 1)
        {
            std::wstring cur;
            std::wstring curDelim;
            for (size_t i = 0; i < words.size(); ++i)
            {
                auto const& [wText, wDelim] = words[i];
                if (wText.size() > maxChunkSize)
                {
                    if (!cur.empty())
                    {
                        outChunks.push_back({ cur, curDelim });
                        cur.clear();
                        curDelim.clear();
                    }
                    RecursivelySubdivide(wText, wDelim, maxChunkSize, outChunks);
                }
                else if (cur.size() + curDelim.size() + wText.size() <= maxChunkSize)
                {
                    cur += curDelim + wText;
                    curDelim = wDelim;
                }
                else
                {
                    if (!cur.empty())
                    {
                        outChunks.push_back({ cur, curDelim });
                    }
                    cur = wText;
                    curDelim = wDelim;
                }
            }
            if (!cur.empty())
            {
                outChunks.push_back({ cur, curDelim + unitDelim });
            }
            return;
        }

        // 4. Emergency hard limit fallback
        size_t pos = 0;
        while (pos < unitText.size())
        {
            size_t take = (std::min)(maxChunkSize, unitText.size() - pos);
            std::wstring piece = unitText.substr(pos, take);
            pos += take;
            std::wstring pieceDelim = (pos >= unitText.size()) ? unitDelim : L"";
            outChunks.push_back({ piece, pieceDelim });
        }
    }

    std::vector<ChunkPart> TextChunker::Split(std::wstring_view text, size_t maxChunkSize)
    {
        if (text.empty())
        {
            return {};
        }

        if (text.size() <= maxChunkSize)
        {
            return { { std::wstring(text), L"" } };
        }

        // Normalize or check paragraph delimiter: prefer \r\n\r\n or \n\n
        std::wstring_view paraDelim = L"\n\n";
        if (text.find(L"\r\n\r\n") != std::wstring_view::npos)
        {
            paraDelim = L"\r\n\r\n";
        }

        auto paragraphs = SplitByDelim(text, paraDelim);
        std::vector<ChunkPart> chunks;

        std::wstring currentChunk;
        std::wstring currentDelim;

        for (size_t i = 0; i < paragraphs.size(); ++i)
        {
            auto const& [paraText, paraAfter] = paragraphs[i];

            if (paraText.size() > maxChunkSize)
            {
                // Flush existing accumulated chunk
                if (!currentChunk.empty())
                {
                    chunks.push_back({ currentChunk, currentDelim });
                    currentChunk.clear();
                    currentDelim.clear();
                }

                // Check if paragraph contains single newlines
                std::wstring_view lineDelim = (paraText.find(L"\r\n") != std::wstring::npos) ? L"\r\n" : L"\n";
                auto lines = SplitByDelim(paraText, lineDelim);

                if (lines.size() > 1)
                {
                    for (size_t j = 0; j < lines.size(); ++j)
                    {
                        auto const& [lineText, lineAfter] = lines[j];
                        std::wstring combinedAfter = (j + 1 == lines.size()) ? paraAfter : lineAfter;

                        if (lineText.size() > maxChunkSize)
                        {
                            if (!currentChunk.empty())
                            {
                                chunks.push_back({ currentChunk, currentDelim });
                                currentChunk.clear();
                                currentDelim.clear();
                            }
                            RecursivelySubdivide(lineText, combinedAfter, maxChunkSize, chunks);
                        }
                        else if (currentChunk.size() + currentDelim.size() + lineText.size() <= maxChunkSize)
                        {
                            currentChunk += currentDelim + lineText;
                            currentDelim = combinedAfter;
                        }
                        else
                        {
                            if (!currentChunk.empty())
                            {
                                chunks.push_back({ currentChunk, currentDelim });
                            }
                            currentChunk = lineText;
                            currentDelim = combinedAfter;
                        }
                    }
                }
                else
                {
                    RecursivelySubdivide(paraText, paraAfter, maxChunkSize, chunks);
                }
            }
            else if (currentChunk.size() + currentDelim.size() + paraText.size() <= maxChunkSize)
            {
                currentChunk += currentDelim + paraText;
                currentDelim = paraAfter;
            }
            else
            {
                if (!currentChunk.empty())
                {
                    chunks.push_back({ currentChunk, currentDelim });
                }
                currentChunk = paraText;
                currentDelim = paraAfter;
            }
        }

        if (!currentChunk.empty())
        {
            chunks.push_back({ currentChunk, currentDelim });
        }

        return chunks;
    }

    std::wstring TextChunker::Combine(
        std::vector<std::wstring> const& translatedParts,
        std::vector<ChunkPart> const& originalChunks)
    {
        if (translatedParts.empty()) return L"";
        if (translatedParts.size() == 1) return translatedParts[0];

        std::wstring combined;
        for (size_t i = 0; i < translatedParts.size(); ++i)
        {
            combined += translatedParts[i];
            if (i < originalChunks.size() && !originalChunks[i].delimiterAfter.empty())
            {
                combined += originalChunks[i].delimiterAfter;
            }
        }
        return combined;
    }
}
