# rendering — OpenGL RAII, shaders, render pipeline

Scope: `include/rendering/`. Namespaces `rendering` and `rendering::gl`. CMake target
**`rendering` (INTERFACE / header-only)**. Parent: [include/AGENTS.md](../AGENTS.md).
Architecture: [docs/rendering_pipeline.md](../../docs/rendering_pipeline.md).

## ⚠️ Entirely gated — read first
**Every header here is wrapped in `#ifdef ENABLE_GL`.** The `rendering` target is only created
when `ENABLE_RENDERING=ON` **and** OpenGL is found (which sets the `ENABLE_GL` compile
definition, and optionally links glfw/glm). With `ENABLE_GL` undefined these headers expand to
nothing — that is intentional, so the default build needs no GPU/driver. **Keep all new code
inside the `#ifdef ENABLE_GL` guard.** Assumes **OpenGL 4.5+ (Direct State Access)**.

## Business role
Shows how to wrap raw OpenGL (a C, handle-based, global-state API) in safe RAII C++ and how to
package fixed-function state into Vulkan-style pipeline objects. Demoed in
`examples/rendering_demo.cpp` (also needs glfw for a context).

## Files
- `gl_resource.h` — `gl::Resource<CreateFn, DeleteFn>` template + aliases `Buffer`, `Texture`,
  `VertexArray`, `Framebuffer`, `Renderbuffer`, plus bespoke `gl::Shader` and `gl::Program`.
  Move-only RAII over one GL object.
- `shader.h` — `compile_shader` → `gl::Shader`, `link_program` / `build_program` → `gl::Program`
  (compile→link→cleanup via RAII; no raw `GLuint` ownership); overloaded `set_uniform`
  (int / float / mat4).
- `render_pipeline.h` — `DepthState`, `BlendState`, `PipelineState` (`.bind()`), and the RAII
  `RenderPass` (binds FBO + viewport, clears, **restores the previous FBO and viewport on destruction**).

## Invariants & business rules (MUST hold)
1. **RAII owns exactly one GL object and is move-only.** `Resource`/`Program` delete copy,
   transfer via `std::exchange(id_, 0)`, and call `glDelete*` only when `id_ != 0`. A
   moved-from wrapper holds `0` and must be a safe no-op on destruction. Preserve this shape
   for any new resource type.
2. **GL calls need a current context.** Constructing a `Resource` calls `glCreate*`
   immediately — these objects must only be created/destroyed on a thread with a live GL
   context. Don't create them as globals or before context init.
3. **Shader helpers must free on failure.** `compile_shader` returns `gl::Shader` and
   `link_program`/`build_program` return `gl::Program`. Failed compile/link throws
   `std::runtime_error` (with the info log); the RAII wrapper deletes the GL object. Do not
   return a raw owning `GLuint` — intermediate shaders must not leak if a later step throws.
4. **`RenderPass` saves and restores framebuffer and viewport state.** It records
   `GL_FRAMEBUFFER_BINDING` and `GL_VIEWPORT` in the ctor and restores both in the dtor. It is
   non-copyable. Don't remove the save/restore — callers rely on stack-scoped passes not
   corrupting global GL state.
5. `PipelineState::bind()` applies program + depth + blend + cull + polygon mode as a unit;
   `cull_face == 0` disables culling. Treat the `*State` structs as immutable descriptors.

## C++ best practices for this module
- Prefer DSA entry points (`glCreateBuffers`, `glProgramUniform*`) — no bind-to-edit.
- Represent every GL object through a RAII wrapper; never hold a bare `GLuint` that owns.
- Keep intrinsic/GL headers (`<GL/gl.h>`) out of non-rendering translation units.

## When editing
- There is **no `rendering_tests.cpp`** (needs a GL context in CI). Validate via
  `examples/rendering_demo.cpp` with `cmake -DENABLE_RENDERING=ON` locally.
- New GL object type → add a `using X = Resource<glCreateX, glDeleteX>;` alias when the
  create/delete signature matches `(GLsizei, GLuint*)`; otherwise write a bespoke wrapper like
  `Program` (which has a different signature).

## Neighbors
Same handle-RAII idea, generalized: [memory `UniqueHandle`](../memory/AGENTS.md).
