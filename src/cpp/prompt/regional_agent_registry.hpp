/**
 * @file regional_agent_registry.hpp
 * @brief The six regional agents of the swarm.
 */

#pragma once

#include <string>
#include <vector>

namespace cupano {

/** Static description of one regional agent in the swarm. */
struct RegionalAgentSpec {
    std::string code;
    std::string languageName;
    std::string focus;
    std::string searchSuffix;
    double baseWeight = 1.0;
};

/** Lookup request for a regional agent by its region code. */
struct RegionCodeQuery {
    std::string code;
};

/** Result of resolving a region code against the registry. */
struct RegionLookup {
    bool found = false;
    RegionalAgentSpec spec;
};

/** Registry of the six regional agents participating in the swarm. */
class RegionalAgentRegistry {
public:
    RegionalAgentRegistry();

    const std::vector<RegionalAgentSpec>& agents() const;
    RegionLookup find(const RegionCodeQuery& query) const;

private:
    std::vector<RegionalAgentSpec> agents_;
};

}  // namespace cupano
