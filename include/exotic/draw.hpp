// Exotic GUI - geometry recording.
//
// A DrawList turns shapes into one vertex/index buffer plus a list of batches.
// Nothing here touches OpenGL: it is pure CPU-side geometry, which is what
// makes it testable without a window.

#pragma once

#include <exotic/types.hpp>

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace exo {

class Font;

/// 20 bytes, matching the renderer's vertex layout exactly.
struct Vertex {
    Vec2 pos;
    Vec2 uv;
    std::uint32_t color = 0;
};

/// A run of indices that share a scissor rectangle and a texture.
struct DrawCmd {
    Rect clip;
    /// 0 means "the renderer's 1x1 white texture", i.e. untextured geometry.
    std::uint32_t texture = 0;
    std::uint32_t index_offset = 0;
    std::uint32_t index_count = 0;
};

/// Records shapes for one frame. Coordinates are framebuffer pixels with the
/// origin in the top-left corner.
class DrawList {
public:
    /// Drops last frame's geometry and resets the clip stack to `viewport`.
    void reset(Rect viewport);

    // --- shapes --------------------------------------------------------------

    /// Filled rectangle; `radius` rounds the corners.
    void rect(Rect r, Color color, float radius = 0.0f);
    void rect_outline(Rect r, Color color, float thickness = 1.0f, float radius = 0.0f);
    void line(Vec2 a, Vec2 b, Color color, float thickness = 1.0f);
    void triangle(Vec2 a, Vec2 b, Vec2 c, Color color);
    void circle(Vec2 center, float radius, Color color, int segments = 0);
    void circle_outline(Vec2 center, float radius, Color color, float thickness = 1.0f, int segments = 0);

    // --- text ----------------------------------------------------------------

    /// Draws `str` with its top-left corner at `pos`, honouring '\n'.
    /// Returns the size of the block that was drawn.
    Vec2 text(Vec2 pos, std::string_view str, Color color, const Font& font);
    /// Same, centred inside `area`.
    Vec2 text_centered(Rect area, std::string_view str, Color color, const Font& font);

    // --- paths (for shapes the helpers above do not cover) --------------------

    void path_clear();
    void path_to(Vec2 point);
    void path_arc(Vec2 center, float radius, float from_radians, float to_radians, int segments);
    void path_rounded_rect(Rect r, float radius);
    /// Fans the current path from its first point - convex shapes only.
    void path_fill(Color color);
    void path_stroke(Color color, float thickness = 1.0f, bool closed = false);

    void convex_polygon(std::span<const Vec2> points, Color color);
    void polyline(std::span<const Vec2> points, Color color, float thickness = 1.0f, bool closed = false);

    // --- clipping ------------------------------------------------------------

    /// Intersects `r` with the active clip and pushes it.
    void push_clip(Rect r);
    void pop_clip();
    Rect clip() const;

    // --- output --------------------------------------------------------------

    bool empty() const { return indices_.empty(); }
    const std::vector<Vertex>& vertices() const { return vertices_; }
    const std::vector<std::uint32_t>& indices() const { return indices_; }
    const std::vector<DrawCmd>& commands() const { return commands_; }

private:
    DrawCmd& batch(std::uint32_t texture);
    void add_quad(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, Rect uv, Color color, std::uint32_t texture);

    std::vector<Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
    std::vector<DrawCmd> commands_;
    std::vector<Rect> clip_stack_;
    std::vector<Vec2> path_;
    Rect clip_;
};

} // namespace exo
