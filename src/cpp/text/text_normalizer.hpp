/**
 * @file text_normalizer.hpp
 * @brief Byte-safe UTF-8 text helpers.
 */

#pragma once

#include "text/text_types.hpp"

#include <cstddef>

namespace cupano {

/** Byte-safe UTF-8 helpers: case folding, trimming, whitespace collapsing. */
class TextNormalizer {
public:
    TextFragment foldCase(const TextFragment& input) const;
    TextFragment trim(const TextFragment& input) const;
    TextFragment collapseWhitespace(const TextFragment& input) const;
    TextFragment stripControlCharacters(const TextFragment& input) const;
    TextFragment truncateUtf8(const TextFragment& input, std::size_t maxBytes) const;
    TextFragment stripLeadingBullets(const TextFragment& input) const;
    TextFragment toUpperAscii(const TextFragment& input) const;
    TextLines splitLines(const TextFragment& input) const;
};

}  // namespace cupano
