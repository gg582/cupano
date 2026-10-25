/**
 * @file roast_table.hpp
 * @brief The fixed multilingual roast table, including roastery dialects.
 */

#pragma once

#include "roast/roast_types.hpp"
#include "text/alias_lexicon.hpp"

#include <vector>

namespace cupano {

/** Builds a fresh copy of the roast alias table (no shared state). */
std::vector<AliasEntry<RoastStage>> buildRoastEntries();

}  // namespace cupano
