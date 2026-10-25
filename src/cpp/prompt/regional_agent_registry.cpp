/**
 * @file regional_agent_registry.cpp
 * @brief RegionalAgentRegistry implementation.
 */

#include "prompt/regional_agent_registry.hpp"

#include <vector>

namespace cupano {

RegionalAgentRegistry::RegionalAgentRegistry()
    : agents_{
          {"KO", "Korean (한국어)",
           "Korean domestic roastery and distributor product descriptions; consumer cupping reviews on "
           "Korean blogs, cafes and marketplaces",
           "원두 컵노트 로스팅", 1.0},
          {"JA", "Japanese (日本語)",
           "Japanese specialty drip and roast-profile references; traditional kissaten vocabulary and "
           "SCAJ tasting terms",
           "コーヒー豆 フレーバー 焙煎度", 1.0},
          {"ZH-TW", "Traditional Chinese (繁體中文, Taiwan)",
           "Cup of Excellence (COE) auction lot pages and Taiwanese precision cupping notes",
           "咖啡豆 風味 杯測 烘焙度", 1.0},
          {"EN", "English",
           "SCA flavor-wheel descriptors; importer and origin lot sheets; international roaster pages",
           "coffee tasting notes roast level", 1.0},
          {"NO", "Norwegian (Norsk)",
           "Nordic light-roast benchmarks and high-acidity flavor representations from Norwegian roasters",
           "kaffe smaksnotater lysbrent", 1.0},
          {"DE", "German (Deutsch)",
           "European specialty roaster benchmarks and direct-trade sensory sheets",
           "Kaffee Geschmacksnoten Röstgrad", 1.0},
      } {}

const std::vector<RegionalAgentSpec>& RegionalAgentRegistry::agents() const { return agents_; }

RegionLookup RegionalAgentRegistry::find(const RegionCodeQuery& query) const {
    for (const RegionalAgentSpec& spec : agents_) {
        if (spec.code == query.code) return RegionLookup{true, spec};
    }
    return RegionLookup{};
}

}  // namespace cupano
