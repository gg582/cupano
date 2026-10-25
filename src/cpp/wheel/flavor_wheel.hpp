/**
 * @file flavor_wheel.hpp
 * @brief The SCA / WCR Coffee Taster's Flavor Wheel (2016) taxonomy.
 *
 * The wheel is reference data, like the roast table: it does not read notes
 * from page text. The LLM places each note on the wheel ("wheel_path"); this
 * class only validates that placement and tells the frequency analysis which
 * segments (and their ancestors) a note covers.
 */

#pragma once

#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"

#include <string>
#include <vector>

namespace cupano {

/** One segment of the wheel. Tier 1 is the inner ring, tier 3 the outer one. */
struct WheelNode {
    std::string name;
    std::string path;    // "Fruity > Berry > Blueberry"
    int parent = -1;     // index into FlavorWheel::nodes(), -1 for tier 1
    int tier = 1;
};

/** Result of placing an LLM-supplied path on the wheel. */
struct WheelPlacement {
    int node = -1;       // deepest matching segment, -1 if the path is not on the wheel
};

class FlavorWheel {
public:
    explicit FlavorWheel(const TextNormalizer& normalizer);

    const std::vector<WheelNode>& nodes() const;

    /** Places "Tier1 > Tier2 > Tier3" (or just a known segment name) on the wheel. */
    WheelPlacement place(const TextFragment& wheelPath) const;

    /** Indices of a node and all of its ancestors, innermost last. */
    std::vector<int> lineage(int node) const;

    /** Compact vocabulary for the prompt: one line per tier-2 group. */
    TextFragment describeForPrompt() const;

private:
    std::string keyOf(const std::string& name) const;
    int findChild(int parent, const std::string& key) const;
    int findAnywhere(const std::string& key) const;

    const TextNormalizer& normalizer_;
    std::vector<WheelNode> nodes_;
};

}  // namespace cupano
