/**
 * @file wheel_frequency_analyzer.hpp
 * @brief Frequency analysis of where the regions placed their notes on the flavor wheel.
 */

#pragma once

#include "analysis/analysis_types.hpp"
#include "core/public_dto.hpp"
#include "wheel/flavor_wheel.hpp"

#include <vector>

namespace cupano {

/** Every wheel segment with its region tally, in wheel order. */
struct WheelTally {
    std::vector<WheelSegment> segments;
};

/**
 * Counts, per wheel segment, how many regions placed at least one note there
 * (a note counts for its segment and every enclosing ring). Segments named by
 * fewer than half of the note-reporting regions are flagged as contested.
 */
class WheelFrequencyAnalyzer {
public:
    explicit WheelFrequencyAnalyzer(const FlavorWheel& wheel);

    WheelTally analyze(const std::vector<RegionalReport>& reports) const;

private:
    const FlavorWheel& wheel_;
};

}  // namespace cupano
