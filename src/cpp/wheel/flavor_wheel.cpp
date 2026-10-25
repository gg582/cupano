/**
 * @file flavor_wheel.cpp
 * @brief FlavorWheel implementation.
 */

#include "wheel/flavor_wheel.hpp"

#include "wheel/flavor_wheel_table.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace cupano {

FlavorWheel::FlavorWheel(const TextNormalizer& normalizer) : normalizer_(normalizer) {
    for (const WheelCategorySpec& category : buildFlavorWheelSpec()) {
        const int categoryIndex = static_cast<int>(nodes_.size());
        nodes_.push_back(WheelNode{category.name, category.name, -1, 1});
        for (const WheelGroupSpec& group : category.groups) {
            const int groupIndex = static_cast<int>(nodes_.size());
            const std::string groupPath = category.name + " > " + group.name;
            nodes_.push_back(WheelNode{group.name, groupPath, categoryIndex, 2});
            for (const std::string& descriptor : group.descriptors) {
                nodes_.push_back(WheelNode{descriptor, groupPath + " > " + descriptor, groupIndex, 3});
            }
        }
    }
}

const std::vector<WheelNode>& FlavorWheel::nodes() const { return nodes_; }

WheelPlacement FlavorWheel::place(const TextFragment& wheelPath) const {
    // Segments are separated by ">" (also "›" / "»"); "/" belongs to names
    // such as "Sour/Fermented", so it is never a separator.
    std::string text = wheelPath.text;
    for (const std::string& arrow : {std::string("›"), std::string("»"), std::string("→")}) {
        for (std::size_t at = text.find(arrow); at != std::string::npos; at = text.find(arrow, at + 1)) {
            text.replace(at, arrow.size(), ">");
        }
    }
    std::vector<std::string> keys;
    std::stringstream stream(text);
    for (std::string part; std::getline(stream, part, '>');) {
        const std::string key = keyOf(part);
        if (!key.empty()) keys.push_back(key);
    }
    if (keys.empty()) return WheelPlacement{};

    // Walk from the inner ring outwards for as long as the path stays on the wheel.
    int current = -1;
    for (const std::string& key : keys) {
        const int next = findChild(current, key);
        if (next < 0) break;
        current = next;
    }
    if (current >= 0) return WheelPlacement{current};
    // The model skipped the inner rings: accept the most specific known name.
    for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
        const int hit = findAnywhere(*it);
        if (hit >= 0) return WheelPlacement{hit};
    }
    return WheelPlacement{};
}

std::vector<int> FlavorWheel::lineage(int node) const {
    std::vector<int> chain;
    for (int at = node; at >= 0 && at < static_cast<int>(nodes_.size()); at = nodes_[static_cast<std::size_t>(at)].parent) {
        chain.push_back(at);
    }
    return chain;
}

TextFragment FlavorWheel::describeForPrompt() const {
    std::ostringstream s;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        const WheelNode& node = nodes_[i];
        if (node.tier != 2) continue;
        s << "- " << node.path;
        bool first = true;
        for (std::size_t j = i + 1; j < nodes_.size() && nodes_[j].tier == 3; ++j) {
            s << (first ? " > " : " | ") << nodes_[j].name;
            first = false;
        }
        s << "\n";
    }
    return TextFragment{s.str()};
}

std::string FlavorWheel::keyOf(const std::string& name) const {
    // Lexical only: case, spacing and punctuation ("Hay-like" == "hay like").
    const std::string folded = normalizer_.foldCase(TextFragment{name}).text;
    std::string key;
    for (char c : folded) {
        if (std::isalnum(static_cast<unsigned char>(c)) || (static_cast<unsigned char>(c) & 0x80)) key.push_back(c);
    }
    return key;
}

int FlavorWheel::findChild(int parent, const std::string& key) const {
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (nodes_[i].parent == parent && keyOf(nodes_[i].name) == key) return static_cast<int>(i);
    }
    return -1;
}

int FlavorWheel::findAnywhere(const std::string& key) const {
    int best = -1;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (keyOf(nodes_[i].name) != key) continue;
        // Same name on two rings ("Green/Vegetative", "Floral"): prefer the outer one.
        if (best < 0 || nodes_[i].tier > nodes_[static_cast<std::size_t>(best)].tier) best = static_cast<int>(i);
    }
    return best;
}

}  // namespace cupano
