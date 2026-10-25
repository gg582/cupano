/**
 * @file regional_report_parser.hpp
 * @brief Validates per-region reports from the parsed LLM JSON.
 */

#pragma once

#include "analysis/analysis_types.hpp"
#include "json/json_reader.hpp"
#include "json/json_value.hpp"
#include "prompt/regional_agent_registry.hpp"
#include "text/text_normalizer.hpp"

#include <string>
#include <vector>

namespace cupano {

/** Converts a parsed JSON tree into validated RegionalReport records. */
class RegionalReportParser {
public:
    RegionalReportParser(const RegionalAgentRegistry& registry, const JsonReader& reader,
                         const TextNormalizer& normalizer);

    ParsedSwarmResponse parse(const JsonValue& root) const;

private:
    bool parseReport(const JsonValue& node, RegionalReport& out) const;
    std::vector<RawSensoryNote> parseNotes(const JsonValue* notesNode) const;
    RawSensoryNote parseNote(const JsonValue& item) const;
    std::vector<std::string> parseStringList(const JsonValue* listNode) const;
    bool containsRegion(const std::vector<RegionalReport>& reports, const std::string& code) const;
    double evidenceFactor(int sourcesFound) const;

    const RegionalAgentRegistry& registry_;
    const JsonReader& reader_;
    const TextNormalizer& normalizer_;
};

}  // namespace cupano
