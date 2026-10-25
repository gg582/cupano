/**
 * @file note_frequency_analyzer.cpp
 * @brief NoteFrequencyAnalyzer implementation.
 */

#include "analysis/note_frequency_analyzer.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace cupano {

namespace {

/** Per-term tally used only inside NoteFrequencyAnalyzer::analyze. */
struct NoteFrequencyBucket {
    std::vector<std::string> surfaceForms;
    std::vector<std::string> categories;
    std::vector<std::string> nativeTerms;
    std::vector<std::string> regions;
    int mentions = 0;
};

}  // namespace

NoteFrequencyAnalyzer::NoteFrequencyAnalyzer(const TextNormalizer& normalizer) : normalizer_(normalizer) {}

AggregatedNotes NoteFrequencyAnalyzer::analyze(const std::vector<RegionalReport>& reports) const {
    std::map<std::string, NoteFrequencyBucket> buckets;
    for (const RegionalReport& report : reports) {
        // A region counts once per term, however many times it repeats it;
        // its mention count for that term is the largest it reported.
        std::map<std::string, int> regionMentions;
        for (const RawSensoryNote& note : report.notes) {
            const NoteKey key = keyOf(note);
            if (key.value.empty()) continue;
            NoteFrequencyBucket& bucket = buckets[key.value];
            bucket.surfaceForms.push_back(note.canonicalTerm);
            if (!note.category.empty()) bucket.categories.push_back(note.category);
            if (!note.nativeTerm.empty() && keyOf(RawSensoryNote{"", note.nativeTerm, "", "", 1}).value != key.value) {
                bucket.nativeTerms.push_back(note.nativeTerm);
            }
            int& mentions = regionMentions[key.value];
            mentions = std::max(mentions, note.mentions);
        }
        for (const auto& entry : regionMentions) {
            NoteFrequencyBucket& bucket = buckets[entry.first];
            bucket.mentions += entry.second;
            bucket.regions.push_back(report.regionCode);
        }
    }

    const int regionsWithNotes = countRegionsWithNotes(reports);
    AggregatedNotes result;
    for (const auto& entry : buckets) {
        const NoteFrequencyBucket& bucket = entry.second;
        SensoryNote note;
        note.descriptor = mostFrequent(bucket.surfaceForms).text;
        note.category = mostFrequent(bucket.categories).text;
        note.regionCount = static_cast<int>(bucket.regions.size());
        note.mentionCount = bucket.mentions;
        note.consensusScore =
            regionsWithNotes > 0 ? static_cast<double>(note.regionCount) / static_cast<double>(regionsWithNotes) : 0.0;
        note.sourceRegions = bucket.regions;
        std::set<std::string> seen;
        for (const std::string& term : bucket.nativeTerms) {
            if (seen.insert(term).second) note.nativeTerms.push_back(term);
        }
        result.ranked.push_back(std::move(note));
    }
    std::sort(result.ranked.begin(), result.ranked.end(), [](const SensoryNote& a, const SensoryNote& b) {
        if (a.regionCount != b.regionCount) return a.regionCount > b.regionCount;
        if (a.mentionCount != b.mentionCount) return a.mentionCount > b.mentionCount;
        return a.descriptor < b.descriptor;
    });
    return result;
}

NoteKey NoteFrequencyAnalyzer::keyOf(const RawSensoryNote& note) const {
    // Purely lexical normalization (case, separators, spacing) so that
    // "Jasmine", "jasmine" and "JASMINE " count as one term. No semantics.
    std::string key = normalizer_.foldCase(TextFragment{note.canonicalTerm}).text;
    std::replace(key.begin(), key.end(), '-', ' ');
    std::replace(key.begin(), key.end(), '_', ' ');
    return NoteKey{normalizer_.trim(normalizer_.collapseWhitespace(TextFragment{key})).text};
}

TextFragment NoteFrequencyAnalyzer::mostFrequent(const std::vector<std::string>& values) const {
    std::map<std::string, int> counts;
    std::string best;
    int bestCount = 0;
    for (const std::string& value : values) {
        const int count = ++counts[value];
        if (count > bestCount) {
            best = value;
            bestCount = count;
        }
    }
    return TextFragment{best};
}

int NoteFrequencyAnalyzer::countRegionsWithNotes(const std::vector<RegionalReport>& reports) const {
    return static_cast<int>(std::count_if(reports.begin(), reports.end(),
                                          [](const RegionalReport& report) { return !report.notes.empty(); }));
}

}  // namespace cupano
