// Exotic GUI - font atlas.
//
// One TrueType face is rasterised into a single 8-bit alpha atlas at load time
// (ASCII + Latin-1, 2x2 oversampled). Drawing text is then just quads, and the
// whole UI still fits in one draw call per clip rectangle.

#pragma once

#include <exotic/types.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace exo {

/// A rasterised font. Needs a current OpenGL context, so create it after the
/// window. Move-only; the destructor releases the atlas texture.
class Font {
public:
    Font();
    ~Font();

    Font(Font&&) noexcept;
    Font& operator=(Font&&) noexcept;
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    /// Loads a .ttf/.otf file. Returns an invalid Font on failure.
    static Font from_file(const std::string& path, float pixel_height);
    static Font from_memory(const std::uint8_t* data, std::size_t size, float pixel_height);
    /// Picks a UI font from the operating system (Segoe UI, then Arial, then
    /// Tahoma on Windows). Returns an invalid Font when none is readable.
    static Font system_ui(float pixel_height);

    bool valid() const;
    /// Why loading failed, when valid() is false.
    const std::string& error() const;

    float pixel_height() const;
    /// Distance between baselines, already including the line gap.
    float line_height() const;
    /// Distance from the top of a line to the baseline.
    float ascent() const;
    /// Negative: distance from the baseline to the bottom of a line.
    float descent() const;

    /// Width of the widest line of `str`, in pixels.
    float measure(std::string_view str) const;
    /// Width and height of the whole block, honouring '\n'.
    Vec2 measure_block(std::string_view str) const;

    /// Places one glyph. `pen` sits on the baseline and is advanced. Returns
    /// false for glyphs with no geometry (spaces, unknown code points), which
    /// still advance the pen.
    bool quad(char32_t codepoint, Vec2& pen, Rect& out_pos, Rect& out_uv) const;
    float advance(char32_t codepoint) const;

    /// OpenGL name of the atlas texture, 0 when invalid.
    std::uint32_t texture() const;
    Vec2 atlas_size() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace exo
