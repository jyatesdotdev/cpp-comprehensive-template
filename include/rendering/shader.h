#pragma once
/// @file shader.h
/// @brief Shader compilation, linking, and uniform helpers.

#ifdef ENABLE_GL

#include "rendering/gl_resource.h"

#include <GL/gl.h>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rendering {

/// @brief Compile a single shader stage.
/// @param type  GL shader type (e.g. `GL_VERTEX_SHADER`, `GL_FRAGMENT_SHADER`).
/// @param source  GLSL source code.
/// @return RAII shader object. Destroyed shaders are deleted even if a later step throws.
/// @throws std::runtime_error If compilation fails.
inline gl::Shader compile_shader(GLenum type, std::string_view source) {
    gl::Shader shader(type);
    const char *src = source.data();
    auto len = static_cast<GLint>(source.size());
    glShaderSource(shader.id(), 1, &src, &len);
    glCompileShader(shader.id());

    GLint ok = 0;
    glGetShaderiv(shader.id(), GL_COMPILE_STATUS, &ok);
    if (!ok) {
        std::array<char, 1024> log{};
        glGetShaderInfoLog(shader.id(), static_cast<GLsizei>(log.size()), nullptr, log.data());
        throw std::runtime_error(std::string("Shader compile error: ") + log.data());
    }
    return shader;
}

/// @brief Link vertex and fragment shaders into a program.
/// @param vert  Compiled vertex shader.
/// @param frag  Compiled fragment shader.
/// @return RAII program object. A failed link deletes the program via the destructor.
/// @throws std::runtime_error If linking fails.
inline gl::Program link_program(const gl::Shader &vert, const gl::Shader &frag) {
    gl::Program prog;
    glAttachShader(prog.id(), vert.id());
    glAttachShader(prog.id(), frag.id());
    glLinkProgram(prog.id());

    GLint ok = 0;
    glGetProgramiv(prog.id(), GL_LINK_STATUS, &ok);
    if (!ok) {
        std::array<char, 1024> log{};
        glGetProgramInfoLog(prog.id(), static_cast<GLsizei>(log.size()), nullptr, log.data());
        throw std::runtime_error(std::string("Program link error: ") + log.data());
    }
    glDetachShader(prog.id(), vert.id());
    glDetachShader(prog.id(), frag.id());
    return prog;
}

/// @brief Build a complete shader program from vertex and fragment source strings.
///
/// Compiles both stages, links, and lets RAII clean up intermediate shaders.
/// @param vert_src  GLSL vertex shader source.
/// @param frag_src  GLSL fragment shader source.
/// @return Linked RAII program.
/// @throws std::runtime_error If compilation or linking fails.
inline gl::Program build_program(std::string_view vert_src, std::string_view frag_src) {
    gl::Shader v = compile_shader(GL_VERTEX_SHADER, vert_src);
    gl::Shader f = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    return link_program(v, f);
}

// ── Uniform setters (convenience) ───────────────────────────────────

/// @brief Set an integer uniform on a program.
/// @param prog  Program handle.
/// @param name  Uniform name.
/// @param val   Integer value.
inline void set_uniform(GLuint prog, const char *name, int val) {
    glProgramUniform1i(prog, glGetUniformLocation(prog, name), val);
}

/// @brief Set a float uniform on a program.
/// @param prog  Program handle.
/// @param name  Uniform name.
/// @param val   Float value.
inline void set_uniform(GLuint prog, const char *name, float val) {
    glProgramUniform1f(prog, glGetUniformLocation(prog, name), val);
}

/// @brief Set a 4×4 matrix uniform on a program.
/// @param prog       Program handle.
/// @param name       Uniform name.
/// @param mat4       Pointer to 16 floats (column-major).
/// @param transpose  Whether to transpose the matrix (default: false).
inline void set_uniform(GLuint prog, const char *name, const float *mat4, bool transpose = false) {
    glProgramUniformMatrix4fv(prog, glGetUniformLocation(prog, name), 1,
                              transpose ? GL_TRUE : GL_FALSE, mat4);
}

} // namespace rendering

#endif // ENABLE_GL
