/**
 * @file taste_match_parser.cpp
 * @brief TasteMatchParser implementation.
 */

#include "analysis/taste_match_parser.hpp"

#include <algorithm>
#include <cmath>

namespace cupano {

TasteMatchParser::TasteMatchParser(const JsonReader& reader, const TextNormalizer& normalizer)
    : reader_(reader), normalizer_(normalizer) {}

TasteMatch TasteMatchParser::parse(const JsonValue& root) const {
    TasteMatch match;
    const JsonValue* node = reader_.member(root, "taste_match");
    if (!node || node->kind != JsonValue::Kind::Object) return match;
    const double score = reader_.numberOr(reader_.member(*node, "score"), -1.0);
    if (score < 0.0) return match;
    match.present = true;
    match.score = static_cast<int>(std::lround(std::min(100.0, score)));
    const TextFragment reason{reader_.stringOr(reader_.member(*node, "reason"), "")};
    match.reason = normalizer_.truncateUtf8(normalizer_.trim(normalizer_.stripControlCharacters(reason)), 4096).text;
    return match;
}

}  // namespace cupano
