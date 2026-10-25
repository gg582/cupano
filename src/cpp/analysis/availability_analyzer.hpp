/**
 * @file availability_analyzer.hpp
 * @brief Decides whether a bean is sold out globally and in the user's country.
 */

#pragma once

#include "analysis/analysis_types.hpp"
#include "core/public_dto.hpp"
#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

#include <string>
#include <vector>

namespace cupano {

/** Inputs for the sold-out analysis. */
struct AvailabilityInput {
    const SellerListings* sellers = nullptr;
    std::string userCountry;
};

/** Stock counts over a subset of sellers. */
struct StockTally {
    int checked = 0;
    int inStock = 0;
    int soldOut = 0;
};

/** Counts stock statuses and decides "sold out everywhere" globally and locally. */
class AvailabilityAnalyzer {
public:
    explicit AvailabilityAnalyzer(const TextNormalizer& normalizer);

    AvailabilityReport analyze(const AvailabilityInput& input) const;

private:
    StockTally tally(const std::vector<SellerListing>& sellers, const std::string& market) const;
    TextFragment verdict(const StockTally& tally) const;

    const TextNormalizer& normalizer_;
};

}  // namespace cupano
