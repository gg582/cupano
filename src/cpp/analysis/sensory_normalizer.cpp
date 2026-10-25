/**
 * @file sensory_normalizer.cpp
 * @brief SensoryNormalizer implementation.
 */

#include "analysis/sensory_normalizer.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace cupano {

SensoryNormalizer::SensoryNormalizer(const SensoryNormalizerCollaborators& collaborators) : c_(collaborators) {}

FinalCupanoProfile SensoryNormalizer::normalize(const AiResponsePayload& payload) const {
    const ParsedSwarmResponse parsed = parseResponse(payload);
    if (!parsed.ok) return failure(payload, parsed.errorMessage);

    std::vector<RegionalReport> reports = parsed.reports;
    const int reportingRegions = static_cast<int>(reports.size());
    const RegionalReport pageReport = buildPageReport(payload.bean);
    if (!pageReport.roastTerm.empty()) reports.push_back(pageReport);
    if (reports.empty() && parsed.sellers.items.empty()) {
        return failure(payload, "The swarm returned no usable regional data for this bean.");
    }

    const AggregatedNotes notes = c_.noteAnalyzer.analyze(reports);
    const RoastConsensus roast = c_.roastCalculator.compute(reports);
    const DiscrepancyReport discrepancies =
        c_.discrepancyDetector.detect(DiscrepancyInput{&roast, &notes, &parsed.modelDiscrepancies, reportingRegions});

    FinalCupanoProfile profile;
    profile.success = true;
    profile.bean = payload.bean;
    profile.reportingRegions = reportingRegions;
    applyRoast(roast, profile);
    profile.summary = parsed.summary;
    profile.discrepancies = discrepancies.items;
    profile.flavorWheel = c_.wheelAnalyzer.analyze(reports).segments;
    profile.availability = c_.availabilityAnalyzer.analyze(AvailabilityInput{&parsed.sellers, payload.user.countryCode});
    // A taste match is only meaningful against preferences the user actually set.
    const bool hasPreferences = !payload.user.preferredProcesses.empty() || !payload.user.tastePreference.empty();
    if (hasPreferences) profile.tasteMatch = parsed.tasteMatch;
    const std::size_t topCount = std::min<std::size_t>(3, notes.ranked.size());
    profile.topNotes.assign(notes.ranked.begin(), notes.ranked.begin() + static_cast<std::ptrdiff_t>(topCount));
    if (reportingRegions == 0) {
        profile.discrepancies.insert(profile.discrepancies.begin(),
                                     "No regional agent returned grounded evidence; only the page's declared roast was used.");
    }
    return profile;
}

ParsedSwarmResponse SensoryNormalizer::parseResponse(const AiResponsePayload& payload) const {
    const JsonDocumentText document = c_.extractor.extract(TextFragment{payload.rawResponseText});
    const JsonParseResult json = c_.parser.parse(document);
    if (!json.ok) {
        ParsedSwarmResponse failed;
        failed.errorMessage = json.errorMessage;
        return failed;
    }
    ParsedSwarmResponse parsed = c_.reportParser.parse(json.root);
    if (!parsed.ok) return parsed;
    parsed.sellers = c_.availabilityParser.parse(json.root);
    parsed.tasteMatch = c_.tasteMatchParser.parse(json.root);
    return parsed;
}

RegionalReport SensoryNormalizer::buildPageReport(const BeanMetadata& bean) const {
    // The roaster's declared roast level is a useful anchor but is marketing
    // copy, so it votes at half weight under the pseudo-region "PAGE". It
    // carries no flavor notes: notes come exclusively from the LLM.
    RegionalReport report;
    report.regionCode = "PAGE";
    report.weight = 0.5;
    report.roastTerm = bean.declaredRoastLevel;
    return report;
}

void SensoryNormalizer::applyRoast(const RoastConsensus& roast, FinalCupanoProfile& profile) const {
    const RoastClassification classification{roast.stage};
    profile.roastLevel = c_.roastScale.broadLabel(classification).text;
    profile.roastStage = c_.roastScale.stageLabel(classification).text;
    profile.roastAgtron = c_.roastScale.agtronLabel(classification).text;
    profile.roastConfidence = roast.confidence;
}

FinalCupanoProfile SensoryNormalizer::failure(const AiResponsePayload& payload, const std::string& message) const {
    FinalCupanoProfile profile;
    profile.success = false;
    profile.errorMessage = message;
    profile.bean = payload.bean;
    applyRoast(RoastConsensus{}, profile);
    profile.availability = c_.availabilityAnalyzer.analyze(AvailabilityInput{nullptr, payload.user.countryCode});
    return profile;
}

}  // namespace cupano
