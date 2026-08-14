#include <exotic/font.hpp>

#include "gl.hpp"
#include "utf8.hpp"

// Third-party header: compiled with warnings off, we do not own its style.
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <cstdlib>
#include <fstream>
#include <vector>

namespace exo {
namespace {

// Two contiguous ranges cover plain English and the accented characters of
// western European languages, which is all a UI font is asked for here.
// ponytail: no CJK, no glyph cache. Both would need a dynamic atlas.
constexpr int kAsciiFirst = 32;
constexpr int kAsciiCount = 95; // 32..126
constexpr int kLatinFirst = 160;
constexpr int kLatinCount = 96; // 160..255

constexpr char32_t kFallbackGlyph = U'?';

std::vector<std::uint8_t> read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    const std::streamoff size = file.tellg();
    if (size <= 0) return {};
    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()), size);
    if (!file) return {};
    return data;
}

} // namespace

struct Font::Impl {
    std::vector<stbtt_packedchar> ascii;
    std::vector<stbtt_packedchar> latin;
    float pixel_height = 0.0f;
    float ascent = 0.0f;
    float descent = 0.0f;
    float line_height = 0.0f;
    int atlas_w = 0;
    int atlas_h = 0;
    std::uint32_t texture = 0;
    std::string error;

    const stbtt_packedchar* find(char32_t cp) const {
        if (cp >= kAsciiFirst && cp < kAsciiFirst + kAsciiCount) {
            return &ascii[static_cast<std::size_t>(static_cast<int>(cp) - kAsciiFirst)];
        }
        if (cp >= kLatinFirst && cp < kLatinFirst + kLatinCount) {
            return &latin[static_cast<std::size_t>(static_cast<int>(cp) - kLatinFirst)];
        }
        if (cp != kFallbackGlyph) return find(kFallbackGlyph);
        return nullptr;
    }
};

Font::Font() : impl_(std::make_unique<Impl>()) {}

Font::~Font() {
    if (impl_ && impl_->texture != 0 && gl::glDeleteTextures) {
        const gl::GLuint name = impl_->texture;
        gl::glDeleteTextures(1, &name);
    }
}

Font::Font(Font&&) noexcept = default;

Font& Font::operator=(Font&&) noexcept = default;

Font Font::from_memory(const std::uint8_t* data, std::size_t size, float pixel_height) {
    Font font;
    Impl& self = *font.impl_;

    if (!data || size == 0) {
        self.error = "empty font data";
        return font;
    }
    if (pixel_height < 1.0f) {
        self.error = "pixel height must be at least 1";
        return font;
    }
    if (!gl::glGenTextures) {
        self.error = "no OpenGL context - create the window before the font";
        return font;
    }

    stbtt_fontinfo info;
    const int offset = stbtt_GetFontOffsetForIndex(data, 0);
    if (offset < 0 || !stbtt_InitFont(&info, data, offset)) {
        self.error = "not a readable TrueType font";
        return font;
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    const float scale = stbtt_ScaleForPixelHeight(&info, pixel_height);

    self.pixel_height = pixel_height;
    self.ascent = static_cast<float>(ascent) * scale;
    self.descent = static_cast<float>(descent) * scale;
    self.line_height = static_cast<float>(ascent - descent + line_gap) * scale;
    self.ascii.resize(kAsciiCount);
    self.latin.resize(kLatinCount);

    // 2x2 oversampling costs atlas space but is the difference between crisp
    // and mushy at UI sizes. Grow the atlas once if the glyphs do not fit.
    std::vector<std::uint8_t> bitmap;
    bool packed = false;
    for (int side = 1024; side <= 2048 && !packed; side *= 2) {
        bitmap.assign(static_cast<std::size_t>(side) * static_cast<std::size_t>(side), 0);

        stbtt_pack_context pack;
        if (!stbtt_PackBegin(&pack, bitmap.data(), side, side, 0, 1, nullptr)) continue;
        stbtt_PackSetOversampling(&pack, 2, 2);

        stbtt_pack_range ranges[2] = {};
        ranges[0].font_size = pixel_height;
        ranges[0].first_unicode_codepoint_in_range = kAsciiFirst;
        ranges[0].num_chars = kAsciiCount;
        ranges[0].chardata_for_range = self.ascii.data();
        ranges[1].font_size = pixel_height;
        ranges[1].first_unicode_codepoint_in_range = kLatinFirst;
        ranges[1].num_chars = kLatinCount;
        ranges[1].chardata_for_range = self.latin.data();

        packed = stbtt_PackFontRanges(&pack, data, 0, ranges, 2) != 0;
        stbtt_PackEnd(&pack);

        if (packed) {
            self.atlas_w = side;
            self.atlas_h = side;
        }
    }

    if (!packed) {
        self.error = "glyphs do not fit in a 2048x2048 atlas - reduce the pixel height";
        return font;
    }

    gl::GLuint texture = 0;
    gl::glGenTextures(1, &texture);
    gl::glBindTexture(gl::GL_TEXTURE_2D, texture);
    gl::glPixelStorei(gl::GL_UNPACK_ALIGNMENT, 1);
    gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, static_cast<gl::GLint>(gl::GL_R8), self.atlas_w, self.atlas_h, 0,
                     gl::GL_RED, gl::GL_UNSIGNED_BYTE, bitmap.data());
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER,
                        static_cast<gl::GLint>(gl::GL_LINEAR));
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER,
                        static_cast<gl::GLint>(gl::GL_LINEAR));
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S,
                        static_cast<gl::GLint>(gl::GL_CLAMP_TO_EDGE));
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T,
                        static_cast<gl::GLint>(gl::GL_CLAMP_TO_EDGE));
    gl::glBindTexture(gl::GL_TEXTURE_2D, 0);

    self.texture = texture;
    return font;
}

Font Font::from_file(const std::string& path, float pixel_height) {
    const std::vector<std::uint8_t> data = read_file(path);
    if (data.empty()) {
        Font font;
        font.impl_->error = "cannot read font file: " + path;
        return font;
    }
    Font font = from_memory(data.data(), data.size(), pixel_height);
    if (!font.valid() && font.impl_->error.find("cannot read") == std::string::npos) {
        font.impl_->error += " (" + path + ")";
    }
    return font;
}

Font Font::system_ui(float pixel_height) {
    const char* windir = std::getenv("WINDIR");
    const std::string fonts = std::string(windir ? windir : "C:\\Windows") + "\\Fonts\\";

    for (const char* face : {"segoeui.ttf", "arial.ttf", "tahoma.ttf", "verdana.ttf"}) {
        Font font = from_file(fonts + face, pixel_height);
        if (font.valid()) return font;
    }

    Font font;
    font.impl_->error = "no usable system UI font found in " + fonts;
    return font;
}

bool Font::valid() const {
    return impl_ && impl_->texture != 0;
}

const std::string& Font::error() const {
    static const std::string moved_from = "font has been moved from";
    return impl_ ? impl_->error : moved_from;
}

float Font::pixel_height() const {
    return impl_ ? impl_->pixel_height : 0.0f;
}

float Font::line_height() const {
    return impl_ ? impl_->line_height : 0.0f;
}

float Font::ascent() const {
    return impl_ ? impl_->ascent : 0.0f;
}

float Font::descent() const {
    return impl_ ? impl_->descent : 0.0f;
}

bool Font::quad(char32_t codepoint, Vec2& pen, Rect& out_pos, Rect& out_uv) const {
    if (!valid()) return false;

    const stbtt_packedchar* glyph = impl_->find(codepoint);
    if (!glyph) return false;

    stbtt_aligned_quad q;
    float x = pen.x;
    float y = pen.y;
    stbtt_GetPackedQuad(glyph, impl_->atlas_w, impl_->atlas_h, 0, &x, &y, &q, 1);
    pen.x = x;

    out_pos = Rect::bounds(q.x0, q.y0, q.x1, q.y1);
    out_uv = Rect::bounds(q.s0, q.t0, q.s1, q.t1);
    return q.x1 > q.x0 && q.y1 > q.y0;
}

float Font::advance(char32_t codepoint) const {
    if (!valid()) return 0.0f;
    const stbtt_packedchar* glyph = impl_->find(codepoint);
    return glyph ? glyph->xadvance : 0.0f;
}

float Font::measure(std::string_view str) const {
    return measure_block(str).x;
}

Vec2 Font::measure_block(std::string_view str) const {
    if (!valid()) return {};

    float widest = 0.0f;
    float width = 0.0f;
    int lines = 1;

    for (std::size_t i = 0; i < str.size();) {
        char32_t cp = 0;
        i += utf8::decode(str, i, cp);

        if (cp == U'\n') {
            if (width > widest) widest = width;
            width = 0.0f;
            ++lines;
            continue;
        }
        if (cp == U'\r') continue;

        width += advance(cp);
    }

    if (width > widest) widest = width;
    return {widest, static_cast<float>(lines) * impl_->line_height};
}

std::uint32_t Font::texture() const {
    return impl_ ? impl_->texture : 0;
}

Vec2 Font::atlas_size() const {
    if (!impl_) return {};
    return {static_cast<float>(impl_->atlas_w), static_cast<float>(impl_->atlas_h)};
}

} // namespace exo
