/**
 * @file roast_scale.hpp
 * @brief Maps roast terms, Agtron readings and roaster gauges onto the roast table.
 */

#pragma once

#include "roast/roast_types.hpp"
#include "text/alias_lexicon.hpp"
#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

namespace cupano {

/**
 * Fixed roast table: maps Agtron readings, roaster N-step scales and the full
 * multilingual roast vocabulary (classic US/JP stage names, KO/JA/ZH-TW/NO/DE
 * terms and roastery dialects such as "Nordic", "Omni", "Blonde") onto the
 * eight Agtron tiles, and the tiles onto the broad five-step scale.
 */
class RoastScale {
public:
    explicit RoastScale(const TextNormalizer& normalizer);

    RoastClassification classify(const TextFragment& term) const;
    RoastLevel broadLevel(const RoastClassification& classification) const;
    TextFragment broadLabel(const RoastClassification& classification) const;
    TextFragment stageLabel(const RoastClassification& classification) const;
    TextFragment agtronLabel(const RoastClassification& classification) const;

private:
    RoastClassification classifyAgtron(const TextFragment& folded) const;
    RoastClassification classifySymbolScale(const TextFragment& term) const;
    RoastClassification classifyFractionScale(const TextFragment& term) const;
    RoastClassification fromScalePosition(const ScalePosition& position) const;

    const TextNormalizer& normalizer_;
    AliasLexicon<RoastStage> lexicon_;
};

}  // namespace cupano
