/**
 * @file roast_consensus_calculator.hpp
 * @brief Weighted cross-regional roast consensus.
 */

#pragma once

#include "analysis/analysis_types.hpp"
#include "roast/roast_scale.hpp"
#include "roast/roast_types.hpp"

#include <string>
#include <vector>

namespace cupano {

/** Weighted roast vote cast by one region. */
struct RoastVote {
    std::string regionCode;
    RoastStage stage = RoastStage::Unknown;
    double weight = 1.0;
};

/** Aggregated roast decision. */
struct RoastConsensus {
    RoastStage stage = RoastStage::Unknown;
    double confidence = 0.0;
    std::vector<RoastVote> votes;
};

/** Computes the weighted roast consensus and its confidence. */
class RoastConsensusCalculator {
public:
    explicit RoastConsensusCalculator(const RoastScale& roastScale);

    RoastConsensus compute(const std::vector<RegionalReport>& reports) const;

private:
    RoastClassification classifyReport(const RegionalReport& report) const;

    const RoastScale& roastScale_;
};

}  // namespace cupano
