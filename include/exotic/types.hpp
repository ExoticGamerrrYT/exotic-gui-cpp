// Exotic GUI - core value types.
//
// Everything here is a plain, constexpr-friendly aggregate: no allocations,
// no virtuals, no dependencies beyond <cstdint>.

#pragma once

#include <cstdint>

namespace exo {

/// 2D point / vector in logical pixels.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;

    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    constexpr Vec2& operator+=(Vec2 o) {
        x += o.x;
        y += o.y;
        return *this;
    }

    constexpr Vec2& operator-=(Vec2 o) {
        x -= o.x;
        y -= o.y;
        return *this;
    }

    friend constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }

    friend constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }

    friend constexpr Vec2 operator-(Vec2 a) { return {-a.x, -a.y}; }

    friend constexpr Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }

    friend constexpr Vec2 operator*(float s, Vec2 a) { return {a.x * s, a.y * s}; }

    friend constexpr bool operator==(Vec2 a, Vec2 b) = default;
};

/// Axis-aligned rectangle: top-left origin, y grows downwards.
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    constexpr Rect() = default;

    constexpr Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}

    constexpr Rect(Vec2 pos, Vec2 size) : x(pos.x), y(pos.y), w(size.x), h(size.y) {}

    /// Build from edges instead of size.
    static constexpr Rect bounds(float left, float top, float right, float bottom) {
        return {left, top, right - left, bottom - top};
    }

    constexpr float left() const { return x; }

    constexpr float top() const { return y; }

    constexpr float right() const { return x + w; }

    constexpr float bottom() const { return y + h; }

    constexpr Vec2 pos() const { return {x, y}; }

    constexpr Vec2 size() const { return {w, h}; }

    constexpr Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }

    constexpr bool empty() const { return w <= 0.0f || h <= 0.0f; }

    constexpr bool contains(Vec2 p) const {
        return p.x >= x && p.y >= y && p.x < x + w && p.y < y + h;
    }

    /// Grow on every side by `d` (negative shrinks).
    constexpr Rect expanded(float d) const { return {x - d, y - d, w + d * 2.0f, h + d * 2.0f}; }

    constexpr Rect shrunk(float d) const { return expanded(-d); }

    constexpr Rect translated(Vec2 d) const { return {x + d.x, y + d.y, w, h}; }

    /// Overlap of two rectangles; empty (w or h <= 0) when they do not touch.
    constexpr Rect intersected(Rect o) const {
        const float l = x > o.x ? x : o.x;
        const float t = y > o.y ? y : o.y;
        const float r = right() < o.right() ? right() : o.right();
        const float b = bottom() < o.bottom() ? bottom() : o.bottom();
        return bounds(l, t, r > l ? r : l, b > t ? b : t);
    }

    friend constexpr bool operator==(Rect, Rect) = default;
};

/// 8-bit sRGB colour with straight (non-premultiplied) alpha.
struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    constexpr Color() = default;

    constexpr Color(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_, std::uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    /// From a 0xRRGGBB literal.
    static constexpr Color rgb(std::uint32_t hex, std::uint8_t alpha = 255) {
        return {static_cast<std::uint8_t>((hex >> 16) & 0xFFu), static_cast<std::uint8_t>((hex >> 8) & 0xFFu),
                static_cast<std::uint8_t>(hex & 0xFFu), alpha};
    }

    /// From a 0xRRGGBBAA literal.
    static constexpr Color rgba(std::uint32_t hex) {
        return {static_cast<std::uint8_t>((hex >> 24) & 0xFFu), static_cast<std::uint8_t>((hex >> 16) & 0xFFu),
                static_cast<std::uint8_t>((hex >> 8) & 0xFFu), static_cast<std::uint8_t>(hex & 0xFFu)};
    }

    /// Byte order expected by the renderer's vertex format (R, G, B, A).
    constexpr std::uint32_t packed() const {
        return static_cast<std::uint32_t>(r) | (static_cast<std::uint32_t>(g) << 8) |
               (static_cast<std::uint32_t>(b) << 16) | (static_cast<std::uint32_t>(a) << 24);
    }

    constexpr Color with_alpha(std::uint8_t alpha) const { return {r, g, b, alpha}; }

    /// Multiply alpha by `f` - handy for disabled or fading widgets.
    constexpr Color faded(float f) const {
        const float v = static_cast<float>(a) * (f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f));
        return {r, g, b, static_cast<std::uint8_t>(v + 0.5f)};
    }

    static constexpr Color lerp(Color from, Color to, float t) {
        const float k = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
        const auto mix = [k](std::uint8_t p, std::uint8_t q) {
            return static_cast<std::uint8_t>(static_cast<float>(p) +
                                             (static_cast<float>(q) - static_cast<float>(p)) * k + 0.5f);
        };
        return {mix(from.r, to.r), mix(from.g, to.g), mix(from.b, to.b), mix(from.a, to.a)};
    }

    static constexpr Color white() { return {255, 255, 255}; }

    static constexpr Color black() { return {0, 0, 0}; }

    static constexpr Color none() { return {0, 0, 0, 0}; }

    friend constexpr bool operator==(Color, Color) = default;
};

} // namespace exo
