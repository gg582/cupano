/**
 * @file normalizer_facade.cpp
 * @brief NormalizerFacade implementation (wires every collaborator).
 */

#include "facade/normalizer_facade.hpp"

namespace cupano {

NormalizerFacade::NormalizerFacade()
    : textNormalizer_(),
      roastScale_(textNormalizer_),
      flavorWheel_(textNormalizer_),
      agentRegistry_(),
      jsonExtractor_(),
      jsonParser_(),
      jsonReader_(),
      sanitizer_(textNormalizer_, roastScale_),
      promptBuilder_(agentRegistry_, flavorWheel_, textNormalizer_),
      reportParser_(agentRegistry_, jsonReader_, textNormalizer_),
      availabilityParser_(jsonReader_, textNormalizer_),
      tasteMatchParser_(jsonReader_, textNormalizer_),
      noteAnalyzer_(textNormalizer_),
      wheelAnalyzer_(flavorWheel_),
      availabilityAnalyzer_(textNormalizer_),
      roastCalculator_(roastScale_),
      discrepancyDetector_(roastScale_, textNormalizer_),
      sensoryNormalizer_(SensoryNormalizerCollaborators{jsonExtractor_, jsonParser_, reportParser_,
                                                        availabilityParser_, tasteMatchParser_, noteAnalyzer_,
                                                        wheelAnalyzer_,
                                                        roastCalculator_, availabilityAnalyzer_,
                                                        discrepancyDetector_, roastScale_}) {}

BeanMetadata NormalizerFacade::extractMetadata(const RawPageData& page) const { return sanitizer_.extract(page); }

SwarmPrompt NormalizerFacade::buildSwarmPrompt(const SwarmPromptConfig& config) const {
    return promptBuilder_.build(config);
}

FinalCupanoProfile NormalizerFacade::normalizeResponse(const AiResponsePayload& payload) const {
    return sensoryNormalizer_.normalize(payload);
}

}  // namespace cupano
