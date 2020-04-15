//==--- glow/src/window/window.cpp ------------------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  window.cpp
/// \brief This file defines the implemenation of the window class.
//
//==------------------------------------------------------------------------==//

#include <ripple/core/log/logger.hpp>
#include <ripple/glow/window/window.hpp>

namespace ripple::glow {

//==--- [con/destruction] --------------------------------------------------==//

Window::Window(
  Window::engine_ptr_t engine,
  const std::string&   title,
  uint32_t             width,
  uint32_t             height)
: _engine(engine) {
  if (!init(title, width, height)) {
    log_error("Failed to initialize window.");
  }
}

//==--- [interface] --------------------------------------------------------==//

auto Window::is_alive() const -> bool {
  return _engine->platform()->is_alive();
}

auto Window::poll_input() -> void {
  _engine->platform()->poll_input();
}

//==--- [private] ----------------------------------------------------------==//

auto Window::init(const std::string& title, uint32_t width, uint32_t height)
  -> bool {
  if (_engine == nullptr) {
    log_error("Window requires a valid engine.");
    return false;
  }

  auto* platform = _engine->platform();
  if (platform == nullptr) {
    log_error("Failed to create the platform.");
    return false;
  }

  platform->set_title(title);
  platform->resize(width, height);

  return true;
}

} // namespace ripple::glow