/**
 * @file analysis_types.hpp
 * @brief Value objects describing a parsed swarm response.
 */

#pragma once

#include "core/public_dto.hpp"

#include <string>
#include <vector>

namespace cupano {

/** One flavor note exactly as a regional agent (the LLM) reported it. */
struct RawSensoryNote {
    std::string nativeTerm;
    std::string canonicalTerm;
    std::string category;
    std::string wheelPath;  // where the LLM placed it: "Fruity > Berry > Blueberry"
    int mentions = 1;
};

/** Everything one regional agent reported about the bean. */
struct RegionalReport {
    std::string regionCode;
    double weight = 1.0;
    int sourcesFound = 0;
    std::string roastTerm;
    std::string roastHint;
    std::vector<RawSensoryNote> notes;
};

/** Seller listings extracted from the model response. */
struct SellerListings {
    std::vector<SellerListing> items;
};

/** Collection of regional reports plus free-form model commentary. */
struct ParsedSwarmResponse {
    bool ok = false;
    std::string errorMessage;
    std::vector<RegionalReport> reports;
    std::vector<std::string> modelDiscrepancies;
    std::string summary;
    SellerListings sellers;
    TasteMatch tasteMatch;
};

}  // namespace cupano
