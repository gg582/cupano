/**
 * @file regional_report_parser.cpp
 * @brief RegionalReportParser implementation.
 */

#include "analysis/regional_report_parser.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace cupano {

RegionalReportParser::RegionalReportParser(const RegionalAgentRegistry& registry, const JsonReader& reader,
                                           const TextNormalizer& normalizer)
    : registry_(registry), reader_(reader), normalizer_(normalizer) {}

ParsedSwarmResponse RegionalReportParser::parse(const JsonValue& root) const {
    ParsedSwarmResponse result;
    if (root.kind != JsonValue::Kind::Object) {
        result.errorMessage = "Model response root is not a JSON object.";
        return result;
    }
    const JsonValue* reports = reader_.member(root, "regional_reports");
    if (!reports || reports->kind != JsonValue::Kind::Array) {
        result.errorMessage = "Model response is missing \"regional_reports\".";
        return result;
    }
    for (const JsonValue& node : reports->items) {
        RegionalReport report;
        if (parseReport(node, report) && !containsRegion(result.reports, report.regionCode)) {
            result.reports.push_back(std::move(report));
        }
    }
    for (const std::string& item : parseStringList(reader_.member(root, "discrepancies"))) {
        if (result.modelDiscrepancies.size() >= 4) break;
        result.modelDiscrepancies.push_back(normalizer_.truncateUtf8(TextFragment{item}, 4096).text);
    }
    const TextFragment summary{reader_.stringOr(reader_.member(root, "summary"), "")};
    result.summary = normalizer_.truncateUtf8(normalizer_.stripControlCharacters(summary), 8192).text;
    result.ok = true;
    return result;
}

bool RegionalReportParser::parseReport(const JsonValue& node, RegionalReport& out) const {
    if (node.kind != JsonValue::Kind::Object) return false;
    std::string code = normalizer_.toUpperAscii(TextFragment{reader_.stringOr(reader_.member(node, "region"), "")}).text;
    std::replace(code.begin(), code.end(), '_', '-');
    const RegionLookup lookup = registry_.find(RegionCodeQuery{normalizer_.trim(TextFragment{code}).text});
    if (!lookup.found) return false;

    out.regionCode = lookup.spec.code;
    out.sourcesFound = std::max(0, static_cast<int>(reader_.numberOr(reader_.member(node, "sources_found"), 0)));
    out.weight = lookup.spec.baseWeight * evidenceFactor(out.sourcesFound);
    out.roastTerm = normalizer_.truncateUtf8(TextFragment{reader_.stringOr(reader_.member(node, "roast_level"), "")}, 80).text;
    out.roastHint = normalizer_.truncateUtf8(TextFragment{reader_.stringOr(reader_.member(node, "roast_level_en"), "")}, 40).text;
    out.notes = parseNotes(reader_.member(node, "notes"));
    // A region that produced neither notes nor a roast reading contributes nothing.
    return !out.notes.empty() || !out.roastTerm.empty() || !out.roastHint.empty();
}

std::vector<RawSensoryNote> RegionalReportParser::parseNotes(const JsonValue* notesNode) const {
    std::vector<RawSensoryNote> notes;
    if (!notesNode || notesNode->kind != JsonValue::Kind::Array) return notes;
    for (const JsonValue& item : notesNode->items) {
        if (notes.size() >= 12) break;
        RawSensoryNote note = parseNote(item);
        if (!note.canonicalTerm.empty()) notes.push_back(std::move(note));
    }
    return notes;
}

RawSensoryNote RegionalReportParser::parseNote(const JsonValue& item) const {
    RawSensoryNote note;
    if (item.kind == JsonValue::Kind::String) {
        note.canonicalTerm = item.string;
    } else if (item.kind == JsonValue::Kind::Object) {
        note.nativeTerm = reader_.stringOr(reader_.member(item, "descriptor"), "");
        note.canonicalTerm = reader_.stringOr(reader_.member(item, "canonical_en"), "");
        note.category = reader_.stringOr(reader_.member(item, "category"), "");
        note.wheelPath = reader_.stringOr(reader_.member(item, "wheel_path"), "");
        note.mentions = static_cast<int>(reader_.numberOr(reader_.member(item, "mentions"), 1.0));
    }
    // The LLM owns canonicalization; if it omitted the English term, its native
    // term is counted as-is rather than being mapped by any fixed table here.
    if (note.canonicalTerm.empty()) note.canonicalTerm = note.nativeTerm;
    note.nativeTerm = normalizer_.truncateUtf8(normalizer_.trim(TextFragment{note.nativeTerm}), 64).text;
    note.canonicalTerm = normalizer_.truncateUtf8(normalizer_.trim(TextFragment{note.canonicalTerm}), 64).text;
    note.category = normalizer_.truncateUtf8(normalizer_.trim(TextFragment{note.category}), 48).text;
    note.wheelPath = normalizer_.truncateUtf8(normalizer_.trim(TextFragment{note.wheelPath}), 160).text;
    note.mentions = std::min(20, std::max(1, note.mentions));
    return note;
}

std::vector<std::string> RegionalReportParser::parseStringList(const JsonValue* listNode) const {
    std::vector<std::string> values;
    if (!listNode || listNode->kind != JsonValue::Kind::Array) return values;
    for (const JsonValue& item : listNode->items) {
        if (item.kind != JsonValue::Kind::String) continue;
        const TextFragment cleaned = normalizer_.trim(normalizer_.stripControlCharacters(TextFragment{item.string}));
        if (!cleaned.text.empty()) values.push_back(cleaned.text);
    }
    return values;
}

bool RegionalReportParser::containsRegion(const std::vector<RegionalReport>& reports, const std::string& code) const {
    return std::any_of(reports.begin(), reports.end(),
                       [&code](const RegionalReport& report) { return report.regionCode == code; });
}

double RegionalReportParser::evidenceFactor(int sourcesFound) const {
    // Ungrounded reports still count, but at half weight; each extra source adds
    // confidence up to a cap so a single prolific region cannot dominate.
    if (sourcesFound <= 0) return 0.5;
    return std::min(1.0, 0.6 + 0.1 * static_cast<double>(sourcesFound));
}

}  // namespace cupano
