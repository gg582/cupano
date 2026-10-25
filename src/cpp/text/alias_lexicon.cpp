/**
 * @file alias_lexicon.cpp
 * @brief AliasLexicon implementation and explicit instantiations.
 */

#include "text/alias_lexicon.hpp"
#include "roast/roast_types.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace cupano {

template <typename TValue>
AliasLexicon<TValue>::AliasLexicon(const TextNormalizer& normalizer, std::vector<AliasEntry<TValue>> entries)
    : normalizer_(normalizer), entries_(std::move(entries)) {
    for (AliasEntry<TValue>& entry : entries_) {
        entry.alias = normalizer_.foldCase(TextFragment{entry.alias}).text;
    }
    std::stable_sort(entries_.begin(), entries_.end(),
                     [](const AliasEntry<TValue>& a, const AliasEntry<TValue>& b) {
                         return a.alias.size() > b.alias.size();
                     });
}

template <typename TValue>
AliasHit<TValue> AliasLexicon<TValue>::findIn(const TextFragment& text) const {
    return findInFolded(normalizer_.foldCase(text).text);
}

template <typename TValue>
AliasHit<TValue> AliasLexicon<TValue>::findInFolded(const std::string& folded) const {
    for (const AliasEntry<TValue>& entry : entries_) {
        if (entry.alias.empty()) continue;
        std::size_t at = folded.find(entry.alias);
        while (at != std::string::npos) {
            if (hasBoundaries(folded, entry.alias, at)) {
                return AliasHit<TValue>{&entry, at};
            }
            at = folded.find(entry.alias, at + 1);
        }
    }
    return AliasHit<TValue>{};
}

template <typename TValue>
bool AliasLexicon<TValue>::hasBoundaries(const std::string& haystack, const std::string& alias,
                                         std::size_t at) const {
    // Latin-script aliases must be whole words ("lime" must not match
    // "sublime"); CJK and Hangul aliases are matched as plain substrings
    // because those scripts do not delimit words with spaces.
    if (!isAsciiOnly(alias)) return true;
    return !isWordCharacterBefore(haystack, at) && !isWordCharacterAt(haystack, at + alias.size());
}

template <typename TValue>
bool AliasLexicon<TValue>::isWordCharacterBefore(const std::string& haystack, std::size_t at) const {
    if (at == 0) return false;
    std::size_t lead = at - 1;
    while (lead > 0 && (static_cast<unsigned char>(haystack[lead]) & 0xC0) == 0x80) --lead;
    return isWordCharacterAt(haystack, lead);
}

template <typename TValue>
bool AliasLexicon<TValue>::isWordCharacterAt(const std::string& haystack, std::size_t at) const {
    if (at >= haystack.size()) return false;
    const unsigned char byte = static_cast<unsigned char>(haystack[at]);
    // ASCII alphanumerics and Latin-1 letters (lead byte 0xC3) count as word
    // characters; Hangul/Kana/Han neighbours do not, so "Kenya의" still matches.
    return std::isalnum(byte) != 0 || byte == 0xC3;
}

template <typename TValue>
bool AliasLexicon<TValue>::isAsciiOnly(const std::string& value) const {
    return std::all_of(value.begin(), value.end(),
                       [](char c) { return static_cast<unsigned char>(c) < 0x80; });
}

template class AliasLexicon<RoastStage>;

template class AliasLexicon<std::string>;

}  // namespace cupano
