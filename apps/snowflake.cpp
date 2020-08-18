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

#include <snowflake/window/window.hpp>

auto run_application() noexcept -> void {
  const char*       name   = "snowflake";
  auto*             engine = snowflake::Engine::create();
  snowflake::Window window(engine, name, 512, 512);

  // auto* renderer = window.renderer();
  // auto* renderer = engine.renderer();
  while (window.is_alive()) {
    window.poll_input();

    // Do some event handling

    auto* driver = engine->driver();
    driver->begin_frame(*(engine->platform()));

    auto cmd =
      driver->request_command_buffer<snowflake::CommandBufferKind::graphics>();

    driver->submit(cmd);

    driver->end_frame(*(engine->platform()));
  }
}

auto main(int argc, const char* argv[]) -> int {
  run_application();
}