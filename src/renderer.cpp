#include "renderer.hpp"

#include <exotic/draw.hpp>

#include "gl.hpp"

#include <cmath>
#include <cstddef>

namespace exo {
namespace {

// The colour arrives as four normalised bytes and the texture is single
// channel: solid geometry samples a white texel, glyphs sample coverage.
constexpr const char* kVertexShader = R"(#version 330 core
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

uniform mat4 u_projection;

out vec2 v_uv;
out vec4 v_color;

void main() {
    v_uv = a_uv;
    v_color = a_color;
    gl_Position = u_projection * vec4(a_position, 0.0, 1.0);
}
)";

constexpr const char* kFragmentShader = R"(#version 330 core
in vec2 v_uv;
in vec4 v_color;

uniform sampler2D u_texture;

out vec4 frag_color;

void main() {
    frag_color = vec4(v_color.rgb, v_color.a * texture(u_texture, v_uv).r);
}
)";

std::string shader_log(gl::GLuint shader, bool is_program) {
    gl::GLint length = 0;
    if (is_program) {
        gl::glGetProgramiv(shader, gl::GL_INFO_LOG_LENGTH, &length);
    } else {
        gl::glGetShaderiv(shader, gl::GL_INFO_LOG_LENGTH, &length);
    }
    if (length <= 1) return {};

    std::string log(static_cast<std::size_t>(length), '\0');
    if (is_program) {
        gl::glGetProgramInfoLog(shader, length, nullptr, log.data());
    } else {
        gl::glGetShaderInfoLog(shader, length, nullptr, log.data());
    }
    log.resize(std::char_traits<char>::length(log.c_str()));
    return log;
}

gl::GLuint compile(gl::GLenum stage, const char* source, std::string& error) {
    const gl::GLuint shader = gl::glCreateShader(stage);
    gl::glShaderSource(shader, 1, &source, nullptr);
    gl::glCompileShader(shader);

    gl::GLint ok = 0;
    gl::glGetShaderiv(shader, gl::GL_COMPILE_STATUS, &ok);
    if (!ok) {
        error = (stage == gl::GL_VERTEX_SHADER ? "vertex shader: " : "fragment shader: ") +
                shader_log(shader, false);
        gl::glDeleteShader(shader);
        return 0;
    }
    return shader;
}

} // namespace

Renderer::Renderer() {
    if (!gl::glCreateProgram) {
        error_ = "OpenGL entry points are not loaded";
        return;
    }

    const gl::GLuint vertex = compile(gl::GL_VERTEX_SHADER, kVertexShader, error_);
    if (!vertex) return;
    const gl::GLuint fragment = compile(gl::GL_FRAGMENT_SHADER, kFragmentShader, error_);
    if (!fragment) {
        gl::glDeleteShader(vertex);
        return;
    }

    const gl::GLuint program = gl::glCreateProgram();
    gl::glAttachShader(program, vertex);
    gl::glAttachShader(program, fragment);
    gl::glLinkProgram(program);
    gl::glDeleteShader(vertex);
    gl::glDeleteShader(fragment);

    gl::GLint linked = 0;
    gl::glGetProgramiv(program, gl::GL_LINK_STATUS, &linked);
    if (!linked) {
        error_ = "shader link: " + shader_log(program, true);
        gl::glDeleteProgram(program);
        return;
    }

    program_ = program;
    u_projection_ = gl::glGetUniformLocation(program_, "u_projection");
    u_texture_ = gl::glGetUniformLocation(program_, "u_texture");

    gl::glGenVertexArrays(1, &vao_);
    gl::glGenBuffers(1, &vbo_);
    gl::glGenBuffers(1, &ibo_);

    gl::glBindVertexArray(vao_);
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
    gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, ibo_);

    const auto stride = static_cast<gl::GLsizei>(sizeof(Vertex));
    gl::glEnableVertexAttribArray(0);
    gl::glVertexAttribPointer(0, 2, gl::GL_FLOAT, gl::GL_FALSE, stride,
                              reinterpret_cast<const void*>(offsetof(Vertex, pos)));
    gl::glEnableVertexAttribArray(1);
    gl::glVertexAttribPointer(1, 2, gl::GL_FLOAT, gl::GL_FALSE, stride,
                              reinterpret_cast<const void*>(offsetof(Vertex, uv)));
    gl::glEnableVertexAttribArray(2);
    gl::glVertexAttribPointer(2, 4, gl::GL_UNSIGNED_BYTE, gl::GL_TRUE, stride,
                              reinterpret_cast<const void*>(offsetof(Vertex, color)));
    gl::glBindVertexArray(0);

    // Untextured geometry samples this, so one shader covers both cases.
    const unsigned char white = 255;
    gl::glGenTextures(1, &white_);
    gl::glBindTexture(gl::GL_TEXTURE_2D, white_);
    gl::glPixelStorei(gl::GL_UNPACK_ALIGNMENT, 1);
    gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, static_cast<gl::GLint>(gl::GL_R8), 1, 1, 0, gl::GL_RED,
                     gl::GL_UNSIGNED_BYTE, &white);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER,
                        static_cast<gl::GLint>(gl::GL_NEAREST));
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER,
                        static_cast<gl::GLint>(gl::GL_NEAREST));
    gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
}

Renderer::~Renderer() {
    if (!gl::glDeleteProgram) return;
    if (white_) gl::glDeleteTextures(1, &white_);
    if (vbo_) gl::glDeleteBuffers(1, &vbo_);
    if (ibo_) gl::glDeleteBuffers(1, &ibo_);
    if (vao_) gl::glDeleteVertexArrays(1, &vao_);
    if (program_) gl::glDeleteProgram(program_);
}

void Renderer::render(const DrawList& list, Vec2 framebuffer_size) {
    if (program_ == 0 || list.empty()) return;
    if (framebuffer_size.x <= 0.0f || framebuffer_size.y <= 0.0f) return;

    gl::glBindVertexArray(vao_);

    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
    gl::glBufferData(gl::GL_ARRAY_BUFFER,
                     static_cast<gl::GLsizeiptr>(list.vertices().size() * sizeof(Vertex)),
                     list.vertices().data(), gl::GL_STREAM_DRAW);

    gl::glBindBuffer(gl::GL_ELEMENT_ARRAY_BUFFER, ibo_);
    gl::glBufferData(gl::GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<gl::GLsizeiptr>(list.indices().size() * sizeof(std::uint32_t)),
                     list.indices().data(), gl::GL_STREAM_DRAW);

    // Pixel coordinates with the origin top-left, mapped to clip space.
    const float w = framebuffer_size.x;
    const float h = framebuffer_size.y;
    const float projection[16] = {
        2.0f / w, 0.0f,      0.0f,  0.0f, //
        0.0f,     -2.0f / h, 0.0f,  0.0f, //
        0.0f,     0.0f,      -1.0f, 0.0f, //
        -1.0f,    1.0f,      0.0f,  1.0f  //
    };

    gl::glUseProgram(program_);
    gl::glUniformMatrix4fv(u_projection_, 1, gl::GL_FALSE, projection);
    gl::glUniform1i(u_texture_, 0);
    gl::glActiveTexture(gl::GL_TEXTURE0);

    gl::glEnable(gl::GL_BLEND);
    gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA, gl::GL_ONE,
                            gl::GL_ONE_MINUS_SRC_ALPHA);
    gl::glDisable(gl::GL_DEPTH_TEST);
    gl::glDisable(gl::GL_CULL_FACE);
    gl::glEnable(gl::GL_SCISSOR_TEST);

    const Rect viewport{0.0f, 0.0f, w, h};
    for (const DrawCmd& cmd : list.commands()) {
        if (cmd.index_count == 0) continue;

        const Rect clip = cmd.clip.intersected(viewport);
        if (clip.empty()) continue;

        // Scissor is in pixels from the bottom-left; round outwards so a clip
        // rectangle never eats the edge pixel of what it contains.
        const int x0 = static_cast<int>(std::floor(clip.left()));
        const int y0 = static_cast<int>(std::floor(clip.top()));
        const int x1 = static_cast<int>(std::ceil(clip.right()));
        const int y1 = static_cast<int>(std::ceil(clip.bottom()));
        gl::glScissor(x0, static_cast<int>(h) - y1, x1 - x0, y1 - y0);

        gl::glBindTexture(gl::GL_TEXTURE_2D, cmd.texture != 0 ? cmd.texture : white_);
        gl::glDrawElements(
            gl::GL_TRIANGLES, static_cast<gl::GLsizei>(cmd.index_count), gl::GL_UNSIGNED_INT,
            reinterpret_cast<const void*>(static_cast<std::size_t>(cmd.index_offset) * sizeof(std::uint32_t)));
    }

    gl::glDisable(gl::GL_SCISSOR_TEST);
    gl::glBindVertexArray(0);
    gl::glUseProgram(0);
}

} // namespace exo
