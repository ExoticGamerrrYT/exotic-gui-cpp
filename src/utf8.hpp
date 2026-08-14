// Shared UTF-8 decoding. Text measurement and text rendering must walk a
// string identically, so they walk it with the same function.

#pragma once

#include <cstddef>
#include <string_view>

namespace exo::utf8 {

inline constexpr char32_t kReplacement = 0xFFFD;

/// Decodes the code point starting at `at` and returns how many bytes it used.
/// Malformed input yields U+FFFD and consumes exactly one byte, so a bad string
/// can neither loop forever nor read past the end.
inline std::size_t decode(std::string_view text, std::size_t at, char32_t& out) {
    const auto lead = static_cast<unsigned char>(text[at]);
    if (lead < 0x80) {
        out = lead;
        return 1;
    }

    int extra = 0;
    char32_t cp = 0;
    if ((lead & 0xE0) == 0xC0) {
        cp = lead & 0x1Fu;
        extra = 1;
    } else if ((lead & 0xF0) == 0xE0) {
        cp = lead & 0x0Fu;
        extra = 2;
    } else if ((lead & 0xF8) == 0xF0) {
        cp = lead & 0x07u;
        extra = 3;
    } else {
        out = kReplacement;
        return 1;
    }

    if (at + static_cast<std::size_t>(extra) >= text.size()) {
        out = kReplacement;
        return 1;
    }

    for (int k = 1; k <= extra; ++k) {
        const auto cont = static_cast<unsigned char>(text[at + static_cast<std::size_t>(k)]);
        if ((cont & 0xC0) != 0x80) {
            out = kReplacement;
            return 1;
        }
        cp = (cp << 6) | (cont & 0x3Fu);
    }

    out = cp;
    return static_cast<std::size_t>(extra) + 1;
}

} // namespace exo::utf8
