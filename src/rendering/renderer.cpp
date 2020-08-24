//==--- snowflake/src/rendering/renderer.cpp --------------- -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  engine.cpp
/// \brief This file defines the implemenation for the engine.
//
//==------------------------------------------------------------------------==//

#include <snowflake/engine/engine.hpp>
#include <snowflake/rendering/renderer.hpp>

namespace snowflake {

/*==--- [init & destroy] ---------------------------------------------------==*/

auto Renderer::init() noexcept -> void {
  // Check everything is supported ...
}

auto Renderer::destroy() noexcept -> void {
  // Cleanup all resources ...
}

/*==--- [rendering] --------------------------------------------------------==*/

auto Renderer::render(const SceneView* view) noexcept -> void {
  auto& driver = engine_.driver();
  auto  cmd_buffer =
    driver.request_command_buffer<CommandBufferKind::graphics>();
  driver.submit(cmd_buffer);
}

auto Renderer::begin_frame() noexcept -> bool {
  auto& driver = engine_.driver();
  return driver.begin_frame(engine_.platform());
}

auto Renderer::end_frame() noexcept -> void {
  auto& driver = engine_.driver();
  driver.end_frame(engine_.platform());
}

} // namespace snowflake