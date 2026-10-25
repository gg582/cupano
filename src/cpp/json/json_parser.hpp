/**
 * @file json_parser.hpp
 * @brief Strict recursive-descent JSON parser.
 */

#pragma once

#include "json/json_value.hpp"

#include <string>

namespace cupano {

/** Strict recursive-descent JSON parser with a bounded nesting depth. */
class JsonParser {
public:
    JsonParseResult parse(const JsonDocumentText& document) const;

private:
    bool parseValue(JsonCursor& cursor, JsonValue& out) const;
    bool parseObject(JsonCursor& cursor, JsonValue& out) const;
    bool parseArray(JsonCursor& cursor, JsonValue& out) const;
    bool parseString(JsonCursor& cursor, std::string& out) const;
    bool parseNumber(JsonCursor& cursor, JsonValue& out) const;
    bool parseLiteral(JsonCursor& cursor, JsonValue& out) const;
    bool parseUnicodeEscape(JsonCursor& cursor, std::string& out) const;
    bool readHex4(JsonCursor& cursor, unsigned int& out) const;
    void appendUtf8(unsigned int codePoint, std::string& out) const;
    void skipWhitespace(JsonCursor& cursor) const;
};

}  // namespace cupano
