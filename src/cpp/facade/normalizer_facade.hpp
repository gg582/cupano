/**
 * @file normalizer_facade.hpp
 * @brief Composition root and the single class exported to JavaScript.
 */

#pragma once

#include "analysis/availability_analyzer.hpp"
#include "analysis/availability_parser.hpp"
#include "analysis/discrepancy_detector.hpp"
#include "analysis/note_frequency_analyzer.hpp"
#include "analysis/regional_report_parser.hpp"
#include "analysis/sensory_normalizer.hpp"
#include "analysis/taste_match_parser.hpp"
#include "analysis/wheel_frequency_analyzer.hpp"
#include "core/public_dto.hpp"
#include "json/json_parser.hpp"
#include "json/json_payload_extractor.hpp"
#include "json/json_reader.hpp"
#include "page/string_sanitizer.hpp"
#include "prompt/regional_agent_registry.hpp"
#include "prompt/swarm_prompt_builder.hpp"
#include "roast/roast_consensus_calculator.hpp"
#include "roast/roast_scale.hpp"
#include "text/text_normalizer.hpp"
#include "wheel/flavor_wheel.hpp"

namespace cupano {

/**
 * Owns every collaborator instance and wires them together. This is the only
 * class exported to JavaScript. Member declaration order matters: services are
 * constructed after the lexicons and parsers they reference.
 */
class NormalizerFacade {
public:
    NormalizerFacade();

    NormalizerFacade(const NormalizerFacade&) = delete;
    NormalizerFacade& operator=(const NormalizerFacade&) = delete;

    BeanMetadata extractMetadata(const RawPageData& page) const;
    SwarmPrompt buildSwarmPrompt(const SwarmPromptConfig& config) const;
    FinalCupanoProfile normalizeResponse(const AiResponsePayload& payload) const;

private:
    TextNormalizer textNormalizer_;
    RoastScale roastScale_;
    FlavorWheel flavorWheel_;
    RegionalAgentRegistry agentRegistry_;
    JsonPayloadExtractor jsonExtractor_;
    JsonParser jsonParser_;
    JsonReader jsonReader_;
    StringSanitizer sanitizer_;
    SwarmPromptBuilder promptBuilder_;
    RegionalReportParser reportParser_;
    AvailabilityParser availabilityParser_;
    TasteMatchParser tasteMatchParser_;
    NoteFrequencyAnalyzer noteAnalyzer_;
    WheelFrequencyAnalyzer wheelAnalyzer_;
    AvailabilityAnalyzer availabilityAnalyzer_;
    RoastConsensusCalculator roastCalculator_;
    DiscrepancyDetector discrepancyDetector_;
    SensoryNormalizer sensoryNormalizer_;
};

}  // namespace cupano
