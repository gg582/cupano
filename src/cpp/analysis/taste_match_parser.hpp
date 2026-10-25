/**
 * @file taste_match_parser.hpp
 * @brief Reads the optional LLM taste-match judgment.
 */

#pragma once

#include "core/public_dto.hpp"
#include "json/json_reader.hpp"
#include "json/json_value.hpp"
#include "text/text_normalizer.hpp"

namespace cupano {

/** Reads the optional LLM taste-match judgment. */
class TasteMatchParser {
public:
    TasteMatchParser(const JsonReader& reader, const TextNormalizer& normalizer);

    TasteMatch parse(const JsonValue& root) const;

private:
    const JsonReader& reader_;
    const TextNormalizer& normalizer_;
};

}  // namespace cupano
