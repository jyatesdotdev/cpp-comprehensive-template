#pragma once
/// @file render_pipeline.h
/// @brief Render pass and pipeline state abstractions.
///
/// Demonstrates how to encapsulate OpenGL state into composable,
/// RAII-managed pipeline objects — a pattern inspired by Vulkan's
/// explicit pipeline model applied to OpenGL.

#ifdef ENABLE_GL

#include <GL/gl.h>
#include <array>
#include <functional>

namespace rendering {

/// @brief Immutable depth/stencil state descriptor.
struct DepthState {
    bool test_enabled  = true;   ///< @brief Enable depth testing.
    bool write_enabled = true;   ///< @brief Enable depth writes.
    GLenum func        = GL_LESS; ///< @brief Depth comparison function.

    /// @brief Apply this depth state to the current GL context.
    void apply() const {
        if (test_enabled) { glEnable(GL_DEPTH_TEST); glDepthFunc(func); }
        else              { glDisable(GL_DEPTH_TEST); }
        glDepthMask(write_enabled ? GL_TRUE : GL_FALSE);
    }
};

/// @brief Immutable blend state descriptor.
struct BlendState {
    bool enabled    = false;                  ///< @brief Enable alpha blending.
    GLenum src      = GL_SRC_ALPHA;           ///< @brief Source blend factor.
    GLenum dst      = GL_ONE_MINUS_SRC_ALPHA; ///< @brief Destination blend factor.

    /// @brief Apply this blend state to the current GL context.
    void apply() const {
        if (enabled) { glEnable(GL_BLEND); glBlendFunc(src, dst); }
        else         { glDisable(GL_BLEND); }
    }
};

/// @brief Pipeline state: shader + fixed-function state, applied as a unit.
struct PipelineState {
    GLuint program = 0;              ///< @brief Shader program handle.
    DepthState depth;                ///< @brief Depth test configuration.
    BlendState blend;                ///< @brief Blend configuration.
    GLenum cull_face = GL_BACK;      ///< @brief Face to cull (0 to disable culling).
    GLenum polygon_mode = GL_FILL;   ///< @brief Polygon rasterization mode.

    /// @brief Bind the full pipeline state to the current GL context.
    void bind() const {
        glUseProgram(program);
        depth.apply();
        blend.apply();
        if (cull_face) { glEnable(GL_CULL_FACE); glCullFace(cull_face); }
        else           { glDisable(GL_CULL_FACE); }
        glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
    }
};

/// @brief RAII render pass: binds framebuffer + viewport, clears, restores on exit.
class RenderPass {
public:
    /// @brief Begin a render pass targeting the given framebuffer.
    /// @param fbo          Framebuffer object (0 = default framebuffer).
    /// @param width        Viewport width in pixels.
    /// @param height       Viewport height in pixels.
    /// @param clear_color  RGBA clear colour (default: opaque black).
    /// @param clear_mask   Bitfield of buffers to clear (default: colour + depth).
    RenderPass(GLuint fbo, int width, int height,
               std::array<float, 4> clear_color = {0.f, 0.f, 0.f, 1.f},
               GLbitfield clear_mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, width, height);
        glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
        glClear(clear_mask);
    }

    ~RenderPass() {
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prev_fbo_));
    }

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    /// @brief Execute a draw call within this pass using the given pipeline.
    /// @param pipeline  Pipeline state to bind before drawing.
    /// @param draw_fn   Callable that issues the actual GL draw commands.
    void draw(const PipelineState& pipeline, std::function<void()> draw_fn) const {
        pipeline.bind();
        draw_fn();
    }

private:
    GLint prev_fbo_ = 0;
};

} // namespace rendering

#endif // ENABLE_GL
