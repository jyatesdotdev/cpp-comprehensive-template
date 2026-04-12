# Rendering Pipeline Guide

Modern graphics programming in C++ revolves around GPU pipeline management, shader
programs, and efficient resource handling. This guide covers OpenGL and Vulkan concepts
with modern C++ patterns.

## Graphics API Overview

### OpenGL (4.5+ / DSA)
- Mature, widely supported, simpler API surface
- Direct State Access (DSA) eliminates bind-to-edit pattern
- Good for prototyping and moderate-complexity renderers
- Single global context, implicit synchronization

### Vulkan
- Explicit GPU control: memory, synchronization, command buffers
- Multi-threaded command recording, lower driver overhead
- Verbose setup (~1000 lines for a triangle) but maximum performance
- Required for advanced techniques (mesh shaders, ray tracing extensions)

### When to Choose What
| Criteria | OpenGL | Vulkan |
|---|---|---|
| Prototyping speed | ✓ | |
| Max GPU throughput | | ✓ |
| Multi-threaded rendering | | ✓ |
| Mobile (Android/iOS) | ES 3.x | ✓ |
| Ray tracing extensions | | ✓ |

## The Graphics Pipeline

```
Vertex Data → [Vertex Shader] → [Tessellation] → [Geometry Shader]
    → [Rasterization] → [Fragment Shader] → Framebuffer
```

### Key Stages
1. **Vertex Shader**: Transforms vertices from model space to clip space (MVP matrix)
2. **Tessellation** (optional): Subdivides geometry on the GPU
3. **Geometry Shader** (optional): Generates/discards primitives (use sparingly — slow)
4. **Rasterization**: Converts primitives to fragments (fixed-function)
5. **Fragment Shader**: Computes per-pixel color, lighting, texturing
6. **Output Merger**: Depth test, stencil test, blending → final framebuffer

### Compute Shaders
Separate from the graphics pipeline. General-purpose GPU compute for:
- Particle systems, physics simulation
- Post-processing (bloom, SSAO, tone mapping)
- Culling (GPU-driven rendering)

## Modern C++ Patterns for Graphics

### 1. RAII Resource Wrappers
GPU resources (buffers, textures, shaders, framebuffers) must be created and destroyed
through API calls. RAII ensures cleanup:

```cpp
// See include/rendering/gl_resource.h
auto vbo = gl::Buffer::create();
vbo.bind(GL_ARRAY_BUFFER);
// Automatically calls glDeleteBuffers on destruction
```

### 2. Shader Compilation Pipeline
Compile shaders at runtime, check errors, link into programs:

```cpp
// See include/rendering/shader.h
auto program = ShaderProgram::from_source(vertex_src, fragment_src);
program.use();
program.set_uniform("u_mvp", mvp_matrix);
```

### 3. Render Pass Abstraction
Encapsulate framebuffer setup, clear operations, and draw calls:

```cpp
RenderPass pass{framebuffer, {0.1f, 0.1f, 0.1f, 1.0f}};
pass.set_pipeline(pipeline);
pass.draw(mesh);
```

### 4. Material System with Type Erasure
Decouple shader parameters from rendering logic using type-erased materials
or variant-based uniform maps.

### 5. GPU Memory Management
- Use persistent mapped buffers for streaming data
- Ring buffers for per-frame uniform updates
- Staging buffers for texture uploads

## Shader Best Practices

### GLSL Structure
```glsl
#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

layout(std140, binding = 0) uniform CameraBlock {
    mat4 view;
    mat4 projection;
};

uniform mat4 u_model;

out VS_OUT {
    vec3 world_pos;
    vec3 normal;
    vec2 texcoord;
} vs_out;

void main() {
    vec4 world = u_model * vec4(a_position, 1.0);
    vs_out.world_pos = world.xyz;
    vs_out.normal = mat3(transpose(inverse(u_model))) * a_normal;
    vs_out.texcoord = a_texcoord;
    gl_Position = projection * view * world;
}
```

### Tips
- Use `layout(location = N)` for explicit attribute binding
- Use Uniform Buffer Objects (UBOs) with `std140` for shared camera/light data
- Avoid branching in fragment shaders when possible
- Use `#include` via shader preprocessing for reusable functions

## Vulkan-Specific Concepts

### Pipeline State Objects (PSOs)
Vulkan bakes all render state into immutable pipeline objects:
- Shader stages, vertex input layout, rasterization state
- Depth/stencil, blending, viewport/scissor (can be dynamic)
- Must create pipeline variants for different render configurations

### Descriptor Sets
Vulkan's resource binding model:
- Descriptor Set Layout defines the shape (what bindings exist)
- Descriptor Pool allocates sets
- Descriptor Sets bind actual resources (buffers, textures, samplers)
- Update once, bind per-draw — much faster than OpenGL's per-uniform calls

### Synchronization
- **Semaphores**: GPU-GPU sync (between queue submissions)
- **Fences**: GPU-CPU sync (wait for frame completion)
- **Pipeline Barriers**: Memory/execution dependencies within command buffers
- **Render Pass Dependencies**: Subpass dependencies for attachment transitions

### Command Buffers
- Record commands once, submit many times (or re-record per frame)
- Secondary command buffers for multi-threaded recording
- Use `vkCmdBeginRenderPass` / `vkCmdEndRenderPass` to scope render passes

## Performance Guidelines

1. **Minimize state changes**: Sort draws by pipeline/material
2. **Batch draw calls**: Use instancing or indirect draws
3. **GPU-driven rendering**: Compute shader culling + `DrawIndirect`
4. **Avoid CPU readback**: Use transform feedback or compute for GPU-side results
5. **Profile with tools**: RenderDoc, Nsight Graphics, GPU PerfStudio
6. **Double/triple buffer**: Overlap CPU and GPU work with ring buffers
7. **Texture atlases**: Reduce bind calls, improve cache coherence

## Template Code

This template provides:
- `rendering/gl_resource.h` — RAII wrappers for OpenGL objects (Buffer, Texture, VAO, Shader, Framebuffer)
- `rendering/shader.h` — Shader compilation, linking, uniform management
- `rendering/render_pipeline.h` — Render pass and pipeline state abstraction
- `examples/rendering_demo.cpp` — Demo showing the patterns (requires ENABLE_RENDERING)

Build with:
```bash
cmake --preset default -DENABLE_RENDERING=ON
cmake --build build
./build/rendering_demo
```

Requires: OpenGL 4.5+, GLFW, GLM (via vcpkg or system packages).

## See Also

- [Architecture](ARCHITECTURE.md) — Rendering module in the dependency graph
- [Best Practices](best_practices.md) — RAII and resource management patterns
- [Cross-Platform Build](cross_platform_build.md) — Enabling the rendering module
- [Third-Party Integration](third_party_integration.md) — vcpkg feature gating for rendering deps
