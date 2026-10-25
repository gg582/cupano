/**
 * @file json_payload_extractor.hpp
 * @brief Locates the JSON object inside free-form LLM text.
 */

#pragma once

#include "json/json_value.hpp"
#include "text/text_types.hpp"

#include <cstddef>
#include <string>

namespace cupano {

/** Locates the outermost JSON object inside free-form LLM text (fences, prose). */
class JsonPayloadExtractor {
public:
    JsonDocumentText extract(const TextFragment& llmText) const;

private:
    std::size_t findObjectEnd(const std::string& text, std::size_t start) const;
};

}  // namespace cupano
