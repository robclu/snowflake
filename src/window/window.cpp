//==--- snowflake/src/window/window.cpp -------------------- -*- C++ -*- ---==//
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

#include <snowflake/window/window.hpp>
#include <wrench/log/logger.hpp>

namespace snowflake {

//==--- [con/destruction] --------------------------------------------------==//

Window::Window(
  Window::EnginePtr engine,
  const char*       title,
  uint32_t          width,
  uint32_t          height) noexcept
: engine_(engine) {
  if (!init(title, width, height)) {
    wrench::log_error("Failed to initialize window.");
  }
}

//==--- [interface] --------------------------------------------------------==//

auto Window::is_alive() const noexcept -> bool {
  return engine_->platform()->is_alive();
}

auto Window::poll_input() noexcept -> void {
  engine_->platform()->poll_input();
}

//==--- [private] ----------------------------------------------------------==//

auto Window::init(const char* title, uint32_t width, uint32_t height) noexcept
  -> bool {
  if (engine_ == nullptr) {
    wrench::log_error("Window requires a valid engine.");
    return false;
  }

  auto* platform = engine_->platform();
  if (platform == nullptr) {
    wrench::log_error("Failed to create the platform.");
    return false;
  }

  platform->set_title(title);
  platform->resize(width, height);

  return true;
}

} // namespace snowflake