/**
 * @file availability_parser.hpp
 * @brief Extracts seller stock listings from the parsed LLM JSON.
 */

#pragma once

#include "analysis/analysis_types.hpp"
#include "json/json_reader.hpp"
#include "json/json_value.hpp"
#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

namespace cupano {

/** Extracts seller stock listings; anything but stock status is ignored. */
class AvailabilityParser {
public:
    AvailabilityParser(const JsonReader& reader, const TextNormalizer& normalizer);

    SellerListings parse(const JsonValue& root) const;

private:
    bool parseSeller(const JsonValue& node, SellerListing& out) const;
    TextFragment normalizeStatus(const TextFragment& status) const;
    TextFragment normalizeMarket(const TextFragment& market) const;
    TextFragment safeUrl(const TextFragment& url) const;

    const JsonReader& reader_;
    const TextNormalizer& normalizer_;
};

}  // namespace cupano
