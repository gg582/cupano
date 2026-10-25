/**
 * @file roast_scale.cpp
 * @brief RoastScale implementation.
 */

#include "roast/roast_scale.hpp"
#include "roast/roast_table.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace cupano {

namespace {

std::string broadLabelText(RoastLevel level) {
    switch (level) {
        case RoastLevel::Light: return "Light";
        case RoastLevel::MediumLight: return "Medium-Light";
        case RoastLevel::Medium: return "Medium";
        case RoastLevel::MediumDark: return "Medium-Dark";
        case RoastLevel::Dark: return "Dark";
        case RoastLevel::Unknown: break;
    }
    return "Unknown";
}

std::string stageLabelText(RoastStage stage) {
    switch (stage) {
        case RoastStage::VeryLight: return "Very Light";
        case RoastStage::Light: return "Light (Cinnamon)";
        case RoastStage::ModeratelyLight: return "Moderately Light (Half City)";
        case RoastStage::LightMedium: return "Light Medium (American)";
        case RoastStage::Medium: return "Medium (City)";
        case RoastStage::ModeratelyDark: return "Moderately Dark (Full City)";
        case RoastStage::Dark: return "Dark (Vienna/French)";
        case RoastStage::VeryDark: return "Very Dark (Italian)";
        case RoastStage::Unknown: break;
    }
    return "Unknown";
}

}  // namespace

RoastScale::RoastScale(const TextNormalizer& normalizer)
    : normalizer_(normalizer), lexicon_(normalizer, buildRoastEntries()) {}

RoastClassification RoastScale::classify(const TextFragment& term) const {
    // Most precise evidence first: an Agtron reading, then a roaster's own
    // N-step scale, then vocabulary.
    const TextFragment folded = normalizer_.foldCase(term);
    for (const RoastClassification& candidate :
         {classifyAgtron(folded), classifySymbolScale(term), classifyFractionScale(term)}) {
        if (candidate.stage != RoastStage::Unknown) return candidate;
    }
    const AliasHit<RoastStage> hit = lexicon_.findIn(term);
    return RoastClassification{hit.entry ? hit.entry->value : RoastStage::Unknown};
}

RoastLevel RoastScale::broadLevel(const RoastClassification& classification) const {
    switch (classification.stage) {
        case RoastStage::VeryLight:
        case RoastStage::Light: return RoastLevel::Light;
        case RoastStage::ModeratelyLight:
        case RoastStage::LightMedium: return RoastLevel::MediumLight;
        case RoastStage::Medium: return RoastLevel::Medium;
        case RoastStage::ModeratelyDark: return RoastLevel::MediumDark;
        case RoastStage::Dark:
        case RoastStage::VeryDark: return RoastLevel::Dark;
        case RoastStage::Unknown: break;
    }
    return RoastLevel::Unknown;
}

TextFragment RoastScale::broadLabel(const RoastClassification& classification) const {
    return TextFragment{broadLabelText(broadLevel(classification))};
}

TextFragment RoastScale::stageLabel(const RoastClassification& classification) const {
    return TextFragment{stageLabelText(classification.stage)};
}

TextFragment RoastScale::agtronLabel(const RoastClassification& classification) const {
    if (classification.stage == RoastStage::Unknown) return TextFragment{};
    return TextFragment{"#" + std::to_string(95 - 10 * static_cast<int>(classification.stage))};
}

RoastClassification RoastScale::classifyAgtron(const TextFragment& folded) const {
    const std::size_t at = folded.text.find("agtron");
    if (at == std::string::npos) return RoastClassification{};
    std::size_t i = at + 6;
    const std::size_t limit = std::min(folded.text.size(), i + 16);
    while (i < limit && !std::isdigit(static_cast<unsigned char>(folded.text[i]))) ++i;
    if (i >= limit) return RoastClassification{};
    const int value = std::atoi(folded.text.c_str() + i);
    if (value < 15 || value > 110) return RoastClassification{};
    // Nearest gourmet tile: #95 -> stage 0 ... #25 -> stage 7.
    const long stage = std::lround((95.0 - static_cast<double>(value)) / 10.0);
    return RoastClassification{static_cast<RoastStage>(std::min(7L, std::max(0L, stage)))};
}

RoastClassification RoastScale::classifySymbolScale(const TextFragment& term) const {
    // Roaster dot/star gauges, e.g. "●●○○○" or "★★★☆☆" (filled = darker).
    const std::vector<std::pair<std::string, std::string>> glyphs = {
        {"●", "○"}, {"★", "☆"}, {"■", "□"}, {"◆", "◇"}, {"⬤", "◯"}};
    for (const auto& glyph : glyphs) {
        ScalePosition position;
        for (std::size_t at = term.text.find(glyph.first); at != std::string::npos;
             at = term.text.find(glyph.first, at + glyph.first.size())) {
            ++position.position;
        }
        int empty = 0;
        for (std::size_t at = term.text.find(glyph.second); at != std::string::npos;
             at = term.text.find(glyph.second, at + glyph.second.size())) {
            ++empty;
        }
        position.size = position.position + empty;
        if (position.size >= 3 && position.position >= 1) return fromScalePosition(position);
    }
    return RoastClassification{};
}

RoastClassification RoastScale::classifyFractionScale(const TextFragment& term) const {
    // Roaster numeric gauges, e.g. "Roast 2/5" or "3 / 10".
    const std::string& t = term.text;
    for (std::size_t slash = t.find('/'); slash != std::string::npos; slash = t.find('/', slash + 1)) {
        std::size_t left = slash;
        while (left > 0 && t[left - 1] == ' ') --left;
        std::size_t numStart = left;
        while (numStart > 0 && std::isdigit(static_cast<unsigned char>(t[numStart - 1]))) --numStart;
        std::size_t right = slash + 1;
        while (right < t.size() && t[right] == ' ') ++right;
        if (numStart == left || right >= t.size() || !std::isdigit(static_cast<unsigned char>(t[right]))) continue;
        const ScalePosition position{std::atoi(t.c_str() + numStart), std::atoi(t.c_str() + right)};
        if (position.size >= 3 && position.size <= 10 && position.position >= 1 && position.position <= position.size) {
            return fromScalePosition(position);
        }
    }
    return RoastClassification{};
}

RoastClassification RoastScale::fromScalePosition(const ScalePosition& position) const {
    const double ratio = static_cast<double>(position.position - 1) / static_cast<double>(position.size - 1);
    return RoastClassification{static_cast<RoastStage>(static_cast<int>(std::lround(ratio * 7.0)))};
}

}  // namespace cupano
