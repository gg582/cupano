/**
 * @file discrepancy_detector.cpp
 * @brief DiscrepancyDetector implementation.
 */

#include "analysis/discrepancy_detector.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace cupano {

DiscrepancyDetector::DiscrepancyDetector(const RoastScale& roastScale, const TextNormalizer& normalizer)
    : roastScale_(roastScale), normalizer_(normalizer) {}

DiscrepancyReport DiscrepancyDetector::detect(const DiscrepancyInput& input) const {
    DiscrepancyReport report;
    if (input.roast) appendRoastSpread(*input.roast, report);
    if (input.notes) appendIsolatedNotes(*input.notes, input.reportingRegions, report);
    if (input.modelDiscrepancies) appendModelNotes(*input.modelDiscrepancies, report);
    return report;
}

void DiscrepancyDetector::appendRoastSpread(const RoastConsensus& roast, DiscrepancyReport& report) const {
    if (roast.votes.size() < 2) return;
    const auto byStage = [](const RoastVote& a, const RoastVote& b) { return a.stage < b.stage; };
    const RoastVote& lightest = *std::min_element(roast.votes.begin(), roast.votes.end(), byStage);
    const RoastVote& darkest = *std::max_element(roast.votes.begin(), roast.votes.end(), byStage);
    const RoastLevel light = roastScale_.broadLevel(RoastClassification{lightest.stage});
    const RoastLevel dark = roastScale_.broadLevel(RoastClassification{darkest.stage});
    if (static_cast<int>(dark) - static_cast<int>(light) < 2) return;
    report.items.push_back("Roast assessments diverge: " + darkest.regionCode + " reports " +
                           roastScale_.stageLabel(RoastClassification{darkest.stage}).text + " while " +
                           lightest.regionCode + " reports " +
                           roastScale_.stageLabel(RoastClassification{lightest.stage}).text + ".");
}

void DiscrepancyDetector::appendIsolatedNotes(const AggregatedNotes& notes, int reportingRegions,
                                              DiscrepancyReport& report) const {
    if (reportingRegions < 3) return;
    int added = 0;
    const std::size_t inspected = std::min<std::size_t>(notes.ranked.size(), 5);
    for (std::size_t i = 0; i < inspected && added < 2; ++i) {
        const SensoryNote& note = notes.ranked[i];
        if (note.regionCount != 1) continue;
        report.items.push_back("\"" + note.descriptor + "\" was reported only by the " + note.sourceRegions.front() +
                               " agent.");
        ++added;
    }
}

void DiscrepancyDetector::appendModelNotes(const std::vector<std::string>& modelNotes,
                                           DiscrepancyReport& report) const {
    for (const std::string& note : modelNotes) {
        if (report.items.size() >= 6) break;
        report.items.push_back(normalizer_.truncateUtf8(TextFragment{note}, 4096).text);
    }
}

}  // namespace cupano
