/**
 * @file page_lexicons.hpp
 * @brief Fixed lexicons used to read product pages (countries, processes, promo clutter).
 */

#pragma once

#include "text/alias_lexicon.hpp"
#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

#include <string>
#include <vector>

namespace cupano {

/** Case-folds labels and orders them longest-first. */
LabelSet makeLabelSet(const TextNormalizer& normalizer, std::vector<std::string> labels);

/** Coffee-producing countries in KO/JA/ZH/NO/DE spellings -> English name. */
std::vector<AliasEntry<std::string>> buildCountryEntries();

/** Processing methods -> canonical English name. */
std::vector<AliasEntry<std::string>> buildProcessEntries();

/** Promotional title clutter to strip from product names. */
std::vector<AliasEntry<std::string>> buildPromoEntries();

/** Words that signal a coffee product page. */
std::vector<AliasEntry<std::string>> buildCoffeeSignalEntries();

}  // namespace cupano
