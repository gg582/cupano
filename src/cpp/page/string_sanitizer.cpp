/**
 * @file string_sanitizer.cpp
 * @brief StringSanitizer implementation.
 */

#include "page/string_sanitizer.hpp"
#include "page/page_lexicons.hpp"
#include "text/text_predicates.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace cupano {

namespace {

/** Byte-length limits applied to untrusted input fields. */
struct InputLimits {
    std::size_t title = 512;
    std::size_t meta = 2048;
    std::size_t body = 60000;
    std::size_t corpusScan = 12000;
    std::size_t fieldValue = 120;
    std::size_t maxLines = 800;
    std::size_t signalLine = 160;
};

}  // namespace

StringSanitizer::StringSanitizer(const TextNormalizer& normalizer, const RoastScale& roastScale)
    : normalizer_(normalizer),
      roastScale_(roastScale),
      promoLexicon_(normalizer, buildPromoEntries()),
      countryLexicon_(normalizer, buildCountryEntries()),
      processLexicon_(normalizer, buildProcessEntries()),
      coffeeSignalLexicon_(normalizer, buildCoffeeSignalEntries()),
      originLabels_(makeLabelSet(normalizer, {"origin", "country", "country of origin", "원산지", "생산국",
                                              "국가", "産地", "生産国", "生產國", "產國", "國家", "产地",
                                              "opprinnelse", "opprinnelsesland", "land", "herkunft",
                                              "herkunftsland", "ursprung", "ursprungsland"})),
      regionLabels_(makeLabelSet(normalizer, {"region", "growing region", "area", "지역", "재배지역", "재배 지역",
                                              "地域", "エリア", "生産地域", "產區", "产区", "區域", "distrikt",
                                              "anbaugebiet", "anbauregion"})),
      producerLabels_(makeLabelSet(normalizer, {"producer", "farm", "farmer", "estate", "washing station",
                                                "농장", "생산자", "農園", "生産者", "農場", "莊園", "庄园", "生產者",
                                                "produsent", "gård", "produzent", "finca"})),
      varietyLabels_(makeLabelSet(normalizer, {"variety", "varietal", "varieties", "cultivar", "품종", "品種",
                                               "品种", "varietet", "sorte", "varietät"})),
      processLabels_(makeLabelSet(normalizer, {"process", "processing", "processing method", "가공", "가공방식",
                                               "가공 방식", "프로세스", "精製", "精製方法", "精製方式", "プロセス",
                                               "處理法", "處理方式", "处理法", "prosess", "prosessering",
                                               "aufbereitung", "verarbeitung", "prozess"})),
      altitudeLabels_(makeLabelSet(normalizer, {"altitude", "elevation", "고도", "재배고도", "재배 고도", "標高",
                                                "海拔", "海拔高度", "høyde", "høyde over havet", "höhe",
                                                "anbauhöhe"})),
      roastLabels_(makeLabelSet(normalizer, {"roast", "roast level", "roast profile", "roasting", "로스팅",
                                             "로스팅 포인트", "배전도", "배전", "焙煎", "焙煎度", "ロースト", "烘焙",
                                             "烘焙度", "烘焙程度", "brenning", "brenningsgrad", "brent", "röstung",
                                             "röstgrad", "röstprofil"})) {}

BeanMetadata StringSanitizer::extract(const RawPageData& page) const {
    const InputLimits limits;
    RawPageData clean;
    clean.url = normalizer_.truncateUtf8(normalizer_.stripControlCharacters(TextFragment{page.url}), 2048).text;
    clean.title = normalizer_.truncateUtf8(normalizer_.stripControlCharacters(TextFragment{page.title}), limits.title).text;
    clean.metaDescription =
        normalizer_.truncateUtf8(normalizer_.stripControlCharacters(TextFragment{page.metaDescription}), limits.meta).text;
    clean.bodyText = normalizer_.truncateUtf8(normalizer_.stripControlCharacters(TextFragment{page.bodyText}), limits.body).text;

    const TextLines lines =
        splitInlineLabels(normalizer_.splitLines(TextFragment{clean.metaDescription + "\n" + clean.bodyText}));
    const TextLines titleSegments = splitTitleSegments(TextFragment{clean.title});

    BeanMetadata bean;
    bean.sourceUrl = clean.url;
    bean.productName = cleanProductName(titleSegments).text;
    bean.roaster = extractRoaster(titleSegments, TextFragment{clean.url}).text;
    bean.originCountry = detectOriginCountry(lines, clean).text;
    bean.region = extractLabeledValue(lines, regionLabels_).text;
    bean.producer = extractLabeledValue(lines, producerLabels_).text;
    bean.variety = extractLabeledValue(lines, varietyLabels_).text;
    bean.process = detectProcess(lines).text;
    bean.altitude = extractLabeledValue(lines, altitudeLabels_).text;
    bean.declaredRoastLevel = detectRoastLevel(lines).text;
    bean.isCoffeeProduct = looksLikeCoffeeProduct(
        normalizer_.truncateUtf8(TextFragment{clean.title + "\n" + clean.metaDescription + "\n" + clean.bodyText},
                                 limits.corpusScan));
    return bean;
}

TextFragment StringSanitizer::stripPromotionalClutter(const TextFragment& input) const {
    std::string text = input.text;
    for (int guard = 0; guard < 24; ++guard) {
        const AliasHit<std::string> hit = promoLexicon_.findIn(TextFragment{text});
        if (!hit.entry) break;
        // foldCase preserves byte length, so folded offsets map 1:1 onto text.
        text.erase(hit.position, hit.entry->alias.size());
    }
    return normalizer_.trim(normalizer_.collapseWhitespace(TextFragment{text}));
}

TextFragment StringSanitizer::removeBracketedTags(const TextFragment& input) const {
    // Square/lenticular brackets in shop titles almost always carry tags such
    // as [무료배송], 【送料無料】 or [NEW]; they are dropped wholesale.
    const std::vector<std::pair<std::string, std::string>> pairs = {
        {"[", "]"}, {"【", "】"}, {"［", "］"}, {"〔", "〕"}, {"《", "》"}};
    std::string text = input.text;
    for (const auto& pair : pairs) {
        std::size_t open = text.find(pair.first);
        while (open != std::string::npos) {
            const std::size_t close = text.find(pair.second, open + pair.first.size());
            if (close == std::string::npos) break;
            text.erase(open, close + pair.second.size() - open);
            open = text.find(pair.first, open);
        }
    }
    return TextFragment{text};
}

TextLines StringSanitizer::splitTitleSegments(const TextFragment& title) const {
    const std::vector<std::string> delimiters = {" | ", " - ", " – ", " — ", " :: ", " · ", " / ", "｜"};
    std::string text = stripPromotionalClutter(removeBracketedTags(title)).text;
    TextLines segments;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t best = std::string::npos;
        std::size_t bestLength = 0;
        for (const std::string& delimiter : delimiters) {
            const std::size_t at = text.find(delimiter, start);
            if (at != std::string::npos && (best == std::string::npos || at < best)) {
                best = at;
                bestLength = delimiter.size();
            }
        }
        const std::size_t end = best == std::string::npos ? text.size() : best;
        const TextFragment segment = normalizer_.trim(TextFragment{text.substr(start, end - start)});
        if (!segment.text.empty()) segments.lines.push_back(segment.text);
        if (best == std::string::npos) break;
        start = best + bestLength;
    }
    return segments;
}

TextFragment StringSanitizer::cleanProductName(const TextLines& titleSegments) const {
    // Shop titles conventionally lead with the product ("Product – Roaster").
    if (titleSegments.lines.empty()) return TextFragment{};
    return normalizer_.truncateUtf8(TextFragment{titleSegments.lines.front()}, 160);
}

TextFragment StringSanitizer::extractRoaster(const TextLines& titleSegments, const TextFragment& url) const {
    if (titleSegments.lines.size() >= 2) {
        const std::string& last = titleSegments.lines.back();
        if (last != cleanProductName(titleSegments).text) return normalizer_.truncateUtf8(TextFragment{last}, 80);
    }
    return hostnameOf(url);
}

TextLines StringSanitizer::splitInlineLabels(const TextLines& lines) const {
    // Shops often write specs inline: "Variety: Heirloom. Process: Natural.".
    // Each "Label:" that follows a sentence break starts its own line, so one
    // field's value never runs into the next field.
    const std::vector<std::string> breaks = {". ", "; ", " | ", " · ", " / ", "。", "．"};
    const InputLimits limits;
    TextLines out;
    for (std::size_t i = 0; i < lines.lines.size() && i < limits.maxLines; ++i) {
        const std::string& line = lines.lines[i];
        std::size_t start = 0;
        std::size_t scan = 0;
        while (scan < line.size()) {
            std::size_t cut = std::string::npos;
            std::size_t cutLength = 0;
            for (const std::string& mark : breaks) {
                const std::size_t at = line.find(mark, scan);
                if (at != std::string::npos && (cut == std::string::npos || at < cut)) {
                    cut = at;
                    cutLength = mark.size();
                }
            }
            if (cut == std::string::npos) break;
            const TextFragment next = normalizer_.trim(TextFragment{line.substr(cut + cutLength)});
            if (startsWithAnyLabel(next)) {
                out.lines.push_back(line.substr(start, cut - start));
                start = cut + cutLength;
            }
            scan = cut + cutLength;
        }
        out.lines.push_back(line.substr(start));
    }
    return out;
}

bool StringSanitizer::startsWithAnyLabel(const TextFragment& text) const {
    for (const LabelSet* set : {&originLabels_, &regionLabels_, &producerLabels_, &varietyLabels_, &processLabels_,
                                &altitudeLabels_, &roastLabels_}) {
        for (const std::string& label : set->labels) {
            const LabelMatch match = matchLabelOnLine(text, label);
            if (match.matched && !match.value.empty()) return true;
        }
    }
    return false;
}

TextFragment StringSanitizer::extractLabeledValue(const TextLines& lines, const LabelSet& labels) const {
    const InputLimits limits;
    const std::size_t count = std::min(lines.lines.size(), limits.maxLines);
    for (std::size_t i = 0; i < count; ++i) {
        const TextFragment line = normalizer_.stripLeadingBullets(TextFragment{lines.lines[i]});
        for (const std::string& label : labels.labels) {
            const LabelMatch match = matchLabelOnLine(line, label);
            if (!match.matched) continue;
            std::string value = match.value;
            if (value.empty() && i + 1 < count) value = lines.lines[i + 1];  // "Label\nValue" layout.
            TextFragment cleaned = normalizer_.trim(normalizer_.collapseWhitespace(TextFragment{value}));
            while (!cleaned.text.empty() && (cleaned.text.back() == '.' || cleaned.text.back() == ';')) {
                cleaned.text.pop_back();
            }
            if (!cleaned.text.empty() && cleaned.text.size() <= limits.signalLine) {
                return normalizer_.truncateUtf8(cleaned, limits.fieldValue);
            }
        }
    }
    return TextFragment{};
}

LabelMatch StringSanitizer::matchLabelOnLine(const TextFragment& line, const std::string& label) const {
    const std::string folded = normalizer_.foldCase(line).text;
    if (!startsWith(folded, label)) return LabelMatch{};
    std::string rest = line.text.substr(label.size());
    while (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
    if (rest.empty()) return LabelMatch{true, ""};
    const std::vector<std::string> separators = {":", "：", "\t", "|", "=", "-", "–"};
    for (const std::string& separator : separators) {
        if (startsWith(rest, separator)) {
            return LabelMatch{true, normalizer_.trim(TextFragment{rest.substr(separator.size())}).text};
        }
    }
    return LabelMatch{};
}

TextFragment StringSanitizer::findSignalLine(const TextLines& lines, const LabelSet& labels) const {
    const InputLimits limits;
    const std::size_t count = std::min(lines.lines.size(), limits.maxLines);
    for (std::size_t i = 0; i < count; ++i) {
        const std::string& line = lines.lines[i];
        if (line.size() > limits.signalLine) continue;
        const std::string folded = normalizer_.foldCase(TextFragment{line}).text;
        for (const std::string& label : labels.labels) {
            if (folded.find(label) != std::string::npos) return TextFragment{line};
        }
    }
    return TextFragment{};
}

TextFragment StringSanitizer::detectOriginCountry(const TextLines& lines, const RawPageData& page) const {
    const InputLimits limits;
    const TextFragment labeled = extractLabeledValue(lines, originLabels_);
    if (!labeled.text.empty()) {
        const AliasHit<std::string> hit = countryLexicon_.findIn(labeled);
        return hit.entry ? TextFragment{hit.entry->value} : labeled;
    }
    const std::vector<TextFragment> corpora = {
        TextFragment{page.title + "\n" + page.metaDescription},
        normalizer_.truncateUtf8(TextFragment{page.bodyText}, limits.corpusScan)};
    for (const TextFragment& corpus : corpora) {
        const AliasHit<std::string> hit = countryLexicon_.findIn(corpus);
        if (hit.entry) return TextFragment{hit.entry->value};
    }
    return TextFragment{};
}

TextFragment StringSanitizer::detectProcess(const TextLines& lines) const {
    const TextFragment labeled = extractLabeledValue(lines, processLabels_);
    const TextFragment candidate = labeled.text.empty() ? findSignalLine(lines, processLabels_) : labeled;
    if (candidate.text.empty()) return candidate;
    const AliasHit<std::string> hit = processLexicon_.findIn(candidate);
    if (hit.entry) return TextFragment{hit.entry->value};
    return labeled;
}

TextFragment StringSanitizer::detectRoastLevel(const TextLines& lines) const {
    const TextFragment labeled = extractLabeledValue(lines, roastLabels_);
    TextFragment candidate = labeled;
    if (candidate.text.empty()) {
        // Unlabeled fallback only trusts short lines: a long line such as
        // "Notes: dark chocolate, roasted almond" must not read as "Dark".
        candidate = findSignalLine(lines, roastLabels_);
        if (candidate.text.size() > 60) candidate = TextFragment{};
    }
    if (candidate.text.empty()) return candidate;
    const RoastClassification classification = roastScale_.classify(candidate);
    if (classification.stage != RoastStage::Unknown) return roastScale_.stageLabel(classification);
    return labeled;
}

bool StringSanitizer::looksLikeCoffeeProduct(const TextFragment& corpus) const {
    return coffeeSignalLexicon_.findIn(corpus).entry != nullptr;
}

TextFragment StringSanitizer::hostnameOf(const TextFragment& url) const {
    std::string host = url.text;
    const std::size_t scheme = host.find("://");
    if (scheme != std::string::npos) host = host.substr(scheme + 3);
    const std::size_t end = host.find_first_of("/?#:");
    if (end != std::string::npos) host = host.substr(0, end);
    host = normalizer_.foldCase(TextFragment{host}).text;
    for (const std::string& prefix : {std::string("www."), std::string("shop."), std::string("store.")}) {
        if (startsWith(host, prefix)) host = host.substr(prefix.size());
    }
    const std::size_t dot = host.find('.');
    return TextFragment{dot == std::string::npos ? host : host.substr(0, dot)};
}

}  // namespace cupano
