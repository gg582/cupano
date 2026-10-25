/**
 * @file json_value.hpp
 * @brief Minimal JSON model (no third-party dependency is bundled into the Wasm).
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace cupano {

struct JsonMember;

/** Tagged JSON value tree. */
struct JsonValue {
    enum class Kind { Null, Boolean, Number, String, Array, Object };
    Kind kind = Kind::Null;
    bool boolean = false;
    double number = 0.0;
    std::string string;
    std::vector<JsonValue> items;
    std::vector<JsonMember> members;
};

/** Key/value pair inside a JSON object. */
struct JsonMember {
    std::string key;
    JsonValue value;
};

/** Raw JSON document text isolated from surrounding LLM prose. */
struct JsonDocumentText {
    bool found = false;
    std::string text;
};

/** Result of parsing a JsonDocumentText. */
struct JsonParseResult {
    bool ok = false;
    std::string errorMessage;
    JsonValue root;
};

/** Mutable cursor owned by a single parse invocation (never shared). */
struct JsonCursor {
    const std::string& text;
    std::size_t position = 0;
    int depth = 0;
};

}  // namespace cupano
