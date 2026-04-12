/// @file rendering_demo.cpp
/// @brief Demonstrates RAII OpenGL wrappers, shader compilation, and render pass.
/// Build with: cmake --preset default -DENABLE_RENDERING=ON

#include <rendering/gl_resource.h>
#include <rendering/render_pipeline.h>
#include <rendering/shader.h>

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>

// ── Shaders ─────────────────────────────────────────────────────────
static constexpr const char* kVertSrc = R"(
#version 450 core
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec3 a_color;
out vec3 v_color;
void main() {
    v_color = a_color;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)";

static constexpr const char* kFragSrc = R"(
#version 450 core
in vec3 v_color;
out vec4 frag_color;
void main() {
    frag_color = vec4(v_color, 1.0);
}
)";

int main() {
    // ── GLFW init ───────────────────────────────────────────────────
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return 1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "Rendering Demo", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    // ── Build shader program ────────────────────────────────────────
    GLuint prog = rendering::build_program(kVertSrc, kFragSrc);

    // ── Triangle data ───────────────────────────────────────────────
    // interleaved: vec2 pos, vec3 color
    static constexpr float vertices[] = {
         0.0f,  0.5f,   1.f, 0.f, 0.f,
        -0.5f, -0.5f,   0.f, 1.f, 0.f,
         0.5f, -0.5f,   0.f, 0.f, 1.f,
    };

    // ── RAII GPU resources ──────────────────────────────────────────
    rendering::gl::VertexArray vao;
    rendering::gl::Buffer vbo;

    glNamedBufferStorage(vbo.id(), sizeof(vertices), vertices, 0);

    // Vertex layout via DSA
    glVertexArrayVertexBuffer(vao.id(), 0, vbo.id(), 0, 5 * sizeof(float));
    glEnableVertexArrayAttrib(vao.id(), 0);
    glEnableVertexArrayAttrib(vao.id(), 1);
    glVertexArrayAttribFormat(vao.id(), 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribFormat(vao.id(), 1, 3, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(vao.id(), 0, 0);
    glVertexArrayAttribBinding(vao.id(), 1, 0);

    // ── Pipeline state ──────────────────────────────────────────────
    rendering::PipelineState pipeline{
        .program = prog,
        .depth = {.test_enabled = false},
        .blend = {},
        .cull_face = 0,
    };

    // ── Render loop ─────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window)) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);

        // RAII render pass: binds FBO 0, sets viewport, clears
        rendering::RenderPass pass(0, w, h, {0.1f, 0.1f, 0.12f, 1.0f});
        pass.draw(pipeline, [&] {
            glBindVertexArray(vao.id());
            glDrawArrays(GL_TRIANGLES, 0, 3);
        });

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
}
