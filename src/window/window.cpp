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

Window::Window(const std::string& title, uint32_t width, uint32_t height)
: _platform(title, width, height) {
  _is_alive = _platform.initialize_vulkan_loader();
  if (!init()) {
    log_error("Failed to initialize window.");
  }
}

//==--- [interface] --------------------------------------------------------==//

auto Window::is_alive() const -> bool {
  return _is_alive;
}

auto Window::poll_input() -> void {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT: {
        _is_alive = false;
        break;
        default: break;
      }
    }
  }
}

//==--- [private] ----------------------------------------------------------==//

auto Window::init() -> bool {
  _engine = Engine::create(_platform);

  if (_engine == nullptr) {
    log_error("Failed to create the engine.");
    return false;
  }
  return true;
}

} // namespace ripple::glow