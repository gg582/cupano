/**
 * @file flavor_wheel_table.hpp
 * @brief The wheel's segment names, as a plain nested list.
 */

#pragma once

#include <string>
#include <vector>

namespace cupano {

/** A tier-2 group and its tier-3 descriptors (empty when the group is itself a leaf). */
struct WheelGroupSpec {
    std::string name;
    std::vector<std::string> descriptors;
};

/** A tier-1 category and its groups. */
struct WheelCategorySpec {
    std::string name;
    std::vector<WheelGroupSpec> groups;
};

/** SCA / WCR Coffee Taster's Flavor Wheel, 2016 edition (9 categories, 3 rings). */
std::vector<WheelCategorySpec> buildFlavorWheelSpec();

}  // namespace cupano
