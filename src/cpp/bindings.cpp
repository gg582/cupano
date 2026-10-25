/**
 * @file bindings.cpp
 * @brief embind registrations: the public DTOs as value_objects and the
 *        NormalizerFacade as the only exported class.
 */
#include "normalizer.hpp"

#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(cupano_core) {
    using namespace emscripten;
    using namespace cupano;

    register_vector<std::string>("VectorString");
    register_vector<SensoryNote>("VectorSensoryNote");
    register_vector<SellerListing>("VectorSellerListing");
    register_vector<WheelSegment>("VectorWheelSegment");

    value_object<RawPageData>("RawPageData")
        .field("url", &RawPageData::url)
        .field("title", &RawPageData::title)
        .field("metaDescription", &RawPageData::metaDescription)
        .field("bodyText", &RawPageData::bodyText)
        .field("documentLanguage", &RawPageData::documentLanguage);

    value_object<BeanMetadata>("BeanMetadata")
        .field("productName", &BeanMetadata::productName)
        .field("roaster", &BeanMetadata::roaster)
        .field("originCountry", &BeanMetadata::originCountry)
        .field("region", &BeanMetadata::region)
        .field("producer", &BeanMetadata::producer)
        .field("variety", &BeanMetadata::variety)
        .field("process", &BeanMetadata::process)
        .field("altitude", &BeanMetadata::altitude)
        .field("declaredRoastLevel", &BeanMetadata::declaredRoastLevel)
        .field("sourceUrl", &BeanMetadata::sourceUrl)
        .field("isCoffeeProduct", &BeanMetadata::isCoffeeProduct);

    value_object<UserContext>("UserContext")
        .field("countryCode", &UserContext::countryCode)
        .field("countryName", &UserContext::countryName)
        .field("preferredProcesses", &UserContext::preferredProcesses)
        .field("tastePreference", &UserContext::tastePreference);

    value_object<SwarmPromptConfig>("SwarmPromptConfig")
        .field("bean", &SwarmPromptConfig::bean)
        .field("user", &SwarmPromptConfig::user)
        .field("notesPerAgent", &SwarmPromptConfig::notesPerAgent)
        .field("sourcesPerAgent", &SwarmPromptConfig::sourcesPerAgent)
        .field("outputLocale", &SwarmPromptConfig::outputLocale);

    value_object<SwarmPrompt>("SwarmPrompt")
        .field("systemInstruction", &SwarmPrompt::systemInstruction)
        .field("userPrompt", &SwarmPrompt::userPrompt);

    value_object<AiResponsePayload>("AiResponsePayload")
        .field("bean", &AiResponsePayload::bean)
        .field("user", &AiResponsePayload::user)
        .field("rawResponseText", &AiResponsePayload::rawResponseText);

    value_object<SensoryNote>("SensoryNote")
        .field("descriptor", &SensoryNote::descriptor)
        .field("category", &SensoryNote::category)
        .field("regionCount", &SensoryNote::regionCount)
        .field("mentionCount", &SensoryNote::mentionCount)
        .field("consensusScore", &SensoryNote::consensusScore)
        .field("sourceRegions", &SensoryNote::sourceRegions)
        .field("nativeTerms", &SensoryNote::nativeTerms);

    value_object<SellerListing>("SellerListing")
        .field("name", &SellerListing::name)
        .field("market", &SellerListing::market)
        .field("status", &SellerListing::status)
        .field("url", &SellerListing::url);

    value_object<AvailabilityReport>("AvailabilityReport")
        .field("userCountry", &AvailabilityReport::userCountry)
        .field("globalStatus", &AvailabilityReport::globalStatus)
        .field("localStatus", &AvailabilityReport::localStatus)
        .field("sellersChecked", &AvailabilityReport::sellersChecked)
        .field("sellersInStock", &AvailabilityReport::sellersInStock)
        .field("localSellersChecked", &AvailabilityReport::localSellersChecked)
        .field("localSellersInStock", &AvailabilityReport::localSellersInStock)
        .field("sellers", &AvailabilityReport::sellers);

    value_object<TasteMatch>("TasteMatch")
        .field("present", &TasteMatch::present)
        .field("score", &TasteMatch::score)
        .field("reason", &TasteMatch::reason);

    value_object<WheelSegment>("WheelSegment")
        .field("name", &WheelSegment::name)
        .field("path", &WheelSegment::path)
        .field("parentPath", &WheelSegment::parentPath)
        .field("tier", &WheelSegment::tier)
        .field("regionCount", &WheelSegment::regionCount)
        .field("mentionCount", &WheelSegment::mentionCount)
        .field("consensusScore", &WheelSegment::consensusScore)
        .field("contested", &WheelSegment::contested)
        .field("sourceRegions", &WheelSegment::sourceRegions);

    value_object<FinalCupanoProfile>("FinalCupanoProfile")
        .field("success", &FinalCupanoProfile::success)
        .field("errorMessage", &FinalCupanoProfile::errorMessage)
        .field("bean", &FinalCupanoProfile::bean)
        .field("topNotes", &FinalCupanoProfile::topNotes)
        .field("roastLevel", &FinalCupanoProfile::roastLevel)
        .field("roastStage", &FinalCupanoProfile::roastStage)
        .field("roastAgtron", &FinalCupanoProfile::roastAgtron)
        .field("roastConfidence", &FinalCupanoProfile::roastConfidence)
        .field("reportingRegions", &FinalCupanoProfile::reportingRegions)
        .field("discrepancies", &FinalCupanoProfile::discrepancies)
        .field("flavorWheel", &FinalCupanoProfile::flavorWheel)
        .field("summary", &FinalCupanoProfile::summary)
        .field("availability", &FinalCupanoProfile::availability)
        .field("tasteMatch", &FinalCupanoProfile::tasteMatch);

    class_<NormalizerFacade>("NormalizerFacade")
        .constructor<>()
        .function("extractMetadata", &NormalizerFacade::extractMetadata)
        .function("buildSwarmPrompt", &NormalizerFacade::buildSwarmPrompt)
        .function("normalizeResponse", &NormalizerFacade::normalizeResponse);
}
#endif
