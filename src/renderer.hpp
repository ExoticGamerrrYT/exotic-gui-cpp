// Internal OpenGL 3.3 backend for DrawList. Not part of the public API: the
// window owns one and feeds it every frame.

#pragma once

#include <exotic/types.hpp>

#include <cstdint>
#include <string>

namespace exo {

class DrawList;

/// One VAO, one streamed vertex buffer, one index buffer, one shader.
/// Requires a current OpenGL context for its whole lifetime.
class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /// Uploads the list and issues one draw call per batch.
    void render(const DrawList& list, Vec2 framebuffer_size);

    bool valid() const { return program_ != 0; }
    /// Shader compile/link diagnostics, empty when everything is fine.
    const std::string& error() const { return error_; }

private:
    std::uint32_t program_ = 0;
    std::uint32_t vao_ = 0;
    std::uint32_t vbo_ = 0;
    std::uint32_t ibo_ = 0;
    std::uint32_t white_ = 0;
    int u_projection_ = -1;
    int u_texture_ = -1;
    std::string error_;
};

} // namespace exo
