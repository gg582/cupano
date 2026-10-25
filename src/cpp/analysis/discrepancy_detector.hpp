/**
 * @file discrepancy_detector.hpp
 * @brief Explains where the regional reports disagree.
 */

#pragma once

#include "analysis/note_frequency_analyzer.hpp"
#include "roast/roast_consensus_calculator.hpp"
#include "roast/roast_scale.hpp"
#include "text/text_normalizer.hpp"

#include <string>
#include <vector>

namespace cupano {

/** Inputs needed to explain cross-regional disagreement. */
struct DiscrepancyInput {
    const RoastConsensus* roast = nullptr;
    const AggregatedNotes* notes = nullptr;
    const std::vector<std::string>* modelDiscrepancies = nullptr;
    int reportingRegions = 0;
};

/** Human-readable list of detected discrepancies. */
struct DiscrepancyReport {
    std::vector<std::string> items;
};

/** Explains where the regional reports disagree. */
class DiscrepancyDetector {
public:
    DiscrepancyDetector(const RoastScale& roastScale, const TextNormalizer& normalizer);

    DiscrepancyReport detect(const DiscrepancyInput& input) const;

private:
    void appendRoastSpread(const RoastConsensus& roast, DiscrepancyReport& report) const;
    void appendIsolatedNotes(const AggregatedNotes& notes, int reportingRegions,
                             DiscrepancyReport& report) const;
    void appendModelNotes(const std::vector<std::string>& modelNotes, DiscrepancyReport& report) const;

    const RoastScale& roastScale_;
    const TextNormalizer& normalizer_;
};

}  // namespace cupano
