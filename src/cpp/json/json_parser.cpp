/**
 * @file json_parser.cpp
 * @brief JsonParser implementation.
 */

#include "json/json_parser.hpp"
#include "text/text_predicates.hpp"

#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>

namespace cupano {

JsonParseResult JsonParser::parse(const JsonDocumentText& document) const {
    JsonParseResult result;
    if (!document.found) {
        result.errorMessage = "No JSON object found in model response.";
        return result;
    }
    JsonCursor cursor{document.text};
    if (!parseValue(cursor, result.root)) {
        result.errorMessage = "Malformed JSON near byte " + std::to_string(cursor.position) + ".";
        return result;
    }
    skipWhitespace(cursor);
    if (cursor.position != cursor.text.size()) {
        result.errorMessage = "Unexpected trailing characters in JSON.";
        return result;
    }
    result.ok = true;
    return result;
}

bool JsonParser::parseValue(JsonCursor& cursor, JsonValue& out) const {
    skipWhitespace(cursor);
    if (cursor.position >= cursor.text.size() || cursor.depth > 64) return false;
    const char c = cursor.text[cursor.position];
    if (c == '{') return parseObject(cursor, out);
    if (c == '[') return parseArray(cursor, out);
    if (c == '"') {
        out.kind = JsonValue::Kind::String;
        return parseString(cursor, out.string);
    }
    if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(cursor, out);
    return parseLiteral(cursor, out);
}

bool JsonParser::parseObject(JsonCursor& cursor, JsonValue& out) const {
    out.kind = JsonValue::Kind::Object;
    ++cursor.position;
    ++cursor.depth;
    skipWhitespace(cursor);
    if (cursor.position < cursor.text.size() && cursor.text[cursor.position] == '}') {
        ++cursor.position;
        --cursor.depth;
        return true;
    }
    while (cursor.position < cursor.text.size()) {
        skipWhitespace(cursor);
        JsonMember member;
        if (cursor.position >= cursor.text.size() || cursor.text[cursor.position] != '"') return false;
        if (!parseString(cursor, member.key)) return false;
        skipWhitespace(cursor);
        if (cursor.position >= cursor.text.size() || cursor.text[cursor.position] != ':') return false;
        ++cursor.position;
        if (!parseValue(cursor, member.value)) return false;
        out.members.push_back(std::move(member));
        skipWhitespace(cursor);
        if (cursor.position >= cursor.text.size()) return false;
        const char next = cursor.text[cursor.position++];
        if (next == '}') {
            --cursor.depth;
            return true;
        }
        if (next != ',') return false;
    }
    return false;
}

bool JsonParser::parseArray(JsonCursor& cursor, JsonValue& out) const {
    out.kind = JsonValue::Kind::Array;
    ++cursor.position;
    ++cursor.depth;
    skipWhitespace(cursor);
    if (cursor.position < cursor.text.size() && cursor.text[cursor.position] == ']') {
        ++cursor.position;
        --cursor.depth;
        return true;
    }
    while (cursor.position < cursor.text.size()) {
        JsonValue item;
        if (!parseValue(cursor, item)) return false;
        out.items.push_back(std::move(item));
        skipWhitespace(cursor);
        if (cursor.position >= cursor.text.size()) return false;
        const char next = cursor.text[cursor.position++];
        if (next == ']') {
            --cursor.depth;
            return true;
        }
        if (next != ',') return false;
    }
    return false;
}

bool JsonParser::parseString(JsonCursor& cursor, std::string& out) const {
    ++cursor.position;  // opening quote
    while (cursor.position < cursor.text.size()) {
        const char c = cursor.text[cursor.position++];
        if (c == '"') return true;
        if (c != '\\') {
            // LLMs occasionally emit raw newlines inside strings; tolerate them.
            out.push_back(static_cast<unsigned char>(c) < 0x20 ? ' ' : c);
            continue;
        }
        if (cursor.position >= cursor.text.size()) return false;
        const char escape = cursor.text[cursor.position++];
        switch (escape) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u':
                if (!parseUnicodeEscape(cursor, out)) return false;
                break;
            default: return false;
        }
    }
    return false;
}

bool JsonParser::parseUnicodeEscape(JsonCursor& cursor, std::string& out) const {
    unsigned int codePoint = 0;
    if (!readHex4(cursor, codePoint)) return false;
    if (codePoint >= 0xD800 && codePoint <= 0xDBFF) {
        const std::size_t save = cursor.position;
        unsigned int low = 0;
        if (cursor.position + 1 < cursor.text.size() && cursor.text[cursor.position] == '\\' &&
            cursor.text[cursor.position + 1] == 'u') {
            cursor.position += 2;
            if (readHex4(cursor, low) && low >= 0xDC00 && low <= 0xDFFF) {
                appendUtf8(0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00), out);
                return true;
            }
        }
        cursor.position = save;
        codePoint = 0xFFFD;  // Lone high surrogate.
    } else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF) {
        codePoint = 0xFFFD;  // Lone low surrogate.
    }
    appendUtf8(codePoint, out);
    return true;
}

bool JsonParser::readHex4(JsonCursor& cursor, unsigned int& out) const {
    if (cursor.position + 4 > cursor.text.size()) return false;
    out = 0;
    for (int i = 0; i < 4; ++i) {
        const char c = cursor.text[cursor.position++];
        out <<= 4;
        if (c >= '0' && c <= '9') out |= static_cast<unsigned int>(c - '0');
        else if (c >= 'a' && c <= 'f') out |= static_cast<unsigned int>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') out |= static_cast<unsigned int>(c - 'A' + 10);
        else return false;
    }
    return true;
}

void JsonParser::appendUtf8(unsigned int codePoint, std::string& out) const {
    if (codePoint < 0x80) {
        out.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

bool JsonParser::parseNumber(JsonCursor& cursor, JsonValue& out) const {
    const std::size_t start = cursor.position;
    while (cursor.position < cursor.text.size()) {
        const char c = cursor.text[cursor.position];
        const bool numeric = (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E';
        if (!numeric) break;
        ++cursor.position;
    }
    const std::string literal = cursor.text.substr(start, cursor.position - start);
    char* end = nullptr;
    const double value = std::strtod(literal.c_str(), &end);
    if (literal.empty() || end != literal.c_str() + literal.size()) return false;
    out.kind = JsonValue::Kind::Number;
    out.number = value;
    return true;
}

bool JsonParser::parseLiteral(JsonCursor& cursor, JsonValue& out) const {
    const std::string rest = cursor.text.substr(cursor.position, 5);
    if (startsWith(rest, "true")) {
        out.kind = JsonValue::Kind::Boolean;
        out.boolean = true;
        cursor.position += 4;
        return true;
    }
    if (startsWith(rest, "false")) {
        out.kind = JsonValue::Kind::Boolean;
        out.boolean = false;
        cursor.position += 5;
        return true;
    }
    if (startsWith(rest, "null")) {
        out.kind = JsonValue::Kind::Null;
        cursor.position += 4;
        return true;
    }
    return false;
}

void JsonParser::skipWhitespace(JsonCursor& cursor) const {
    while (cursor.position < cursor.text.size()) {
        const char c = cursor.text[cursor.position];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') break;
        ++cursor.position;
    }
}

}  // namespace cupano
