#pragma once
/// @file shader.h
/// @brief Shader compilation, linking, and uniform helpers.

#ifdef ENABLE_GL

#include <GL/gl.h>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rendering {

/// @brief Compile a single shader stage.
/// @param type  GL shader type (e.g. `GL_VERTEX_SHADER`, `GL_FRAGMENT_SHADER`).
/// @param source  GLSL source code.
/// @return Compiled shader object handle.
/// @throws std::runtime_error If compilation fails.
inline GLuint compile_shader(GLenum type, std::string_view source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.data();
    auto len = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        std::array<char, 1024> log{};
        glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile error: ") + log.data());
    }
    return shader;
}

/// @brief Link vertex and fragment shaders into a program.
/// @param vert  Compiled vertex shader handle.
/// @param frag  Compiled fragment shader handle.
/// @return Linked program handle.
/// @throws std::runtime_error If linking fails.
inline GLuint link_program(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        std::array<char, 1024> log{};
        glGetProgramInfoLog(prog, static_cast<GLsizei>(log.size()), nullptr, log.data());
        glDeleteProgram(prog);
        throw std::runtime_error(std::string("Program link error: ") + log.data());
    }
    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    return prog;
}

/// @brief Build a complete shader program from vertex and fragment source strings.
///
/// Compiles both stages, links, and cleans up intermediate shaders.
/// @param vert_src  GLSL vertex shader source.
/// @param frag_src  GLSL fragment shader source.
/// @return Linked program handle.
/// @throws std::runtime_error If compilation or linking fails.
inline GLuint build_program(std::string_view vert_src, std::string_view frag_src) {
    GLuint v = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint f = 0;
    try {
        f = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    } catch (...) {
        glDeleteShader(v);
        throw;
    }
    GLuint prog = 0;
    try {
        prog = link_program(v, f);
    } catch (...) {
        glDeleteShader(v);
        glDeleteShader(f);
        throw;
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return prog;
}

// ── Uniform setters (convenience) ───────────────────────────────────

/// @brief Set an integer uniform on a program.
/// @param prog  Program handle.
/// @param name  Uniform name.
/// @param val   Integer value.
inline void set_uniform(GLuint prog, const char* name, int val) {
    glProgramUniform1i(prog, glGetUniformLocation(prog, name), val);
}

/// @brief Set a float uniform on a program.
/// @param prog  Program handle.
/// @param name  Uniform name.
/// @param val   Float value.
inline void set_uniform(GLuint prog, const char* name, float val) {
    glProgramUniform1f(prog, glGetUniformLocation(prog, name), val);
}

/// @brief Set a 4×4 matrix uniform on a program.
/// @param prog       Program handle.
/// @param name       Uniform name.
/// @param mat4       Pointer to 16 floats (column-major).
/// @param transpose  Whether to transpose the matrix (default: false).
inline void set_uniform(GLuint prog, const char* name, const float* mat4, bool transpose = false) {
    glProgramUniformMatrix4fv(prog, glGetUniformLocation(prog, name),
                              1, transpose ? GL_TRUE : GL_FALSE, mat4);
}

} // namespace rendering

#endif // ENABLE_GL
