/**
 * @file roast_consensus_calculator.cpp
 * @brief RoastConsensusCalculator implementation.
 */

#include "roast/roast_consensus_calculator.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace cupano {

RoastConsensusCalculator::RoastConsensusCalculator(const RoastScale& roastScale) : roastScale_(roastScale) {}

RoastConsensus RoastConsensusCalculator::compute(const std::vector<RegionalReport>& reports) const {
    RoastConsensus consensus;
    double weightSum = 0.0;
    double weightedLevel = 0.0;
    for (const RegionalReport& report : reports) {
        const RoastClassification classification = classifyReport(report);
        if (classification.stage == RoastStage::Unknown) continue;
        consensus.votes.push_back(RoastVote{report.regionCode, classification.stage, report.weight});
        weightSum += report.weight;
        weightedLevel += report.weight * static_cast<double>(classification.stage);
    }
    if (weightSum <= 0.0) return consensus;

    const double mean = weightedLevel / weightSum;
    double variance = 0.0;
    for (const RoastVote& vote : consensus.votes) {
        const double delta = static_cast<double>(vote.stage) - mean;
        variance += vote.weight * delta * delta;
    }
    variance /= weightSum;
    consensus.stage = static_cast<RoastStage>(static_cast<int>(std::lround(mean)));
    // Agreement term: a standard deviation of three tiles (~one and a half broad
    // steps) zeroes confidence. Coverage term: under three voters caps it.
    const double agreement = std::max(0.0, 1.0 - std::sqrt(variance) / 3.0);
    const double coverage = std::min(1.0, static_cast<double>(consensus.votes.size()) / 3.0);
    consensus.confidence = agreement * coverage;
    return consensus;
}

RoastClassification RoastConsensusCalculator::classifyReport(const RegionalReport& report) const {
    // The source's own wording goes through the full roast table first; the
    // model's English mapping is only a fallback for terms the table lacks.
    const RoastClassification native = roastScale_.classify(TextFragment{report.roastTerm});
    if (native.stage != RoastStage::Unknown) return native;
    return roastScale_.classify(TextFragment{report.roastHint});
}

}  // namespace cupano
