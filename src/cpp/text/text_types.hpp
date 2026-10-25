/**
 * @file text_types.hpp
 * @brief Value objects flowing between text-processing stages.
 */

#pragma once

#include <string>
#include <vector>

namespace cupano {

/** A single piece of free text flowing between text-processing stages. */
struct TextFragment {
    std::string text;
};

/** A multilingual set of labels (e.g. "Origin", "원산지", "産地"). */
struct LabelSet {
    std::vector<std::string> labels;
};

/** Ordered list of lines produced from a text block. */
struct TextLines {
    std::vector<std::string> lines;
};

/** Outcome of testing a single line against a single label. */
struct LabelMatch {
    bool matched = false;
    std::string value;
};

}  // namespace cupano
