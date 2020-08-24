//==--- src/engine/window.cpp ------------------------------ -*- C++ -*- ---==//
//
//                              Snowflake
//
//                      Copyright (c) 2020 Rob Clucas
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  window.cpp
/// \brief This file defines the implemenation of the window class.
//
//==------------------------------------------------------------------------==//

#include <snowflake/engine/window.hpp>
#include <wrench/log/logger.hpp>

namespace snowflake {

//==--- [con/destruction] --------------------------------------------------==//

Window::Window(
  EnginePtr engine, const char* title, uint32_t width, uint32_t height) noexcept
: engine_(engine) {
  assert(engine_ != nullptr && "Can't create window with invalid engine!");
  init(title, width, height);
}

//==--- [interface] --------------------------------------------------------==//

auto Window::is_alive() const noexcept -> bool {
  return engine_->platform().is_alive();
}

auto Window::poll_input() noexcept -> void {
  engine_->platform().poll_input();
}

//==--- [private] ----------------------------------------------------------==//

auto Window::init(const char* title, uint32_t width, uint32_t height) noexcept
  -> bool {
  engine_->platform().set_title(title);
  engine_->platform().resize(width, height);

  return true;
}

} // namespace snowflake