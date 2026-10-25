/**
 * @file text_predicates.hpp
 * @brief Small pure string predicates shared across translation units.
 */

#pragma once

#include <string>

namespace cupano {

/** True when value begins with prefix (byte-wise). */
inline bool startsWith(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace cupano
