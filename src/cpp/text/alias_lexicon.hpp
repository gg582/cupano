/**
 * @file alias_lexicon.hpp
 * @brief Generic alias -> canonical value lexicon (longest alias first, boundary-aware).
 */

#pragma once

#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace cupano {

/** A surface-form alias pointing to a canonical value (generic lexicon row). */
template <typename TValue>
struct AliasEntry {
    std::string alias;
    TValue value;
};

/** Location of an alias inside a searched text (entry is null when nothing matched). */
template <typename TValue>
struct AliasHit {
    const AliasEntry<TValue>* entry = nullptr;
    std::size_t position = 0;
};

/** Generic alias -> value lexicon using longest-alias-first, boundary-aware matching. */
template <typename TValue>
class AliasLexicon {
public:
    AliasLexicon(const TextNormalizer& normalizer, std::vector<AliasEntry<TValue>> entries);

    /** Returns the first alias (longest aliases win) occurring in the case-folded text. */
    AliasHit<TValue> findIn(const TextFragment& text) const;

private:
    AliasHit<TValue> findInFolded(const std::string& folded) const;
    bool hasBoundaries(const std::string& haystack, const std::string& alias, std::size_t at) const;
    bool isWordCharacterBefore(const std::string& haystack, std::size_t at) const;
    bool isWordCharacterAt(const std::string& haystack, std::size_t at) const;
    bool isAsciiOnly(const std::string& value) const;

    const TextNormalizer& normalizer_;
    std::vector<AliasEntry<TValue>> entries_;
};

/** Appends one canonical value under several surface-form aliases. */
template <typename TValue>
inline void addAliases(std::vector<AliasEntry<TValue>>& out, const TValue& value,
                std::initializer_list<const char*> aliases) {
    for (const char* alias : aliases) {
        out.push_back(AliasEntry<TValue>{alias, value});
    }
}

}  // namespace cupano
