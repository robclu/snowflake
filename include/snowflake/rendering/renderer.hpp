//==--- snwoflake/renderer/renderer.hpp -------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  renderer.hpp
/// \brief This file defines a renderer.
//
//==------------------------------------------------------------------------==//

#ifndef SNOWFLAKE_RENDERER_RENDERER_HPP
#define SNOWFLAKE_RENDERER_RENDERER_HPP

namespace snowflake {

class Engine;
class View;

/**
 * The Renderer is an object which is used to generate drawing commands for
 * a given View which is a representation of a Scene thtough a specific
 * viewport (i.e a Camera). A renderer is created from an Engine.
 *
 * The typical usage of the renderer is:
 *
 * ~~~{.cpp}
 * Renderer* renderer = engine()->create_renderer();
 * while (must_render) {
 *   renderer->begin_frame();
 *   renderer->render(view);
 *   renderer->end_frame();
 * }
 * engine.destroy(renderer);
 * ~~~
 */
class Renderer {
 public:
  /**
   * Constructor to intialize the renderer with the \p engine.
   */
  explicit Renderer(Engine* engine) noexcept : engine_(engine) {}

  /**
   * Returns a reference to the engine.
   */
  auto engine() noexcept -> Engine& {
    return *engine_;
  }

  /*
   * Returns a const pointer to the engine
   */
  auto engine() const noexcept -> const Engine& {
    return *engine_;
  }

  /**
   * Render a View into this renderer's window.
   *
   * This is the main rendering method, where most of CPU work is done
   * generating rendering commmands which are executed asynchronously on the
   * Engine's render thread.
   *
   * Commands will be generated for the following stages:
   *
   * 1. Shadow maps
   * 2. Depth pre-pass
   * 3. Color pass
   *  - Occlusion queries
   *  - Forward pass for opaque objects
   *  - AO
   *  - SSR
   *  - Transparent objects
   * 4. Post-processing pass
   *  - TAA
   *  - Motion blur
   *  - Bloom
   *
   * \param view A pointer to the view to render.
   *
   * \note If multiple Renderer instances exist, then calls to this method must
   *       be externally synchronized by the caller.
   *
   * \note This method is does heavy processing, but is threaded internally
   *       to reduce the latency.
   */
  auto render(const View* view) noexcept -> void;

 private:
  Engine* engine_ = nullptr; //!< Pointer to the engine.
};

} // namespace snowflake

#endif // SNOWFLAKE_RENDERER_RENDERER_HPP
