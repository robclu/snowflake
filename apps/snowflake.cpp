//==--- apps/snowflake.cpp --------------------------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  snowflake.hpp
/// \brief This is an example application for snowflake.
//
//==------------------------------------------------------------------------==//

#include <snowflake/engine/engine.hpp>
#include <snowflake/engine/window.hpp>
#include <snowflake/rendering/renderer.hpp>
#include <snowflake/rendering/scene_view.hpp>

auto run_application() noexcept -> void {
  using namespace snowflake;
  const char* name   = "snowflake";
  Engine&     engine = snowflake::Engine::create();
  Window      window(&engine, name, 512, 512);

  // Empty for now ...
  SceneView view;

  // Create the scene ...

  Renderer* renderer = engine.create_renderer();
  while (window.is_alive()) {
    window.poll_input();

    // Do some event handling

    if (renderer->begin_frame()) {
      renderer->render(&view);
      renderer->end_frame();
    }
  }

  engine.destroy(renderer);
}

auto main(int argc, const char* argv[]) -> int {
  run_application();
}