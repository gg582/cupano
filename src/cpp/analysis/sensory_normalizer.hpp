/**
 * @file sensory_normalizer.hpp
 * @brief Turns a raw LLM response into the final cupano profile.
 */

#pragma once

#include "analysis/analysis_types.hpp"
#include "analysis/availability_analyzer.hpp"
#include "analysis/availability_parser.hpp"
#include "analysis/discrepancy_detector.hpp"
#include "analysis/note_frequency_analyzer.hpp"
#include "analysis/regional_report_parser.hpp"
#include "analysis/taste_match_parser.hpp"
#include "analysis/wheel_frequency_analyzer.hpp"
#include "core/public_dto.hpp"
#include "json/json_parser.hpp"
#include "json/json_payload_extractor.hpp"
#include "roast/roast_consensus_calculator.hpp"
#include "roast/roast_scale.hpp"

#include <string>

namespace cupano {

/** Every collaborator SensoryNormalizer delegates to (injected by reference). */
struct SensoryNormalizerCollaborators {
    const JsonPayloadExtractor& extractor;
    const JsonParser& parser;
    const RegionalReportParser& reportParser;
    const AvailabilityParser& availabilityParser;
    const TasteMatchParser& tasteMatchParser;
    const NoteFrequencyAnalyzer& noteAnalyzer;
    const WheelFrequencyAnalyzer& wheelAnalyzer;
    const RoastConsensusCalculator& roastCalculator;
    const AvailabilityAnalyzer& availabilityAnalyzer;
    const DiscrepancyDetector& discrepancyDetector;
    const RoastScale& roastScale;
};

/** Normalizes and aggregates multi-regional cupping notes into a final profile. */
class SensoryNormalizer {
public:
    explicit SensoryNormalizer(const SensoryNormalizerCollaborators& collaborators);

    FinalCupanoProfile normalize(const AiResponsePayload& payload) const;

private:
    ParsedSwarmResponse parseResponse(const AiResponsePayload& payload) const;
    RegionalReport buildPageReport(const BeanMetadata& bean) const;
    void applyRoast(const RoastConsensus& roast, FinalCupanoProfile& profile) const;
    FinalCupanoProfile failure(const AiResponsePayload& payload, const std::string& message) const;

    SensoryNormalizerCollaborators c_;
};

}  // namespace cupano
