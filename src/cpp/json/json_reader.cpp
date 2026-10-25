/**
 * @file json_reader.cpp
 * @brief JsonReader implementation.
 */

#include "json/json_reader.hpp"

#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

namespace cupano {

const JsonValue* JsonReader::member(const JsonValue& object, const std::string& key) const {
    if (object.kind != JsonValue::Kind::Object) return nullptr;
    for (const JsonMember& m : object.members) {
        if (m.key == key) return &m.value;
    }
    return nullptr;
}

std::string JsonReader::stringOr(const JsonValue* value, const std::string& fallback) const {
    if (!value) return fallback;
    if (value->kind == JsonValue::Kind::String) return value->string;
    if (value->kind == JsonValue::Kind::Number) {
        std::ostringstream stream;
        stream << value->number;
        return stream.str();
    }
    return fallback;
}

double JsonReader::numberOr(const JsonValue* value, double fallback) const {
    if (!value) return fallback;
    if (value->kind == JsonValue::Kind::Number && std::isfinite(value->number)) return value->number;
    if (value->kind == JsonValue::Kind::String) {
        char* end = nullptr;
        const double parsed = std::strtod(value->string.c_str(), &end);
        if (end != value->string.c_str() && std::isfinite(parsed)) return parsed;
    }
    return fallback;
}

}  // namespace cupano
