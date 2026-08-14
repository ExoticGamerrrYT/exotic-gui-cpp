#include <exotic/draw.hpp>

#include <exotic/font.hpp>

#include "utf8.hpp"

#include <cmath>

namespace exo {
namespace {

constexpr float kPi = 3.14159265358979323846f;

/// Untextured geometry samples the renderer's 1x1 white texture, so any UV works.
constexpr Rect kWhiteUv{0.0f, 0.0f, 0.0f, 0.0f};

int auto_segments(float radius) {
    const int n = static_cast<int>(radius * 0.75f) + 8;
    return n < 8 ? 8 : (n > 96 ? 96 : n);
}

} // namespace

void DrawList::reset(Rect viewport) {
    vertices_.clear();
    indices_.clear();
    commands_.clear();
    clip_stack_.clear();
    path_.clear();
    clip_ = viewport;
}

DrawCmd& DrawList::batch(std::uint32_t texture) {
    if (!commands_.empty()) {
        DrawCmd& last = commands_.back();
        if (last.index_count == 0) {
            last.clip = clip_;
            last.texture = texture;
            last.index_offset = static_cast<std::uint32_t>(indices_.size());
            return last;
        }
        if (last.texture == texture && last.clip == clip_) return last;
    }
    commands_.push_back(DrawCmd{clip_, texture, static_cast<std::uint32_t>(indices_.size()), 0});
    return commands_.back();
}

void DrawList::add_quad(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Rect uv, Color color, std::uint32_t texture) {
    if (clip_.empty() || color.a == 0) return;

    DrawCmd& cmd = batch(texture);
    const auto base = static_cast<std::uint32_t>(vertices_.size());
    const std::uint32_t packed = color.packed();

    vertices_.push_back({p0, {uv.left(), uv.top()}, packed});
    vertices_.push_back({p1, {uv.right(), uv.top()}, packed});
    vertices_.push_back({p2, {uv.right(), uv.bottom()}, packed});
    vertices_.push_back({p3, {uv.left(), uv.bottom()}, packed});

    for (const std::uint32_t offset : {0u, 1u, 2u, 0u, 2u, 3u}) {
        indices_.push_back(base + offset);
    }
    cmd.index_count += 6;
}

void DrawList::rect(Rect r, Color color, float radius) {
    if (r.empty()) return;
    if (radius <= 0.0f) {
        add_quad({r.left(), r.top()}, {r.right(), r.top()}, {r.right(), r.bottom()}, {r.left(), r.bottom()},
                 kWhiteUv, color, 0);
        return;
    }
    path_rounded_rect(r, radius);
    path_fill(color);
}

void DrawList::rect_outline(Rect r, Color color, float thickness, float radius) {
    if (r.empty() || thickness <= 0.0f) return;
    // Inset by half the thickness so the stroke stays inside `r` instead of
    // straddling the edge and bleeding over neighbouring widgets.
    const float half = thickness * 0.5f;
    path_rounded_rect(r.shrunk(half), radius > half ? radius - half : 0.0f);
    path_stroke(color, thickness, true);
}

void DrawList::line(Vec2 a, Vec2 b, Color color, float thickness) {
    const Vec2 points[] = {a, b};
    polyline(points, color, thickness, false);
}

void DrawList::triangle(Vec2 a, Vec2 b, Vec2 c, Color color) {
    const Vec2 points[] = {a, b, c};
    convex_polygon(points, color);
}

void DrawList::circle(Vec2 center, float radius, Color color, int segments) {
    if (radius <= 0.0f) return;
    if (segments <= 0) segments = auto_segments(radius);
    path_clear();
    for (int i = 0; i < segments; ++i) {
        const float t = kPi * 2.0f * static_cast<float>(i) / static_cast<float>(segments);
        path_to({center.x + std::cos(t) * radius, center.y + std::sin(t) * radius});
    }
    path_fill(color);
}

void DrawList::circle_outline(Vec2 center, float radius, Color color, float thickness, int segments) {
    if (radius <= 0.0f) return;
    if (segments <= 0) segments = auto_segments(radius);
    path_clear();
    for (int i = 0; i < segments; ++i) {
        const float t = kPi * 2.0f * static_cast<float>(i) / static_cast<float>(segments);
        path_to({center.x + std::cos(t) * radius, center.y + std::sin(t) * radius});
    }
    path_stroke(color, thickness, true);
}

Vec2 DrawList::text(Vec2 pos, std::string_view str, Color color, const Font& font) {
    if (str.empty() || !font.valid()) return {};

    const std::uint32_t texture = font.texture();
    const float line_height = font.line_height();
    Vec2 pen{pos.x, pos.y + font.ascent()};
    float widest = 0.0f;
    int lines = 1;

    for (std::size_t i = 0; i < str.size();) {
        char32_t cp = 0;
        i += utf8::decode(str, i, cp);

        if (cp == U'\n') {
            if (pen.x - pos.x > widest) widest = pen.x - pos.x;
            pen.x = pos.x;
            pen.y += line_height;
            ++lines;
            continue;
        }
        if (cp == U'\r') continue;

        Rect glyph;
        Rect uv;
        if (font.quad(cp, pen, glyph, uv)) {
            add_quad({glyph.left(), glyph.top()}, {glyph.right(), glyph.top()},
                     {glyph.right(), glyph.bottom()}, {glyph.left(), glyph.bottom()}, uv, color, texture);
        }
    }

    if (pen.x - pos.x > widest) widest = pen.x - pos.x;
    return {widest, static_cast<float>(lines) * line_height};
}

Vec2 DrawList::text_centered(Rect area, std::string_view str, Color color, const Font& font) {
    const Vec2 size = font.measure_block(str);
    // Snapping to whole pixels keeps glyph edges crisp.
    const Vec2 pos{std::round(area.x + (area.w - size.x) * 0.5f),
                   std::round(area.y + (area.h - size.y) * 0.5f)};
    return text(pos, str, color, font);
}

void DrawList::path_clear() {
    path_.clear();
}

void DrawList::path_to(Vec2 point) {
    path_.push_back(point);
}

void DrawList::path_arc(Vec2 center, float radius, float from_radians, float to_radians, int segments) {
    if (segments <= 0) segments = auto_segments(radius);
    for (int i = 0; i <= segments; ++i) {
        const float t =
            from_radians + (to_radians - from_radians) * static_cast<float>(i) / static_cast<float>(segments);
        path_to({center.x + std::cos(t) * radius, center.y + std::sin(t) * radius});
    }
}

void DrawList::path_rounded_rect(Rect r, float radius) {
    path_clear();

    const float limit = (r.w < r.h ? r.w : r.h) * 0.5f;
    const float rad = radius < 0.0f ? 0.0f : (radius > limit ? limit : radius);

    if (rad <= 0.0f) {
        path_to({r.left(), r.top()});
        path_to({r.right(), r.top()});
        path_to({r.right(), r.bottom()});
        path_to({r.left(), r.bottom()});
        return;
    }

    // y grows downwards, so angle 0 points right and pi/2 points down.
    const int seg = auto_segments(rad) / 4 + 1;
    path_arc({r.left() + rad, r.top() + rad}, rad, kPi, kPi * 1.5f, seg);
    path_arc({r.right() - rad, r.top() + rad}, rad, kPi * 1.5f, kPi * 2.0f, seg);
    path_arc({r.right() - rad, r.bottom() - rad}, rad, 0.0f, kPi * 0.5f, seg);
    path_arc({r.left() + rad, r.bottom() - rad}, rad, kPi * 0.5f, kPi, seg);
}

void DrawList::path_fill(Color color) {
    convex_polygon(path_, color);
    path_clear();
}

void DrawList::path_stroke(Color color, float thickness, bool closed) {
    polyline(path_, color, thickness, closed);
    path_clear();
}

void DrawList::convex_polygon(std::span<const Vec2> points, Color color) {
    if (points.size() < 3 || color.a == 0 || clip_.empty()) return;

    DrawCmd& cmd = batch(0);
    const auto base = static_cast<std::uint32_t>(vertices_.size());
    const std::uint32_t packed = color.packed();

    for (const Vec2 p : points) {
        vertices_.push_back({p, {kWhiteUv.x, kWhiteUv.y}, packed});
    }
    for (std::size_t i = 1; i + 1 < points.size(); ++i) {
        indices_.push_back(base);
        indices_.push_back(base + static_cast<std::uint32_t>(i));
        indices_.push_back(base + static_cast<std::uint32_t>(i) + 1);
    }
    cmd.index_count += static_cast<std::uint32_t>(points.size() - 2) * 3;
}

void DrawList::polyline(std::span<const Vec2> points, Color color, float thickness, bool closed) {
    if (points.size() < 2 || color.a == 0 || thickness <= 0.0f) return;

    // ponytail: one quad per segment, no mitred joins. Invisible on the 1-2px
    // borders this library draws; revisit if thick polylines ever matter.
    const float half = thickness * 0.5f;
    const std::size_t count = closed ? points.size() : points.size() - 1;

    for (std::size_t i = 0; i < count; ++i) {
        const Vec2 a = points[i];
        const Vec2 b = points[(i + 1) % points.size()];
        Vec2 d = b - a;
        const float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len < 1e-6f) continue;
        d = d * (1.0f / len);
        const Vec2 n{-d.y * half, d.x * half};
        add_quad(a + n, b + n, b - n, a - n, kWhiteUv, color, 0);
    }
}

void DrawList::push_clip(Rect r) {
    clip_stack_.push_back(clip_);
    clip_ = clip_.intersected(r);
}

void DrawList::pop_clip() {
    if (clip_stack_.empty()) return;
    clip_ = clip_stack_.back();
    clip_stack_.pop_back();
}

Rect DrawList::clip() const {
    return clip_;
}

} // namespace exo
