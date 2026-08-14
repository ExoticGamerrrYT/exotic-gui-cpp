// Minimal OpenGL 3.3 core loader.
//
// The alternative was pulling in glad or GLEW; neither is worth a dependency
// (and a build-time Python step) for the ~45 entry points this renderer uses.
// Every function is resolved through the address getter the platform layer
// hands us, so no OpenGL header or import library is needed anywhere.

#pragma once

#include <cstddef>

namespace exo::gl {

using GLenum = unsigned int;
using GLboolean = unsigned char;
using GLbitfield = unsigned int;
using GLbyte = signed char;
using GLubyte = unsigned char;
using GLshort = short;
using GLushort = unsigned short;
using GLint = int;
using GLuint = unsigned int;
using GLsizei = int;
using GLfloat = float;
using GLclampf = float;
using GLchar = char;
using GLintptr = std::ptrdiff_t;
using GLsizeiptr = std::ptrdiff_t;

#if defined(_WIN32)
#define EXO_GLAPI __stdcall
#else
#define EXO_GLAPI
#endif

// --- constants ---------------------------------------------------------------

inline constexpr GLenum GL_FALSE = 0;
inline constexpr GLenum GL_TRUE = 1;
inline constexpr GLenum GL_NO_ERROR = 0;
inline constexpr GLenum GL_ZERO = 0;
inline constexpr GLenum GL_ONE = 1;
inline constexpr GLenum GL_TRIANGLES = 0x0004;
inline constexpr GLenum GL_SRC_ALPHA = 0x0302;
inline constexpr GLenum GL_ONE_MINUS_SRC_ALPHA = 0x0303;
inline constexpr GLenum GL_CULL_FACE = 0x0B44;
inline constexpr GLenum GL_DEPTH_TEST = 0x0B71;
inline constexpr GLenum GL_BLEND = 0x0BE2;
inline constexpr GLenum GL_SCISSOR_TEST = 0x0C11;
inline constexpr GLenum GL_UNPACK_ALIGNMENT = 0x0CF5;
inline constexpr GLenum GL_TEXTURE_2D = 0x0DE1;
inline constexpr GLenum GL_UNSIGNED_BYTE = 0x1401;
inline constexpr GLenum GL_UNSIGNED_INT = 0x1405;
inline constexpr GLenum GL_FLOAT = 0x1406;
inline constexpr GLenum GL_VENDOR = 0x1F00;
inline constexpr GLenum GL_RENDERER = 0x1F01;
inline constexpr GLenum GL_VERSION = 0x1F02;
inline constexpr GLenum GL_RED = 0x1903;
inline constexpr GLenum GL_R8 = 0x8229;
inline constexpr GLenum GL_NEAREST = 0x2600;
inline constexpr GLenum GL_LINEAR = 0x2601;
inline constexpr GLenum GL_TEXTURE_MAG_FILTER = 0x2800;
inline constexpr GLenum GL_TEXTURE_MIN_FILTER = 0x2801;
inline constexpr GLenum GL_TEXTURE_WRAP_S = 0x2802;
inline constexpr GLenum GL_TEXTURE_WRAP_T = 0x2803;
inline constexpr GLenum GL_CLAMP_TO_EDGE = 0x812F;
inline constexpr GLenum GL_COLOR_BUFFER_BIT = 0x00004000;
inline constexpr GLenum GL_TEXTURE0 = 0x84C0;
inline constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
inline constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
inline constexpr GLenum GL_STREAM_DRAW = 0x88E0;
inline constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
inline constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
inline constexpr GLenum GL_COMPILE_STATUS = 0x8B81;
inline constexpr GLenum GL_LINK_STATUS = 0x8B82;
inline constexpr GLenum GL_INFO_LOG_LENGTH = 0x8B84;
inline constexpr GLenum GL_MULTISAMPLE = 0x809D;

// --- entry points ------------------------------------------------------------

// clang-format off
#define EXO_GL_FUNCTIONS(X)                                                                                  \
    X(void,           glEnable,                  (GLenum))                                                   \
    X(void,           glDisable,                 (GLenum))                                                   \
    X(void,           glBlendFuncSeparate,       (GLenum, GLenum, GLenum, GLenum))                           \
    X(void,           glViewport,                (GLint, GLint, GLsizei, GLsizei))                           \
    X(void,           glScissor,                 (GLint, GLint, GLsizei, GLsizei))                           \
    X(void,           glClearColor,              (GLfloat, GLfloat, GLfloat, GLfloat))                       \
    X(void,           glClear,                   (GLbitfield))                                               \
    X(void,           glGenTextures,             (GLsizei, GLuint*))                                         \
    X(void,           glDeleteTextures,          (GLsizei, const GLuint*))                                   \
    X(void,           glBindTexture,             (GLenum, GLuint))                                           \
    X(void,           glTexImage2D,              (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,     \
                                                  GLenum, const void*))                                      \
    X(void,           glTexParameteri,           (GLenum, GLenum, GLint))                                    \
    X(void,           glPixelStorei,             (GLenum, GLint))                                            \
    X(void,           glDrawElements,            (GLenum, GLsizei, GLenum, const void*))                     \
    X(const GLubyte*, glGetString,               (GLenum))                                                   \
    X(GLenum,         glGetError,                (void))                                                     \
    X(void,           glActiveTexture,           (GLenum))                                                   \
    X(void,           glGenBuffers,              (GLsizei, GLuint*))                                         \
    X(void,           glDeleteBuffers,           (GLsizei, const GLuint*))                                   \
    X(void,           glBindBuffer,              (GLenum, GLuint))                                           \
    X(void,           glBufferData,              (GLenum, GLsizeiptr, const void*, GLenum))                  \
    X(GLuint,         glCreateShader,            (GLenum))                                                   \
    X(void,           glShaderSource,            (GLuint, GLsizei, const GLchar* const*, const GLint*))      \
    X(void,           glCompileShader,           (GLuint))                                                   \
    X(void,           glGetShaderiv,             (GLuint, GLenum, GLint*))                                   \
    X(void,           glGetShaderInfoLog,        (GLuint, GLsizei, GLsizei*, GLchar*))                       \
    X(void,           glDeleteShader,            (GLuint))                                                   \
    X(GLuint,         glCreateProgram,           (void))                                                     \
    X(void,           glAttachShader,            (GLuint, GLuint))                                           \
    X(void,           glLinkProgram,             (GLuint))                                                   \
    X(void,           glGetProgramiv,            (GLuint, GLenum, GLint*))                                   \
    X(void,           glGetProgramInfoLog,       (GLuint, GLsizei, GLsizei*, GLchar*))                       \
    X(void,           glDeleteProgram,           (GLuint))                                                   \
    X(void,           glUseProgram,              (GLuint))                                                   \
    X(GLint,          glGetUniformLocation,      (GLuint, const GLchar*))                                    \
    X(void,           glUniformMatrix4fv,        (GLint, GLsizei, GLboolean, const GLfloat*))                \
    X(void,           glUniform1i,               (GLint, GLint))                                             \
    X(void,           glEnableVertexAttribArray, (GLuint))                                                   \
    X(void,           glVertexAttribPointer,     (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*))   \
    X(void,           glGenVertexArrays,         (GLsizei, GLuint*))                                         \
    X(void,           glDeleteVertexArrays,      (GLsizei, const GLuint*))                                   \
    X(void,           glBindVertexArray,         (GLuint))
// clang-format on

#define EXO_GL_DECLARE(ret, name, params)                                                                    \
    using PFN_##name = ret(EXO_GLAPI*) params;                                                               \
    extern PFN_##name name;

EXO_GL_FUNCTIONS(EXO_GL_DECLARE)
#undef EXO_GL_DECLARE

/// Signature of the platform's address getter (glfwGetProcAddress).
using ProcLoader = void* (*)(const char*);

/// Resolves every entry point above. Returns false and fills `missing` with the
/// first unavailable function when the driver is too old.
bool load(ProcLoader loader, const char** missing);

} // namespace exo::gl
