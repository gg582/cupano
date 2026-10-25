/**
 * @file json_payload_extractor.cpp
 * @brief JsonPayloadExtractor implementation.
 */

#include "json/json_payload_extractor.hpp"

#include <cstddef>
#include <string>

namespace cupano {

JsonDocumentText JsonPayloadExtractor::extract(const TextFragment& llmText) const {
    const std::string& text = llmText.text;
    std::size_t start = text.find('{');
    int attempts = 0;
    while (start != std::string::npos && attempts < 8) {
        const std::size_t end = findObjectEnd(text, start);
        if (end != std::string::npos) {
            return JsonDocumentText{true, text.substr(start, end - start + 1)};
        }
        start = text.find('{', start + 1);
        ++attempts;
    }
    return JsonDocumentText{};
}

std::size_t JsonPayloadExtractor::findObjectEnd(const std::string& text, std::size_t start) const {
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (std::size_t i = start; i < text.size(); ++i) {
        const char c = text[i];
        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            inString = true;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::string::npos;
}

}  // namespace cupano
