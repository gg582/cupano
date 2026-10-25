/**
 * @file wheel_frequency_analyzer.cpp
 * @brief WheelFrequencyAnalyzer implementation.
 */

#include "analysis/wheel_frequency_analyzer.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <vector>

namespace cupano {

WheelFrequencyAnalyzer::WheelFrequencyAnalyzer(const FlavorWheel& wheel) : wheel_(wheel) {}

WheelTally WheelFrequencyAnalyzer::analyze(const std::vector<RegionalReport>& reports) const {
    const std::vector<WheelNode>& nodes = wheel_.nodes();
    WheelTally tally;
    tally.segments.resize(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        WheelSegment& segment = tally.segments[i];
        segment.name = nodes[i].name;
        segment.path = nodes[i].path;
        segment.tier = nodes[i].tier;
        if (nodes[i].parent >= 0) segment.parentPath = nodes[static_cast<std::size_t>(nodes[i].parent)].path;
    }

    int regionsWithNotes = 0;
    for (const RegionalReport& report : reports) {
        if (report.notes.empty()) continue;
        ++regionsWithNotes;
        // A region counts once per segment; its mentions there are the largest it reported.
        std::map<int, int> regionMentions;
        for (const RawSensoryNote& note : report.notes) {
            const WheelPlacement placement = wheel_.place(TextFragment{note.wheelPath});
            if (placement.node < 0) continue;
            for (int node : wheel_.lineage(placement.node)) {
                int& mentions = regionMentions[node];
                mentions = std::max(mentions, note.mentions);
            }
        }
        for (const auto& entry : regionMentions) {
            WheelSegment& segment = tally.segments[static_cast<std::size_t>(entry.first)];
            segment.regionCount += 1;
            segment.mentionCount += entry.second;
            segment.sourceRegions.push_back(report.regionCode);
        }
    }

    for (WheelSegment& segment : tally.segments) {
        if (regionsWithNotes == 0 || segment.regionCount == 0) continue;
        segment.consensusScore = static_cast<double>(segment.regionCount) / static_cast<double>(regionsWithNotes);
        segment.contested = regionsWithNotes >= 2 && segment.consensusScore < 0.5;
    }
    return tally;
}

}  // namespace cupano
