/**
 * @file string_sanitizer.hpp
 * @brief Turns raw page text into a BeanMetadata entity.
 */

#pragma once

#include "core/public_dto.hpp"
#include "roast/roast_scale.hpp"
#include "text/alias_lexicon.hpp"
#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

#include <string>

namespace cupano {

/** Strips promotional clutter from page text and extracts core bean entities. */
class StringSanitizer {
public:
    StringSanitizer(const TextNormalizer& normalizer, const RoastScale& roastScale);

    BeanMetadata extract(const RawPageData& page) const;

private:
    TextFragment stripPromotionalClutter(const TextFragment& input) const;
    TextFragment removeBracketedTags(const TextFragment& input) const;
    TextLines splitTitleSegments(const TextFragment& title) const;
    TextFragment cleanProductName(const TextLines& titleSegments) const;
    TextFragment extractRoaster(const TextLines& titleSegments, const TextFragment& url) const;
    TextLines splitInlineLabels(const TextLines& lines) const;
    bool startsWithAnyLabel(const TextFragment& text) const;
    TextFragment extractLabeledValue(const TextLines& lines, const LabelSet& labels) const;
    LabelMatch matchLabelOnLine(const TextFragment& line, const std::string& label) const;
    TextFragment findSignalLine(const TextLines& lines, const LabelSet& labels) const;
    TextFragment detectOriginCountry(const TextLines& lines, const RawPageData& page) const;
    TextFragment detectProcess(const TextLines& lines) const;
    TextFragment detectRoastLevel(const TextLines& lines) const;
    bool looksLikeCoffeeProduct(const TextFragment& corpus) const;
    TextFragment hostnameOf(const TextFragment& url) const;

    const TextNormalizer& normalizer_;
    const RoastScale& roastScale_;
    AliasLexicon<std::string> promoLexicon_;
    AliasLexicon<std::string> countryLexicon_;
    AliasLexicon<std::string> processLexicon_;
    AliasLexicon<std::string> coffeeSignalLexicon_;
    LabelSet originLabels_;
    LabelSet regionLabels_;
    LabelSet producerLabels_;
    LabelSet varietyLabels_;
    LabelSet processLabels_;
    LabelSet altitudeLabels_;
    LabelSet roastLabels_;
};

}  // namespace cupano
