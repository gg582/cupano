/**
 * @file public_dto.hpp
 * @brief Public DTOs exchanged with JavaScript through embind value_objects.
 */

#pragma once

#include <string>
#include <vector>

namespace cupano {

/** Raw, untrusted text captured from the current product page by the JS layer. */
struct RawPageData {
    std::string url;
    std::string title;
    std::string metaDescription;
    std::string bodyText;
    std::string documentLanguage;
};

/** Structured coffee bean entity distilled from a RawPageData snapshot. */
struct BeanMetadata {
    std::string productName;
    std::string roaster;
    std::string originCountry;
    std::string region;
    std::string producer;
    std::string variety;
    std::string process;
    std::string altitude;
    std::string declaredRoastLevel;
    std::string sourceUrl;
    bool isCoffeeProduct = false;
};

/** The person using cupano: market for availability and personal taste. */
struct UserContext {
    std::string countryCode;
    std::string countryName;
    std::vector<std::string> preferredProcesses;
    std::string tastePreference;
};

/** Caller-tunable parameters for the swarm orchestration prompt. */
struct SwarmPromptConfig {
    BeanMetadata bean;
    UserContext user;
    int notesPerAgent = 5;
    int sourcesPerAgent = 3;
    std::string outputLocale;
};

/** Prompt pair ready to be dispatched to an LLM endpoint. */
struct SwarmPrompt {
    std::string systemInstruction;
    std::string userPrompt;
};

/** Raw LLM output paired with the bean and user it describes. */
struct AiResponsePayload {
    BeanMetadata bean;
    UserContext user;
    std::string rawResponseText;
};

/** One flavor note ranked by how frequently the regional agents reported it. */
struct SensoryNote {
    std::string descriptor;
    std::string category;
    int regionCount = 0;
    int mentionCount = 0;
    double consensusScore = 0.0;
    std::vector<std::string> sourceRegions;
    std::vector<std::string> nativeTerms;
};

/**
 * One flavor-wheel segment with how many regions placed notes in it (directly
 * or in a more specific child). `contested` marks segments named by fewer
 * than half of the regions that reported notes: where opinions split.
 */
struct WheelSegment {
    std::string name;
    std::string path;
    std::string parentPath;
    int tier = 1;
    int regionCount = 0;
    int mentionCount = 0;
    double consensusScore = 0.0;
    bool contested = false;
    std::vector<std::string> sourceRegions;
};

/** One store listing the LLM checked for stock (price/shipping deliberately ignored). */
struct SellerListing {
    std::string name;
    std::string market;
    std::string status;
    std::string url;
};

/** Sold-out verdicts derived by counting seller stock statuses. */
struct AvailabilityReport {
    std::string userCountry;
    std::string globalStatus;
    std::string localStatus;
    int sellersChecked = 0;
    int sellersInStock = 0;
    int localSellersChecked = 0;
    int localSellersInStock = 0;
    std::vector<SellerListing> sellers;
};

/** LLM judgment of how well the bean fits the user's stated preferences. */
struct TasteMatch {
    bool present = false;
    int score = 0;
    std::string reason;
};

/** Final unified profile returned to the JS layer for rendering. */
struct FinalCupanoProfile {
    bool success = false;
    std::string errorMessage;
    BeanMetadata bean;
    std::vector<SensoryNote> topNotes;
    std::string roastLevel;
    std::string roastStage;
    std::string roastAgtron;
    double roastConfidence = 0.0;
    int reportingRegions = 0;
    std::vector<std::string> discrepancies;
    std::vector<WheelSegment> flavorWheel;  // every wheel segment, in wheel order
    std::string summary;
    AvailabilityReport availability;
    TasteMatch tasteMatch;
};

}  // namespace cupano
