/**
 * @file json_reader.hpp
 * @brief Type-tolerant read accessors over a JsonValue tree.
 */

#pragma once

#include "json/json_value.hpp"

#include <string>

namespace cupano {

/** Read-only, type-tolerant accessors over a JsonValue tree. */
class JsonReader {
public:
    const JsonValue* member(const JsonValue& object, const std::string& key) const;
    std::string stringOr(const JsonValue* value, const std::string& fallback) const;
    double numberOr(const JsonValue* value, double fallback) const;
};

}  // namespace cupano
