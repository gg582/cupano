/**
 * @file availability_analyzer.cpp
 * @brief AvailabilityAnalyzer implementation.
 */

#include "analysis/availability_analyzer.hpp"

#include <string>
#include <vector>

namespace cupano {

AvailabilityAnalyzer::AvailabilityAnalyzer(const TextNormalizer& normalizer) : normalizer_(normalizer) {}

AvailabilityReport AvailabilityAnalyzer::analyze(const AvailabilityInput& input) const {
    AvailabilityReport report;
    report.userCountry = normalizer_.toUpperAscii(normalizer_.trim(TextFragment{input.userCountry})).text;
    if (input.sellers) report.sellers = input.sellers->items;

    const StockTally all = tally(report.sellers, "");
    report.sellersChecked = all.checked;
    report.sellersInStock = all.inStock;
    report.globalStatus = verdict(all).text;

    if (report.userCountry.empty()) {
        report.localStatus = "UNKNOWN";
        return report;
    }
    const StockTally local = tally(report.sellers, report.userCountry);
    report.localSellersChecked = local.checked;
    report.localSellersInStock = local.inStock;
    report.localStatus = verdict(local).text;
    return report;
}

StockTally AvailabilityAnalyzer::tally(const std::vector<SellerListing>& sellers, const std::string& market) const {
    StockTally result;
    for (const SellerListing& seller : sellers) {
        if (!market.empty() && seller.market != market) continue;
        if (seller.status == "unknown") continue;
        ++result.checked;
        if (seller.status == "in_stock") ++result.inStock;
        if (seller.status == "sold_out") ++result.soldOut;
    }
    return result;
}

TextFragment AvailabilityAnalyzer::verdict(const StockTally& tally) const {
    // SOLD_OUT only when every seller with a known status is sold out.
    if (tally.checked == 0) return TextFragment{"UNKNOWN"};
    return TextFragment{tally.inStock > 0 ? "AVAILABLE" : "SOLD_OUT"};
}

}  // namespace cupano
