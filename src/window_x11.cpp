//==--- ripple/glow/src/window_x11.cpp --------------------- -*- C++ -*- ---==//
//
//                            Ripple - Glow
//
//                      Copyright (c) 2020 Ripple
//
//  This file is distributed under the MIT License. See LICENSE for details.
//
//==------------------------------------------------------------------------==//
//
/// \file  window_x11.cpp
/// \brief This file defines the implemenation of an X11 window.
//
//==------------------------------------------------------------------------==//

#include <SDL_syswm.h>
#include <cassert>
#include <ripple/glow/window/window_x11.hpp>

namespace ripple::glow {

auto X11Window::create_native_window() -> void {
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  assert(SDL_GetWindowWMInfo(this->_window, &wmi));
  this->_native_window = static_cast<Window>(wmi.info.x11.window);
}

} // namespace ripple::glow