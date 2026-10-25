/**
 * @file text_normalizer.cpp
 * @brief TextNormalizer implementation.
 */

#include "text/text_normalizer.hpp"
#include "text/text_predicates.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace cupano {

TextFragment TextNormalizer::foldCase(const TextFragment& input) const {
    // Byte-length preserving: ASCII A-Z and Latin-1 supplement capitals
    // (U+00C0..U+00DE, except U+00D7) are lowered in place. Other scripts
    // (Hangul, Kana, Han) have no case and pass through untouched.
    std::string out = input.text;
    for (std::size_t i = 0; i < out.size(); ++i) {
        const unsigned char byte = static_cast<unsigned char>(out[i]);
        if (byte >= 'A' && byte <= 'Z') {
            out[i] = static_cast<char>(byte + 32);
        } else if (byte == 0xC3 && i + 1 < out.size()) {
            const unsigned char next = static_cast<unsigned char>(out[i + 1]);
            if (next >= 0x80 && next <= 0x9E && next != 0x97) {
                out[i + 1] = static_cast<char>(next + 0x20);
            }
            ++i;
        }
    }
    return TextFragment{out};
}

TextFragment TextNormalizer::trim(const TextFragment& input) const {
    const std::string& s = input.text;
    auto isSpace = [](unsigned char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; };
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && isSpace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && isSpace(static_cast<unsigned char>(s[end - 1]))) --end;
    return TextFragment{s.substr(begin, end - begin)};
}

TextFragment TextNormalizer::collapseWhitespace(const TextFragment& input) const {
    const std::string& s = input.text;
    std::string out;
    out.reserve(s.size());
    bool pendingSpace = false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        bool isSpace = c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
        std::size_t width = 1;
        if (c == 0xC2 && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0xA0) {
            isSpace = true;  // U+00A0 NO-BREAK SPACE
            width = 2;
        } else if (c == 0xE3 && i + 2 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x80 &&
                   static_cast<unsigned char>(s[i + 2]) == 0x80) {
            isSpace = true;  // U+3000 IDEOGRAPHIC SPACE
            width = 3;
        }
        if (isSpace) {
            pendingSpace = !out.empty();
            i += width - 1;
            continue;
        }
        if (pendingSpace) out.push_back(' ');
        pendingSpace = false;
        out.push_back(static_cast<char>(c));
    }
    return TextFragment{out};
}

TextFragment TextNormalizer::stripControlCharacters(const TextFragment& input) const {
    std::string out;
    out.reserve(input.text.size());
    for (char c : input.text) {
        const unsigned char byte = static_cast<unsigned char>(c);
        if ((byte < 0x20 && byte != '\n' && byte != '\t') || byte == 0x7F) continue;
        out.push_back(c);
    }
    return TextFragment{out};
}

TextFragment TextNormalizer::truncateUtf8(const TextFragment& input, std::size_t maxBytes) const {
    if (input.text.size() <= maxBytes) return input;
    std::size_t cut = maxBytes;
    // Never split a multi-byte sequence: back off over continuation bytes.
    while (cut > 0 && (static_cast<unsigned char>(input.text[cut]) & 0xC0) == 0x80) --cut;
    return TextFragment{input.text.substr(0, cut)};
}

TextFragment TextNormalizer::stripLeadingBullets(const TextFragment& input) const {
    const std::vector<std::string> bullets = {"-", "*", "+", ">", "•", "·", "・", "■", "□", "▶", "▷", "◆", "◇", "●", "○", "※", "✓", "✔"};
    std::string s = input.text;
    bool stripped = true;
    while (stripped && !s.empty()) {
        stripped = false;
        for (const std::string& bullet : bullets) {
            if (startsWith(s, bullet)) {
                s.erase(0, bullet.size());
                stripped = true;
            }
        }
        while (!s.empty() && s[0] == ' ') {
            s.erase(0, 1);
            stripped = true;
        }
    }
    return TextFragment{s};
}

TextFragment TextNormalizer::toUpperAscii(const TextFragment& input) const {
    std::string out = input.text;
    for (char& c : out) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
    }
    return TextFragment{out};
}

TextLines TextNormalizer::splitLines(const TextFragment& input) const {
    TextLines result;
    std::string current;
    auto flush = [&]() {
        TextFragment line = trim(TextFragment{current});
        if (!line.text.empty()) result.lines.push_back(line.text);
        current.clear();
    };
    for (char c : input.text) {
        if (c == '\n' || c == '\r') {
            flush();
        } else {
            current.push_back(c);
        }
    }
    flush();
    return result;
}

}  // namespace cupano
