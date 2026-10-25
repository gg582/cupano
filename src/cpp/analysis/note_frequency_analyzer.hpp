/**
 * @file note_frequency_analyzer.hpp
 * @brief Frequency analysis over LLM-supplied canonical flavor terms.
 */

#pragma once

#include "analysis/analysis_types.hpp"
#include "core/public_dto.hpp"
#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

#include <string>
#include <vector>

namespace cupano {

/** Case/whitespace-normalized key used to count identical canonical terms. */
struct NoteKey {
    std::string value;
};

/** Ranked list of aggregated notes. */
struct AggregatedNotes {
    std::vector<SensoryNote> ranked;
};

/**
 * Pure frequency analysis over LLM-supplied canonical terms: groups identical
 * terms, counts reporting regions and source mentions, and ranks by frequency.
 * It performs no semantic mapping of its own.
 */
class NoteFrequencyAnalyzer {
public:
    explicit NoteFrequencyAnalyzer(const TextNormalizer& normalizer);

    AggregatedNotes analyze(const std::vector<RegionalReport>& reports) const;

private:
    NoteKey keyOf(const RawSensoryNote& note) const;
    TextFragment mostFrequent(const std::vector<std::string>& values) const;
    int countRegionsWithNotes(const std::vector<RegionalReport>& reports) const;

    const TextNormalizer& normalizer_;
};

}  // namespace cupano
