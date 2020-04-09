//==--- glow/src/vk/window.cpp -------------- -*- C++ -*- ---==//
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

#include <ripple/glow/vk/window/window.hpp>

namespace ripple::glow::vk {

Window::Window(const std::string& title, uint32_t width, uint32_t height)
: _platform(title, width, height) {
  _is_alive = _platform.initialize_vulkan_loader();
}

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

} // namespace ripple::glow::vk