/**
 * @file availability_parser.cpp
 * @brief AvailabilityParser implementation.
 */

#include "analysis/availability_parser.hpp"
#include "text/text_predicates.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace cupano {

AvailabilityParser::AvailabilityParser(const JsonReader& reader, const TextNormalizer& normalizer)
    : reader_(reader), normalizer_(normalizer) {}

SellerListings AvailabilityParser::parse(const JsonValue& root) const {
    SellerListings listings;
    const JsonValue* availability = reader_.member(root, "availability");
    const JsonValue* sellers = availability ? reader_.member(*availability, "sellers") : nullptr;
    if (!sellers || sellers->kind != JsonValue::Kind::Array) return listings;
    for (const JsonValue& node : sellers->items) {
        if (listings.items.size() >= 24) break;
        SellerListing seller;
        if (parseSeller(node, seller)) listings.items.push_back(std::move(seller));
    }
    return listings;
}

bool AvailabilityParser::parseSeller(const JsonValue& node, SellerListing& out) const {
    if (node.kind != JsonValue::Kind::Object) return false;
    const TextFragment name{reader_.stringOr(reader_.member(node, "name"), "")};
    out.name = normalizer_.truncateUtf8(normalizer_.trim(normalizer_.stripControlCharacters(name)), 80).text;
    out.market = normalizeMarket(TextFragment{reader_.stringOr(reader_.member(node, "market"), "")}).text;
    out.status = normalizeStatus(TextFragment{reader_.stringOr(reader_.member(node, "status"), "")}).text;
    out.url = safeUrl(TextFragment{reader_.stringOr(reader_.member(node, "url"), "")}).text;
    return !out.name.empty();
}

TextFragment AvailabilityParser::normalizeStatus(const TextFragment& status) const {
    // Tolerates casing/spelling variants of the three enum values the prompt asks for.
    std::string folded = normalizer_.foldCase(normalizer_.trim(status)).text;
    std::replace(folded.begin(), folded.end(), '-', '_');
    std::replace(folded.begin(), folded.end(), ' ', '_');
    if (folded == "in_stock" || folded == "available" || folded == "instock") return TextFragment{"in_stock"};
    if (folded == "sold_out" || folded == "out_of_stock" || folded == "soldout" || folded == "unavailable") {
        return TextFragment{"sold_out"};
    }
    return TextFragment{"unknown"};
}

TextFragment AvailabilityParser::normalizeMarket(const TextFragment& market) const {
    const std::string upper = normalizer_.toUpperAscii(normalizer_.trim(market)).text;
    if (upper == "GLOBAL" || upper.empty()) return TextFragment{"GLOBAL"};
    const bool isIsoCode = upper.size() == 2 && std::isalpha(static_cast<unsigned char>(upper[0])) &&
                           std::isalpha(static_cast<unsigned char>(upper[1]));
    // Unparseable markets still count toward the global tally but never match the user's country.
    return TextFragment{isIsoCode ? upper : "OTHER"};
}

TextFragment AvailabilityParser::safeUrl(const TextFragment& url) const {
    // Only absolute http(s) links without whitespace or quotes reach the UI.
    const std::string trimmed = normalizer_.trim(url).text;
    const std::string folded = normalizer_.foldCase(TextFragment{trimmed}).text;
    if (!startsWith(folded, "https://") && !startsWith(folded, "http://")) return TextFragment{};
    if (trimmed.size() > 500 || trimmed.find_first_of(" \t\"'<>") != std::string::npos) return TextFragment{};
    return TextFragment{trimmed};
}

}  // namespace cupano
