#pragma once
/// @file gl_resource.h
/// @brief RAII wrappers for OpenGL objects using modern C++ patterns.
///
/// Each wrapper owns a single GL object, is move-only, and calls the
/// appropriate glDelete* on destruction. Requires OpenGL 4.5+ (DSA).

#ifdef ENABLE_GL

#include <GL/gl.h>
#include <utility>

namespace rendering::gl {

/// @brief Generic RAII handle for a single OpenGL object.
/// @tparam CreateFn  GL creation function, e.g. `glCreateBuffers`.
/// @tparam DeleteFn  GL deletion function, e.g. `glDeleteBuffers`.
template <auto CreateFn, auto DeleteFn>
class Resource {
public:
    /// @brief Create the GL object via CreateFn.
    Resource() { CreateFn(1, &id_); }
    /// @brief Destroy the GL object via DeleteFn.
    ~Resource() { if (id_) DeleteFn(1, &id_); }

    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    /// @brief Move-construct, taking ownership from @p o.
    Resource(Resource&& o) noexcept : id_(std::exchange(o.id_, 0)) {}
    /// @brief Move-assign, taking ownership from @p o.
    Resource& operator=(Resource&& o) noexcept {
        if (this != &o) {
            if (id_) DeleteFn(1, &id_);
            id_ = std::exchange(o.id_, 0);
        }
        return *this;
    }

    /// @brief Get the raw GL object handle.
    /// @return GL object name (0 if moved-from).
    [[nodiscard]] GLuint id() const noexcept { return id_; }
    /// @brief Check whether this handle owns a valid GL object.
    explicit operator bool() const noexcept { return id_ != 0; }

private:
    GLuint id_ = 0;
};

// ── Concrete resource types (OpenGL 4.5 DSA) ───────────────────────
// Usage: gl::Buffer buf;  // creates via glCreateBuffers
//        buf.id();        // get raw handle
//        // automatically deleted when buf goes out of scope

using Buffer       = Resource<glCreateBuffers,       glDeleteBuffers>;       ///< @brief RAII buffer object.
using Texture      = Resource<glCreateTextures,       glDeleteTextures>;      ///< @brief RAII texture object.
using VertexArray  = Resource<glCreateVertexArrays,   glDeleteVertexArrays>;  ///< @brief RAII vertex array object.
using Framebuffer  = Resource<glCreateFramebuffers,   glDeleteFramebuffers>;  ///< @brief RAII framebuffer object.
using Renderbuffer = Resource<glCreateRenderbuffers,   glDeleteRenderbuffers>; ///< @brief RAII renderbuffer object.

/// @brief RAII wrapper for an OpenGL shader program (different create/delete signature).
class Program {
public:
    /// @brief Create a new shader program via `glCreateProgram`.
    Program() : id_(glCreateProgram()) {}
    /// @brief Delete the program via `glDeleteProgram`.
    ~Program() { if (id_) glDeleteProgram(id_); }

    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;
    /// @brief Move-construct, taking ownership from @p o.
    Program(Program&& o) noexcept : id_(std::exchange(o.id_, 0)) {}
    /// @brief Move-assign, taking ownership from @p o.
    Program& operator=(Program&& o) noexcept {
        if (this != &o) { if (id_) glDeleteProgram(id_); id_ = std::exchange(o.id_, 0); }
        return *this;
    }

    /// @brief Get the raw program handle.
    /// @return GL program name (0 if moved-from).
    [[nodiscard]] GLuint id() const noexcept { return id_; }
    /// @brief Bind this program for use (`glUseProgram`).
    void use() const { glUseProgram(id_); }

private:
    GLuint id_ = 0;
};

} // namespace rendering::gl

#endif // ENABLE_GL
